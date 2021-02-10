//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxwrapper.h
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

#include "docaax.h"

/// \cond ignore

#pragma once

#include "public.sdk/source/vst/basewrapper/basewrapper.h"
#include "base/thread/include/flock.h"
#include <bitset>
#include <list>
#include <memory>

struct AAX_Plugin_Desc;
struct AAX_Effect_Desc;
class AAX_IComponentDescriptor;

class AAXWrapper_Parameters;
class AAXWrapper_GUI;

namespace Steinberg {
namespace Vst {
class IAudioProcessor;
class IEditController;
}
}

struct AAXWrapper_Context
{
	void* ptr[1]; // array of numDataPointers pointers
};

//------------------------------------------------------------------------
class AAXWrapper : public Steinberg::Vst::BaseWrapper,
                   public Steinberg::Vst::IComponentHandler2,
                   public Steinberg::Vst::IVst3ToAAXWrapper
{
public:
	// static creation method (will owned factory)
	static AAXWrapper* create (Steinberg::IPluginFactory* factory,
	                           const Steinberg::TUID vst3ComponentID, AAX_Plugin_Desc* desc,
	                           AAXWrapper_Parameters* p);

	AAXWrapper (Steinberg::Vst::BaseWrapper::SVST3Config& config, AAXWrapper_Parameters* p, AAX_Plugin_Desc* desc);
	~AAXWrapper ();

	//--- VST 3 Interfaces  ------------------------------------------------------
	// IHostApplication
	Steinberg::tresult PLUGIN_API getName (Steinberg::Vst::String128 name) SMTG_OVERRIDE;

	// IComponentHandler
	Steinberg::tresult PLUGIN_API beginEdit (Steinberg::Vst::ParamID tag) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API performEdit (
	    Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API endEdit (Steinberg::Vst::ParamID tag) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API restartComponent (Steinberg::int32 flags) SMTG_OVERRIDE;

	// IComponentHandler2
	Steinberg::tresult PLUGIN_API setDirty (Steinberg::TBool state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API requestOpenEditor (
	    Steinberg::FIDString name = Steinberg::Vst::ViewType::kEditor) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API startGroupEdit () SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API finishGroupEdit () SMTG_OVERRIDE;

	// FUnknown
	DEF_INTERFACES_2 (Steinberg::Vst::IComponentHandler2, Steinberg::Vst::IVst3ToAAXWrapper, BaseWrapper);
	REFCOUNT_METHODS (BaseWrapper);

	// AAXWrapper_Parameters callbacks
	void setGUI (AAXWrapper_GUI* gui) { aaxGUI = gui; }
	Steinberg::int32 /*AAX_Result*/ getParameterInfo (const char* aaxId,
	                                                  Steinberg::Vst::ParameterInfo& paramInfo);
	Steinberg::int32 /*AAX_Result*/ ResetFieldData (Steinberg::int32 index, void* inData,
	                                                Steinberg::uint32 inDataSize);
	Steinberg::int32 Process (AAXWrapper_Context* instance);

	Steinberg::int32 getNumMIDIports () const { return countMIDIports; }

	void setSideChainEnable (bool enable);
	bool generatePageTables (const char* outputFile);

	static void DescribeAlgorithmComponent (AAX_IComponentDescriptor* outDesc,
	                                        const AAX_Effect_Desc* desc,
	                                        const AAX_Plugin_Desc* pdesc);

	//--- ---------------------------------------------------------------------
	Steinberg::int32 getNumAAXOutputs () const { return aaxOutputs; }

	//------------------------------------------------------------------------
	// BaseWrapper overrides ---------------------------------
	//------------------------------------------------------------------------
	bool init () SMTG_OVERRIDE;
	bool _sizeWindow (Steinberg::int32 width, Steinberg::int32 height) SMTG_OVERRIDE;
	void onTimer (Steinberg::Timer* timer) SMTG_OVERRIDE;

	Steinberg::int32 _getChunk (void** data, bool isPreset) SMTG_OVERRIDE;
	Steinberg::int32 _setChunk (void* data, Steinberg::int32 byteSize, bool isPreset) SMTG_OVERRIDE;
	void setupProcessTimeInfo () SMTG_OVERRIDE;

//------------------------------------------------------------------------
private:
	void processOutputParametersChanges () SMTG_OVERRIDE;
	
	Steinberg::tresult setupBusArrangements (AAX_Plugin_Desc* desc);
	Steinberg::int32 countSidechainBusChannels (Steinberg::Vst::BusDirection dir,
	                                            Steinberg::uint64& scBusBitset);

	void guessActiveOutputs (float** out, Steinberg::int32 num);
	void updateActiveOutputState ();

	AAXWrapper_Parameters* aaxParams = nullptr;
	AAXWrapper_GUI* aaxGUI = nullptr;

	Steinberg::int32 aaxOutputs = 0;

	Steinberg::Base::Thread::FLock syncCalls; // synchronize calls expected in the same thread in VST3
	AAX_Plugin_Desc* pluginDesc = nullptr;
	Steinberg::int32 countMIDIports = 0;

	// as of ProTools 12 (?) the context struct does no longer allow unused slots,
	//  so we have to generate indices into the context struct dynamically
	// context pointer to AAXWrapper always first
	static const Steinberg::int32 idxContext = 0;
	static const Steinberg::int32 idxBufferSize = 1;
	Steinberg::int32 idxInputChannels = -1;
	Steinberg::int32 idxOutputChannels = -1;
	Steinberg::int32 idxSideChainInputChannels = -1;
	Steinberg::int32 idxMidiPorts = -1;
	Steinberg::int32 idxAuxOutputs = -1;
	Steinberg::int32 idxMeters = -1;
	Steinberg::int32 numDataPointers = 0;

	static const Steinberg::int32 maxActiveChannels = 128;
	std::bitset<maxActiveChannels> activeChannels;
	std::bitset<maxActiveChannels> propagatedChannels;

	Steinberg::int32 cntMeters = 0;
	std::unique_ptr<Steinberg::int32[]> meterIds;

	struct GetChunkMessage;
	void* mainThread = nullptr;
	Steinberg::Base::Thread::FLock msgQueueLock;
	std::list<GetChunkMessage*> msgQueue;
	bool wantsSetChunk = false;
	
	bool mSimulateBypass = false;
	bool mBypass = false;
	float mBypassGain = 1.0;
	float* mMetersTmp = nullptr;

	friend class AAXWrapper_Parameters;
	friend class AAXWrapper_GUI;
};
