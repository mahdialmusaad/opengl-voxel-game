#include "utils/thread.h"

#include "directives/dword.h"

#include "io/logs.h"

#if VX_WINDOWS == 1

#include <windows.h>

#include "directives/dcast.h"

VX_THREAD_TYPE vxthr_create_thread(VX_THREAD_RETURN_TYPE (VX_THREAD_FUNCTION_CALL *function)(void *), void *thread_arg)
{
	HANDLE thread = CreateThread(VX_NULL, 0, VX_REINT_CAST(LPTHREAD_START_ROUTINE, function), thread_arg, 0, VX_NULL);
	if (!thread) return 0;
	return thread;
}
int vxthr_join_thread(VX_THREAD_TYPE thread)
{
	return WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0;
}
int vxthr_detach_thread(VX_THREAD_TYPE thread)
{
	return CloseHandle(thread) != 0;
}


int vxthr_exchange_init(vxthr_exchangeable *exch)
{
	exch->mutex = CreateMutex(VX_NULL, 0, VX_NULL);
	if (!exch->mutex) return 0;
	return 1;
}
int vxthr_exchange_lock(vxthr_exchangeable *exch, int blocking)
{
	VX_THREAD_PID_TYPE caller = GetCurrentThreadId();
	if (caller == exch->owner) return 1;

	DWORD result = WaitForSingleObject(exch->mutex, blocking ? INFINITE : 0u);
	exch->is_locked = result == WAIT_OBJECT_0;
	if (exch->is_locked) exch->owner = caller;

	return exch->is_locked;
}
int vxthr_exchange_unlock(vxthr_exchangeable *exch)
{
	if (exch->owner != GetCurrentThreadId()) { vxlog_msg(VX_LOG_WARNING_BIT, "Attempted to unlock from a different thread."); return 0; }
	exch->is_locked = ReleaseMutex(exch->mutex) == 0;
	exch->owner = 0u;
	return !exch->is_locked;
}
int vxthr_exchange_end(vxthr_exchangeable *exch)
{
	return CloseHandle(exch->mutex) != 0;
}


void vxthr_wait_milli(unsigned int milliseconds)
{
	Sleep(milliseconds);
}

size_t vxthr_get_memusage(VX_NO_ARG)
{
	MEMORYSTATUSEX mem_state;
	mem_state.dwLength = sizeof mem_state;
	GlobalMemoryStatusEx(&mem_state);
	return VX_CAST(size_t, mem_state.ullTotalPhys);
}

int vxthr_get_threads(VX_NO_ARG)
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return VX_CAST(int, info.dwNumberOfProcessors);
}

#else

#include <pthread.h>
#include <stdio.h>

VX_THREAD_TYPE vxthr_create_thread(VX_THREAD_RETURN_TYPE (VX_THREAD_FUNCTION_CALL *function)(void *), void *thread_arg)
{
	pthread_t thread;
	if (pthread_create(&thread, VX_NULL, function, thread_arg) != 0) return 0;
	return thread;
}
int vxthr_join_thread(VX_THREAD_TYPE thread)
{
	return pthread_join(thread, VX_NULL) == 0;
}
int vxthr_detach_thread(VX_THREAD_TYPE thread)
{
	return pthread_detach(thread) == 0;
}


int vxthr_exchange_init(vxthr_exchangeable *exch)
{
	if (pthread_mutex_init(&exch->mutex, VX_NULL) != 0) return 0;
	return 1;
}
int vxthr_exchange_lock(vxthr_exchangeable *exch, int blocking)
{
	exch->is_locked = 1;

	VX_THREAD_PID_TYPE caller = gettid();
	if (caller == exch->owner) return 1;

	int success;
	if (blocking) success = pthread_mutex_lock(&exch->mutex) == 0;
	else success = pthread_mutex_trylock(&exch->mutex) == 0;

	if (success) exch->owner = caller;
	return success;
}
int vxthr_exchange_unlock(vxthr_exchangeable *exch)
{
	if (exch->owner != gettid()) { vxlog_msg(VX_LOG_WARNING_BIT, "Attempted to unlock from a different thread."); return 0; }
	pthread_mutex_unlock(&exch->mutex);
	exch->is_locked = 0;
	exch->owner = 0;
	return 1;
}
int vxthr_exchange_end(vxthr_exchangeable *exch)
{
	return pthread_mutex_destroy(&exch->mutex) == 0;
}


void vxthr_wait_milli(unsigned int milliseconds)
{
	struct timespec target_time = { 0, milliseconds * 1000u * 1000u }, remaining;
	nanosleep(&target_time, &remaining);
}

size_t vxthr_get_memusage(VX_NO_ARG)
{
	FILE *mem_file = fopen("/proc/self/statm", "r");
	if (!mem_file) return 0u;
	size_t resident_kb;
	fscanf(mem_file, "%*s %zu", &resident_kb);
	fclose(mem_file);
	return resident_kb * 1024u;
}

#if defined (__linux__) || defined (__linux) || (defined(__APPLE__) && defined(__MACH__))
# define VX_SYSCONF
# include "directives/dcast.h"
#else
# include <sys/sysctl.h>
#endif

int vxthr_get_threads(VX_NO_ARG)
{
#if defined (VX_SYSCONF)
	return VX_CAST(int, sysconf(_SC_NPROCESSORS_ONLN));
#else
	int thr_count, mib[4] = {
		CTL_HW,
	#if defined (HW_NCPU)
		HW_NCPU;
	#else
		HW_AVAILCPU;
	#endif
		0, 0
	};

	size_t len = sizeof thr_count; 
	sysctl(mib, 2, &thr_count, &len, NULL, 0);
	return thr_count;
#endif
}

#endif
