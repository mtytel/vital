//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxwrapper_parameters.cpp
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

#include "aaxwrapper_parameters.h"
#include "aaxwrapper.h"
#include "aaxwrapper_description.h"

#include "AAX_CBinaryDisplayDelegate.h"
#include "AAX_CBinaryTaperDelegate.h"
#include "AAX_CLinearTaperDelegate.h"
#include "AAX_CNumberDisplayDelegate.h"
#include "AAX_CUnitDisplayDelegateDecorator.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/base/futils.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace Steinberg::Base::Thread;

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

IPluginFactory* PLUGIN_API GetPluginFactory ();
extern bool InitModule ();

const char* kBypassId = "Byp";

ParameterInfo kParamInfoBypass = {CCONST ('B', 'y', 'p', 0),
                                  STR ("Bypass"),
                                  STR ("Bypass"),
                                  STR (""),
                                  1,
                                  0,
                                  -1,
                                  Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass};

//------------------------------------------------------------------------
// AAXWrapper_Parameters
//------------------------------------------------------------------------
AAXWrapper_Parameters::AAXWrapper_Parameters (int32_t plugIndex)
: AAX_CEffectParameters (), mSimulateBypass (false)
{
	HLOG (HAPI, "%s", __FUNCTION__);

	InitModule (); // the engine is already initialized, but we have to match the DeinitModule in
	// the VST2Wrapper destructor

	AAX_Effect_Desc* effDesc = AAXWrapper_GetDescription ();
	mPluginDesc = effDesc->mPluginDesc + plugIndex;
	mWrapper = AAXWrapper::create (GetPluginFactory (), effDesc->mVST3PluginID, mPluginDesc, this);
	if (!mWrapper)
		return;

#if DEVELOPMENT
	static bool writePagetableFile;
	if (writePagetableFile) // use debugger to set variable or jump into function
		mWrapper->generatePageTables ("c:/tmp/pagetable.xml");
#endif

	// if no VST3 Bypass found then simulate it
	mSimulateBypass = (mWrapper->mBypassParameterID == Vst::kNoParamId);

	if (mParamNames.size () < (size_t)mWrapper->mNumParams)
	{
		mParamNames.resize (mWrapper->mNumParams);
		for (size_t i = 0; i < mParamNames.size (); i++)
			mParamNames[i].set (mWrapper->mParameterMap[i].vst3ID);
	}
}

//------------------------------------------------------------------------
AAXWrapper_Parameters::~AAXWrapper_Parameters ()
{
	delete mWrapper;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::EffectInit ()
{
	HLOG (HAPI, "%s", __FUNCTION__);

	if (AAX_IController* ctrl = Controller ())
	{
		AAX_CSampleRate sampleRate;
		if (ctrl->GetSampleRate (&sampleRate) == AAX_SUCCESS)
			mWrapper->_setSampleRate (sampleRate);
		if (mWrapper->mProcessor)
			ctrl->SetSignalLatency (mWrapper->mProcessor->getLatencySamples ());
	}

	for (int32 i = 0; i < mWrapper->mNumParams; i++)
	{
		AAX_CParamID iParameterID = mParamNames[i];
		ParameterInfo paramInfo = {0};
		if (AAX_Result result = mWrapper->getParameterInfo (iParameterID, paramInfo))
			return result;

		String title = paramInfo.title;
		AAX_IParameter* param = 0;
		param = new AAX_CParameter<double> (
		    iParameterID, AAX_CString (title), paramInfo.defaultNormalizedValue,
		    AAX_CLinearTaperDelegate<double> (0, 1),
		    AAX_CUnitDisplayDelegateDecorator<double> (AAX_CNumberDisplayDelegate<double> (),
		                                               AAX_CString (title)),
		    true);

		mParameterManager.AddParameter (param);
	}

	if (mSimulateBypass)
	{
		AAX_IParameter* param = 0;
		param = new AAX_CParameter<bool> (kBypassId, AAX_CString ("Bypass"), 0,
		                                  AAX_CBinaryTaperDelegate<bool> (),
		                                  AAX_CBinaryDisplayDelegate<bool> ("off", "on"), true);

		mParameterManager.AddParameter (param);
	}
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::ResetFieldData (AAX_CFieldIndex index, void* inData,
                                                  uint32_t inDataSize) const
{
	HLOG (HAPI, "%s", __FUNCTION__);

	return mWrapper->ResetFieldData (index, inData, inDataSize);
}

//------------------------------------------------------------------------
// METHOD:	AAX_UpdateMIDINodes
// This will be called by the host if there are MIDI packets that need
// to be handled in the Data Model.
//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::UpdateMIDINodes (AAX_CFieldIndex inFieldIndex,
                                                   AAX_CMidiPacket& inPacket)
{
	HLOG (HAPI, "%s", __FUNCTION__);

	AAX_Result result;
	result = AAX_SUCCESS;

	// Do some MIDI work if necessary.

	return result;
}

//------------------------------------------------------------------------
int32 AAXWrapper_Parameters::getParameterInfo (AAX_CParamID aaxId,
                                               Vst::ParameterInfo& paramInfo) const
{
	AAX_Result result = mWrapper->getParameterInfo (aaxId, paramInfo);
	if (result != AAX_SUCCESS)
	{
		if (mSimulateBypass && strcmp (aaxId, kBypassId) == 0)
		{
			paramInfo = kParamInfoBypass;
			result = AAX_SUCCESS;
		}
	}
	return result;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetNumberOfParameters (int32_t* oNumControls) const
{
	HLOG (HAPI, "%s", __FUNCTION__);
	*oNumControls = mWrapper->mNumParams;
	if (mSimulateBypass)
		*oNumControls += 1;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetMasterBypassParameter (AAX_IString* oIDString) const
{
	HLOG (HAPI, "%s", __FUNCTION__);

	*oIDString = mSimulateBypass ? kBypassId : AAX_CID (mWrapper->mBypassParameterID);
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterIsAutomatable (AAX_CParamID iParameterID,
                                                             AAX_CBoolean* oAutomatable) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	ParameterInfo paramInfo = {0};
	if (AAX_Result result = getParameterInfo (iParameterID, paramInfo))
		return result;

	*oAutomatable = (paramInfo.flags & ParameterInfo::kCanAutomate) != 0;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterNumberOfSteps (AAX_CParamID iParameterID,
                                                             int32_t* oNumSteps) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	ParameterInfo paramInfo = {0};
	if (AAX_Result result = getParameterInfo (iParameterID, paramInfo))
		return result;

	*oNumSteps = paramInfo.stepCount + 1;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterName (AAX_CParamID iParameterID,
                                                    AAX_IString* oName) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	ParameterInfo paramInfo = {0};
	if (AAX_Result result = getParameterInfo (iParameterID, paramInfo))
		return result;

	*oName = String (paramInfo.title);
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterNameOfLength (AAX_CParamID iParameterID,
                                                            AAX_IString* oName,
                                                            int32_t iNameLength) const
{
	HLOG (HAPI, "%s(id=%s, len=%d)", __FUNCTION__, iParameterID, iNameLength);

	ParameterInfo paramInfo = {0};
	if (AAX_Result result = getParameterInfo (iParameterID, paramInfo))
		return result;

	if (iNameLength >= tstrlen (paramInfo.title))
		*oName = String (paramInfo.title);
	else
	{
		if (iNameLength < tstrlen (paramInfo.shortTitle))
			paramInfo.shortTitle[iNameLength] = 0;
		*oName = String (paramInfo.shortTitle);
	}
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterDefaultNormalizedValue (AAX_CParamID iParameterID,
                                                                      double* oValue) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	ParameterInfo paramInfo = {0};
	if (AAX_Result result = getParameterInfo (iParameterID, paramInfo))
		return result;

	*oValue = paramInfo.defaultNormalizedValue;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::SetParameterDefaultNormalizedValue (AAX_CParamID iParameterID,
                                                                      double iValue)
{
	HLOG (HAPI, "%s(id=%s, value=%lf)", __FUNCTION__, iParameterID, iValue);

	return AAX_ERROR_UNIMPLEMENTED;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterType (AAX_CParamID iParameterID,
                                                    AAX_EParameterType* oParameterType) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	ParameterInfo paramInfo = {0};
	if (AAX_Result result = getParameterInfo (iParameterID, paramInfo))
		return result;
	*oParameterType =
	    paramInfo.stepCount == 0 ? AAX_eParameterType_Continuous : AAX_eParameterType_Discrete;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterOrientation (
    AAX_CParamID iParameterID, AAX_EParameterOrientation* oParameterOrientation) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	*oParameterOrientation = AAX_eParameterOrientation_BottomMinTopMax; // we don't care
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameter (AAX_CParamID iParameterID,
                                                AAX_IParameter** oParameter)
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	SMTG_ASSERT (!"the host is not supposed to retrieve the AAX_IParameter interface");
	return AAX_ERROR_UNIMPLEMENTED;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterIndex (AAX_CParamID iParameterID,
                                                     int32_t* oControlIndex) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	Vst::ParamID id = getVstParamID (iParameterID);
	if (id == -1)
	{
		if (mSimulateBypass && strcmp (iParameterID, kBypassId) == 0)
		{
			*oControlIndex = mWrapper->mNumParams;
			return AAX_SUCCESS;
		}
		return AAX_ERROR_INVALID_PARAMETER_ID;
	}

	std::map<ParamID, int32>::iterator iter = mWrapper->mParamIndexMap.find (id);
	if (iter == mWrapper->mParamIndexMap.end ())
		return AAX_ERROR_INVALID_PARAMETER_ID;

	*oControlIndex = iter->second;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterIDFromIndex (int32_t iControlIndex,
                                                           AAX_IString* oParameterIDString) const
{
	HLOG (HAPI, "%s(idx=%x)", __FUNCTION__, iControlIndex);

	if ((size_t)iControlIndex >= mWrapper->mParameterMap.size ())
	{
		if (mSimulateBypass && iControlIndex == mWrapper->mNumParams)
		{
			oParameterIDString->Set (kBypassId);
			return AAX_SUCCESS;
		}
		return AAX_ERROR_INVALID_PARAMETER_INDEX;
	}

	*oParameterIDString = mParamNames[iControlIndex];
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterValueInfo (AAX_CParamID iParameterID,
                                                         int32_t iSelector, int32_t* oValue) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	*oValue = 0;
	return AAX_ERROR_UNIMPLEMENTED;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterValueFromString (
    AAX_CParamID iParameterID, double* oValue, const AAX_IString& iValueString) const
{
	HLOG (HAPI, "%s(id=%s, string=%s)", __FUNCTION__, iParameterID, iValueString.Get ());

	Vst::ParamID id = getVstParamID (iParameterID);
	if (id == -1)
	{
		if (mSimulateBypass && strcmp (iParameterID, kBypassId) == 0)
		{
			*oValue = strcmp (iValueString.Get (), "on") == 0;
			return AAX_SUCCESS;
		}
		return AAX_ERROR_INVALID_PARAMETER_ID;
	}
	String tmp (iValueString.Get ());
	if (mWrapper->mController->getParamValueByString (id, (Vst::TChar*)tmp.text16 (), *oValue) !=
	    kResultTrue)
		return AAX_ERROR_INVALID_PARAMETER_ID;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterStringFromValue (AAX_CParamID iParameterID,
                                                               double iValue,
                                                               AAX_IString* oValueString,
                                                               int32_t maxLength) const
{
	HLOG (HAPI, "%s(id=%s, value=%lf)", __FUNCTION__, iParameterID, iValue);

	Vst::ParamID id = getVstParamID (iParameterID);
	if (id == -1)
	{
		if (mSimulateBypass && strcmp (iParameterID, kBypassId) == 0)
		{
			oValueString->Set (iValue >= 0.5 ? "on" : "off");
			return AAX_SUCCESS;
		}
		return AAX_ERROR_INVALID_PARAMETER_ID;
	}

	String128 tmp = {0};
	if (mWrapper->mController->getParamStringByValue (id, iValue, tmp) != kResultTrue)
		return AAX_ERROR_INVALID_PARAMETER_ID;

	if (maxLength < tstrlen (tmp))
		tmp[maxLength] = 0;
	*oValueString = String (tmp);
	//		String str (tmp);
	//		str.copyTo8 (text, 0, kVstMaxParamStrLen);
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterValueString (AAX_CParamID iParameterID,
                                                           AAX_IString* oValueString,
                                                           int32_t iMaxLength) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	double value;
	if (AAX_Result result = GetParameterNormalizedValue (iParameterID, &value))
		return result;

	return GetParameterStringFromValue (iParameterID, value, oValueString, iMaxLength);
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetParameterNormalizedValue (AAX_CParamID iParameterID,
                                                               double* oValuePtr) const
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	Vst::ParamID id = getVstParamID (iParameterID);
	if (id == -1)
	{
		if (mSimulateBypass && strcmp (iParameterID, kBypassId) == 0)
		{
			*oValuePtr = (mWrapper->mBypass ? 1 : 0);
			return AAX_SUCCESS;
		}
		return AAX_ERROR_INVALID_PARAMETER_ID;
	}

	ParamValue value = 0;
	if (!mWrapper->getLastParamChange (id, value))
		value = mWrapper->mController->getParamNormalized (id);
	*oValuePtr = value;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::SetParameterNormalizedValue (AAX_CParamID iParameterID,
                                                               double iValue)
{
	HLOG (HAPI, "%s(id=%s, value=%lf)", __FUNCTION__, iParameterID, iValue);

	Vst::ParamID id = getVstParamID (iParameterID);
	if (id == -1)
	{
		if (mSimulateBypass && strcmp (iParameterID, kBypassId) == 0)
			return AAX_SUCCESS;
		return AAX_ERROR_INVALID_PARAMETER_ID;
	}

	// mWrapper->addParameterChange (id, iValue, 0);

	if (auto ad = AutomationDelegate ())
	{
		// Touch the control, Send that token, Release the control
		ad->PostTouchRequest (iParameterID);
		ad->PostSetValueRequest (iParameterID, iValue);
		ad->PostReleaseRequest (iParameterID);
	}

	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::SetParameterNormalizedRelative (AAX_CParamID iParameterID,
                                                                  double iValue)
{
	HLOG (HAPI, "%s(id=%s, value=%lf)", __FUNCTION__, iParameterID, iValue);

	Vst::ParamID id = getVstParamID (iParameterID);
	if (id == -1)
	{
		if (mSimulateBypass && strcmp (iParameterID, kBypassId) == 0)
		{
			mWrapper->mBypass = (mWrapper->mBypass + iValue >= 0.5);
			mWrapper->_setBypass (mWrapper->mBypass);
			return AAX_SUCCESS;
		}
		return AAX_ERROR_INVALID_PARAMETER_ID;
	}

	ParamValue value = 0;
	if (!mWrapper->getLastParamChange (id, value))
		value = mWrapper->mController->getParamNormalized (id);

	value = value + iValue;
	if (value < 0)
		value = 0;
	else if (value > 1)
		value = 1;

	SetParameterNormalizedValue (iParameterID, value);
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::TouchParameter (AAX_CParamID iParameterID)
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	if (auto ad = AutomationDelegate ())
		return ad->PostTouchRequest (iParameterID);

	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::ReleaseParameter (AAX_CParamID iParameterID)
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	if (auto ad = AutomationDelegate ())
		return ad->PostReleaseRequest (iParameterID);

	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::UpdateParameterTouch (AAX_CParamID iParameterID,
                                                        AAX_CBoolean iTouchState)
{
	HLOG (HAPI, "%s(id=%s)", __FUNCTION__, iParameterID);

	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::UpdateParameterNormalizedValue (AAX_CParamID iParameterID,
                                                                  double iValue,
                                                                  AAX_EUpdateSource iSource)
{
	HLOG (HAPI, "%s(id=%s, value=%lf)", __FUNCTION__, iParameterID, iValue);

	Vst::ParamID id = getVstParamID (iParameterID);
	if (id == -1)
	{
		if (mSimulateBypass && strcmp (iParameterID, kBypassId) == 0)
		{
			mWrapper->mBypass = iValue >= 0.5;
			mWrapper->_setBypass (mWrapper->mBypass);
		}
		else
			return AAX_ERROR_INVALID_PARAMETER_ID;
	}
	else
		mWrapper->addParameterChange (id, iValue, 0);
#if 1
	return AAX_CEffectParameters::UpdateParameterNormalizedValue (iParameterID, iValue, iSource);
#else
	if (AutomationDelegate ())
		AutomationDelegate ()->PostCurrentValue (iParameterID, iValue);

	// if (AAX_Result result = SetParameterNormalizedValue (iParameterID, iValue))
	//	return result;

	// Now the control has changed
	AAX_Result result = mPacketDispatcher.SetDirty (iParameterID);

	++mNumPlugInChanges;
	return result;
#endif
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::_UpdateParameterNormalizedRelative (AAX_CParamID iParameterID,
                                                                      double iValue)
{
	HLOG (HAPI, "%s(id=%s, value=%lf)", __FUNCTION__, iParameterID, iValue);

	if (AAX_Result result = SetParameterNormalizedRelative (iParameterID, iValue))
		return result;

	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::_GenerateCoefficients ()
{
	HLOG (HAPI, "%s", __FUNCTION__);

	AAX_Result result = mPacketDispatcher.Dispatch ();

	return result;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetNumberOfChunks (int32_t* numChunks) const
{
	HLOG (HAPI, "%s", __FUNCTION__);

	*numChunks = 1;
	return AAX_SUCCESS;
}

const AAX_CTypeID AAXWRAPPER_CONTROLS_CHUNK_ID = CCONST ('a', 'w', 'c', 'k');
const char AAXWRAPPER_CONTROLS_CHUNK_DESCRIPTION[] = "AAXWrapper State";

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetChunkIDFromIndex (int32_t index, AAX_CTypeID* chunkID) const
{
	HLOG (HAPI, "%s", __FUNCTION__);

	if (index != 0)
		return AAX_ERROR_INVALID_CHUNK_INDEX;

	*chunkID = AAXWRAPPER_CONTROLS_CHUNK_ID;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetChunkSize (AAX_CTypeID chunkID, uint32_t* oSize) const
{
	HLOG (HAPI, "%s", __FUNCTION__);

	if (chunkID != AAXWRAPPER_CONTROLS_CHUNK_ID)
		return AAX_ERROR_INVALID_CHUNK_ID;

	FGuard guard (mWrapper->syncCalls);
	bool isPreset = false;
	void* data;
	*oSize = mWrapper->_getChunk (&data, isPreset);
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::GetChunk (AAX_CTypeID chunkID, AAX_SPlugInChunk* oChunk) const
{
	HLOG (HAPI, "%s", __FUNCTION__);

	if (chunkID != AAXWRAPPER_CONTROLS_CHUNK_ID)
		return AAX_ERROR_INVALID_CHUNK_ID;

	FGuard guard (mWrapper->syncCalls);
	// assume GetChunkSize called before and size of oChunk correct
	oChunk->fVersion = 1;
	oChunk->fSize = mWrapper->mChunk.getSize ();
	memcpy (oChunk->fData, mWrapper->mChunk.getData (), mWrapper->mChunk.getSize ());
	strncpy (reinterpret_cast<char*> (oChunk->fName), AAXWRAPPER_CONTROLS_CHUNK_DESCRIPTION, 31);
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::SetChunk (AAX_CTypeID chunkID, const AAX_SPlugInChunk* iChunk)
{
	HLOG (HAPI, "%s", __FUNCTION__);

	if (chunkID != AAXWRAPPER_CONTROLS_CHUNK_ID)
		return AAX_ERROR_INVALID_CHUNK_ID;

	FGuard guard (mWrapper->syncCalls);
	bool isPreset = false;
	mWrapper->_setChunk (const_cast<char*> (iChunk->fData), iChunk->fSize, isPreset);
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::CompareActiveChunk (const AAX_SPlugInChunk* iChunk,
                                                      AAX_CBoolean* oIsEqual) const
{
	HLOG (HAPI, "%s", __FUNCTION__);

	return AAX_ERROR_UNIMPLEMENTED;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::_GetNumberOfChanges (int32_t* oValue) const
{
	HLOG (HAPI, "%s", __FUNCTION__);

	*oValue = mNumPlugInChanges;
	return AAX_SUCCESS;
}

//------------------------------------------------------------------------
void AAXWrapper_Parameters::setDirty (bool state)
{
	if (state)
		mNumPlugInChanges++;
}

//------------------------------------------------------------------------
AAX_Result AAXWrapper_Parameters::NotificationReceived (AAX_CTypeID iNotificationType,
                                                        const void* iNotificationData,
                                                        uint32_t iNotificationDataSize)
{
	switch (iNotificationType)
	{
		case AAX_eNotificationEvent_SideChainBeingConnected:
			mWrapper->setSideChainEnable (true);
			break;
		case AAX_eNotificationEvent_SideChainBeingDisconnected:
			mWrapper->setSideChainEnable (false);
			break;
		case AAX_eNotificationEvent_SignalLatencyChanged:
		{
			int32_t outSample;
			Controller ()->GetSignalLatency (&outSample);
			
			if (mPluginDesc)
				mPluginDesc->mLatency = outSample;

			if (!mWrapper->isActive ())
			{
				mWrapper->_suspend ();
				mWrapper->_resume ();
			}
			break;
		}
	}

	return AAX_CEffectParameters::NotificationReceived (
	    iNotificationType, iNotificationData, iNotificationDataSize);
}
