//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/thread/source/fcondition.cpp
// Created by  : Steinberg, 1995
// Description : signals...
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2018, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#define DEBUG_LOCK 0

#include "base/thread/include/fcondition.h"
#include "base/source/fdebug.h"

#include <limits.h>
#include <stdlib.h>

//------------------------------------------------------------------------
#if SMTG_PTHREADS
//------------------------------------------------------------------------
#if __MACH__
extern "C" {
#include <mach/clock.h>
#include <sched.h>
#include <mach/mach_init.h>
#include <mach/task.h>
#include <mach/task_policy.h>
#include <mach/thread_act.h>
#include <mach/semaphore.h>
#include <mach/thread_policy.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
}
#include <dlfcn.h>

#else
#include <fcntl.h>
#endif

#include <errno.h>

#if SMTG_OS_MACOS
#include <TargetConditionals.h>
#if !SMTG_OS_IOS
#include <CoreServices/CoreServices.h>
#endif
#endif

#include <sys/time.h>
//------------------------------------------------------------------------
#elif SMTG_OS_WINDOWS
#include <windows.h>
#endif

namespace Steinberg {
namespace Base {
namespace Thread {

//------------------------------------------------------------------------
/**	FCondition CTOR.
 *  @param name - can be used to set the name of the event.
 */
FCondition::FCondition (const char8* name)
{
#if SMTG_PTHREADS
	pthread_mutex_init (&mutex, 0);
	pthread_cond_init (&cond, 0);
	state = 0;
	waiters = 0;
#if DEVELOPMENT
	waits = 0;
	signalCount = 0;
#endif

#elif SMTG_OS_WINDOWS
	// use name if existing
	event = CreateEventA (0, FALSE, FALSE, name);

#endif
}

//------------------------------------------------------------------------
FCondition::~FCondition ()
{
#if SMTG_PTHREADS
	pthread_mutex_destroy (&mutex);
	pthread_cond_destroy (&cond);

#elif SMTG_OS_WINDOWS
	CloseHandle (event);

#endif
}

//------------------------------------------------------------------------
void FCondition::signal ()
{
#if SMTG_PTHREADS
	pthread_mutex_lock (&mutex);
	state = 1;
#if DEVELOPMENT
	signalCount++;
#endif
	pthread_cond_signal (&cond);
	pthread_mutex_unlock (&mutex);

#elif SMTG_OS_WINDOWS
	BOOL result = PulseEvent (event);
	if (!result)
	{
		SMTG_PRINTSYSERROR;
	}

#endif
}

//------------------------------------------------------------------------
void FCondition::signalAll ()
{
#if SMTG_PTHREADS
	pthread_mutex_lock (&mutex);
	state = waiters + 1;

#if DEVELOPMENT
	signalCount++;
#endif
	pthread_cond_broadcast (&cond);
	pthread_mutex_unlock (&mutex);

#elif SMTG_OS_WINDOWS
	BOOL result = SetEvent (event);
	if (!result)
	{
		SMTG_PRINTSYSERROR;
	}

#endif
}

//------------------------------------------------------------------------
void FCondition::wait ()
{
#if SMTG_PTHREADS
	pthread_mutex_lock (&mutex);
#if DEVELOPMENT
	waits++;
#endif
	waiters++;
	while (!state)
		pthread_cond_wait (&cond, &mutex);
	if (--waiters == 0)
		state = 0;
	else
		--state;
	pthread_mutex_unlock (&mutex);

#elif SMTG_OS_WINDOWS
	WaitForSingleObject (event, INFINITE);

#endif
}

//------------------------------------------------------------------------
bool FCondition::waitTimeout (int32 milliseconds)
{
#if SMTG_PTHREADS
	if (milliseconds == -1)
	{ // infinite timeout
		wait ();
		return true;
	}

	struct timespec time;

#if __MACH__
	time.tv_sec = milliseconds / 1000;
	time.tv_nsec = 1000000 * (milliseconds - (time.tv_sec * 1000));

	pthread_mutex_lock (&mutex);
#if DEVELOPMENT
	waits++;
#endif

	waiters++;

	bool result = true;
	while (!state)
	{
		int32 err = pthread_cond_timedwait_relative_np (&cond, &mutex, &time);
		if (err == ETIMEDOUT)
		{
			result = false;
			break;
		}
		else
			result = true;
	}
	if (--waiters == 0)
		state = 0;
	else
		--state;
	pthread_mutex_unlock (&mutex);
	return result;

#else
	clock_gettime (CLOCK_REALTIME, &time);
	time.tv_nsec += milliseconds * 1000; // ?????????

	pthread_mutex_lock (&mutex);
	bool result = false;
	if (pthread_cond_timedwait (&cond, &mutex, &time) == 0)
		result = true;
	pthread_mutex_unlock (&mutex);
	return result;

#endif

#elif SMTG_OS_WINDOWS
	if (milliseconds == -1)
		milliseconds = INFINITE;

	DWORD result = WaitForSingleObject (event, milliseconds);
	return result == WAIT_TIMEOUT ? false : true;

#endif

#if !SMTG_OS_WINDOWS
	//	WARNING ("Return false on time out not implemented!")
	return true;
#endif
}

//------------------------------------------------------------------------
void FCondition::reset ()
{
#if SMTG_OS_WINDOWS
	ResetEvent (event);
#elif SMTG_PTHREADS
	state = 0;
#endif
}

} // Thread
} // Base
} // Steinberg
