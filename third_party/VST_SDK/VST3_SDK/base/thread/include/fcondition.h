//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/thread/include/fcondition.h
// Created by  : Steinberg, 1995
// Description : the threads and locks and signals...
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

//----------------------------------------------------------------------------------
/** @file base/thread/include/fcondition.h
	signal. */
/** @defgroup baseThread Thread Handling */
//----------------------------------------------------------------------------------
#pragma once

#include "pluginterfaces/base/ftypes.h"

#if SMTG_PTHREADS 
#include <pthread.h>
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace Base {
namespace Thread {

//------------------------------------------------------------------------
/**	FCondition - wraps the signal and wait calls in win32
@ingroup baseThread	*/
//------------------------------------------------------------------------
class FCondition
{
public:
//------------------------------------------------------------------------

	/** Condition constructor.
	 *  @param name name of condition
	 */
	FCondition (const char8* name = nullptr /* "FCondition" */ );

	/** Condition destructor.
	 */
	~FCondition ();
	
	/** Signals one thread.
	 */
	void signal ();

	/** Signals all threads.
	 */
	void signalAll ();

	/** Waits for condition.
	 */
	void wait ();

	/** Waits for condition with timeout
	 *  @param timeout time out in milliseconds
	 *  @return false if timed out
	 */
	bool waitTimeout (int32 timeout = -1);

	/** Resets condition.
	 */
	void reset ();

#if SMTG_OS_WINDOWS
	/** Gets condition handle.
	 *  @return handle
	 */
	void* getHandle () { return event; }
#endif

//------------------------------------------------------------------------
private:
#if SMTG_PTHREADS
	pthread_mutex_t mutex;		///< Mutex object
	pthread_cond_t cond;		///< Condition object
	int32 state;				///< Use to hold the state of the signal
	int32 waiters;				///< Number of waiters

	#if DEVELOPMENT
	int32     waits;			///< Waits count
	int32     signalCount;		///< Signals count
	#endif
#elif SMTG_OS_WINDOWS
	void* event;				///< Event handle
#endif
};

} // Thread
} // Base
} // Steinberg

