//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxwrapper_description.h
// Created by  : Steinberg, 08/2017
// Description : VST 3 -> AAX Wrapper
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

/// \cond ignore

#pragma once

#include "base/source/fstring.h"

using namespace Steinberg;

struct AAX_Aux_Desc
{
	const char* mName;
	int32 mChannels; // -1 for same as output channel
};

struct AAX_Meter_Desc
{
	const char* mName;
	uint32 mID;
	uint32 mOrientation; // see AAX_EMeterOrientation
	uint32 mType; // see AAX_EMeterType
};

struct AAX_MIDI_Desc
{
	const char* mName;
	uint32 mMask;
};

struct AAX_Plugin_Desc
{
	const char* mEffectID; // unique for each channel layout as in "com.steinberg.aaxwrapper.mono"
	const char* mName;
	uint32 mPlugInIDNative; // unique for each channel layout
	uint32 mPlugInIDAudioSuite; // unique for each channel layout

	int32 mInputChannels;
	int32 mOutputChannels;
	int32 mSideChainInputChannels;

	AAX_MIDI_Desc* mMIDIports;
	AAX_Aux_Desc* mAuxOutputChannels; // zero terminated
	AAX_Meter_Desc* mMeters;

	uint32 mLatency;
};

struct AAX_Effect_Desc
{
	const char* mManufacturer;
	const char* mProduct;

	uint32 mManufacturerID;
	uint32 mProductID;
	const char* mCategory;
	TUID mVST3PluginID;
	uint32 mVersion;

	const char* mPageFile;

	AAX_Plugin_Desc* mPluginDesc;
};

// reference this in the Plug-In to force inclusion of the wrapper in the link
extern int AAXWrapper_linkAnchor;

AAX_Effect_Desc* AAXWrapper_GetDescription (); // to be defined by the Plug-In
