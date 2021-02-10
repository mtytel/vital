//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/test/ringbuffertest.h
// Created by  : Steinberg, 03/2018
// Description : Test ringbuffer
// Flags       : clang-format SMTGSequencer
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "base/source/fobject.h"
#include "pluginterfaces/test/itest.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
class RingBufferTest : public ITest, public FObject
{
public:
	RingBufferTest () = default;

	bool PLUGIN_API setup () SMTG_OVERRIDE;
	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;
	bool PLUGIN_API teardown () SMTG_OVERRIDE;
	const tchar* PLUGIN_API getDescription () SMTG_OVERRIDE;

	OBJ_METHODS (RingBufferTest, FObject)
	DEF_INTERFACES_1 (ITest, FObject)
	REFCOUNT_METHODS (FObject)
private:
	bool testPushUntilFull (ITestResult* testResult) const;
	bool testPopUntilEmpty (ITestResult* testResult) const;
	bool testRoundtrip (ITestResult* restResult) const;
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
