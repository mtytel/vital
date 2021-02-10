//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxlibrary.cpp
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

// instead of linking to a library, we just include the sources here to have
// full control over compile settings

#define I18N_LIB 1
#define PLUGIN_SDK_BUILD 1
#define DPA_PLUGIN_BUILD 1
#define INITACFIDS // Make sure all of the AVX2 uids are defined.
#define UNICODE 1

#ifdef _WIN32
#ifndef WIN32
#define WIN32 // for CMutex.cpp
#endif
#define WINDOWS_VERSION 1 // for AAXWrapper_GUI.h
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder"
#endif

#include "AAX_Atomic.h"

#include "../Interfaces/ACF/CACFClassFactory.cpp"
#include "../Libs/AAXLibrary/source/AAX_CACFUnknown.cpp"
#include "../Libs/AAXLibrary/source/AAX_CChunkDataParser.cpp"
#include "../Libs/AAXLibrary/source/AAX_VDescriptionHost.cpp"
#include "../Libs/AAXLibrary/source/AAX_CEffectDirectData.cpp"
#include "../Libs/AAXLibrary/source/AAX_CEffectGUI.cpp"
#include "../Libs/AAXLibrary/source/AAX_CEffectParameters.cpp"
#include "../Libs/AAXLibrary/source/AAX_CHostProcessor.cpp"
#include "../Libs/AAXLibrary/source/AAX_CHostServices.cpp"
#include "../Libs/AAXLibrary/source/AAX_CMutex.cpp"
#include "../Libs/AAXLibrary/source/AAX_CPacketDispatcher.cpp"
#include "../Libs/AAXLibrary/source/AAX_CParameter.cpp"
#include "../Libs/AAXLibrary/source/AAX_CParameterManager.cpp"
#include "../Libs/AAXLibrary/source/AAX_CString.cpp"
#include "../Libs/AAXLibrary/source/AAX_CUIDs.cpp"
#include "../Libs/AAXLibrary/source/AAX_CommonConversions.cpp"
#include "../Libs/AAXLibrary/source/AAX_IEffectDirectData.cpp"
#include "../Libs/AAXLibrary/source/AAX_IEffectGUI.cpp"
#include "../Libs/AAXLibrary/source/AAX_IEffectParameters.cpp"
#include "../Libs/AAXLibrary/source/AAX_IHostProcessor.cpp"
#include "../Libs/AAXLibrary/source/AAX_Init.cpp"
#include "../Libs/AAXLibrary/source/AAX_Properties.cpp"
#include "../Libs/AAXLibrary/source/AAX_VAutomationDelegate.cpp"
#include "../Libs/AAXLibrary/source/AAX_VCollection.cpp"
#include "../Libs/AAXLibrary/source/AAX_VComponentDescriptor.cpp"
#include "../Libs/AAXLibrary/source/AAX_VController.cpp"
#include "../Libs/AAXLibrary/source/AAX_VEffectDescriptor.cpp"
#include "../Libs/AAXLibrary/source/AAX_VFeatureInfo.cpp"
#include "../Libs/AAXLibrary/source/AAX_VHostProcessorDelegate.cpp"
#include "../Libs/AAXLibrary/source/AAX_VHostServices.cpp"
#include "../Libs/AAXLibrary/source/AAX_VPageTable.cpp"
#include "../Libs/AAXLibrary/source/AAX_VPrivateDataAccess.cpp"
#include "../Libs/AAXLibrary/source/AAX_VPropertyMap.cpp"
#include "../Libs/AAXLibrary/source/AAX_VTransport.cpp"
#include "../Libs/AAXLibrary/source/AAX_VViewContainer.cpp"

#ifdef _WIN32
#include "../Libs/AAXLibrary/source/AAX_CAutoreleasePool.Win.cpp"
#else
//#include "../Libs/AAXLibrary/source/AAX_CAutoreleasePool.OSX.mm"
#endif


#undef min
#undef max

// put at the very end, uses "using namespace std"
#include "../Libs/AAXLibrary/source/AAX_SliderConversions.cpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
