#pragma once
#ifndef SOURCE_UTILS_THREAD_VXL_HDR
#define SOURCE_UTILS_THREAD_VXL_HDR
/* Portable threading functions. */

#include "directives/dextern.h"
#include "directives/dos.h"

#if VX_WINDOWS == 1
#include <windows.h>
# define VX_THREAD_RETURN_TYPE DWORD
# define VX_THREAD_MUTEX_TYPE HANDLE
# define VX_THREAD_FUNCTION_CALL WINAPI
# define VX_THREAD_PID_TYPE DWORD
# define VX_THREAD_TYPE HANDLE
#else
# include <pthread.h>
# include <unistd.h>
# define VX_THREAD_RETURN_TYPE void *
# define VX_THREAD_MUTEX_TYPE pthread_mutex_t
# define VX_THREAD_FUNCTION_CALL
# define VX_THREAD_PID_TYPE pid_t
# define VX_THREAD_TYPE pthread_t
#endif

#define VX_THREAD_RETURN_VALUE ((VX_THREAD_RETURN_TYPE)(0))
#define VX_THREAD_FUNCTION(name) VX_THREAD_RETURN_TYPE VX_THREAD_FUNCTION_CALL name(void *thread_arg)

VX_C_START

/* Create a thread with the given function and argument. */
VX_THREAD_TYPE vxthr_create_thread(VX_THREAD_RETURN_TYPE (VX_THREAD_FUNCTION_CALL *function)(void *), void *thread_arg);
/* Wait for the given thread to complete. */
int vxthr_join_thread(VX_THREAD_TYPE thread);
/* Detach the given thread. */
int vxthr_detach_thread(VX_THREAD_TYPE thread);

/* Thread mutex object. */
typedef struct {
	VX_THREAD_MUTEX_TYPE mutex;
	VX_THREAD_PID_TYPE owner;
	int is_locked;
} vxthr_exchangeable;

/* Initialize the given thread exchange object. */
int vxthr_exchange_init(vxthr_exchangeable *exch);
/* Unlock the thread exchange object for use by other threads. */
int vxthr_exchange_unlock(vxthr_exchangeable *exch);
/* Lock the exchange object for the current thread to use. If blocking is not
   specified, this will have to be repeatedly called until it returns 1. */
int vxthr_exchange_lock(vxthr_exchangeable *exch, int blocking);
/* Clean exchange object at the end of use. */
int vxthr_exchange_end(vxthr_exchangeable *exch);

/* Waits for the specific number of milliseconds. */
void vxthr_wait_milli(unsigned int milliseconds);
/* Get memory usage in bytes. */
size_t vxthr_get_memusage(VX_NO_ARG);
/* Number of threads available. */
int vxthr_get_threads(VX_NO_ARG);

VX_C_END

#endif
