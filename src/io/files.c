#include "io/files.h"

#include "directives/dword.h"
#include "directives/dcast.h"
#include "directives/dfree.h"
#include "directives/dos.h"

#if defined(__APPLE__) && defined(__MACH__)
# include <mach-o/dyld.h>
#endif

#if VX_UNIX == 1
# include <sys/stat.h>
# include <unistd.h>
# include <dirent.h>
#endif

#if VX_WINDOWS == 1
# include <windows.h>
# include <direct.h>
# define mkdir(path, mode) _mkdir(path)
#endif

#if defined(PATH_MAX)
# define VX_PATH_MAX PATH_MAX
#elif defined(MAX_PATH)
# define VX_PATH_MAX (MAX_PATH)
#else
# define VX_PATH_MAX (4096u)
#endif 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *vxfile_exec_dir;

char *vxfile_parent_from_path(char *path)
{
	if (!path) return path;
	char *parent_path = strrchr(path, '/');
	if (!parent_path && !(parent_path = strrchr(path, '\\'))) return path;
	path[(parent_path - path) + 1] = '\0';
	return path;
}

int vxfile_directory_exists(const char *path)
{
#if VX_WINDOWS == 1
	DWORD ftyp = GetFileAttributesA(path);
	if (ftyp & FILE_ATTRIBUTE_DIRECTORY) return 1;
	return 0;
#else
	struct stat info;
	if (stat(path, &info) == -1) return 0;
	return S_ISDIR(info.st_mode);
#endif
}

int vxfile_file_exists(const char *path)
{
#if VX_WINDOWS == 1
	DWORD ftyp = GetFileAttributesA(path);
	if ((ftyp == INVALID_FILE_ATTRIBUTES) || (ftyp & FILE_ATTRIBUTE_DIRECTORY)) return 0;
	return 1;
#else
	struct stat info;
	if (stat(path, &info) == -1) return 0;
	return S_ISREG(info.st_mode);
#endif
}

int vxfile_remove_file(const char *path)
{
	return remove(path) == -1 ? 0 : 1;
}

int vxfile_create_directory(const char *path)
{
	return mkdir(path, S_IRWXU | S_IRWXO | S_IRWXG) == -1 ? 0 : 1;
}

char *vxfile_read(const char *filename, size_t *optional_file_size)
{
	FILE *file = fopen(filename, "rb");
	if (!file) return VX_NULL;

	/* Get file length by seeking to end to reserve exact size. */
	fseek(file, 0, SEEK_END);
	const size_t file_size = VX_CAST(size_t, ftell(file));
	char *contents = VX_CAST(char *, malloc(file_size + 1u));
	if (!contents) return VX_NULL;
	contents[file_size] = '\0';
	fseek(file, 0, SEEK_SET);

	const size_t num_read = fread(contents, 1u, file_size, file);
	if (num_read < file_size) VX_FREE(contents);

	if (optional_file_size) *optional_file_size = file_size;

	fclose(file);
	return contents;
}

int vxfile_write(const char *filename, const void *contents, size_t contents_bytes)
{
	FILE *file = fopen(filename, "wb");
	if (!file) return 0;

	const size_t num_wrote = fwrite(contents, 1u, contents_bytes, file);
	const int result = (num_wrote < contents_bytes) ? 0 : 1;

	fclose(file);
	return result;
}

char *vxfile_get_exec_path(char **result, char *argv_first)
{
	char *determined_path = VX_CAST(char *, calloc(VX_PATH_MAX, 1u));
	if (!determined_path) return VX_NULL;
#if VX_WINDOWS == 1
	(void)(argv_first);
	GetModuleFileName(NULL, determined_path, VX_PATH_MAX);
	return *result = determined_path;
#elif VX_UNIX == 1
	/* Read from OS-specific proc file. */
	const char *proc_file =
# if defined(__linux__)
	"/proc/self/exe";
# elif defined(__FreeBSD__)
	"/proc/curproc/file";
# elif (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	"/proc/self/path/a.out";
# else
	VX_NULL;
#endif
	if (proc_file) {
		const ssize_t len = readlink(proc_file, determined_path, VX_PATH_MAX);
		if (len > 0) return *result = determined_path; 
	}

	/* Full path already. */
	if (*argv_first == '/') {
		memcpy(determined_path, argv_first, strlen(argv_first));
		return *result = determined_path; 
	}

	/* Get from symlink if path is one. */
	struct stat argv_status;
	if (determined_path && lstat(argv_first, &argv_status) == 0) {
		if (readlink(argv_first, determined_path, VX_PATH_MAX) < VX_CAST(ssize_t, VX_PATH_MAX)) return *result = determined_path;
		else return VX_NULL;
	}

	/* Relative directory, append to cwd. */
	if (determined_path && strchr(argv_first, '/')) {
		size_t argv_len = strlen(argv_first) + 1u;
		if (!getcwd(determined_path, VX_PATH_MAX - argv_len)) return VX_NULL;
		memcpy(determined_path + strlen(determined_path), argv_first, argv_len);
		return *result = determined_path;
	}

	/* Check environment variables. */
	char *possible_env = getenv(argv_first);
	if (possible_env) {
		memcpy(determined_path, possible_env, strlen(possible_env));
		return *result = determined_path;
	}

	return VX_NULL;
#else
	memcpy(determined_path, argv_first, strlen(argv_first) + 1u);
	return *result = determined_path;
#endif
}
