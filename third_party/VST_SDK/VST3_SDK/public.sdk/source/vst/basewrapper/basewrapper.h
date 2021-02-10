//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/basewrapper/basewrapper.h
// Created by  : Steinberg, 01/2018
// Description : Base Wrapper
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

#pragma once

/// \cond ignore

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstunits.h"

#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/processdata.h"

#include "base/source/fstring.h"
#include "base/source/timer.h"

#include <map>
#include <vector>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
class BaseEditorWrapper : public IPlugFrame
{
public:
//------------------------------------------------------------------------
	BaseEditorWrapper (IEditController* controller);
	~BaseEditorWrapper ();

	static bool hasEditor (IEditController* controller);

	bool getRect (ViewRect& rect);
	virtual bool _open (void* ptr);
	virtual void _close ();

	bool _setKnobMode (Vst::KnobMode val);

	// IPlugFrame
	tresult PLUGIN_API resizeView (IPlugView* view, ViewRect* newSize) SMTG_OVERRIDE;

	// FUnknown
	tresult PLUGIN_API queryInterface (const char* _iid, void** obj) SMTG_OVERRIDE;
	uint32 PLUGIN_API addRef () SMTG_OVERRIDE { return 1; }
	uint32 PLUGIN_API release () SMTG_OVERRIDE { return 1; }
//------------------------------------------------------------------------
protected:
	void createView ();

	IPtr<IEditController> mController;
	IPtr<IPlugView> mView;

	ViewRect mViewRect;
};

//------------------------------------------------------------------------
const int32 kMaxEvents = 2048;

//-------------------------------------------------------------------------------------------------------
class BaseWrapper : public IHostApplication,
                    public IComponentHandler,
                    public IUnitHandler,
                    public ITimerCallback
{
public:
	struct SVST3Config
	{
		IPluginFactory* factory = nullptr;
		IAudioProcessor* processor = nullptr;
		IEditController* controller = nullptr;
		FUID vst3ComponentID;
	};

	BaseWrapper (SVST3Config& config);
	virtual ~BaseWrapper ();

	virtual bool init ();

	virtual void _canDoubleReplacing (bool val) {}
	virtual void _setInitialDelay (int32 delay) {}
	virtual void _noTail (bool val) {}

	virtual void _ioChanged () {}
	virtual void _updateDisplay () {}
	virtual void _setNumInputs (int32 inputs) { mNumInputs = inputs; }
	virtual void _setNumOutputs (int32 outputs) { mNumOutputs = outputs; }
	virtual bool _sizeWindow (int32 width, int32 height) = 0;
	virtual int32 _getChunk (void** data, bool isPreset);
	virtual int32 _setChunk (void* data, int32 byteSize, bool isPreset);

	virtual bool getEditorSize (int32& width, int32& height) const;

	bool isActive () const { return mActive; }
	int32 getNumInputs () const { return mNumInputs; }
	int32 getNumOutputs () const { return mNumOutputs; }

	BaseEditorWrapper* getEditor () const { return mEditor; }

	//--- ---------------------------------------------------------------------
	// VST 3 Interfaces  ------------------------------------------------------
	// FUnknown
	tresult PLUGIN_API queryInterface (const char* iid, void** obj) SMTG_OVERRIDE;
	uint32 PLUGIN_API addRef () SMTG_OVERRIDE { return 1; }
	uint32 PLUGIN_API release () SMTG_OVERRIDE { return 1; }

	// IHostApplication
	tresult PLUGIN_API createInstance (TUID cid, TUID iid, void** obj) SMTG_OVERRIDE;

	// IComponentHandler
	tresult PLUGIN_API restartComponent (int32 flags) SMTG_OVERRIDE;

	// IUnitHandler
	tresult PLUGIN_API notifyUnitSelection (UnitID unitId) SMTG_OVERRIDE;
	tresult PLUGIN_API notifyProgramListChange (ProgramListID listId,
	                                            int32 programIndex) SMTG_OVERRIDE;

	// ITimer
	void onTimer (Timer* timer) SMTG_OVERRIDE;

//-------------------------------------------------------------------------------------------------------
protected:
	virtual void setupParameters ();
	virtual void setupProcessTimeInfo () = 0;
	virtual void processOutputEvents () {}
	virtual void processOutputParametersChanges () {}

	void _setSampleRate (float newSamplerate);
	bool setupProcessing (int32 processModeOverwrite = -1);
	void _processReplacing (float** inputs, float** outputs, int32 sampleFrames);
	void _processDoubleReplacing (double** inputs, double** outputs, int32 sampleFrames);

	template <class T>
	void setProcessingBuffers (T** inputs, T** outputs);
	void doProcess (int32 sampleFrames);

	void processMidiEvent (Event& toAdd, char* midiData, bool isLive = false, int32 noteLength = 0,
	                       float noteOffVelocity = 1.f, float detune = 0.f);

	void setEventPPQPositions ();

	void _setEditor (BaseEditorWrapper* editor);

	bool _setBlockSize (int32 newBlockSize);
	float _getParameter (int32 index) const;

	void _suspend ();
	void _resume ();
	void _startProcess ();
	void _stopProcess ();
	bool _setBypass (bool onOff);

	virtual void setupBuses ();
	void initMidiCtrlerAssignment ();
	void getUnitPath (UnitID unitID, String& path) const;

	int32 countMainBusChannels (BusDirection dir, uint64& mainBusBitset);

	/**	Returns the last param change from guiTransfer queue. */
	bool getLastParamChange (ParamID id, ParamValue& value);

	void addParameterChange (ParamID id, ParamValue value, int32 sampleOffset);

	void setVendorName (char* name);
	void setEffectName (char* name);
	void setEffectVersion (char* version);
	void setSubCategories (char* string);

	bool getProgramListAndUnit (int32 midiChannel, UnitID& unitId, ProgramListID& programListId);
	bool getProgramListInfoByProgramListID (ProgramListID programListId, ProgramListInfo& info);

	static const int32 kMaxProgramChangeParameters = 16;
	ParamID mProgramChangeParameterIDs[kMaxProgramChangeParameters]; // for each MIDI channel
	int32 mProgramChangeParameterIdxs[kMaxProgramChangeParameters]; // for each MIDI channel

	FUID mVst3EffectClassID;

	// vst3 data
	IPtr<IAudioProcessor> mProcessor;
	IPtr<IComponent> mComponent;
	IPtr<IEditController> mController;
	IPtr<IUnitInfo> mUnitInfo;
	IPtr<IMidiMapping> mMidiMapping;

	IPtr<BaseEditorWrapper> mEditor;

	int32 mVst3SampleSize = kSample32;
	int32 mVst3processMode = kRealtime;

	char mName[PClassInfo::kNameSize];
	char mVendor[PFactoryInfo::kNameSize];
	char mSubCategories[PClassInfo2::kSubCategoriesSize];
	int32 mVersion = 0;

	struct ParamMapEntry
	{
		ParamID vst3ID;
		int32 vst3Index;
	};

	std::vector<ParamMapEntry> mParameterMap;
	std::map<ParamID, int32> mParamIndexMap;
	ParamID mBypassParameterID = kNoParamId;
	ParamID mProgramParameterID = kNoParamId;
	int32 mProgramParameterIdx = -1;

	HostProcessData mProcessData;
	ProcessContext mProcessContext;
	ParameterChanges mInputChanges;
	ParameterChanges mOutputChanges;
	IPtr<EventList> mInputEvents;
	IPtr<EventList> mOutputEvents;

	uint64 mMainAudioInputBuses = 0;
	uint64 mMainAudioOutputBuses = 0;

	ParameterChangeTransfer mInputTransfer;
	ParameterChangeTransfer mOutputTransfer;
	ParameterChangeTransfer mGuiTransfer;

	MemoryStream mChunk;

	IPtr<Timer> mTimer;
	IPtr<IPluginFactory> mFactory;

	int32 mNumPrograms {0};
	float mSampleRate {44100};
	int32 mBlockSize {256};
	int32 mNumParams {0};
	int32 mCurProgram {-1};
	int32 mNumInputs {0};
	int32 mNumOutputs {0};

	enum
	{
		kMaxMidiMappingBusses = 4
	};
	int32* mMidiCCMapping[kMaxMidiMappingBusses][16];

	bool mComponentInitialized = false;
	bool mControllerInitialized = false;
	bool mComponentsConnected = false;
	bool mUseExportedBypass = true;

	bool mActive = false;
	bool mProcessing = false;
	bool mHasEventInputBuses = false;
	bool mHasEventOutputBuses = false;

	bool mUseIncIndex = true;
};

const uint8 kNoteOff = 0x80; ///< note, off velocity
const uint8 kNoteOn = 0x90; ///< note, on velocity
const uint8 kPolyPressure = 0xA0; ///< note, pressure
const uint8 kController = 0xB0; ///< controller, value
const uint8 kProgramChangeStatus = 0xC0; ///< program change
const uint8 kAfterTouchStatus = 0xD0; ///< channel pressure
const uint8 kPitchBendStatus = 0xE0; ///< lsb, msb

const float kMidiScaler = 1.f / 127.f;
static const uint8 kChannelMask = 0x0F;
static const uint8 kStatusMask = 0xF0;
static const uint32 kDataMask = 0x7F;
//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

/// \endcond
