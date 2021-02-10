//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxwrapper.cpp
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

#include "aaxwrapper.h"

#include "AAX.h"
#include "AAX_Assert.h"
#include "AAX_Errors.h"
#include "AAX_ICollection.h"
#include "AAX_IComponentDescriptor.h"
#include "AAX_IEffectDescriptor.h"
#include "AAX_IMIDINode.h"
#include "AAX_IPropertyMap.h"
#include "AAX_IViewContainer.h"

#include "aaxwrapper_description.h"
#include "aaxwrapper_gui.h"
#include "aaxwrapper_parameters.h"

#include "base/thread/include/fcondition.h"

#define USE_TRACE 1

#if USE_TRACE
#define HAPI AAX_eTracePriorityHost_Normal
#define HLOG AAX_TRACE

#else
#define HAPI 0
#if SMTG_OS_WINDOWS
#define HLOG __noop
#else
#define HLOG(...) \
	do            \
	{             \
	} while (false)
#endif
#endif

#if SMTG_OS_WINDOWS
#include <windows.h>
#define getCurrentThread() ((void*)(size_t)GetCurrentThreadId ())
#else
#include <pthread.h>
#define getCurrentThread() (pthread_self ())
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace Steinberg::Base::Thread;

//------------------------------------------------------------------------
//------------------------------------------------------------------------
AAXWrapper::AAXWrapper (BaseWrapper::SVST3Config& config, AAXWrapper_Parameters* p, AAX_Plugin_Desc* desc)
: BaseWrapper (config), aaxParams (p), pluginDesc (desc)
{
	HLOG (HAPI, "%s", __FUNCTION__);

	mUseExportedBypass = true; 
	mUseIncIndex = false;

	mainThread = getCurrentThread ();

	// must be in lock step with DescribeAlgorithmComponent
	int32 idx = idxBufferSize + 1;
	if (desc->mInputChannels || desc->mOutputChannels)
		idxInputChannels = idx++;
	if (desc->mOutputChannels)
		idxOutputChannels = idx++;
	if (desc->mSideChainInputChannels)
		idxSideChainInputChannels = idx++;

	if (desc->mMIDIports)
	{
		for (AAX_MIDI_Desc* mdesc = desc->mMIDIports; mdesc->mName; mdesc++)
			countMIDIports++;

		if (countMIDIports > 0)
		{
			idxMidiPorts = idx;
			idx += countMIDIports;
		}
	}

	int32 numAuxOutputs = 0;
	aaxOutputs = desc->mOutputChannels;
	if (desc->mAuxOutputChannels)
	{
		for (AAX_Aux_Desc *adesc = desc->mAuxOutputChannels; adesc->mName; adesc++, numAuxOutputs++)
			aaxOutputs += adesc->mChannels < 0 ? desc->mOutputChannels : adesc->mChannels;

		if (numAuxOutputs > 0)
		{
			idxAuxOutputs = idx;
			idx += numAuxOutputs;
		}
	}

	if (desc->mMeters)
	{
		idxMeters = idx++;

		for (auto mdesc = desc->mMeters; mdesc->mName; mdesc++)
			cntMeters++;
		meterIds.reset (new Steinberg::int32 (cntMeters));
		cntMeters = 0;
		for (auto mdesc = desc->mMeters; mdesc->mName; mdesc++)
			meterIds[cntMeters++] = mdesc->mID;
	}
	
	numDataPointers = idx;
}

//------------------------------------------------------------------------
AAXWrapper::~AAXWrapper ()
{
	HLOG (HAPI, "%s", __FUNCTION__);
}

//------------------------------------------------------------------------
tresult PLUGIN_API AAXWrapper::getName (String128 name)
{
	String str ("AAXWrapper");
	str.copyTo16 (name, 0, 127);
	return kResultTrue;
}

//------------------------------------------------------------------------
Vst::ParamID getVstParamID (AAX_CParamID aaxid)
{
	if (aaxid[0] != 'p')
		return -1;
	Vst::ParamID id;
	if (sscanf (aaxid + 1, "%x", &id) != 1)
		return -1;
	return id;
}

//------------------------------------------------------------------------
// IComponentHandler
//------------------------------------------------------------------------
tresult PLUGIN_API AAXWrapper::beginEdit (ParamID tag)
{
	HLOG (HAPI, "%s(tag=%x)", __FUNCTION__, tag);

	AAX_CID aaxid (tag);
	aaxParams->TouchParameter (aaxid);
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AAXWrapper::performEdit (ParamID tag, ParamValue valueNormalized)
{
	HLOG (HAPI, "%s(tag=%x, value=%lf)", __FUNCTION__, tag, valueNormalized);

	AAX_CID aaxid (tag);
	aaxParams->SetParameterNormalizedValue (aaxid, valueNormalized);
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AAXWrapper::endEdit (ParamID tag)
{
	HLOG (HAPI, "%s(tag=%x)", __FUNCTION__, tag);

	AAX_CID aaxid (tag);
	aaxParams->ReleaseParameter (aaxid);
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AAXWrapper::setDirty (TBool state)
{
	aaxParams->setDirty (state != 0);
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AAXWrapper::requestOpenEditor (FIDString name)
{
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AAXWrapper::startGroupEdit ()
{
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AAXWrapper::finishGroupEdit ()
{
	return kResultFalse;
}

//------------------------------------------------------------------------
bool AAXWrapper::init ()
{
	bool res = BaseWrapper::init ();

	if (mController && BaseEditorWrapper::hasEditor (mController))
		_setEditor (new BaseEditorWrapper (mController));

	return res;
}

//------------------------------------------------------------------------
void AAXWrapper::setupProcessTimeInfo ()
{
	mProcessContext.state = 0;
	mProcessContext.sampleRate = mSampleRate;

	if (AAX_ITransport* transport = aaxParams->Transport ())
	{
		int64_t splPos, ppqPos, loopStart, loopEnd;
		bool playing, looping;
		if (transport->GetCurrentNativeSampleLocation (&splPos) == AAX_SUCCESS)
			mProcessContext.projectTimeSamples = (TSamples)splPos;

		if (transport->GetCurrentTickPosition (&ppqPos) == AAX_SUCCESS)
		{
			mProcessContext.projectTimeMusic = ppqPos / 960000.0;
			mProcessContext.state |= ProcessContext::kProjectTimeMusicValid;
		}
		else
			mProcessContext.projectTimeMusic = 0;

		if (transport->GetCurrentTempo (&mProcessContext.tempo) == AAX_SUCCESS)
			mProcessContext.state |= ProcessContext::kTempoValid;

		if (transport->GetCurrentLoopPosition (&looping, &loopStart, &loopEnd) == AAX_SUCCESS)
		{
			mProcessContext.cycleStartMusic = loopStart / 960000.0;
			mProcessContext.cycleEndMusic = loopEnd / 960000.0;
			mProcessContext.state |= ProcessContext::kCycleValid;
			if (looping)
				mProcessContext.state |= ProcessContext::kCycleActive;
		}
		int32_t num, den;
		if (transport->GetCurrentMeter (&num, &den) == AAX_SUCCESS)
		{
			mProcessContext.timeSigNumerator = num;
			mProcessContext.timeSigDenominator = den;
			mProcessContext.state |= ProcessContext::kTimeSigValid;
		}
		else
			mProcessContext.timeSigNumerator = mProcessContext.timeSigDenominator = 4;

		if (transport->IsTransportPlaying (&playing) == AAX_SUCCESS)
		{
			mProcessContext.state |= (playing ? ProcessContext::kPlaying : 0);
		}
		mProcessData.processContext = &mProcessContext;
	}
	else
		mProcessData.processContext = nullptr;
}

//------------------------------------------------------------------------
bool AAXWrapper::_sizeWindow (int32 width, int32 height)
{
	HLOG (HAPI, "%s(width=%x, height=%x)", __FUNCTION__, width, height);

	AAX_ASSERT (mainThread == getCurrentThread ());

	if (aaxGUI)
	{
		if (AAX_IViewContainer* vc = aaxGUI->GetViewContainer ())
		{
			AAX_Point inSize (height, width);
			if (vc->SetViewSize (inSize) == AAX_SUCCESS)
			{
				return true;
			}
		}
	}
	return false;
}

//------------------------------------------------------------------------
struct AAXWrapper::GetChunkMessage : public FCondition
{
	void* mData = nullptr;
	int32 mDataSize = 0;
	int32 mResult = 0;
};

//------------------------------------------------------------------------
int32 AAXWrapper::_getChunk (void** data, bool isPreset)
{
	if (wantsSetChunk)
	{
		// isPreset is always false for AAX, so we can ignore it
		*data = mChunk.getData ();
		return mChunk.getSize ();
	}
	if (mainThread == getCurrentThread ())
		return BaseWrapper::_getChunk (data, isPreset);

	auto* msg = NEW GetChunkMessage;
	{
		FGuard guard (msgQueueLock);
		msgQueue.push_back (msg);
	}
	msg->wait ();

	*data = msg->mData;
	return msg->mResult;
}

//------------------------------------------------------------------------
int32 AAXWrapper::_setChunk (void* data, int32 byteSize, bool isPreset)
{
	if (mainThread == getCurrentThread ())
		return BaseWrapper::_setChunk (data, byteSize, isPreset);

	FGuard guard (msgQueueLock);
	mChunk.setSize (byteSize);
	memcpy (mChunk.getData (), data, byteSize);
	wantsSetChunk = true;
	return 0;
}

//------------------------------------------------------------------------
void AAXWrapper::onTimer (Timer* timer)
{
	BaseWrapper::onTimer (timer);

	AAX_ASSERT (mainThread == getCurrentThread ());

	if (wantsSetChunk)
	{
		FGuard guard (msgQueueLock);
		BaseWrapper::_setChunk (mChunk.getData (), mChunk.getSize (), false);
		wantsSetChunk = false;
	}

	updateActiveOutputState ();

	while (!msgQueue.empty ())
	{
		GetChunkMessage* msg = nullptr;
		{
			FGuard guard (msgQueueLock);
			if (!msgQueue.empty ())
			{
				msg = msgQueue.front ();
				msgQueue.pop_front ();
			}
		}
		if (msg)
		{
			msg->mResult = BaseWrapper::_getChunk (&msg->mData, false);
			msg->signal ();
		}
	}
}

//------------------------------------------------------------------------
int32 AAXWrapper::getParameterInfo (AAX_CParamID aaxId, Vst::ParameterInfo& paramInfo)
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, aaxId);

	Vst::ParamID id = getVstParamID (aaxId);
	if (id == -1)
		return AAX_ERROR_INVALID_PARAMETER_ID;

	std::map<ParamID, int32>::iterator iter = mParamIndexMap.find (id);
	if (iter == mParamIndexMap.end ())
		return AAX_ERROR_INVALID_PARAMETER_ID;

	if (mController->getParameterInfo (iter->second, paramInfo) != kResultTrue)
		return AAX_ERROR_INVALID_PARAMETER_ID;

	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
bool AAXWrapper::generatePageTables (const char* outputFile)
{
#if 0 && DEVELOPMENT
	FILE* fh = fopen (outputFile, "w");
	if (!fh)
		return false;

	fprintf (fh, "<?xml version='1.0' encoding='US-ASCII' standalone='yes'?>\n"
				 "<PageTables vers='6.4.0.89'>\n"
					"\t<PageTableLayouts>\n");

	AAX_Effect_Desc* effDesc = AAXWrapper_GetDescription ();
	for (const AAX_Plugin_Desc* pdesc = effDesc->mPluginDesc; pdesc->mEffectID; pdesc++)
	{
		uint32_t manId = effDesc->mManufacturerID; SWAP_32 (manId);
		uint32_t prodId = effDesc->mProductID; SWAP_32 (prodId);
		uint32_t plugId = pdesc->mPlugInIDNative; SWAP_32 (plugId);
		fprintf (fh,	"\t\t<Plugin manID='%4.4s' prodID='%4.4s' plugID='%4.4s'>\n"
							"\t\t\t<Desc>%s</Desc>\n"
							"\t\t\t<Layout>PageTable 1</Layout>\n"
							"\t\t</Plugin>",
				&manId, &prodId, &plugId, pdesc->mName);
		fprintf (fh,		"\t\t\t<PageTable type='PgTL' pgsz='1'>\n");

		int32 cntParams = controller->getParameterCount ();
		for (int32 i = 0; i < cntParams; i++)
		{
			Vst::ParameterInfo info;
			if (controller->getParameterInfo (i, info) == kResultTrue &&
				(info.flags & ParameterInfo::kCanAutomate))
			{
				AAX_CID id (info.id);
				String stitle (info.shortTitle);
				fprintf (fh,	"\t\t\t\t<Page num='%d'><ID>%s</ID></Page><!-- %s -->\n", i + 1, (const char*) id, stitle.text8 ());
			}
		}
		fprintf (fh, "\t\t\t</PageTable>\n");
	}

	fprintf (fh, "\t</PageTableLayouts>\n");
	fprintf (fh, "\t</PageTableLayouts>\n");
	fprintf (fh, "\t<Editor vers='1.1.0.1'>\n"
					"\t\t<PluginList>\n"
						"\t\t\t<RTAS>\n");
	for (const AAX_Plugin_Desc* pdesc = effDesc->mPluginDesc; pdesc->mEffectID; pdesc++)
	{
		uint32_t manId = effDesc->mManufacturerID; SWAP_32 (manId);
		uint32_t prodId = effDesc->mProductID; SWAP_32 (prodId);
		uint32_t plugId = pdesc->mPlugInIDNative; SWAP_32 (plugId);
		fprintf (fh,	"\t\t\t\t<PluginID manID='%4.4s' prodID='%4.4s' plugID='%4.4s'>\n"
							"\t\t\t\t\t<MenuStr>%s</MenuStr>\n"
						"\t\t\t\t</PluginID>\n",
				&manId, &prodId, &plugId, pdesc->mName);
	}
	fprintf (fh,	"\t\t\t</RTAS>\n"
				"\t\t</PluginList>\n"
			"\t</Editor>\n");
	fprintf (fh, "</PageTables>\n");

	fclose (fh);
#endif
	return true;
}

//------------------------------------------------------------------------
static int32 getChannelsStem (int32 channels)
{
	switch (channels)
	{
		case 1: return AAX_eStemFormat_Mono;
		case 2: return AAX_eStemFormat_Stereo;
		case 3: return AAX_eStemFormat_LCR;
		case 4:
			return AAX_eStemFormat_Ambi_1_ACN; //  AAX_eStemFormat_Quad
		case 5: return AAX_eStemFormat_5_0;
		case 6:
			return AAX_eStemFormat_5_1; // AAX_eStemFormat_6_0
		case 7:
			return AAX_eStemFormat_6_1; // AAX_eStemFormat_7_0_DTS
		case 8: return AAX_eStemFormat_7_1_DTS;
		case 9:
			return AAX_eStemFormat_Ambi_2_ACN; // AAX_eStemFormat_7_0_2;
		case 10: return AAX_eStemFormat_7_1_2;
		case 16: return AAX_eStemFormat_Ambi_3_ACN;
	}
	return AAX_eStemFormat_None;
}

//-------------------------------------------------------------------------------
static SpeakerArrangement numChannelsToSpeakerArrangement (int32 numChannels)
{
	switch (numChannels)
	{
		case 1: return SpeakerArr::kMono;
		case 2: return SpeakerArr::kStereo;
		case 3: return SpeakerArr::k30Cine;
		case 4: return SpeakerArr::kAmbi1stOrderACN;
		case 5: return SpeakerArr::k50;
		case 6: return SpeakerArr::k51;
		case 7: return SpeakerArr::k61Cine;
		case 8: return SpeakerArr::k71Cine;
		case 9: return SpeakerArr::kAmbi2cdOrderACN;
		case 10: return SpeakerArr::k71_2;
		case 16: return SpeakerArr::kAmbi3rdOrderACN;
	}
	return 0;
}

//------------------------------------------------------------------------
// static
//------------------------------------------------------------------------
AAXWrapper* AAXWrapper::create (IPluginFactory* factory, const TUID vst3ComponentID,
                                AAX_Plugin_Desc* desc, AAXWrapper_Parameters* params)
{
	// mostly a copy of BaseWrapper::create
	if (!factory)
		return nullptr;

	BaseWrapper::SVST3Config config;
	config.processor = nullptr;
	config.factory = factory;

	FReleaser factoryReleaser (factory);

	factory->createInstance (vst3ComponentID, IAudioProcessor::iid, (void**)&config.processor);
	if (!config.processor)
		return nullptr;

	config.controller = nullptr;
	if (config.processor->queryInterface (IEditController::iid, (void**)&config.controller) != kResultTrue)
	{
		FUnknownPtr<IComponent> component (config.processor);
		if (component)
		{
			TUID editorCID;
			if (component->getControllerClassId (editorCID) == kResultTrue)
			{
				factory->createInstance (editorCID, IEditController::iid, (void**)&config.controller);
			}
		}
	}
	config.vst3ComponentID = FUID::fromTUID (vst3ComponentID);

	auto* wrapper = new AAXWrapper (config, params, desc);
	if (wrapper->init () == false || wrapper->setupBusArrangements (desc) != kResultOk)
	{
		wrapper->release ();
		return nullptr;
	}
	wrapper->setupBuses (); // again to adjust to changes done by setupBusArrangements

	// vstwrapper ignores side chain channels, pretend they are main inputs
	uint64 scBusChannels;
	wrapper->countSidechainBusChannels (kInput, scBusChannels);
	wrapper->mMainAudioInputBuses |= scBusChannels;

	FUnknownPtr<IPluginFactory2> factory2 (factory);
	if (factory2)
	{
		PFactoryInfo factoryInfo;
		if (factory2->getFactoryInfo (&factoryInfo) == kResultTrue)
			wrapper->setVendorName (factoryInfo.vendor);

		for (int32 i = 0; i < factory2->countClasses (); i++)
		{
			Steinberg::PClassInfo2 classInfo2;
			if (factory2->getClassInfo2 (i, &classInfo2) == Steinberg::kResultTrue)
			{
				if (memcmp (classInfo2.cid, vst3ComponentID, sizeof (TUID)) == 0)
				{
					wrapper->setSubCategories (classInfo2.subCategories);
					wrapper->setEffectName (classInfo2.name);

					if (classInfo2.vendor[0] != 0)
						wrapper->setVendorName (classInfo2.vendor);

					break;
				}
			}
		}
	}

	return wrapper;
}

//-------------------------------------------------------------------------------
int32 AAXWrapper::countSidechainBusChannels (BusDirection dir, uint64& scBusBitset)
{
	int32 result = 0;
	scBusBitset = 0;

	int32 busCount = mComponent->getBusCount (kAudio, dir);
	for (int32 i = 0; i < busCount; i++)
	{
		BusInfo busInfo = {0};
		if (mComponent->getBusInfo (kAudio, dir, i, busInfo) == kResultTrue)
		{
			if (busInfo.busType == kAux)
			{
				result += busInfo.channelCount;
				scBusBitset |= (uint64 (1) << i);
				// no longer activate side chains by default, use the host
				// notifications instead
				// mComponent->activateBus (kAudio, dir, i, true);
			}
		}
	}
	return result;
}

//------------------------------------------------------------------------
tresult AAXWrapper::setupBusArrangements (AAX_Plugin_Desc* desc)
{
	int32 inputBusCount =
	    (desc->mInputChannels > 0 ? 1 : 0) + (desc->mSideChainInputChannels > 0 ? 1 : 0);
	int32 outputBusCount = (desc->mOutputChannels > 0 ? 1 : 0);

	if (AAX_Aux_Desc* aux = desc->mAuxOutputChannels)
		while ((aux++)->mName)
			outputBusCount++;

	std::vector<SpeakerArrangement> inputs (inputBusCount);
	std::vector<SpeakerArrangement> outputs (outputBusCount);

	int32 in = 0;
	if (desc->mInputChannels)
		inputs[in++] = numChannelsToSpeakerArrangement (desc->mInputChannels);
	if (desc->mSideChainInputChannels)
		inputs[in++] = numChannelsToSpeakerArrangement (desc->mSideChainInputChannels);

	if (desc->mOutputChannels)
		outputs[0] = numChannelsToSpeakerArrangement (desc->mOutputChannels);

	if (AAX_Aux_Desc* aux = desc->mAuxOutputChannels)
		for (int32 i = 0; aux[i].mName; i++)
			outputs[i + 1] = numChannelsToSpeakerArrangement (aux[i].mChannels);

	return mProcessor->setBusArrangements (inputs.data (), inputBusCount, outputs.data (),
	                                       outputBusCount);
}

//------------------------------------------------------------------------
void AAXWrapper::guessActiveOutputs (float** out, int32 num)
{
	// a channel is considered inactive if the output pointer is to the
	// same location as one of it's neighboring channels (ProTools seems
	// to direct all inactive channels to the same output).
	// FIXME: this heuristic fails for mono outputs
	std::bitset<maxActiveChannels> active;
	for (int32 i = 0; i < num; i++)
	{
		auto prev = (i > 0 ? out[i - 1] : nullptr);
		auto next = (i + 1 < num ? out[i + 1] : nullptr);
		active[i] = (out[i] != prev && out[i] != next);
	}
	activeChannels = active;
}

//------------------------------------------------------------------------
void AAXWrapper::updateActiveOutputState ()
{
	// some additional copying to avoid missing updates
	std::bitset<maxActiveChannels> channels = activeChannels;
	if (channels == propagatedChannels)
		return;
	propagatedChannels = channels;

	int32 channelPos = 0;
	int32 busCount = mComponent->getBusCount (kAudio, kOutput);
	for (int32 i = 0; i < busCount; i++)
	{
		BusInfo busInfo = {0};
		if (mComponent->getBusInfo (kAudio, kOutput, i, busInfo) == kResultTrue)
		{
			bool active = false;
			for (int32 c = 0; c < busInfo.channelCount; c++)
				if (channels[channelPos + c])
					active = true;

			channelPos += busInfo.channelCount;
			mComponent->activateBus (kAudio, kOutput, i, active);
		}
	}
}

//------------------------------------------------------------------------
void AAXWrapper::setSideChainEnable (bool enable)
{
	int32 busCount = mComponent->getBusCount (kAudio, kInput);
	for (int32 i = 0; i < busCount; i++)
	{
		BusInfo busInfo = {0};
		if (mComponent->getBusInfo (kAudio, kInput, i, busInfo) == kResultTrue)
		{
			if (busInfo.busType == kAux)
			{
				mComponent->activateBus (kAudio, kInput, i, enable);
				break;
			}
		}
	}
}

//------------------------------------------------------------------------
// Context structure
//------------------------------------------------------------------------

//------------------------------------------------------------------------
using fnCreateParameters = AAX_IEffectParameters* AAX_CALLBACK ();

template <int32_t pluginIndex>
struct CP
{
	static AAX_IEffectParameters* AAX_CALLBACK Create_Parameters ()
	{
		auto p = new AAXWrapper_Parameters (pluginIndex);
		if (!p->getWrapper ())
		{
			delete p;
			p = nullptr;
		}
		return p;
	}
};

//------------------------------------------------------------------------
AAX_IEffectGUI* AAX_CALLBACK Create_GUI ()
{
	return new AAXWrapper_GUI ();
}

//------------------------------------------------------------------------
int32_t AAX_CALLBACK AlgorithmInitFunction (const AAXWrapper_Context* inInstance,
                                            AAX_EComponentInstanceInitAction inAction)
{
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
Steinberg::int32 AAXWrapper::ResetFieldData (Steinberg::int32 index, void* inData,
                                             Steinberg::uint32 inDataSize)
{
	if (index == idxContext && inDataSize == sizeof (AAXWrapper*))
	{
		_suspend ();
		_resume ();
		*(AAXWrapper**)inData = this;
	}
	else
		// Default implementation is just to zero out all data.
		memset (inData, 0, inDataSize);
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
Steinberg::int32 AAXWrapper::Process (AAXWrapper_Context* instance)
{
	//--- ------ Retrieve instance-specific information ---------
	// Memory blocks
	const int32_t bufferSize = *static_cast<int32_t*> (instance->ptr[idxBufferSize]);
	AAX_ASSERT (bufferSize <= 1024);

	int32 cntMidiPorts = getNumMIDIports ();
	for (int32 m = 0; m < cntMidiPorts; m++)
	{
		auto* midiNode = static_cast<AAX_IMIDINode*> (instance->ptr[idxMidiPorts + m]);
		AAX_CMidiStream* midiBuffer = midiNode->GetNodeBuffer ();

		//- Check incoming MIDI packets ()
		//
		for (uint32_t i = 0; i < midiBuffer->mBufferSize; i++)
		{
			AAX_CMidiPacket& buf = midiBuffer->mBuffer[i];
			if (buf.mLength > 0)
			{
				Event toAdd = { m, static_cast<int32>(buf.mTimestamp), 0 };
				bool isLive = buf.mIsImmediate || buf.mTimestamp == 0;
				processMidiEvent (toAdd, (char*)&buf.mData[0], isLive);
			}
		}
	}

	auto pdI = idxInputChannels < 0 ?
	               nullptr :
	               static_cast<float**> (instance->ptr[idxInputChannels]); // First input
	float* inputs[16] = {nullptr};
	if (pdI && idxSideChainInputChannels >= 0)
	{
		if (auto psc = instance->ptr[idxSideChainInputChannels])
		{
			int32 scChannel = *static_cast<int32_t*> (psc);

			int32 idx = pluginDesc->mInputChannels;
			memcpy (inputs, pdI, idx * sizeof (inputs[0]));
			for (int32 i = 0; i < pluginDesc->mSideChainInputChannels; i++)
				inputs[idx + i] = pdI[scChannel];
			pdI = inputs;
		}
	}

	// First output
	float** const AAX_RESTRICT pdO = static_cast<float**> (instance->ptr[idxOutputChannels]);

	int32 cntOut = getNumOutputs ();
	int32 aaxOut = getNumAAXOutputs ();
	float* outputs[maxActiveChannels];
	int32 mainOuts = pluginDesc->mOutputChannels;
	if (mainOuts == 6)
	{
		// sort Surround channels from AAX (L C R Ls Rs LFE) to VST (L R C LFE Ls Rs)
		outputs[0] = pdO[0];
		outputs[1] = pdO[2];
		outputs[2] = pdO[1];
		outputs[3] = pdO[5];
		outputs[4] = pdO[3];
		outputs[5] = pdO[4];
	}
	else
		mainOuts = 0;
	for (int32 i = mainOuts; i < aaxOut; i++)
		outputs[i] = pdO[i];
	float buf[1024];
	for (int32 i = aaxOut; i < cntOut; i++)
		outputs[i] = buf;
	guessActiveOutputs (outputs, cntOut);

	mMetersTmp = (cntMeters > 0) ? *static_cast<float**> (instance->ptr[idxMeters]) : nullptr;

	_processReplacing (pdI, outputs, bufferSize);

	mMetersTmp = nullptr;

	// apply bypass if not supported ----------------------
	if (mSimulateBypass)
	{
		static const float kDiffGain = 0.001f;
		if (mBypass)
		{
			int32 bufPos = 0;
			while (mBypassGain > 0 && bufPos < bufferSize)
			{
				for (int32 i = 0; i < cntOut; i++)
					outputs[i][bufPos] *= mBypassGain;
				mBypassGain -= kDiffGain;
				bufPos++;
			}
			for (int32 i = 0; i < cntOut; i++)
				memset (&outputs[i][bufPos], 0, (bufferSize - bufPos) * sizeof (float));
		}
		else if (mBypassGain < 1)
		{
			int32 bufPos = 0;
			while (mBypassGain < 1 && bufPos < bufferSize)
			{
				for (int32 i = 0; i < cntOut; i++)
					outputs[i][bufPos] *= mBypassGain;
				mBypassGain += kDiffGain;
				bufPos++;
			}
		}
	}

	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
void AAXWrapper::processOutputParametersChanges ()
{
	if (!mMetersTmp)
		return;

	uint32 found = 0;

	// VU Meter readout
	for (uint32 i = 0, count = mOutputChanges.getParameterCount (); i < count; i++)
	{
		auto queue = mOutputChanges.getParameterData (i);
		if (!queue)
			break;
		for (int32 m = 0; m < cntMeters; m++)
		{
			if (meterIds[m] == queue->getParameterId ())
			{
				int32 sampleOffset;
				ParamValue value;
				queue->getPoint (queue->getPointCount () - 1, sampleOffset, value);
				mMetersTmp[m] = value;
				found++;
				break;
			}
		}
		if (found == cntMeters)
			break;
	}
}

//-----------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API AAXWrapper::restartComponent (Steinberg::int32 flags)
{
	tresult result = BaseWrapper::restartComponent (flags);

	//--- ----------------------
	if (flags & kLatencyChanged)
	{
		if (aaxParams && mProcessor)
		{
			AAX_IController* ctrler = aaxParams->Controller ();
			if (ctrler)
				ctrler->SetSignalLatency (mProcessor->getLatencySamples ());
		}
		result = kResultTrue;
	}
	return result;
}

//------------------------------------------------------------------------
static void AAX_CALLBACK AlgorithmProcessFunction (AAXWrapper_Context* const inInstancesBegin[],
                                                   const void* inInstancesEnd)
{
	//--- ------ Iterate over plug-in instances ---------//
	for (AAXWrapper_Context* const* walk = inInstancesBegin; walk < inInstancesEnd; ++walk)
	{
		AAXWrapper_Context* AAX_RESTRICT instance = *walk;

		// first element is Context
		AAXWrapper* wrapper = *reinterpret_cast<AAXWrapper**> (instance->ptr[0]);
		if (!wrapper)
			continue;

		wrapper->Process (instance);

	} // End instance-iteration loop
}

//------------------------------------------------------------------------
static int32 vst3Category2AAXPlugInCategory (const char* cat)
{
	static const int32 PDA_ePlugInCategory_Effect =
	    AAX_ePlugInCategory_None; // does not exist anymore?

	int32 result = AAX_ePlugInCategory_None;

	if (strstr (cat, "Fx") != nullptr)
	{
		result = PDA_ePlugInCategory_Effect;
	}

	if (strstr (cat, "Instrument") != nullptr || strstr (cat, "Generator") != nullptr)
	{
		if (strstr (cat, "External") != nullptr)
			result |= AAX_ePlugInCategory_HWGenerators;
		else
			result |= AAX_ePlugInCategory_SWGenerators;
	}

	if (strstr (cat, "Delay") != nullptr)
		result |= AAX_ePlugInCategory_Delay;

	if (strstr (cat, "Distortion") != nullptr)
		result |= AAX_ePlugInCategory_Harmonic;

	if (strstr (cat, "Dynamics") != nullptr)
		result |= AAX_ePlugInCategory_Dynamics;

	if (strstr (cat, "EQ") != nullptr)
		result |= AAX_ePlugInCategory_EQ;

	if (strstr (cat, "Mastering") != nullptr)
		result |= AAX_ePlugInCategory_Dither;

	if (strstr (cat, "Modulation") != nullptr)
		result |= AAX_ePlugInCategory_Modulation;

	if (strstr (cat, "Pitch Shift") != nullptr)
		result |= AAX_ePlugInCategory_PitchShift;

	if (strstr (cat, "Restoration") != nullptr)
		result |= AAX_ePlugInCategory_NoiseReduction;

	if (strstr (cat, "Reverb") != nullptr)
		result |= AAX_ePlugInCategory_Reverb;

	if (strstr (cat, "Spatial") != nullptr || strstr (cat, "Surround") != nullptr ||
	    strstr (cat, "Up-Downmix") != nullptr)
	{
		result |= AAX_ePlugInCategory_SoundField;
	}

	return result;
}

//------------------------------------------------------------------------
// ROUTINE:	DescribeAlgorithmComponent
// Algorithm component description
//------------------------------------------------------------------------
void AAXWrapper::DescribeAlgorithmComponent (AAX_IComponentDescriptor* outDesc,
                                             const AAX_Effect_Desc* desc,
                                             const AAX_Plugin_Desc* pdesc)
{
	// Describe algorithm's context structure
	//
	// Subscribe context fields to host-provided services or information
	HLOG (HAPI, "%s", __FUNCTION__);

	AAX_Result err = AAX_SUCCESS;

	// must be in lock step with AAXWrapper ctor
	int32 idx = idxBufferSize + 1;

	// ProTools does not like instruments without inputs (maybe because they are treated as
	// inserts?)
	int32 inChannels = pdesc->mInputChannels;
	if (inChannels == 0)
		inChannels = pdesc->mOutputChannels;
	if (inChannels)
		err = outDesc->AddAudioIn (idx++);
	AAX_ASSERT (err == AAX_SUCCESS);

	if (pdesc->mOutputChannels)
		err = outDesc->AddAudioOut (idx++);
	AAX_ASSERT (err == AAX_SUCCESS);

	err = outDesc->AddAudioBufferLength (AAXWrapper::idxBufferSize);
	AAX_ASSERT (err == AAX_SUCCESS);

	if (pdesc->mSideChainInputChannels)
		err = outDesc->AddSideChainIn (idx++); // max 1 side chain!?
	AAX_ASSERT (err == AAX_SUCCESS);

	if (pdesc->mMIDIports)
	{
		for (AAX_MIDI_Desc* mdesc = pdesc->mMIDIports; mdesc->mName; mdesc++)
		{
			err = outDesc->AddMIDINode (idx++, AAX_eMIDINodeType_LocalInput, mdesc->mName,
			                            mdesc->mMask);
			AAX_ASSERT (err == 0);
		}
	}

	if (pdesc->mAuxOutputChannels)
	{
		for (AAX_Aux_Desc* auxdesc = pdesc->mAuxOutputChannels; auxdesc->mName; auxdesc++)
		{
			int32 ch = (auxdesc->mChannels < 0 ? pdesc->mOutputChannels : auxdesc->mChannels);
			err = outDesc->AddAuxOutputStem (idx++, getChannelsStem (ch), auxdesc->mName);
			AAX_ASSERT (err == 0);
		}
	}
	if (pdesc->mMeters)
	{
		uint32 cntMeters = 0;
		for (AAX_Meter_Desc* mdesc = pdesc->mMeters; mdesc->mName; mdesc++)
			cntMeters++;
		std::unique_ptr<Steinberg::int32[]> meterIds; 
		meterIds.reset (new Steinberg::int32 (cntMeters));
		cntMeters = 0;
		for (AAX_Meter_Desc* mdesc = pdesc->mMeters; mdesc->mName; mdesc++)
			meterIds[cntMeters++] = mdesc->mID;

		err = outDesc->AddMeters (idx++, (AAX_CTypeID*)meterIds.get (), cntMeters);
		AAX_ASSERT (err == AAX_SUCCESS);
	}

	// Register context fields as private data
	err = outDesc->AddPrivateData (AAXWrapper::idxContext, sizeof (void*),
	                               AAX_ePrivateDataOptions_DefaultOptions);
	AAX_ASSERT (err == AAX_SUCCESS);

	// Register processing callbacks
	//
	// Create a property map
	AAX_IPropertyMap* properties = outDesc->NewPropertyMap ();
	AAX_ASSERT (properties);
	if (!properties)
		return;
	//
	// Generic properties
	properties->AddProperty (AAX_eProperty_ManufacturerID, desc->mManufacturerID);
	properties->AddProperty (AAX_eProperty_ProductID, desc->mProductID);
	properties->AddProperty (AAX_eProperty_CanBypass, true);
	properties->AddProperty (AAX_eProperty_LatencyContribution, pdesc->mLatency);
	// properties->AddProperty (AAX_eProperty_UsesClientGUI, true); // Uses auto-GUI

	//
	// Stem format -specific properties
	//	AAX_IPropertyMap* propertiesMono = outDesc->NewPropertyMap();
	//	AAX_ASSERT (propertiesMono);
	//	if ( !propertiesMono ) return;
	//	properties->Clone(propertiesMono);
	//
	if (pdesc->mInputChannels)
		properties->AddProperty (AAX_eProperty_InputStemFormat,
		                         getChannelsStem (pdesc->mInputChannels));
	else if (pdesc->mOutputChannels)
		properties->AddProperty (AAX_eProperty_InputStemFormat,
		                         getChannelsStem (pdesc->mOutputChannels));

	if (pdesc->mOutputChannels)
		properties->AddProperty (AAX_eProperty_OutputStemFormat,
		                         getChannelsStem (pdesc->mOutputChannels));
	if (pdesc->mSideChainInputChannels)
	{
		properties->AddProperty (AAX_eProperty_SupportsSideChainInput, true);
#if 0 // only mono supported, setting stem format causes plugin load failure
		properties->AddProperty (AAX_eProperty_SideChainStemFormat,
								 getChannelsStem (pdesc->mSideChainInputChannels));
#endif
	}

	properties->AddProperty (AAX_eProperty_PlugInID_Native, pdesc->mPlugInIDNative);
	properties->AddProperty (AAX_eProperty_PlugInID_AudioSuite, pdesc->mPlugInIDAudioSuite);

	// Register callbacks
	// Native (Native and AudioSuite)
	err = outDesc->AddProcessProc_Native<AAXWrapper_Context> (AlgorithmProcessFunction, properties,
	                                                          AlgorithmInitFunction,
	                                                          nullptr /*AlgorithmBackgroundFunction*/);
	AAX_ASSERT (err == AAX_SUCCESS);
}

//------------------------------------------------------------------------
static AAX_Result GetPlugInDescription (AAX_IEffectDescriptor* outDescriptor,
                                        const AAX_Effect_Desc* desc, const AAX_Plugin_Desc* pdesc)
{
	HLOG (HAPI, "%s", __FUNCTION__);

	int err = AAX_SUCCESS;
	AAX_IComponentDescriptor* compDesc = outDescriptor->NewComponentDescriptor ();
	if (!compDesc)
		return AAX_ERROR_NULL_OBJECT;

	// Effect identifiers
	outDescriptor->AddName (pdesc->mName);
	outDescriptor->AddCategory (vst3Category2AAXPlugInCategory (desc->mCategory));

	// Effect components
	//
	// Algorithm component
	AAXWrapper::DescribeAlgorithmComponent (compDesc, desc, pdesc);
	err = outDescriptor->AddComponent (compDesc);
	AAX_ASSERT (err == AAX_SUCCESS);

	// Data model
	int32 plugIndex = pdesc - desc->mPluginDesc;
	fnCreateParameters* fn = nullptr;
	switch (plugIndex)
	{
#define ADDPROCPTR(N) \
	case N: fn = &CP<N>::Create_Parameters; break;
		ADDPROCPTR (0)	ADDPROCPTR (1)	ADDPROCPTR (2)	ADDPROCPTR (3)
		ADDPROCPTR (4)	ADDPROCPTR (5)	ADDPROCPTR (6)	ADDPROCPTR (7)
		ADDPROCPTR (8)	ADDPROCPTR (9)	ADDPROCPTR (10) ADDPROCPTR (11)
		ADDPROCPTR (12)	ADDPROCPTR (13)	ADDPROCPTR (14) ADDPROCPTR (15)
	}
	AAX_ASSERT (fn);
	err = outDescriptor->AddProcPtr ((void*)fn, kAAX_ProcPtrID_Create_EffectParameters);
	AAX_ASSERT (err == AAX_SUCCESS);

	if (desc->mPageFile)
		outDescriptor->AddResourceInfo (AAX_eResourceType_PageTable, desc->mPageFile);

	// Effect's meter display properties
	if (pdesc->mMeters)
	{
		for (AAX_Meter_Desc* mdesc = pdesc->mMeters; mdesc->mName; mdesc++)
		{
			AAX_IPropertyMap* meterProperties = outDescriptor->NewPropertyMap ();
			if (!meterProperties)
				return AAX_ERROR_NULL_OBJECT;

			// Support different meter types offered by AAX here
			meterProperties->AddProperty (AAX_eProperty_Meter_Type, mdesc->mType);

			meterProperties->AddProperty (AAX_eProperty_Meter_Orientation, mdesc->mOrientation);
			outDescriptor->AddMeterDescription (mdesc->mID, mdesc->mName, meterProperties);
		}
	}

	// plugin supplied GUI
	err = outDescriptor->AddProcPtr ((void*)Create_GUI, kAAX_ProcPtrID_Create_EffectGUI);
	AAX_ASSERT (err == AAX_SUCCESS);

	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result GetEffectDescriptions (AAX_ICollection* outCollection)
{
	HLOG (HAPI, "%s", __FUNCTION__);

	AAX_Result result = AAX_ERROR_NULL_OBJECT;

	AAX_Effect_Desc* effDesc = AAXWrapper_GetDescription ();
	for (const AAX_Plugin_Desc* pdesc = effDesc->mPluginDesc; pdesc->mEffectID; pdesc++)
	{
		if (AAX_IEffectDescriptor* plugInDescriptor = outCollection->NewDescriptor ())
		{
			result = GetPlugInDescription (plugInDescriptor, effDesc, pdesc);
			if (result == AAX_SUCCESS)
				result = outCollection->AddEffect (pdesc->mEffectID, plugInDescriptor);
			AAX_ASSERT (result == AAX_SUCCESS);
		}
	}

	outCollection->SetManufacturerName (effDesc->mManufacturer);

	/* needed ?
	char packageName[512];
	sprintf (packageName, "%s AAX Plug-In", effDesc->mProduct);
	outCollection->AddPackageName (packageName);

	sprintf (packageName, "%s Plug-In", effDesc->mProduct);
	outCollection->AddPackageName (packageName);*/

	outCollection->AddPackageName (effDesc->mProduct);

	if (strlen (effDesc->mProduct) > 16)
	{
		char packageShortName[17] = { 0 };
		strncpy (packageShortName, effDesc->mProduct, 16);
		outCollection->AddPackageName (packageShortName);
	}

	outCollection->SetPackageVersion (effDesc->mVersion);

	return result;
}
