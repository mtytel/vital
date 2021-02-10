//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/test/ringbuffertest.cpp
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

#include "public.sdk/source/vst/utility/test/ringbuffertest.h"
#include "public.sdk/source/vst/utility/ringbuffer.h"
#include "pluginterfaces/base/fstrdefs.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
bool RingBufferTest::testPushUntilFull (ITestResult* testResult) const
{
	OneReaderOneWriter::RingBuffer<uint32> rb (4);
	if (!rb.push (0))
		return false;
	if (!rb.push (1))
		return false;
	if (!rb.push (2))
		return false;
	if (!rb.push (3))
		return false;
	if (!rb.push (4))
		return true;
	return false;
}

//------------------------------------------------------------------------
bool RingBufferTest::testPopUntilEmpty (ITestResult* testResult) const
{
	OneReaderOneWriter::RingBuffer<uint32> rb (4);
	if (!rb.push (0))
		return false;
	if (!rb.push (1))
		return false;
	if (!rb.push (2))
		return false;
	if (!rb.push (3))
		return false;

	uint32 value;
	if (!rb.pop (value) || value != 0)
		return false;
	if (!rb.pop (value) || value != 1)
		return false;
	if (!rb.pop (value) || value != 2)
		return false;
	if (!rb.pop (value) || value != 3)
		return false;
	if (!rb.pop (value))
		return true;

	return false;
}

//------------------------------------------------------------------------
bool RingBufferTest::testRoundtrip (ITestResult* restResult) const
{
	OneReaderOneWriter::RingBuffer<uint32> rb (2);
	uint32 value;

	for (auto i = 0u; i < rb.size () * 2; ++i)
	{
		if (!rb.push (i))
			return false;
		if (!rb.pop (value) || value != i)
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool PLUGIN_API RingBufferTest::setup ()
{
	return true;
}

//------------------------------------------------------------------------
bool PLUGIN_API RingBufferTest::run (ITestResult* testResult)
{
	auto failed = !testPushUntilFull (testResult);
	failed |= !testPopUntilEmpty (testResult);
	failed |= !testRoundtrip (testResult);
	return !failed;
}

//------------------------------------------------------------------------
bool PLUGIN_API RingBufferTest::teardown ()
{
	return true;
}

//------------------------------------------------------------------------
const tchar* PLUGIN_API RingBufferTest::getDescription ()
{
	return STR ("RingBuffer Tests");
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
