//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vst2wrapper/basewrapper.cpp
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

/// \cond ignore

#include "public.sdk/source/vst/basewrapper/basewrapper.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"

#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/base/keycodes.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/vstpresetkeys.h"

#include "base/source/fstreamer.h"

#include <cstdio>
#include <cstdlib>
#include <limits>

extern bool DeinitModule (); //! Called in BaseWrapper destructor

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// some Globals
//------------------------------------------------------------------------
// In order to speed up hasEditor function gPluginHasEditor can be set in EditController::initialize
enum
{
	kDontKnow = -1,
	kNoEditor = 0,
	kEditor,
};

// default: kDontKnow which uses createView to find out
using EditorAvailability = int32;
EditorAvailability gPluginHasEditor = kDontKnow;

// Set to 'true' in EditController::initialize
// default: VST 3 kIsProgramChange parameter will not be exported
bool gExportProgramChangeParameters = false;

//------------------------------------------------------------------------
BaseEditorWrapper::BaseEditorWrapper (IEditController* controller)
: mController (controller), mView (nullptr)
{
}

//------------------------------------------------------------------------
BaseEditorWrapper::~BaseEditorWrapper ()
{
	if (mView)
		_close ();

	mController = nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API BaseEditorWrapper::queryInterface (const char* _iid, void** obj)
{
	QUERY_INTERFACE (_iid, obj, FUnknown::iid, FUnknown)
	QUERY_INTERFACE (_iid, obj, IPlugFrame::iid, IPlugFrame)

	*obj = nullptr;
	return kNoInterface;
}

//------------------------------------------------------------------------
tresult PLUGIN_API BaseEditorWrapper::resizeView (IPlugView* view, ViewRect* newSize)
{
	tresult result = kResultFalse;
	if (view && newSize)
	{
		result = view->onSize (newSize);
	}

	return result;
}

//------------------------------------------------------------------------
bool BaseEditorWrapper::hasEditor (IEditController* controller)
{
	/* Some Plug-ins might have large GUIs. In order to speed up hasEditor function while
	 * initializing the Plug-in gPluginHasEditor can be set in EditController::initialize
	 * beforehand. */
	bool result = false;
	if (gPluginHasEditor == kEditor)
	{
		result = true;
	}
	else if (gPluginHasEditor == kNoEditor)
	{
		result = false;
	}
	else
	{
		IPlugView* view = controller ? controller->createView (ViewType::kEditor) : nullptr;
		FReleaser viewRel (view);
		result = (view != nullptr);
	}

	return result;
}

//------------------------------------------------------------------------
void BaseEditorWrapper::createView ()
{
	if (mView == nullptr && mController != nullptr)
	{
		mView = owned (mController->createView (ViewType::kEditor));
		mView->setFrame (this);

#if SMTG_OS_MACOS
#if SMTG_PLATFORM_64
		if (mView && mView->isPlatformTypeSupported (kPlatformTypeNSView) != kResultTrue)
#else
		if (mView && mView->isPlatformTypeSupported (kPlatformTypeHIView) != kResultTrue)
#endif
		{
			mView = nullptr;
			mController = nullptr;
		}
#endif
	}
}

//------------------------------------------------------------------------
bool BaseEditorWrapper::getRect (ViewRect& rect)
{
	createView ();
	if (!mView)
		return false;

	if (mView->getSize (&rect) == kResultTrue)
	{
		if ((rect.bottom - rect.top) > 0 && (rect.right - rect.left) > 0)
		{
			mViewRect = rect;
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
bool BaseEditorWrapper::_open (void* ptr)
{
	createView ();

	if (mView)
	{
#if SMTG_OS_WINDOWS
		FIDString type = kPlatformTypeHWND;
#elif SMTG_OS_MACOS && SMTG_PLATFORM_64
		FIDString type = kPlatformTypeNSView;
#elif SMTG_OS_MACOS
		FIDString type = kPlatformTypeHIView;
#endif
		return mView->attached (ptr, type) == kResultTrue;
	}
	return false;
}

//------------------------------------------------------------------------
void BaseEditorWrapper::_close ()
{
	if (mView)
	{
		mView->setFrame (nullptr);
		mView->removed ();
		mView = nullptr;
	}
}

//------------------------------------------------------------------------
bool BaseEditorWrapper::_setKnobMode (Vst::KnobMode val)
{
	bool result = false;
	FUnknownPtr<IEditController2> editController2 (mController);
	if (editController2)
		result = editController2->setKnobMode (val) == kResultTrue;

	return result;
}

//------------------------------------------------------------------------
// MemoryStream with attributes to add information "preset or project"
//------------------------------------------------------------------------
class VstPresetStream : public MemoryStream, Vst::IStreamAttributes
{
public:
	VstPresetStream () {}
	VstPresetStream (void* memory, TSize memorySize) : MemoryStream (memory, memorySize) {}

	//---from Vst::IStreamAttributes-----
	tresult PLUGIN_API getFileName (String128 name) SMTG_OVERRIDE { return kNotImplemented; }
	IAttributeList* PLUGIN_API getAttributes () SMTG_OVERRIDE { return &attrList; }

//------------------------------------------------------------------------
	DELEGATE_REFCOUNT (MemoryStream)
	tresult PLUGIN_API queryInterface (const TUID iid, void** obj) SMTG_OVERRIDE
	{
		QUERY_INTERFACE (iid, obj, IStreamAttributes::iid, IStreamAttributes)
		return MemoryStream::queryInterface (iid, obj);
	}

protected:
	HostAttributeList attrList;
};

//------------------------------------------------------------------------
// BaseWrapper
//------------------------------------------------------------------------
BaseWrapper::BaseWrapper (SVST3Config& config)
{
	mProcessor = owned (config.processor);
	mController = owned (config.controller);
	mFactory = config.factory;	// share it
	mVst3EffectClassID = config.vst3ComponentID;

	memset (mName, 0, sizeof (mName));
	memset (mVendor, 0, sizeof (mVendor));
	memset (mSubCategories, 0, sizeof (mSubCategories));
	
	memset (&mProcessContext, 0, sizeof (ProcessContext));
	mProcessContext.sampleRate = 44100;
	mProcessContext.tempo = 120;

	for (int32 b = 0; b < kMaxMidiMappingBusses; b++)
	{
		for (int32 i = 0; i < 16; i++)
		{
			mMidiCCMapping[b][i] = nullptr;
		}
	}
	for (int32 i = 0; i < kMaxProgramChangeParameters; i++)
	{
		mProgramChangeParameterIDs[i] = kNoParamId;
		mProgramChangeParameterIdxs[i] = -1;
	}
}

//------------------------------------------------------------------------
BaseWrapper::~BaseWrapper ()
{
	mTimer = nullptr;

	mProcessData.unprepare ();

	//---Disconnect components
	if (mComponentsConnected)
	{
		FUnknownPtr<IConnectionPoint> cp1 (mProcessor);
		FUnknownPtr<IConnectionPoint> cp2 (mController);
		if (cp1 && cp2)
		{
			cp1->disconnect (cp2);
			cp2->disconnect (cp1);
		}
	}

	//---Terminate Controller Component
	if (mController)
	{
		mController->setComponentHandler (nullptr);
		if (mControllerInitialized)
			mController->terminate ();
		mControllerInitialized = false;
	}

	//---Terminate Processor Component
	if (mComponent && mComponentInitialized)
	{
		mComponent->terminate ();
		mComponentInitialized = false;
	}

	mInputEvents = nullptr;
	mOutputEvents = nullptr;
	mUnitInfo = nullptr;
	mMidiMapping = nullptr;

	if (mMidiCCMapping[0])
		for (int32 b = 0; b < kMaxMidiMappingBusses; b++)
			for (int32 i = 0; i < 16; i++)
				delete mMidiCCMapping[b][i];

	mEditor = nullptr;
	mController = nullptr;
	mProcessor = nullptr;
	mComponent = nullptr;
	mFactory = nullptr;

	DeinitModule ();
}

//------------------------------------------------------------------------
bool BaseWrapper::init ()
{
	// VST 3 stuff -----------------------------------------------
	if (mProcessor)
		mProcessor->queryInterface (IComponent::iid, (void**)&mComponent);
	if (mController)
	{
		mController->queryInterface (IUnitInfo::iid, (void**)&mUnitInfo);
		mController->queryInterface (IMidiMapping::iid, (void**)&mMidiMapping);
	}

	//---init the processor component
	mComponentInitialized = true;
	if (mComponent->initialize ((IHostApplication*)this) != kResultTrue)
		return false;

	//---init the controller component
	if (mController)
	{
		// do not initialize 2 times the component if it is singleComponent
		if (FUnknownPtr<IEditController> (mComponent).getInterface () != mController)
		{
			mControllerInitialized = true;
			if (mController->initialize ((IHostApplication*)this) != kResultTrue)
				return false;
		}

		//---set this class as Component Handler
		mController->setComponentHandler (this);

		//---connect the 2 components
		FUnknownPtr<IConnectionPoint> cp1 (mProcessor);
		FUnknownPtr<IConnectionPoint> cp2 (mController);
		if (cp1 && cp2)
		{
			cp1->connect (cp2);
			cp2->connect (cp1);

			mComponentsConnected = true;
		}

		//---inform the component "controller" with the component "processor" state
		MemoryStream stream;
		if (mComponent->getState (&stream) == kResultTrue)
		{
			stream.seek (0, IBStream::kIBSeekSet, nullptr);
			mController->setComponentState (&stream);
		}
	}

	// Wrapper -----------------------------------------------
	if (mProcessor)
	{
		if (mProcessor->canProcessSampleSize (kSample64) == kResultTrue)
		{
			_canDoubleReplacing (true); // supports double precision processing

			// we use the 64 as default only if 32 bit not supported
			if (mProcessor->canProcessSampleSize (kSample32) != kResultTrue)
				mVst3SampleSize = kSample64;
			else
				mVst3SampleSize = kSample32;
		}

		// latency -------------------------------
		_setInitialDelay (mProcessor->getLatencySamples ());

		if (mProcessor->getTailSamples () == kNoTail)
			_noTail (true);

		setupProcessing (); // initialize vst3 component processing parameters
	}

	// parameters
	setupParameters ();

	// setup inputs and outputs
	setupBuses ();

	// find out programs of root unit --------------------------
	mNumPrograms = 0;
	if (mUnitInfo)
	{
		int32 programListCount = mUnitInfo->getProgramListCount ();
		if (programListCount > 0)
		{
			ProgramListID rootUnitProgramListId = kNoProgramListId;
			for (int32 i = 0; i < mUnitInfo->getUnitCount (); i++)
			{
				UnitInfo unit = {0};
				if (mUnitInfo->getUnitInfo (i, unit) == kResultTrue)
				{
					if (unit.id == kRootUnitId)
					{
						rootUnitProgramListId = unit.programListId;
						break;
					}
				}
			}

			if (rootUnitProgramListId != kNoProgramListId)
			{
				for (int32 i = 0; i < programListCount; i++)
				{
					ProgramListInfo progList = {0};
					if (mUnitInfo->getProgramListInfo (i, progList) == kResultTrue)
					{
						if (progList.id == rootUnitProgramListId)
						{
							mNumPrograms = progList.programCount;
							break;
						}
					}
				}
			}
		}
	}

	if (mTimer == nullptr)
		mTimer = owned (Timer::create (this, 50));

	initMidiCtrlerAssignment ();

	return true;
}

//------------------------------------------------------------------------
void BaseWrapper::_suspend ()
{
	_stopProcess ();

	if (mComponent)
		mComponent->setActive (false);
	mActive = false;
}

//------------------------------------------------------------------------
void BaseWrapper::_resume ()
{
	mChunk.setSize (0);

	if (mComponent)
		mComponent->setActive (true);
	mActive = true;
}

//------------------------------------------------------------------------
void BaseWrapper::_startProcess ()
{
	if (mProcessor && mProcessing == false)
	{
		mProcessing = true;
		mProcessor->setProcessing (true);
	}
}

//------------------------------------------------------------------------
void BaseWrapper::_stopProcess ()
{
	if (mProcessor && mProcessing)
	{
		mProcessor->setProcessing (false);
		mProcessing = false;
	}
}

//------------------------------------------------------------------------
void BaseWrapper::_setEditor (BaseEditorWrapper* editor)
{
	mEditor = owned (editor);
}

//------------------------------------------------------------------------
bool BaseWrapper::_setBlockSize (int32 newBlockSize)
{
	if (mProcessing)
		return false;

	if (mBlockSize != newBlockSize)
	{
		mBlockSize = newBlockSize;
		setupProcessing ();
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool BaseWrapper::setupProcessing (int32 processModeOverwrite)
{
	if (!mProcessor)
		return false;

	ProcessSetup setup;
	if (processModeOverwrite >= 0)
		setup.processMode = processModeOverwrite;
	else
		setup.processMode = mVst3processMode;
	setup.maxSamplesPerBlock = mBlockSize;
	setup.sampleRate = mSampleRate;
	setup.symbolicSampleSize = mVst3SampleSize;

	return mProcessor->setupProcessing (setup) == kResultTrue;
}

//------------------------------------------------------------------------
bool BaseWrapper::getEditorSize (int32& width, int32& height) const
{
	if (auto* editor = getEditor ())
	{
		ViewRect rect;
		if (editor->getRect (rect))
		{
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------
float BaseWrapper::_getParameter (int32 index) const
{
	if (!mController)
		return 0.f;

	if (index < (int32)mParameterMap.size ())
	{
		ParamID id = mParameterMap.at (index).vst3ID;
		return (float)mController->getParamNormalized (id);
	}

	return 0.f;
}

//------------------------------------------------------------------------
void BaseWrapper::addParameterChange (ParamID id, ParamValue value, int32 sampleOffset)
{
	mGuiTransfer.addChange (id, value, sampleOffset);
	mInputTransfer.addChange (id, value, sampleOffset);
}

//------------------------------------------------------------------------
/*!	Usually VST 2 hosts call setParameter (...) and getParameterDisplay (...) synchronously.
In setParameter (...) param changes get queued (guiTransfer) and transfered in idle (::onTimer).
The ::onTimer call almost always comes AFTER getParameterDisplay (...) and therefore returns an
old
value. To avoid sending back old values, getLastParamChange (...) returns the latest value
from the guiTransfer queue. */
//------------------------------------------------------------------------
bool BaseWrapper::getLastParamChange (ParamID id, ParamValue& value)
{
	ParameterChanges changes;
	mGuiTransfer.transferChangesTo (changes);
	for (int32 i = 0; i < changes.getParameterCount (); ++i)
	{
		IParamValueQueue* queue = changes.getParameterData (i);
		if (queue)
		{
			if (queue->getParameterId () == id)
			{
				int32 points = queue->getPointCount ();
				if (points > 0)
				{
					mGuiTransfer.transferChangesFrom (changes);
					int32 sampleOffset = 0;
					return queue->getPoint (points - 1, sampleOffset, value) == kResultTrue;
				}
			}
		}
	}

	mGuiTransfer.transferChangesFrom (changes);
	return false;
}

//------------------------------------------------------------------------
void BaseWrapper::getUnitPath (UnitID unitID, String& path) const
{
	if (!mUnitInfo)
		return;

	//! Build the unit path up to the root unit (e.g. "Modulators.LFO 1.". Separator is a ".")
	for (int32 unitIndex = 0; unitIndex < mUnitInfo->getUnitCount (); ++unitIndex)
	{
		UnitInfo info = {0};
		mUnitInfo->getUnitInfo (unitIndex, info);
		if (info.id == unitID)
		{
			String unitName (info.name);
			unitName.append (".");
			path.insertAt (0, unitName);
			if (info.parentUnitId != kRootUnitId)
				getUnitPath (info.parentUnitId, path);

			break;
		}
	}
}
//------------------------------------------------------------------------
int32 BaseWrapper::_getChunk (void** data, bool isPreset)
{
	// Host stores Plug-in state. Returns the size in bytes of the chunk (Plug-in allocates the data
	// array)
	MemoryStream componentStream;
	if (mComponent && mComponent->getState (&componentStream) != kResultTrue)
		componentStream.setSize (0);

	MemoryStream controllerStream;
	if (mController && mController->getState (&controllerStream) != kResultTrue)
		controllerStream.setSize (0);

	if (componentStream.getSize () + controllerStream.getSize () == 0)
		return 0;

	mChunk.setSize (0);
	IBStreamer acc (&mChunk, kLittleEndian);

	acc.writeInt64 (componentStream.getSize ());
	acc.writeInt64 (controllerStream.getSize ());

	acc.writeRaw (componentStream.getData (), (int32)componentStream.getSize ());
	acc.writeRaw (controllerStream.getData (), (int32)controllerStream.getSize ());

	int32 chunkSize = (int32)mChunk.getSize ();
	*data = mChunk.getData ();
	return chunkSize;
}

//------------------------------------------------------------------------
int32 BaseWrapper::_setChunk (void* data, int32 byteSize, bool isPreset)
{
	if (!mComponent)
		return 0;

	// throw away all previously queued parameter changes, they are obsolete
	mGuiTransfer.removeChanges ();
	mInputTransfer.removeChanges ();

	MemoryStream chunk (data, byteSize);
	IBStreamer acc (&chunk, kLittleEndian);

	int64 componentDataSize = 0;
	int64 controllerDataSize = 0;

	acc.readInt64 (componentDataSize);
	acc.readInt64 (controllerDataSize);

	VstPresetStream componentStream (((char*)data) + acc.tell (), componentDataSize);
	VstPresetStream controllerStream (((char*)data) + acc.tell () + componentDataSize,
	                                  controllerDataSize);

	mComponent->setState (&componentStream);
	componentStream.seek (0, IBStream::kIBSeekSet, nullptr);

	if (mController)
	{
		if (!isPreset)
		{
			if (Vst::IAttributeList* attr = componentStream.getAttributes ())
				attr->setString (Vst::PresetAttributes::kStateType,
				                 String (Vst::StateType::kProject));
			if (Vst::IAttributeList* attr = controllerStream.getAttributes ())
				attr->setString (Vst::PresetAttributes::kStateType,
				                 String (Vst::StateType::kProject));
		}
		mController->setComponentState (&componentStream);
		mController->setState (&controllerStream);
	}

	return 0;
}

//------------------------------------------------------------------------
bool BaseWrapper::_setBypass (bool onOff)
{
	if (mBypassParameterID != kNoParamId)
	{
		addParameterChange (mBypassParameterID, onOff ? 1.0 : 0.0, 0);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool BaseWrapper::getProgramListAndUnit (int32 midiChannel, UnitID& unitId,
                                         ProgramListID& programListId)
{
	programListId = kNoProgramListId;
	unitId = -1;
	if (!mUnitInfo)
		return false;

	// use the first input event bus (VST 2 has only 1 bus for event)
	if (mUnitInfo->getUnitByBus (kEvent, kInput, 0, midiChannel, unitId) == kResultTrue)
	{
		for (int32 i = 0, unitCount = mUnitInfo->getUnitCount (); i < unitCount; i++)
		{
			UnitInfo unitInfoStruct = {0};
			if (mUnitInfo->getUnitInfo (i, unitInfoStruct) == kResultTrue)
			{
				if (unitId == unitInfoStruct.id)
				{
					programListId = unitInfoStruct.programListId;
					return programListId != kNoProgramListId;
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
bool BaseWrapper::getProgramListInfoByProgramListID (ProgramListID programListId,
                                                     ProgramListInfo& info)
{
	if (mUnitInfo)
	{
		int32 programListCount = mUnitInfo->getProgramListCount ();
		for (int32 i = 0; i < programListCount; i++)
		{
			memset (&info, 0, sizeof (ProgramListInfo));
			if (mUnitInfo->getProgramListInfo (i, info) == kResultTrue)
			{
				if (info.id == programListId)
				{
					return true;
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void BaseWrapper::setVendorName (char* name)
{
	memcpy (mVendor, name, sizeof (mVendor));
}

//-----------------------------------------------------------------------------
void BaseWrapper::setEffectName (char* effectName)
{
	memcpy (mName, effectName, sizeof (mName));
}

//-----------------------------------------------------------------------------
void BaseWrapper::setEffectVersion (char* version)
{
	if (!version)
		mVersion = 0;
	else
	{
		int32 major = 1;
		int32 minor = 0;
		int32 subminor = 0;
		int32 subsubminor = 0;
		int32 ret = sscanf (version, "%d.%d.%d.%d", &major, &minor, &subminor, &subsubminor);
		mVersion = (major & 0xff) << 24;
		if (ret > 3)
			mVersion += (subsubminor & 0xff);
		if (ret > 2)
			mVersion += (subminor & 0xff) << 8;
		if (ret > 1)
			mVersion += (minor & 0xff) << 16;
	}
}

//-----------------------------------------------------------------------------
void BaseWrapper::setSubCategories (char* string)
{
	memcpy (mSubCategories, string, sizeof (mSubCategories));
}

//-----------------------------------------------------------------------------
void BaseWrapper::setupBuses ()
{
	if (!mComponent)
		return;

	mProcessData.prepare (*mComponent, 0, mVst3SampleSize);

	_setNumInputs (countMainBusChannels (kInput, mMainAudioInputBuses));
	_setNumOutputs (countMainBusChannels (kOutput, mMainAudioOutputBuses));

	mHasEventInputBuses = mComponent->getBusCount (kEvent, kInput) > 0;
	mHasEventOutputBuses = mComponent->getBusCount (kEvent, kOutput) > 0;

	if (mHasEventInputBuses)
	{
		if (mInputEvents == nullptr)
			mInputEvents = owned (new EventList (kMaxEvents));
	}
	else
		mInputEvents = nullptr;

	if (mHasEventOutputBuses)
	{
		if (mOutputEvents == nullptr)
			mOutputEvents = owned (new EventList (kMaxEvents));
	}
	else
		mOutputEvents = nullptr;
}

//-----------------------------------------------------------------------------
void BaseWrapper::setupParameters ()
{
	mParameterMap.clear ();
	mParamIndexMap.clear ();
	mBypassParameterID = mProgramParameterID = kNoParamId;
	mProgramParameterIdx = -1;

	std::vector<ParameterInfo> programParameterInfos;
	std::vector<int32> programParameterIdxs;

	int32 paramCount = mController ? mController->getParameterCount () : 0;
	int32 numParamID = 0;
	for (int32 i = 0; i < paramCount; i++)
	{
		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (i, paramInfo) == kResultTrue)
		{
			//--- ------------------------------------------
			if ((paramInfo.flags & ParameterInfo::kIsBypass) != 0)
			{
				if (mBypassParameterID == kNoParamId)
					mBypassParameterID = paramInfo.id;
				if (mUseExportedBypass)
				{
					ParamMapEntry entry = {paramInfo.id, i};
					mParameterMap.push_back (entry);
					mParamIndexMap[paramInfo.id] = mUseIncIndex ? numParamID : i;
					numParamID++;
				}
			}
			//--- ------------------------------------------
			else if ((paramInfo.flags & ParameterInfo::kIsProgramChange) != 0)
			{
				programParameterInfos.push_back (paramInfo);
				programParameterIdxs.push_back (i);
				if (paramInfo.unitId == kRootUnitId)
				{
					if (mProgramParameterID == kNoParamId)
					{
						mProgramParameterID = paramInfo.id;
						mProgramParameterIdx = i;
					}
				}

				if (gExportProgramChangeParameters == true)
				{
					ParamMapEntry entry = {paramInfo.id, i};
					mParameterMap.push_back (entry);
					mParamIndexMap[paramInfo.id] = mUseIncIndex ? numParamID : i;
					numParamID++;
				}
			}
			//--- ------------------------------------------
			// do not export read only parameters
			else if ((paramInfo.flags & ParameterInfo::kIsReadOnly) == 0)
			{
				ParamMapEntry entry = {paramInfo.id, i};
				mParameterMap.push_back (entry);
				mParamIndexMap[paramInfo.id] = mUseIncIndex ? numParamID : i;
				numParamID++;
			}
		}
	}

	mNumParams = (int32)mParameterMap.size ();

	mInputTransfer.setMaxParameters (paramCount);
	mOutputTransfer.setMaxParameters (paramCount);
	mGuiTransfer.setMaxParameters (paramCount);
	mInputChanges.setMaxParameters (paramCount);
	mOutputChanges.setMaxParameters (paramCount);

	for (int32 midiChannel = 0; midiChannel < kMaxProgramChangeParameters; midiChannel++)
	{
		mProgramChangeParameterIDs[midiChannel] = kNoParamId;
		mProgramChangeParameterIdxs[midiChannel] = -1;

		UnitID unitId;
		ProgramListID programListId;
		if (getProgramListAndUnit (midiChannel, unitId, programListId))
		{
			for (int32 i = 0; i < (int32)programParameterInfos.size (); i++)
			{
				const ParameterInfo& paramInfo = programParameterInfos.at (i);
				if (paramInfo.unitId == unitId)
				{
					mProgramChangeParameterIDs[midiChannel] = paramInfo.id;
					mProgramChangeParameterIdxs[midiChannel] = programParameterIdxs.at (i);
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void BaseWrapper::initMidiCtrlerAssignment ()
{
	if (!mMidiMapping || !mComponent)
		return;

	int32 busses = Min<int32> (mComponent->getBusCount (kEvent, kInput), kMaxMidiMappingBusses);

	if (!mMidiCCMapping[0][0])
	{
		for (int32 b = 0; b < busses; b++)
			for (int32 i = 0; i < 16; i++)
				mMidiCCMapping[b][i] = NEW int32[Vst::kCountCtrlNumber];
	}

	ParamID paramID;
	for (int32 b = 0; b < busses; b++)
	{
		for (int16 ch = 0; ch < 16; ch++)
		{
			for (int32 i = 0; i < Vst::kCountCtrlNumber; i++)
			{
				paramID = kNoParamId;
				if (mMidiMapping->getMidiControllerAssignment (b, ch, (CtrlNumber)i, paramID) ==
				    kResultTrue)
				{
					// TODO check if tag is associated to a parameter
					mMidiCCMapping[b][ch][i] = paramID;
				}
				else
					mMidiCCMapping[b][ch][i] = kNoParamId;
			}
		}
	}
}

//-----------------------------------------------------------------------------
void BaseWrapper::_setSampleRate (float newSamplerate)
{
	if (mProcessing)
		return;

	if (newSamplerate != mSampleRate)
	{
		mSampleRate = newSamplerate;
		setupProcessing ();
	}
}

//-----------------------------------------------------------------------------
int32 BaseWrapper::countMainBusChannels (BusDirection dir, uint64& mainBusBitset)
{
	int32 result = 0;
	mainBusBitset = 0;

	int32 busCount = mComponent->getBusCount (kAudio, dir);
	for (int32 i = 0; i < busCount; i++)
	{
		BusInfo busInfo = {0};
		if (mComponent->getBusInfo (kAudio, dir, i, busInfo) == kResultTrue)
		{
			if (busInfo.busType == kMain)
			{
				result += busInfo.channelCount;
				mainBusBitset |= (uint64 (1) << i);

				mComponent->activateBus (kAudio, dir, i, true);
			}
			else if (busInfo.flags & BusInfo::kDefaultActive)
			{
				mComponent->activateBus (kAudio, dir, i, false);
			}
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
void BaseWrapper::processMidiEvent (Event& toAdd, char* midiData, bool isLive, int32 noteLength,
                                    float noteOffVelocity, float detune)
{
	uint8 status = midiData[0] & kStatusMask;
	uint8 channel = midiData[0] & kChannelMask;

	// not allowed
	if (channel >= 16)
		return;

	if (isLive)
		toAdd.flags |= Event::kIsLive;

	//--- -----------------------------
	switch (status)
	{
		case kNoteOn:
		case kNoteOff:
		{
			if (status == kNoteOff || midiData[2] == 0) // note off
			{
				toAdd.type = Event::kNoteOffEvent;
				toAdd.noteOff.channel = channel;
				toAdd.noteOff.pitch = midiData[1];
				toAdd.noteOff.velocity = noteOffVelocity;
				toAdd.noteOff.noteId = -1; // TODO ?
			}
			else if (status == kNoteOn) // note on
			{
				toAdd.type = Event::kNoteOnEvent;
				toAdd.noteOn.channel = channel;
				toAdd.noteOn.pitch = midiData[1];
				toAdd.noteOn.tuning = detune;
				toAdd.noteOn.velocity = (float)midiData[2] * kMidiScaler;
				toAdd.noteOn.length = noteLength;
				toAdd.noteOn.noteId = -1; // TODO ?
			}
			mInputEvents->addEvent (toAdd);
		}
		break;
		//--- -----------------------------
		case kPolyPressure:
		{
			toAdd.type = Vst::Event::kPolyPressureEvent;
			toAdd.polyPressure.channel = channel;
			toAdd.polyPressure.pitch = midiData[1] & kDataMask;
			toAdd.polyPressure.pressure = (midiData[2] & kDataMask) * kMidiScaler;
			toAdd.polyPressure.noteId = -1; // TODO ?

			mInputEvents->addEvent (toAdd);
		}
		break;
		//--- -----------------------------
		case kController:
		{
			if (toAdd.busIndex < kMaxMidiMappingBusses && mMidiCCMapping[toAdd.busIndex][channel])
			{
				ParamID paramID =
				    mMidiCCMapping[toAdd.busIndex][channel][static_cast<size_t> (midiData[1])];
				if (paramID != kNoParamId)
				{
					ParamValue value = (double)midiData[2] * kMidiScaler;

					int32 index = 0;
					IParamValueQueue* queue = mInputChanges.addParameterData (paramID, index);
					if (queue)
					{
						queue->addPoint (toAdd.sampleOffset, value, index);
					}
					mGuiTransfer.addChange (paramID, value, toAdd.sampleOffset);
				}
			}
		}
		break;
		//--- -----------------------------
		case kPitchBendStatus:
		{
			if (toAdd.busIndex < kMaxMidiMappingBusses && mMidiCCMapping[toAdd.busIndex][channel])
			{
				ParamID paramID = mMidiCCMapping[toAdd.busIndex][channel][Vst::kPitchBend];
				if (paramID != kNoParamId)
				{
					const double kPitchWheelScaler = 1. / (double)0x3FFF;

					const int32 ctrl = (midiData[1] & kDataMask) | (midiData[2] & kDataMask) << 7;
					ParamValue value = kPitchWheelScaler * (double)ctrl;

					int32 index = 0;
					IParamValueQueue* queue = mInputChanges.addParameterData (paramID, index);
					if (queue)
					{
						queue->addPoint (toAdd.sampleOffset, value, index);
					}
					mGuiTransfer.addChange (paramID, value, toAdd.sampleOffset);
				}
			}
		}
		break;
		//--- -----------------------------
		case kAfterTouchStatus:
		{
			if (toAdd.busIndex < kMaxMidiMappingBusses && mMidiCCMapping[toAdd.busIndex][channel])
			{
				ParamID paramID = mMidiCCMapping[toAdd.busIndex][channel][Vst::kAfterTouch];
				if (paramID != kNoParamId)
				{
					ParamValue value = (ParamValue) (midiData[1] & kDataMask) * kMidiScaler;

					int32 index = 0;
					IParamValueQueue* queue = mInputChanges.addParameterData (paramID, index);
					if (queue)
					{
						queue->addPoint (toAdd.sampleOffset, value, index);
					}
					mGuiTransfer.addChange (paramID, value, toAdd.sampleOffset);
				}
			}
		}
		break;
		//--- -----------------------------
		case kProgramChangeStatus:
		{
			if (mProgramChangeParameterIDs[channel] != kNoParamId &&
			    mProgramChangeParameterIdxs[channel] != -1)
			{
				ParameterInfo paramInfo = {0};
				if (mController->getParameterInfo (mProgramChangeParameterIdxs[channel],
				                                   paramInfo) == kResultTrue)
				{
					int32 program = midiData[1];
					if (paramInfo.stepCount > 0 && program <= paramInfo.stepCount)
					{
						ParamValue normalized =
						    (ParamValue)program / (ParamValue)paramInfo.stepCount;
						addParameterChange (mProgramChangeParameterIDs[channel], normalized,
						                    toAdd.sampleOffset);
					}
				}
			}
		}
		break;
	}
}

//-----------------------------------------------------------------------------
template <class T>
inline void BaseWrapper::setProcessingBuffers (T** inputs, T** outputs)
{
	// set processing buffers
	int32 sourceIndex = 0;
	for (int32 i = 0; i < mProcessData.numInputs; i++)
	{
		AudioBusBuffers& buffers = mProcessData.inputs[i];
		if (mMainAudioInputBuses & (uint64 (1) << i))
		{
			for (int32 j = 0; j < buffers.numChannels; j++)
			{
				buffers.channelBuffers32[j] = (Sample32*)inputs[sourceIndex++];
			}
		}
		else
			buffers.silenceFlags = HostProcessData::kAllChannelsSilent;
	}

	sourceIndex = 0;
	for (int32 i = 0; i < mProcessData.numOutputs; i++)
	{
		AudioBusBuffers& buffers = mProcessData.outputs[i];
		buffers.silenceFlags = 0;
		if (mMainAudioOutputBuses & (uint64 (1) << i))
		{
			for (int32 j = 0; j < buffers.numChannels; j++)
			{
				buffers.channelBuffers32[j] = (Sample32*)outputs[sourceIndex++];
			}
		}
	}
}

//-----------------------------------------------------------------------------
inline void BaseWrapper::setEventPPQPositions ()
{
	if (!mInputEvents)
		return;

	int32 eventCount = mInputEvents->getEventCount ();
	if (eventCount > 0 && (mProcessContext.state & ProcessContext::kTempoValid) &&
	    (mProcessContext.state & ProcessContext::kProjectTimeMusicValid))
	{
		TQuarterNotes projectTimeMusic = mProcessContext.projectTimeMusic;
		double secondsToQuarterNoteScaler = mProcessContext.tempo / 60.0;
		double multiplicator = secondsToQuarterNoteScaler / mSampleRate;

		for (int32 i = 0; i < eventCount; i++)
		{
			Event* e = mInputEvents->getEventByIndex (i);
			if (e)
			{
				TQuarterNotes localTimeMusic = e->sampleOffset * multiplicator;
				e->ppqPosition = projectTimeMusic + localTimeMusic;
			}
		}
	}
}

//-----------------------------------------------------------------------------
inline void BaseWrapper::doProcess (int32 sampleFrames)
{
	if (!mProcessor)
		return;

	mProcessData.numSamples = sampleFrames;

	if (mProcessing == false)
		_startProcess ();

	mProcessData.inputEvents = mInputEvents;
	mProcessData.outputEvents = mOutputEvents;

	setupProcessTimeInfo ();
	setEventPPQPositions ();

	mInputTransfer.transferChangesTo (mInputChanges);

	mProcessData.inputParameterChanges = &mInputChanges;
	mProcessData.outputParameterChanges = &mOutputChanges;
	mOutputChanges.clearQueue ();

	// VST 3 process call
	mProcessor->process (mProcessData);

	processOutputParametersChanges ();

	mOutputTransfer.transferChangesFrom (mOutputChanges);
	processOutputEvents ();

	// clear input parameters and events
	mInputChanges.clearQueue ();
	if (mInputEvents)
		mInputEvents->clear ();
}

//-----------------------------------------------------------------------------
void BaseWrapper::_processReplacing (float** inputs, float** outputs, int32 sampleFrames)
{
	if (mProcessData.symbolicSampleSize != kSample32)
		return;

	setProcessingBuffers<float> (inputs, outputs);
	doProcess (sampleFrames);
}

//-----------------------------------------------------------------------------
void BaseWrapper::_processDoubleReplacing (double** inputs, double** outputs, int32 sampleFrames)
{
	if (mProcessData.symbolicSampleSize != kSample64)
		return;

	setProcessingBuffers<double> (inputs, outputs);
	doProcess (sampleFrames);
}

//-----------------------------------------------------------------------------
void BaseWrapper::onTimer (Timer*)
{
	if (!mController)
		return;

	ParamID id;
	ParamValue value;
	int32 sampleOffset;

	while (mOutputTransfer.getNextChange (id, value, sampleOffset))
	{
		mController->setParamNormalized (id, value);
	}
	while (mGuiTransfer.getNextChange (id, value, sampleOffset))
	{
		mController->setParamNormalized (id, value);
	}
}

// FUnknown
//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::queryInterface (const char* iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, FUnknown::iid, Vst::IHostApplication)
	QUERY_INTERFACE (iid, obj, Vst::IHostApplication::iid, Vst::IHostApplication)
	QUERY_INTERFACE (iid, obj, Vst::IComponentHandler::iid, Vst::IComponentHandler)
	QUERY_INTERFACE (iid, obj, Vst::IUnitHandler::iid, Vst::IUnitHandler)

	*obj = nullptr;
	return kNoInterface;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::restartComponent (int32 flags)
{
	tresult result = kResultFalse;

	//--- ----------------------
	if (flags & kIoChanged)
	{
		setupBuses ();
		_ioChanged ();
		result = kResultTrue;
	}

	//--- ----------------------
	if ((flags & kParamValuesChanged) || (flags & kParamTitlesChanged))
	{
		_updateDisplay ();
		result = kResultTrue;
	}

	//--- ----------------------
	if (flags & kLatencyChanged)
	{
		if (mProcessor)
			_setInitialDelay (mProcessor->getLatencySamples ());

		_ioChanged ();
		result = kResultTrue;
	}

	//--- ----------------------
	if (flags & kMidiCCAssignmentChanged)
	{
		initMidiCtrlerAssignment ();
		result = kResultTrue;
	}

	// kReloadComponent is Not supported

	return result;
}

//-----------------------------------------------------------------------------
// IHostApplication
//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::createInstance (TUID cid, TUID iid, void** obj)
{
	FUID classID (FUID::fromTUID (cid));
	FUID interfaceID (FUID::fromTUID (iid));
	if (classID == IMessage::iid && interfaceID == IMessage::iid)
	{
		*obj = new HostMessage;
		return kResultTrue;
	}
	else if (classID == IAttributeList::iid && interfaceID == IAttributeList::iid)
	{
		*obj = new HostAttributeList;
		return kResultTrue;
	}
	*obj = nullptr;
	return kResultFalse;
}

//-----------------------------------------------------------------------------
// IUnitHandler
//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::notifyUnitSelection (UnitID unitId)
{
	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::notifyProgramListChange (ProgramListID listId, int32 programIndex)
{
	// TODO -> redirect to hasMidiProgramsChanged somehow...
	return kResultTrue;
}

//-----------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//-----------------------------------------------------------------------------

/// \endcond
