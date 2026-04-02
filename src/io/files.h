#pragma once
#ifndef SOURCE_IO_FILES_VXL_HDR
#define SOURCE_IO_FILES_VXL_HDR
/* File handling functions. */

#include "directives/dextern.h"

#include <stddef.h>

/* The directory the executable was at during startup. */
extern char *vxfile_exec_dir;

VX_C_START

/* Gets the file contents from a given filename and optionally the file size. */
char *vxfile_read(const char *filename, size_t *optional_file_size);
/* Creates or overwrites the given file with the given contents and size. */
int vxfile_write(const char *filename, const void *contents, size_t contents_bytes);

/* Removes the given file. */
int vxfile_remove_file(const char *path);
/* Creates the given path. */
int vxfile_create_directory(const char *path);

/* Checks if a given directory exists. */
int vxfile_directory_exists(const char *path);
/* Checks if a given file exists. */
int vxfile_file_exists(const char *path);

/* Determines the canonical path of the executable.
   Stores result in first string and does not change it if an error occurred.
   Requires first argv string for path checks and as a fallback. */
char *vxfile_get_exec_path(char **result, char *argv_first);
/* Modifies the given string path to be that of the parent directory. */
char *vxfile_parent_from_path(char *path);

VX_C_END

#endif
