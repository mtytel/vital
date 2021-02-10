//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vst2wrapper/vst2wrapper.h
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

#pragma once

#include "docvst2.h"

/// \cond ignore
#include "public.sdk/source/vst/basewrapper/basewrapper.h"
#include "public.sdk/source/vst2.x/audioeffectx.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

class Vst2MidiEventQueue;

//------------------------------------------------------------------------
class Vst2Wrapper : public BaseWrapper, public ::AudioEffectX, public IVst3ToVst2Wrapper
{
public:
	// will owned factory
	static AudioEffect* create (IPluginFactory* factory, const TUID vst3ComponentID,
		VstInt32 vst2ID, audioMasterCallback audioMaster);

	Vst2Wrapper (BaseWrapper::SVST3Config& config, audioMasterCallback audioMaster, VstInt32 vst2ID);
	virtual ~Vst2Wrapper ();

	//--- ------------------------------------------------------
	//--- BaseWrapper ------------------------------------------
	bool init () SMTG_OVERRIDE;
	void _canDoubleReplacing (bool val) SMTG_OVERRIDE;
	void _setInitialDelay (int32 delay) SMTG_OVERRIDE;
	void _noTail (bool val) SMTG_OVERRIDE;
	void setupProcessTimeInfo () SMTG_OVERRIDE;

	void setupParameters () SMTG_OVERRIDE;
	void setupBuses () SMTG_OVERRIDE;

	void _ioChanged () SMTG_OVERRIDE;
	void _updateDisplay () SMTG_OVERRIDE;
	void _setNumInputs (int32 inputs) SMTG_OVERRIDE;
	void _setNumOutputs (int32 outputs) SMTG_OVERRIDE;
	bool _sizeWindow (int32 width, int32 height) SMTG_OVERRIDE;

	//--- ---------------------------------------------------------------------
	// VST 3 Interfaces  ------------------------------------------------------
	// IComponentHandler
	tresult PLUGIN_API beginEdit (ParamID tag) SMTG_OVERRIDE;
	tresult PLUGIN_API performEdit (ParamID tag, ParamValue valueNormalized) SMTG_OVERRIDE;
	tresult PLUGIN_API endEdit (ParamID tag) SMTG_OVERRIDE;

	// IHostApplication
	tresult PLUGIN_API getName (String128 name) SMTG_OVERRIDE;

	//--- ---------------------------------------------------------------------
	// VST 2 AudioEffectX overrides -----------------------------------------------
	void suspend () SMTG_OVERRIDE; // Called when Plug-in is switched to off
	void resume () SMTG_OVERRIDE; // Called when Plug-in is switched to on
	VstInt32 startProcess () SMTG_OVERRIDE;
	VstInt32 stopProcess () SMTG_OVERRIDE;

	// Called when the sample rate changes (always in a suspend state)
	void setSampleRate (float newSamplerate) SMTG_OVERRIDE;

	// Called when the maximum block size changes
	// (always in a suspend state). Note that the
	// sampleFrames in Process Calls could be
	// smaller than this block size, but NOT bigger.
	void setBlockSize (VstInt32 newBlockSize) SMTG_OVERRIDE;

	float getParameter (VstInt32 index) SMTG_OVERRIDE;
	void setParameter (VstInt32 index, float value) SMTG_OVERRIDE;

	void setProgram (VstInt32 program) SMTG_OVERRIDE;
	void setProgramName (char* name) SMTG_OVERRIDE;
	void getProgramName (char* name) SMTG_OVERRIDE;
	bool getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text) SMTG_OVERRIDE;

	void getParameterLabel (VstInt32 index, char* label) SMTG_OVERRIDE;
	void getParameterDisplay (VstInt32 index, char* text) SMTG_OVERRIDE;
	void getParameterName (VstInt32 index, char* text) SMTG_OVERRIDE;
	bool canParameterBeAutomated (VstInt32 index) SMTG_OVERRIDE;
	bool string2parameter (VstInt32 index, char* text) SMTG_OVERRIDE;
	bool getParameterProperties (VstInt32 index, VstParameterProperties* p) SMTG_OVERRIDE;

	VstInt32 getChunk (void** data, bool isPreset = false) SMTG_OVERRIDE;
	VstInt32 setChunk (void* data, VstInt32 byteSize, bool isPreset = false) SMTG_OVERRIDE;

	bool getInputProperties (VstInt32 index, VstPinProperties* properties) SMTG_OVERRIDE;
	bool getOutputProperties (VstInt32 index, VstPinProperties* properties) SMTG_OVERRIDE;
	bool setSpeakerArrangement (VstSpeakerArrangement* pluginInput,
		VstSpeakerArrangement* pluginOutput) SMTG_OVERRIDE;
	bool getSpeakerArrangement (VstSpeakerArrangement** pluginInput,
		VstSpeakerArrangement** pluginOutput) SMTG_OVERRIDE;
	bool setBypass (bool onOff) SMTG_OVERRIDE;

	bool setProcessPrecision (VstInt32 precision) SMTG_OVERRIDE;
	VstInt32 getNumMidiInputChannels () SMTG_OVERRIDE;
	VstInt32 getNumMidiOutputChannels () SMTG_OVERRIDE;
	VstInt32 getGetTailSize () SMTG_OVERRIDE;
	bool getEffectName (char* name) SMTG_OVERRIDE;
	bool getVendorString (char* text) SMTG_OVERRIDE;
	VstInt32 getVendorVersion () SMTG_OVERRIDE;
	VstIntPtr vendorSpecific (VstInt32 lArg, VstIntPtr lArg2, void* ptrArg,
		float floatArg) SMTG_OVERRIDE;
	VstPlugCategory getPlugCategory () SMTG_OVERRIDE;
	VstInt32 canDo (char* text) SMTG_OVERRIDE;

	VstInt32 getMidiProgramName (VstInt32 channel, MidiProgramName* midiProgramName) SMTG_OVERRIDE;
	VstInt32 getCurrentMidiProgram (VstInt32 channel,
		MidiProgramName* currentProgram) SMTG_OVERRIDE;
	VstInt32 getMidiProgramCategory (VstInt32 channel, MidiProgramCategory* category) SMTG_OVERRIDE;
	bool hasMidiProgramsChanged (VstInt32 channel) SMTG_OVERRIDE;
	bool getMidiKeyName (VstInt32 channel, MidiKeyName* keyName) SMTG_OVERRIDE;

	// finally process...
	void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames) SMTG_OVERRIDE;
	void processDoubleReplacing (double** inputs, double** outputs,
		VstInt32 sampleFrames) SMTG_OVERRIDE;
	VstInt32 processEvents (VstEvents* events) SMTG_OVERRIDE;

	bool getPinProperties (BusDirection dir, VstInt32 pinIndex, VstPinProperties* properties);
	bool pinIndexToBusChannel (BusDirection dir, VstInt32 pinIndex, int32& busIndex,
		int32& busChannel);

	//--- ------------------------------------------------------
	DEFINE_INTERFACES
		DEF_INTERFACE (Vst::IVst3ToVst2Wrapper)
	END_DEFINE_INTERFACES (BaseWrapper)
	REFCOUNT_METHODS (BaseWrapper)

	static VstInt32 vst3ToVst2SpeakerArr (SpeakerArrangement vst3Arr);
	static SpeakerArrangement vst2ToVst3SpeakerArr (VstInt32 vst2Arr);
	static VstInt32 vst3ToVst2Speaker (Speaker vst3Speaker);
	static void setupVst2Arrangement (VstSpeakerArrangement*& vst2arr,
		Vst::SpeakerArrangement vst3Arrangement);

	//------------------------------------------------------------------------
protected:
	VstSpeakerArrangement* mVst2InputArrangement {nullptr};
	VstSpeakerArrangement* mVst2OutputArrangement {nullptr};
	Vst2MidiEventQueue* mVst2OutputEvents {nullptr};
	VstInt32 mCurrentProcessLevel {kVstProcessLevelRealtime};

	void updateProcessLevel ();

	bool setupMidiProgram (int32 midiChannel, ProgramListID programListId,
	                       MidiProgramName& midiProgramName);
	int32 lookupProgramCategory (int32 midiChannel, String128 instrumentAttribute);
	struct ProgramCategory
	{
		MidiProgramCategory vst2Category;
		String128 vst3InstrumentAttribute;
	};
	std::vector<std::vector<ProgramCategory>> mProgramCategories;
	void setupProgramCategories ();
	static uint32 makeCategoriesRecursive (std::vector<ProgramCategory>& channelCategories,
	                                       String128 vst3Category);

	void processOutputEvents () SMTG_OVERRIDE;

};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

/** Must be implemented externally. */
extern ::AudioEffect* createEffectInstance (audioMasterCallback audioMaster);

/// \endcond
