//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vst2wrapper/docvst2.h
// Created by  : Steinberg, 01/2009
// Description : VST 3 -> VST 2 Wrapper
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

/**
**************************************
\page vst2xwrapper VST 3 - VST 2.x Wrapper
**************************************

Helper Class wrapping a VST 3 Plug-in to a VST 2.4 Plug-in

***************************
\section VST2Introduction Introduction
***************************
The VST 3 SDK comes with a helper class which wraps one VST 3 Audio Processor and Edit Controller to
a VST 2.x Plug-in.
\n\n
***************************
\section VST2howdoesitwork How does it work?
***************************
You just need to add public.sdk/source/vst/vst2wrapper/vst2wrapper.sdk.cpp to your project and add
the following code somewhere in your sources:
\code

#include "public.sdk/source/vst/vst2wrapper/vst2wrapper.h"

//------------------------------------------------------------------------
::AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return Steinberg::Vst::Vst2Wrapper::create (GetPluginFactory (), kAudioProcessorCID, kVst2UniqueID, audioMaster);
}

\endcode
 */
 