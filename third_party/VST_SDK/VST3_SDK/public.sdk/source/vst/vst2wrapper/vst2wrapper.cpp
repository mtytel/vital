//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vst2wrapper/vst2wrapper.cpp
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

#include "public.sdk/source/vst/vst2wrapper/vst2wrapper.h"

#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst2.x/aeffeditor.h"

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

extern bool DeinitModule (); //! Called in Vst2Wrapper destructor

//------------------------------------------------------------------------
// some Defines
//------------------------------------------------------------------------
// should be kVstMaxParamStrLen if we want to respect the VST 2 specification!!!
#define kVstExtMaxParamStrLen 32

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//! The parameter's name contains the unit path (e.g. "Modulators.LFO 1.frequency")
bool vst2WrapperFullParameterPath = true;

//------------------------------------------------------------------------
// Vst2EditorWrapper Declaration
//------------------------------------------------------------------------
class Vst2EditorWrapper : public AEffEditor, public BaseEditorWrapper
{
public:
//------------------------------------------------------------------------
	Vst2EditorWrapper (AudioEffect* effect, IEditController* controller);

	//--- from BaseEditorWrapper ---------------------
	void _close () SMTG_OVERRIDE;

	//--- from AEffEditor-------------------
	bool getRect (ERect** rect) SMTG_OVERRIDE;
	bool open (void* ptr) SMTG_OVERRIDE;
	void close () SMTG_OVERRIDE { _close (); }
	bool setKnobMode (VstInt32 val) SMTG_OVERRIDE
	{
		return BaseEditorWrapper::_setKnobMode (static_cast<Vst::KnobMode> (val));
	}

	///< Receives key down event. Return true only if key was really used!
	bool onKeyDown (VstKeyCode& keyCode) SMTG_OVERRIDE;
	///< Receives key up event. Return true only if key was really used!
	bool onKeyUp (VstKeyCode& keyCode) SMTG_OVERRIDE;
	///< Handles mouse wheel event, distance is positive or negative to indicate wheel direction.
	bool onWheel (float distance) SMTG_OVERRIDE;

	//--- IPlugFrame ----------------------------
	tresult PLUGIN_API resizeView (IPlugView* view, ViewRect* newSize) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
	ERect mERect;
};

//------------------------------------------------------------------------
bool areSizeEquals (const ViewRect &r1, const ViewRect& r2)
{
	if (r1.getHeight() != r2.getHeight ())
		return false;
	if (r1.getWidth() != r2.getWidth ())
		return false;
	return true;
}

//------------------------------------------------------------------------
// Vst2EditorWrapper Implementation
//------------------------------------------------------------------------
Vst2EditorWrapper::Vst2EditorWrapper (AudioEffect* effect, IEditController* controller)
: BaseEditorWrapper (controller), AEffEditor (effect)
{
}

//------------------------------------------------------------------------
tresult PLUGIN_API Vst2EditorWrapper::resizeView (IPlugView* view, ViewRect* newSize)
{
	tresult result = kResultFalse;
	if (view && newSize && effect)
	{
		if (areSizeEquals (*newSize, mViewRect))
			return kResultTrue;

		auto* effectx = dynamic_cast<AudioEffectX*> (effect);
		if (effectx && effectx->sizeWindow (newSize->getWidth (), newSize->getHeight ()))
		{
			result = view->onSize (newSize);
		}
	}

	return result;
}

//------------------------------------------------------------------------
bool Vst2EditorWrapper::getRect (ERect** rect)
{
	ViewRect size;
	if (BaseEditorWrapper::getRect (size))
	{
		mERect.left = (VstInt16)size.left;
		mERect.top = (VstInt16)size.top;
		mERect.right = (VstInt16)size.right;
		mERect.bottom = (VstInt16)size.bottom;

		*rect = &mERect;
		return true;
	}

	*rect = nullptr;
	return false;
}

//------------------------------------------------------------------------
bool Vst2EditorWrapper::open (void* ptr)
{
	AEffEditor::open (ptr);
	return BaseEditorWrapper::_open (ptr);
}

//------------------------------------------------------------------------
void Vst2EditorWrapper::_close ()
{
	BaseEditorWrapper::_close ();
	AEffEditor::close ();
}

//------------------------------------------------------------------------
bool Vst2EditorWrapper::onKeyDown (VstKeyCode& keyCode)
{
	if (!mView)
		return false;

	return mView->onKeyDown (VirtualKeyCodeToChar (keyCode.virt), keyCode.virt, keyCode.modifier) ==
	       kResultTrue;
}

//------------------------------------------------------------------------
bool Vst2EditorWrapper::onKeyUp (VstKeyCode& keyCode)
{
	if (!mView)
		return false;

	return mView->onKeyUp (VirtualKeyCodeToChar (keyCode.virt), keyCode.virt, keyCode.modifier) ==
	       kResultTrue;
}

//------------------------------------------------------------------------
bool Vst2EditorWrapper::onWheel (float distance)
{
	if (!mView)
		return false;

	return mView->onWheel (distance) == kResultTrue;
}

//------------------------------------------------------------------------
// Vst2MidiEventQueue Declaration
//------------------------------------------------------------------------
class Vst2MidiEventQueue
{
public:
//------------------------------------------------------------------------
	Vst2MidiEventQueue (int32 maxEventCount);
	~Vst2MidiEventQueue ();

	bool isEmpty () const { return eventList->numEvents == 0; }
	bool add (const VstMidiEvent& e);
	bool add (const VstMidiSysexEvent& e);
	void flush ();

	operator VstEvents* () { return eventList; }
//------------------------------------------------------------------------
protected:
	VstEvents* eventList;
	int32 maxEventCount;
};

//------------------------------------------------------------------------
// Vst2MidiEventQueue Implementation
//------------------------------------------------------------------------
Vst2MidiEventQueue::Vst2MidiEventQueue (int32 _maxEventCount) : maxEventCount (_maxEventCount)
{
	eventList = (VstEvents*)new char[sizeof (VstEvents) + (maxEventCount - 2) * sizeof (VstEvent*)];
	eventList->numEvents = 0;
	eventList->reserved = 0;

	int32 eventSize = sizeof (VstMidiSysexEvent) > sizeof (VstMidiEvent) ?
	                      sizeof (VstMidiSysexEvent) :
	                      sizeof (VstMidiEvent);

	for (int32 i = 0; i < maxEventCount; i++)
	{
		char* eventBuffer = new char[eventSize];
		memset (eventBuffer, 0, eventSize);
		eventList->events[i] = (VstEvent*)eventBuffer;
	}
}

//------------------------------------------------------------------------
Vst2MidiEventQueue::~Vst2MidiEventQueue ()
{
	for (int32 i = 0; i < maxEventCount; i++)
		delete[] (char*) eventList->events[i];

	delete[] (char*) eventList;
}

//------------------------------------------------------------------------
bool Vst2MidiEventQueue::add (const VstMidiEvent& e)
{
	if (eventList->numEvents >= maxEventCount)
		return false;

	auto* dst = (VstMidiEvent*)eventList->events[eventList->numEvents++];
	memcpy (dst, &e, sizeof (VstMidiEvent));
	dst->type = kVstMidiType;
	dst->byteSize = sizeof (VstMidiEvent);
	return true;
}

//------------------------------------------------------------------------
bool Vst2MidiEventQueue::add (const VstMidiSysexEvent& e)
{
	if (eventList->numEvents >= maxEventCount)
		return false;

	auto* dst = (VstMidiSysexEvent*)eventList->events[eventList->numEvents++];
	memcpy (dst, &e, sizeof (VstMidiSysexEvent));
	dst->type = kVstSysExType;
	dst->byteSize = sizeof (VstMidiSysexEvent);
	return true;
}

//------------------------------------------------------------------------
void Vst2MidiEventQueue::flush ()
{
	eventList->numEvents = 0;
}

//------------------------------------------------------------------------
// Vst2Wrapper
//------------------------------------------------------------------------
Vst2Wrapper::Vst2Wrapper (BaseWrapper::SVST3Config& config, audioMasterCallback audioMaster,
                          VstInt32 vst2ID)
: BaseWrapper (config), AudioEffectX (audioMaster, 0, 0)
{
	mUseExportedBypass = false;
	mUseIncIndex = true;

	setUniqueID (vst2ID);
	canProcessReplacing (true); // supports replacing output
	programsAreChunks (true);
}

//------------------------------------------------------------------------
Vst2Wrapper::~Vst2Wrapper ()
{
	//! editor needs to be destroyed BEFORE DeinitModule. Therefore destroy it here already
	//  instead of AudioEffect destructor
	if (mEditor)
	{
		setEditor (nullptr);
		mEditor->release ();
		mEditor = nullptr;
	}

	if (mVst2InputArrangement)
		free (mVst2InputArrangement);
	if (mVst2OutputArrangement)
		free (mVst2OutputArrangement);

	delete mVst2OutputEvents;
	mVst2OutputEvents = nullptr;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::init ()
{
	bool res = BaseWrapper::init ();

	numPrograms = cEffect.numPrograms = mNumPrograms;

	if (mController)
	{
		if (BaseEditorWrapper::hasEditor (mController))
		{
			auto* editor = new Vst2EditorWrapper (this, mController);
			_setEditor (editor);
			setEditor (editor);
		}
	}
	return res;
}

//------------------------------------------------------------------------
void Vst2Wrapper::_canDoubleReplacing (bool val)
{
	canDoubleReplacing (val);
}

//------------------------------------------------------------------------
void Vst2Wrapper::_setInitialDelay (int32 delay)
{
	setInitialDelay (delay);
}

//------------------------------------------------------------------------
void Vst2Wrapper::_noTail (bool val)
{
	noTail (val);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setupBuses ()
{
	BaseWrapper::setupBuses ();

	if (mHasEventOutputBuses)
	{
		if (mVst2OutputEvents == nullptr)
			mVst2OutputEvents = new Vst2MidiEventQueue (kMaxEvents);
	}
	else
	{
		if (mVst2OutputEvents)
		{
			delete mVst2OutputEvents;
			mVst2OutputEvents = nullptr;
		}
	}
}
//------------------------------------------------------------------------
void Vst2Wrapper::setupProcessTimeInfo ()
{
	VstTimeInfo* vst2timeInfo = AudioEffectX::getTimeInfo (0xffffffff);
	if (vst2timeInfo)
	{
		const uint32 portableFlags =
		    ProcessContext::kPlaying | ProcessContext::kCycleActive | ProcessContext::kRecording |
		    ProcessContext::kSystemTimeValid | ProcessContext::kProjectTimeMusicValid |
		    ProcessContext::kBarPositionValid | ProcessContext::kCycleValid |
		    ProcessContext::kTempoValid | ProcessContext::kTimeSigValid |
		    ProcessContext::kSmpteValid | ProcessContext::kClockValid;

		mProcessContext.state = ((uint32)vst2timeInfo->flags) & portableFlags;
		mProcessContext.sampleRate = vst2timeInfo->sampleRate;
		mProcessContext.projectTimeSamples = (TSamples)vst2timeInfo->samplePos;

		if (mProcessContext.state & ProcessContext::kSystemTimeValid)
			mProcessContext.systemTime = (TSamples)vst2timeInfo->nanoSeconds;
		else
			mProcessContext.systemTime = 0;

		if (mProcessContext.state & ProcessContext::kProjectTimeMusicValid)
			mProcessContext.projectTimeMusic = vst2timeInfo->ppqPos;
		else
			mProcessContext.projectTimeMusic = 0;

		if (mProcessContext.state & ProcessContext::kBarPositionValid)
			mProcessContext.barPositionMusic = vst2timeInfo->barStartPos;
		else
			mProcessContext.barPositionMusic = 0;

		if (mProcessContext.state & ProcessContext::kCycleValid)
		{
			mProcessContext.cycleStartMusic = vst2timeInfo->cycleStartPos;
			mProcessContext.cycleEndMusic = vst2timeInfo->cycleEndPos;
		}
		else
			mProcessContext.cycleStartMusic = mProcessContext.cycleEndMusic = 0.0;

		if (mProcessContext.state & ProcessContext::kTempoValid)
			mProcessContext.tempo = vst2timeInfo->tempo;
		else
			mProcessContext.tempo = 120.0;

		if (mProcessContext.state & ProcessContext::kTimeSigValid)
		{
			mProcessContext.timeSigNumerator = vst2timeInfo->timeSigNumerator;
			mProcessContext.timeSigDenominator = vst2timeInfo->timeSigDenominator;
		}
		else
			mProcessContext.timeSigNumerator = mProcessContext.timeSigDenominator = 4;

		mProcessContext.frameRate.flags = 0;
		if (mProcessContext.state & ProcessContext::kSmpteValid)
		{
			mProcessContext.smpteOffsetSubframes = vst2timeInfo->smpteOffset;
			switch (vst2timeInfo->smpteFrameRate)
			{
				case kVstSmpte24fps: ///< 24 fps
					mProcessContext.frameRate.framesPerSecond = 24;
					break;
				case kVstSmpte25fps: ///< 25 fps
					mProcessContext.frameRate.framesPerSecond = 25;
					break;
				case kVstSmpte2997fps: ///< 29.97 fps
					mProcessContext.frameRate.framesPerSecond = 30;
					mProcessContext.frameRate.flags = FrameRate::kPullDownRate;
					break;
				case kVstSmpte30fps: ///< 30 fps
					mProcessContext.frameRate.framesPerSecond = 30;
					break;
				case kVstSmpte2997dfps: ///< 29.97 drop
					mProcessContext.frameRate.framesPerSecond = 30;
					mProcessContext.frameRate.flags =
					    FrameRate::kPullDownRate | FrameRate::kDropRate;
					break;
				case kVstSmpte30dfps: ///< 30 drop
					mProcessContext.frameRate.framesPerSecond = 30;
					mProcessContext.frameRate.flags = FrameRate::kDropRate;
					break;
				case kVstSmpteFilm16mm: // not a smpte rate
				case kVstSmpteFilm35mm:
					mProcessContext.state &= ~ProcessContext::kSmpteValid;
					break;
				case kVstSmpte239fps: ///< 23.9 fps
					mProcessContext.frameRate.framesPerSecond = 24;
					mProcessContext.frameRate.flags = FrameRate::kPullDownRate;
					break;
				case kVstSmpte249fps: ///< 24.9 fps
					mProcessContext.frameRate.framesPerSecond = 25;
					mProcessContext.frameRate.flags = FrameRate::kPullDownRate;
					break;
				case kVstSmpte599fps: ///< 59.9 fps
					mProcessContext.frameRate.framesPerSecond = 60;
					mProcessContext.frameRate.flags = FrameRate::kPullDownRate;
					break;
				case kVstSmpte60fps: ///< 60 fps
					mProcessContext.frameRate.framesPerSecond = 60;
					break;
				default: mProcessContext.state &= ~ProcessContext::kSmpteValid; break;
			}
		}
		else
		{
			mProcessContext.smpteOffsetSubframes = 0;
			mProcessContext.frameRate.framesPerSecond = 0;
		}

		///< MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest)
		if (mProcessContext.state & ProcessContext::kClockValid)
			mProcessContext.samplesToNextClock = vst2timeInfo->samplesToNextClock;
		else
			mProcessContext.samplesToNextClock = 0;

		mProcessData.processContext = &mProcessContext;
	}
	else
		mProcessData.processContext = nullptr;
}

//------------------------------------------------------------------------
void Vst2Wrapper::suspend ()
{
	BaseWrapper::_suspend ();
}

//------------------------------------------------------------------------
void Vst2Wrapper::resume ()
{
	AudioEffectX::resume ();
	BaseWrapper::_resume ();
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::startProcess ()
{
	BaseWrapper::_startProcess ();
	return 0;
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::stopProcess ()
{
	BaseWrapper::_stopProcess ();
	return 0;
}

//------------------------------------------------------------------------
void Vst2Wrapper::setSampleRate (float newSamplerate)
{
	BaseWrapper::_setSampleRate (newSamplerate);
	AudioEffectX::setSampleRate (mSampleRate);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setBlockSize (VstInt32 newBlockSize)
{
	if (BaseWrapper::_setBlockSize (newBlockSize))
		AudioEffectX::setBlockSize (newBlockSize);
}

//------------------------------------------------------------------------
float Vst2Wrapper::getParameter (VstInt32 index)
{
	return BaseWrapper::_getParameter (index);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setParameter (VstInt32 index, float value)
{
	if (!mController)
		return;

	if (index < (int32)mParameterMap.size ())
	{
		ParamID id = mParameterMap.at (index).vst3ID;
		addParameterChange (id, (ParamValue)value, 0);
	}
}

//------------------------------------------------------------------------
void Vst2Wrapper::setProgram (VstInt32 program)
{
	if (mProgramParameterID != kNoParamId && mController != nullptr && mProgramParameterIdx != -1)
	{
		AudioEffectX::setProgram (program);

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (mProgramParameterIdx, paramInfo) == kResultTrue)
		{
			if (paramInfo.stepCount > 0 && program <= paramInfo.stepCount)
			{
				ParamValue normalized = (ParamValue)program / (ParamValue)paramInfo.stepCount;
				addParameterChange (mProgramParameterID, normalized, 0);
			}
		}
	}
}

//------------------------------------------------------------------------
void Vst2Wrapper::setProgramName (char* name)
{
	// not supported in VST 3
}

//------------------------------------------------------------------------
void Vst2Wrapper::getProgramName (char* name)
{
	// name of the current program. Limited to #kVstMaxProgNameLen.
	*name = 0;
	if (mUnitInfo)
	{
		ProgramListInfo listInfo = {0};
		if (mUnitInfo->getProgramListInfo (0, listInfo) == kResultTrue)
		{
			String128 tmp = {0};
			if (mUnitInfo->getProgramName (listInfo.id, curProgram, tmp) == kResultTrue)
			{
				String str (tmp);
				str.copyTo8 (name, 0, kVstMaxProgNameLen);
			}
		}
	}
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getProgramNameIndexed (VstInt32, VstInt32 index, char* name)
{
	*name = 0;
	if (mUnitInfo)
	{
		ProgramListInfo listInfo = {0};
		if (mUnitInfo->getProgramListInfo (0, listInfo) == kResultTrue)
		{
			String128 tmp = {0};
			if (mUnitInfo->getProgramName (listInfo.id, index, tmp) == kResultTrue)
			{
				String str (tmp);
				str.copyTo8 (name, 0, kVstMaxProgNameLen);
				return true;
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterLabel (VstInt32 index, char* label)
{
	// units in which parameter \e index is displayed (i.e. "sec", "dB", "type", etc...). Limited to
	// #kVstMaxParamStrLen.
	*label = 0;
	if (mController)
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			String str (paramInfo.units);
			str.copyTo8 (label, 0, kVstMaxParamStrLen);
		}
	}
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterDisplay (VstInt32 index, char* text)
{
	// string representation ("0.5", "-3", "PLATE", etc...) of the value of parameter \e index.
	// Limited to #kVstMaxParamStrLen.
	*text = 0;
	if (mController)
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			String128 tmp = {0};
			ParamValue value = 0;
			if (!getLastParamChange (paramInfo.id, value))
				value = mController->getParamNormalized (paramInfo.id);

			if (mController->getParamStringByValue (paramInfo.id, value, tmp) == kResultTrue)
			{
				String str (tmp);
				str.copyTo8 (text, 0, kVstMaxParamStrLen);
			}
		}
	}
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterName (VstInt32 index, char* text)
{
	// name ("Time", "Gain", "RoomType", etc...) of parameter \e index. Limited to
	// #kVstExtMaxParamStrLen.
	*text = 0;
	if (mController && index < (int32)mParameterMap.size ())
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			String str;
			if (vst2WrapperFullParameterPath)
			{
				//! The parameter's name contains the unit path (e.g. "LFO 1.freq") as well
				if (mUnitInfo)
				{
					getUnitPath (paramInfo.unitId, str);
				}
			}
			str.append (paramInfo.title);

			if (str.length () > kVstExtMaxParamStrLen)
			{
				//! In case the string's length exceeds the limit, try parameter's title without
				// unit path.
				str = paramInfo.title;
			}
			if (str.length () > kVstExtMaxParamStrLen)
			{
				str = paramInfo.shortTitle;
			}
			str.copyTo8 (text, 0, kVstExtMaxParamStrLen);
		}
	}
}

//------------------------------------------------------------------------
bool Vst2Wrapper::canParameterBeAutomated (VstInt32 index)
{
	if (mController && index < (int32)mParameterMap.size ())
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
			return (paramInfo.flags & ParameterInfo::kCanAutomate) != 0;
	}
	return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::string2parameter (VstInt32 index, char* text)
{
	if (mController && index < (int32)mParameterMap.size ())
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			TChar tString[1024] = {0};
			String tmp (text);
			tmp.copyTo16 (tString, 0, 1023);

			ParamValue valueNormalized = 0.0;

			if (mController->getParamValueByString (paramInfo.id, tString, valueNormalized))
			{
				setParameterAutomated (index, (float)valueNormalized);
				// TODO: check if setParameterAutomated is correct
			}
		}
	}
	return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getParameterProperties (VstInt32 index, VstParameterProperties* p)
{
	if (mController && index < (int32)mParameterMap.size ())
	{
		p->label[0] = 0;
		p->shortLabel[0] = 0;

		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			String str (paramInfo.title);
			str.copyTo8 (p->label, 0, kVstMaxLabelLen);

			String str2 (paramInfo.shortTitle);
			str.copyTo8 (p->shortLabel, 0, kVstMaxShortLabelLen);

			if (paramInfo.stepCount == 0) // continuous
			{
				p->flags |= kVstParameterCanRamp;
			}
			else if (paramInfo.stepCount == 1) // on / off
			{
				p->flags |= kVstParameterIsSwitch;
			}
			else
			{
				p->minInteger = 0;
				p->maxInteger = paramInfo.stepCount;
				p->flags |= kVstParameterUsesIntegerMinMax;
			}

			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getChunk (void** data, bool isPreset)
{
	return BaseWrapper::_getChunk (data, isPreset);
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::setChunk (void* data, VstInt32 byteSize, bool isPreset)
{
	return BaseWrapper::_setChunk (data, byteSize, isPreset);
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::vst3ToVst2SpeakerArr (SpeakerArrangement vst3Arr)
{
	switch (vst3Arr)
	{
		case SpeakerArr::kMono: return kSpeakerArrMono;
		case SpeakerArr::kStereo: return kSpeakerArrStereo;
		case SpeakerArr::kStereoSurround: return kSpeakerArrStereoSurround;
		case SpeakerArr::kStereoCenter: return kSpeakerArrStereoCenter;
		case SpeakerArr::kStereoSide: return kSpeakerArrStereoSide;
		case SpeakerArr::kStereoCLfe: return kSpeakerArrStereoCLfe;
		case SpeakerArr::k30Cine: return kSpeakerArr30Cine;
		case SpeakerArr::k30Music: return kSpeakerArr30Music;
		case SpeakerArr::k31Cine: return kSpeakerArr31Cine;
		case SpeakerArr::k31Music: return kSpeakerArr31Music;
		case SpeakerArr::k40Cine: return kSpeakerArr40Cine;
		case SpeakerArr::k40Music: return kSpeakerArr40Music;
		case SpeakerArr::k41Cine: return kSpeakerArr41Cine;
		case SpeakerArr::k41Music: return kSpeakerArr41Music;
		case SpeakerArr::k50: return kSpeakerArr50;
		case SpeakerArr::k51: return kSpeakerArr51;
		case SpeakerArr::k60Cine: return kSpeakerArr60Cine;
		case SpeakerArr::k60Music: return kSpeakerArr60Music;
		case SpeakerArr::k61Cine: return kSpeakerArr61Cine;
		case SpeakerArr::k61Music: return kSpeakerArr61Music;
		case SpeakerArr::k70Cine: return kSpeakerArr70Cine;
		case SpeakerArr::k70Music: return kSpeakerArr70Music;
		case SpeakerArr::k71Cine: return kSpeakerArr71Cine;
		case SpeakerArr::k71Music: return kSpeakerArr71Music;
		case SpeakerArr::k80Cine: return kSpeakerArr80Cine;
		case SpeakerArr::k80Music: return kSpeakerArr80Music;
		case SpeakerArr::k81Cine: return kSpeakerArr81Cine;
		case SpeakerArr::k81Music: return kSpeakerArr81Music;
		case SpeakerArr::k102: return kSpeakerArr102;
	}

	return kSpeakerArrUserDefined;
}

//------------------------------------------------------------------------
SpeakerArrangement Vst2Wrapper::vst2ToVst3SpeakerArr (VstInt32 vst2Arr)
{
	switch (vst2Arr)
	{
		case kSpeakerArrMono: return SpeakerArr::kMono;
		case kSpeakerArrStereo: return SpeakerArr::kStereo;
		case kSpeakerArrStereoSurround: return SpeakerArr::kStereoSurround;
		case kSpeakerArrStereoCenter: return SpeakerArr::kStereoCenter;
		case kSpeakerArrStereoSide: return SpeakerArr::kStereoSide;
		case kSpeakerArrStereoCLfe: return SpeakerArr::kStereoCLfe;
		case kSpeakerArr30Cine: return SpeakerArr::k30Cine;
		case kSpeakerArr30Music: return SpeakerArr::k30Music;
		case kSpeakerArr31Cine: return SpeakerArr::k31Cine;
		case kSpeakerArr31Music: return SpeakerArr::k31Music;
		case kSpeakerArr40Cine: return SpeakerArr::k40Cine;
		case kSpeakerArr40Music: return SpeakerArr::k40Music;
		case kSpeakerArr41Cine: return SpeakerArr::k41Cine;
		case kSpeakerArr41Music: return SpeakerArr::k41Music;
		case kSpeakerArr50: return SpeakerArr::k50;
		case kSpeakerArr51: return SpeakerArr::k51;
		case kSpeakerArr60Cine: return SpeakerArr::k60Cine;
		case kSpeakerArr60Music: return SpeakerArr::k60Music;
		case kSpeakerArr61Cine: return SpeakerArr::k61Cine;
		case kSpeakerArr61Music: return SpeakerArr::k61Music;
		case kSpeakerArr70Cine: return SpeakerArr::k70Cine;
		case kSpeakerArr70Music: return SpeakerArr::k70Music;
		case kSpeakerArr71Cine: return SpeakerArr::k71Cine;
		case kSpeakerArr71Music: return SpeakerArr::k71Music;
		case kSpeakerArr80Cine: return SpeakerArr::k80Cine;
		case kSpeakerArr80Music: return SpeakerArr::k80Music;
		case kSpeakerArr81Cine: return SpeakerArr::k81Cine;
		case kSpeakerArr81Music: return SpeakerArr::k81Music;
		case kSpeakerArr102: return SpeakerArr::k102;
	}

	return 0;
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::vst3ToVst2Speaker (Vst::Speaker vst3Speaker)
{
	switch (vst3Speaker)
	{
		case Vst::kSpeakerM: return ::kSpeakerM;
		case Vst::kSpeakerL: return ::kSpeakerL;
		case Vst::kSpeakerR: return ::kSpeakerR;
		case Vst::kSpeakerC: return ::kSpeakerC;
		case Vst::kSpeakerLfe: return ::kSpeakerLfe;
		case Vst::kSpeakerLs: return ::kSpeakerLs;
		case Vst::kSpeakerRs: return ::kSpeakerRs;
		case Vst::kSpeakerLc: return ::kSpeakerLc;
		case Vst::kSpeakerRc: return ::kSpeakerRc;
		case Vst::kSpeakerS: return ::kSpeakerS;
		case Vst::kSpeakerSl: return ::kSpeakerSl;
		case Vst::kSpeakerSr: return ::kSpeakerSr;
		case Vst::kSpeakerTc: return ::kSpeakerTm;
		case Vst::kSpeakerTfl: return ::kSpeakerTfl;
		case Vst::kSpeakerTfc: return ::kSpeakerTfc;
		case Vst::kSpeakerTfr: return ::kSpeakerTfr;
		case Vst::kSpeakerTrl: return ::kSpeakerTrl;
		case Vst::kSpeakerTrc: return ::kSpeakerTrc;
		case Vst::kSpeakerTrr: return ::kSpeakerTrr;
		case Vst::kSpeakerLfe2: return ::kSpeakerLfe2;
	}
	return ::kSpeakerUndefined;
}

//------------------------------------------------------------------------
static const char* gSpeakerNames[] = {
    "M", ///< Mono (M)
    "L", ///< Left (L)
    "R", ///< Right (R)
    "C", ///< Center (C)
    "Lfe", ///< Subbass (Lfe)
    "Ls", ///< Left Surround (Ls)
    "Rs", ///< Right Surround (Rs)
    "Lc", ///< Left of Center (Lc)
    "Rc", ///< Right of Center (Rc)
    "Cs", ///< Center of Surround (Cs) = Surround (S)
    "Sl", ///< Side Left (Sl)
    "Sr", ///< Side Right (Sr)
    "Tm", ///< Top Middle (Tm)
    "Tfl", ///< Top Front Left (Tfl)
    "Tfc", ///< Top Front Center (Tfc)
    "Tfr", ///< Top Front Right (Tfr)
    "Trl", ///< Top Rear Left (Trl)
    "Trc", ///< Top Rear Center (Trc)
    "Trr", ///< Top Rear Right (Trr)
    "Lfe2" ///< Subbass 2 (Lfe2)
};
static const int32 kNumSpeakerNames = sizeof (gSpeakerNames) / sizeof (char*);

//------------------------------------------------------------------------
bool Vst2Wrapper::pinIndexToBusChannel (BusDirection dir, VstInt32 pinIndex, int32& busIndex,
                                        int32& busChannel)
{
	AudioBusBuffers* busBuffers = dir == kInput ? mProcessData.inputs : mProcessData.outputs;
	int32 busCount = dir == kInput ? mProcessData.numInputs : mProcessData.numOutputs;
	uint64 mainBusFlags = dir == kInput ? mMainAudioInputBuses : mMainAudioOutputBuses;

	int32 sourceIndex = 0;
	for (busIndex = 0; busIndex < busCount; busIndex++)
	{
		AudioBusBuffers& buffers = busBuffers[busIndex];
		if (mainBusFlags & (uint64 (1) << busIndex))
		{
			for (busChannel = 0; busChannel < buffers.numChannels; busChannel++)
			{
				if (pinIndex == sourceIndex)
				{
					return true;
				}
				sourceIndex++;
			}
		}
	}
	return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getPinProperties (BusDirection dir, VstInt32 pinIndex,
                                    VstPinProperties* properties)
{
	int32 busIndex = -1;
	int32 busChannelIndex = -1;

	if (pinIndexToBusChannel (dir, pinIndex, busIndex, busChannelIndex))
	{
		BusInfo busInfo = {0};
		if (mComponent && mComponent->getBusInfo (kAudio, dir, busIndex, busInfo) == kResultTrue)
		{
			properties->flags = kVstPinIsActive; // ????

			String name (busInfo.name);
			name.copyTo8 (properties->label, 0, kVstMaxLabelLen);

			if (busInfo.channelCount == 1)
			{
				properties->flags |= kVstPinUseSpeaker;
				properties->arrangementType = kSpeakerArrMono;
			}
			if (busInfo.channelCount == 2)
			{
				properties->flags |= kVstPinUseSpeaker;
				properties->flags |= kVstPinIsStereo;
				properties->arrangementType = kSpeakerArrStereo;
			}
			else if (busInfo.channelCount > 2)
			{
				Vst::SpeakerArrangement arr = 0;
				if (mProcessor && mProcessor->getBusArrangement (dir, busIndex, arr) == kResultTrue)
				{
					properties->flags |= kVstPinUseSpeaker;
					properties->arrangementType = vst3ToVst2SpeakerArr (arr);
				}
				else
				{
					return false;
				}
			}

			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getInputProperties (VstInt32 index, VstPinProperties* properties)
{
	return getPinProperties (kInput, index, properties);
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getOutputProperties (VstInt32 index, VstPinProperties* properties)
{
	return getPinProperties (kOutput, index, properties);
}

//------------------------------------------------------------------------
bool Vst2Wrapper::setSpeakerArrangement (VstSpeakerArrangement* pluginInput,
                                         VstSpeakerArrangement* pluginOutput)
{
	if (!mProcessor || !mComponent)
		return false;

	Vst::SpeakerArrangement newInputArr = 0;
	Vst::SpeakerArrangement newOutputArr = 0;
	Vst::SpeakerArrangement outputArr = 0;
	Vst::SpeakerArrangement inputArr = 0;

	int32 inputBusCount = mComponent->getBusCount (kAudio, kInput);
	int32 outputBusCount = mComponent->getBusCount (kAudio, kOutput);

	if (inputBusCount > 0)
		if (mProcessor->getBusArrangement (kInput, 0, inputArr) != kResultTrue)
			return false;
	if (outputBusCount > 0)
		if (mProcessor->getBusArrangement (kOutput, 0, outputArr) != kResultTrue)
			return false;

	if (pluginInput)
	{
		newInputArr = vst2ToVst3SpeakerArr (pluginInput->type);
		if (newInputArr == 0)
			return false;
	}
	if (pluginOutput)
	{
		newOutputArr = vst2ToVst3SpeakerArr (pluginOutput->type);
		if (newOutputArr == 0)
			return false;
	}

	if (newInputArr == 0)
		newInputArr = inputArr;
	if (newOutputArr == 0)
		newOutputArr = outputArr;

	if (newInputArr != inputArr || newOutputArr != outputArr)
	{
		if (mProcessor->setBusArrangements (
		        &newInputArr, (newInputArr > 0 && inputBusCount > 0) ? 1 : 0, &newOutputArr,
		        (newOutputArr > 0 && outputBusCount > 0) ? 1 : 0) != kResultTrue)
			return false;

		restartComponent (kIoChanged);
	}

	return true;
}

//------------------------------------------------------------------------
void Vst2Wrapper::setupVst2Arrangement (VstSpeakerArrangement*& vst2arr,
                                        Vst::SpeakerArrangement vst3Arrangement)
{
	int32 numChannels = Vst::SpeakerArr::getChannelCount (vst3Arrangement);

	if (vst2arr && (numChannels == 0 || (numChannels > vst2arr->numChannels && numChannels > 8)))
	{
		free (vst2arr);
		vst2arr = nullptr;
		if (numChannels == 0)
			return;
	}

	if (vst2arr == nullptr)
	{
		int32 channelOverhead = numChannels > 8 ? numChannels - 8 : 0;
		uint32 structSize =
		    sizeof (VstSpeakerArrangement) + channelOverhead * sizeof (VstSpeakerProperties);
		vst2arr = (VstSpeakerArrangement*)malloc (structSize);
		memset (vst2arr, 0, structSize);
	}

	if (vst2arr)
	{
		vst2arr->type = vst3ToVst2SpeakerArr (vst3Arrangement);
		vst2arr->numChannels = numChannels;

		Speaker vst3TestSpeaker = 1;

		for (int32 i = 0; i < numChannels; i++)
		{
			VstSpeakerProperties& props = vst2arr->speakers[i];

			// find nextSpeaker in vst3 arrangement
			Speaker vst3Speaker = 0;
			while (vst3Speaker == 0 && vst3TestSpeaker != 0)
			{
				if (vst3Arrangement & vst3TestSpeaker)
					vst3Speaker = vst3TestSpeaker;

				vst3TestSpeaker <<= 1;
			}

			if (vst3Speaker)
			{
				props.type = vst3ToVst2Speaker (vst3Speaker);
				if (props.type >= 0 && props.type < kNumSpeakerNames)
					strncpy (props.name, gSpeakerNames[props.type], kVstMaxNameLen);
				else
					sprintf (props.name, "%d", i + 1);
			}
		}
	}
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getSpeakerArrangement (VstSpeakerArrangement** pluginInput,
                                         VstSpeakerArrangement** pluginOutput)
{
	if (!mProcessor)
		return false;

	Vst::SpeakerArrangement inputArr = 0;
	Vst::SpeakerArrangement outputArr = 0;

	if (mProcessor->getBusArrangement (kInput, 0, inputArr) != kResultTrue)
		inputArr = 0;

	if (mProcessor->getBusArrangement (kOutput, 0, outputArr) != kResultTrue)
		outputArr = 0;

	setupVst2Arrangement (mVst2InputArrangement, inputArr);
	setupVst2Arrangement (mVst2OutputArrangement, outputArr);

	*pluginInput = mVst2InputArrangement;
	*pluginOutput = mVst2OutputArrangement;

	return mVst2InputArrangement != nullptr && mVst2OutputArrangement != nullptr;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::setBypass (bool onOff)
{
	return BaseWrapper::_setBypass (onOff);
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::setProcessPrecision (VstInt32 precision)
{
	int32 newVst3SampleSize = -1;

	if (precision == kVstProcessPrecision32)
		newVst3SampleSize = kSample32;
	else if (precision == kVstProcessPrecision64)
		newVst3SampleSize = kSample64;

	if (newVst3SampleSize != mVst3SampleSize)
	{
		if (mProcessor && mProcessor->canProcessSampleSize (newVst3SampleSize) == kResultTrue)
		{
			mVst3SampleSize = newVst3SampleSize;
			setupProcessing ();

			setupBuses ();

			return true;
		}
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getNumMidiInputChannels ()
{
	if (!mComponent)
		return 0;

	int32 busCount = mComponent->getBusCount (kEvent, kInput);
	if (busCount > 0)
	{
		BusInfo busInfo = {0};
		if (mComponent->getBusInfo (kEvent, kInput, 0, busInfo) == kResultTrue)
		{
			return busInfo.channelCount;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getNumMidiOutputChannels ()
{
	if (!mComponent)
		return 0;

	int32 busCount = mComponent->getBusCount (kEvent, kOutput);
	if (busCount > 0)
	{
		BusInfo busInfo = {0};
		if (mComponent->getBusInfo (kEvent, kOutput, 0, busInfo) == kResultTrue)
		{
			return busInfo.channelCount;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getGetTailSize ()
{
	if (mProcessor)
		return mProcessor->getTailSamples ();

	return 0;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::getEffectName (char* effectName)
{
	if (mName[0])
	{
		strncpy (effectName, mName, kVstMaxEffectNameLen);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::getVendorString (char* text)
{
	if (mVendor[0])
	{
		strncpy (text, mVendor, kVstMaxVendorStrLen);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getVendorVersion ()
{
	return mVersion;
}

//-----------------------------------------------------------------------------
VstIntPtr Vst2Wrapper::vendorSpecific (VstInt32 lArg, VstIntPtr lArg2, void* ptrArg, float floatArg)
{
	switch (lArg)
	{
		case 'stCA':
		case 'stCa':
			switch (lArg2)
			{
				//--- -------
				case 'FUID':
					if (ptrArg && mVst3EffectClassID.isValid ())
					{
						memcpy ((char*)ptrArg, mVst3EffectClassID, 16);
						return 1;
					}
					break;
				//--- -------
				case 'Whee':
					if (editor)
						editor->onWheel (floatArg);
					return 1;
					break;
			}
	}

	return AudioEffectX::vendorSpecific (lArg, lArg2, ptrArg, floatArg);
}

//-----------------------------------------------------------------------------
VstPlugCategory Vst2Wrapper::getPlugCategory ()
{
	if (mSubCategories[0])
	{
		if (strstr (mSubCategories, "Analyzer"))
			return kPlugCategAnalysis;

		else if (strstr (mSubCategories, "Delay") || strstr (mSubCategories, "Reverb"))
			return kPlugCategRoomFx;

		else if (strstr (mSubCategories, "Dynamics") || strstr (mSubCategories, "Mastering"))
			return kPlugCategMastering;

		else if (strstr (mSubCategories, "Restoration"))
			return kPlugCategRestoration;

		else if (strstr (mSubCategories, "Generator"))
			return kPlugCategGenerator;

		else if (strstr (mSubCategories, "Spatial"))
			return kPlugCategSpacializer;

		else if (strstr (mSubCategories, "Fx"))
			return kPlugCategEffect;

		else if (strstr (mSubCategories, "Instrument"))
			return kPlugCategSynth;
	}
	return kPlugCategUnknown;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::canDo (char* text)
{
	if (stricmp (text, "sendVstEvents") == 0)
	{
		return -1;
	}
	else if (stricmp (text, "sendVstMidiEvent") == 0)
	{
		return mHasEventOutputBuses ? 1 : -1;
	}
	else if (stricmp (text, "receiveVstEvents") == 0)
	{
		return -1;
	}
	else if (stricmp (text, "receiveVstMidiEvent") == 0)
	{
		return mHasEventInputBuses ? 1 : -1;
	}
	else if (stricmp (text, "receiveVstTimeInfo") == 0)
	{
		return 1;
	}
	else if (stricmp (text, "offline") == 0)
	{
		if (mProcessing)
			return 0;
		if (mVst3processMode == kOffline)
			return 1;

		bool canOffline = setupProcessing (kOffline);
		setupProcessing ();
		return canOffline ? 1 : -1;
	}
	else if (stricmp (text, "midiProgramNames") == 0)
	{
		if (mUnitInfo)
		{
			UnitID unitId = -1;
			if (mUnitInfo->getUnitByBus (kEvent, kInput, 0, 0, unitId) == kResultTrue &&
			    unitId >= 0)
				return 1;
		}
		return -1;
	}
	else if (stricmp (text, "bypass") == 0)
	{
		return mBypassParameterID != kNoParamId ? 1 : -1;
	}
	return 0; // do not know
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getMidiProgramName (VstInt32 channel, MidiProgramName* midiProgramName)
{
	UnitID unitId;
	ProgramListID programListId;
	if (mUnitInfo && getProgramListAndUnit (channel, unitId, programListId))
	{
		if (midiProgramName)
			setupMidiProgram (channel, programListId, *midiProgramName);

		ProgramListInfo programListInfo;
		if (getProgramListInfoByProgramListID (programListId, programListInfo))
			return programListInfo.programCount;
	}
	return 0;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getCurrentMidiProgram (VstInt32 channel, MidiProgramName* currentProgram)
{
	if (mUnitInfo && mController)
	{
		UnitID unitId;
		ProgramListID programListId;
		if (getProgramListAndUnit (channel, unitId, programListId))
		{
			// find program selector parameter
			int32 parameterCount = mController->getParameterCount ();
			for (int32 i = 0; i < parameterCount; i++)
			{
				ParameterInfo parameterInfo = {0};
				if (mController->getParameterInfo (i, parameterInfo) == kResultTrue)
				{
					if ((parameterInfo.flags & ParameterInfo::kIsProgramChange) != 0 &&
					    parameterInfo.unitId == unitId)
					{
						ParamValue normalized = mController->getParamNormalized (parameterInfo.id);
						int32 discreteValue =
						    Min<int32> ((int32) (normalized * (parameterInfo.stepCount + 1)),
						                parameterInfo.stepCount);

						if (currentProgram)
						{
							currentProgram->thisProgramIndex = discreteValue;
							setupMidiProgram (channel, programListId, *currentProgram);
						}

						return discreteValue;
					}
				}
			}
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::setupMidiProgram (int32 midiChannel, ProgramListID programListId,
                                    MidiProgramName& midiProgramName)
{
	if (mUnitInfo)
	{
		String128 string128 = {0};

		if (mUnitInfo->getProgramName (programListId, midiProgramName.thisProgramIndex,
		                               string128) == kResultTrue)
		{
			String str (string128);
			str.copyTo8 (midiProgramName.name, 0, 64);

			midiProgramName.midiProgram = midiProgramName.thisProgramIndex;
			midiProgramName.midiBankMsb = -1;
			midiProgramName.midiBankLsb = -1;
			midiProgramName.parentCategoryIndex = -1;
			midiProgramName.flags = 0;

			if (mUnitInfo->getProgramInfo (programListId, midiProgramName.thisProgramIndex,
			                               PresetAttributes::kInstrument, string128) == kResultTrue)
			{
				midiProgramName.parentCategoryIndex =
				    lookupProgramCategory (midiChannel, string128);
			}
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
int32 Vst2Wrapper::lookupProgramCategory (int32 midiChannel, String128 instrumentAttribute)
{
	std::vector<ProgramCategory>& channelCategories = mProgramCategories[midiChannel];

	for (uint32 categoryIndex = 0; categoryIndex < channelCategories.size (); categoryIndex++)
	{
		ProgramCategory& cat = channelCategories[categoryIndex];
		if (memcmp (instrumentAttribute, cat.vst3InstrumentAttribute, sizeof (String128)) == 0)
			return categoryIndex;
	}

	return -1;
}

//-----------------------------------------------------------------------------
uint32 Vst2Wrapper::makeCategoriesRecursive (std::vector<ProgramCategory>& channelCategories,
                                             String128 vst3Category)
{
	for (uint32 categoryIndex = 0; categoryIndex < channelCategories.size (); categoryIndex++)
	{
		ProgramCategory& cat = channelCategories[categoryIndex];
		if (memcmp (vst3Category, cat.vst3InstrumentAttribute, sizeof (String128)) == 0)
		{
			return categoryIndex;
		}
	}

	int32 parentCategorIndex = -1;

	String128 str;
	String strAcc (str);
	strAcc.copyTo16 (vst3Category, 0, 127);
	int32 len = strAcc.length ();
	String singleName;

	char16 divider = '|';
	for (int32 strIndex = len - 1; strIndex >= 0; strIndex--)
	{
		bool isDivider = str[strIndex] == divider;
		str[strIndex] = 0; // zero out rest
		if (isDivider)
		{
			singleName.assign (vst3Category + strIndex + 1);
			parentCategorIndex = makeCategoriesRecursive (channelCategories, str);
			break;
		}
	}

	// make new
	ProgramCategory cat = {};
	memcpy (cat.vst3InstrumentAttribute, vst3Category, sizeof (String128));
	singleName.copyTo8 (cat.vst2Category.name, 0, kVstMaxNameLen);
	cat.vst2Category.parentCategoryIndex = parentCategorIndex;
	cat.vst2Category.thisCategoryIndex = (int32)channelCategories.size ();

	channelCategories.push_back (cat);

	return cat.vst2Category.thisCategoryIndex;
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::setupProgramCategories ()
{
	mProgramCategories.clear ();
	if (mUnitInfo && mComponent)
	{
		if (mComponent->getBusCount (kEvent, kInput) > 0)
		{
			for (int32 channel = 0; channel < 16; channel++) // the 16 channels
			{
				// make vector for channel
				mProgramCategories.push_back (std::vector<ProgramCategory> ());
				std::vector<ProgramCategory>& channelCategories = mProgramCategories[channel];

				// scan program list of channel and find categories
				UnitID unitId;
				ProgramListID programListId;
				if (getProgramListAndUnit (channel, unitId, programListId))
				{
					ProgramListInfo programListInfo;
					if (getProgramListInfoByProgramListID (programListId, programListInfo))
					{
						for (int32 programIndex = 0; programIndex < programListInfo.programCount;
						     programIndex++)
						{
							String128 string128 = {0};
							if (mUnitInfo->getProgramInfo (programListId, programIndex,
							                               PresetAttributes::kInstrument,
							                               string128) == kResultTrue)
							{
								makeCategoriesRecursive (channelCategories, string128);
							}
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getMidiProgramCategory (VstInt32 channel, MidiProgramCategory* category)
{
	// try to rebuild it each time
	setupProgramCategories ();

	if (channel >= (VstInt32)mProgramCategories.size ())
		return 0;

	std::vector<ProgramCategory>& channelCategories = mProgramCategories[channel];
	if (category && category->thisCategoryIndex < (VstInt32)channelCategories.size ())
	{
		ProgramCategory& cat = channelCategories[category->thisCategoryIndex];
		if (cat.vst2Category.thisCategoryIndex == category->thisCategoryIndex)
			memcpy (category, &cat.vst2Category, sizeof (MidiProgramCategory));
	}
	return (VstInt32)channelCategories.size ();
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::hasMidiProgramsChanged (VstInt32 channel)
{
	// names of programs or program categories have changed
	return false;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::getMidiKeyName (VstInt32 channel, MidiKeyName* keyName)
{
	UnitID unitId;
	ProgramListID programListId;
	if (mUnitInfo && getProgramListAndUnit (channel, unitId, programListId))
	{
		String128 string128 = {0};
		if (mUnitInfo->getProgramPitchName (programListId, keyName->thisProgramIndex,
		                                    keyName->thisKeyNumber, string128))
		{
			String str (string128);
			str.copyTo8 (keyName->keyName, 0, kVstMaxNameLen);
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::setupParameters ()
{
	BaseWrapper::setupParameters ();
	cEffect.numParams = mNumParams;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::processEvents (VstEvents* events)
{
	if (mInputEvents == nullptr)
		return 0;
	mInputEvents->clear ();

	for (int32 i = 0; i < events->numEvents; i++)
	{
		VstEvent* e = events->events[i];
		if (e->type == kVstMidiType)
		{
			auto* midiEvent = (VstMidiEvent*)e;
			Event toAdd = {0, midiEvent->deltaFrames, 0};
			processMidiEvent (toAdd, &midiEvent->midiData[0],
			                  midiEvent->flags & kVstMidiEventIsRealtime, midiEvent->noteLength,
			                  midiEvent->noteOffVelocity * kMidiScaler, midiEvent->detune);
		}
		//--- -----------------------------
		else if (e->type == kVstSysExType)
		{
			Event toAdd = {0, e->deltaFrames};
			VstMidiSysexEvent& src = *(VstMidiSysexEvent*)e;
			toAdd.type = Event::kDataEvent;
			toAdd.data.type = DataEvent::kMidiSysEx;
			toAdd.data.size = src.dumpBytes;
			toAdd.data.bytes = (uint8*)src.sysexDump;
			mInputEvents->addEvent (toAdd);
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
inline void Vst2Wrapper::processOutputEvents ()
{
	if (mVst2OutputEvents && mOutputEvents && mOutputEvents->getEventCount () > 0)
	{
		mVst2OutputEvents->flush ();

		Event e = {0};
		for (int32 i = 0, total = mOutputEvents->getEventCount (); i < total; i++)
		{
			if (mOutputEvents->getEvent (i, e) != kResultOk)
				break;

			//---SysExclusif----------------
			if (e.type == Event::kDataEvent && e.data.type == DataEvent::kMidiSysEx)
			{
				VstMidiSysexEvent sysexEvent = {0};
				sysexEvent.deltaFrames = e.sampleOffset;
				sysexEvent.dumpBytes = e.data.size;
				sysexEvent.sysexDump = (char*)e.data.bytes;

				if (!mVst2OutputEvents->add (sysexEvent))
					break;
			}
			else
			{
				VstMidiEvent midiEvent = {0};
				midiEvent.deltaFrames = e.sampleOffset;
				if (e.flags & Event::kIsLive)
					midiEvent.flags = kVstMidiEventIsRealtime;
				char* midiData = midiEvent.midiData;

				switch (e.type)
				{
					//--- ---------------------
					case Event::kNoteOnEvent:
						midiData[0] = (char)(kNoteOn | (e.noteOn.channel & kChannelMask));
						midiData[1] = (char)(e.noteOn.pitch & kDataMask);
						midiData[2] =
						    (char)((int32) (e.noteOn.velocity * 127.f + 0.4999999f) & kDataMask);
						if (midiData[2] == 0) // zero velocity => note off
							midiData[0] = (char)(kNoteOff | (e.noteOn.channel & kChannelMask));
						midiEvent.detune = (char)e.noteOn.tuning;
						midiEvent.noteLength = e.noteOn.length;
						break;

					//--- ---------------------
					case Event::kNoteOffEvent:
						midiData[0] = (char)(kNoteOff | (e.noteOff.channel & kChannelMask));
						midiData[1] = (char)(e.noteOff.pitch & kDataMask);
						midiData[2] = midiEvent.noteOffVelocity =
						    (char)((int32) (e.noteOff.velocity * 127.f + 0.4999999f) & kDataMask);
						break;

						break;
				}

				if (!mVst2OutputEvents->add (midiEvent))
					break;
			}
		}

		mOutputEvents->clear ();

		sendVstEventsToHost (*mVst2OutputEvents);
	}
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::updateProcessLevel ()
{
	auto currentLevel = getCurrentProcessLevel ();
	if (mCurrentProcessLevel != currentLevel)
	{
		mCurrentProcessLevel = currentLevel;
		if (mCurrentProcessLevel == kVstProcessLevelOffline)
			mVst3processMode = kOffline;
		else
			mVst3processMode = kRealtime;

		bool callStartStop = mProcessing;

		if (callStartStop)
			stopProcess ();

		setupProcessing ();

		if (callStartStop)
			startProcess ();
	}
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	updateProcessLevel ();

	_processReplacing (inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
	updateProcessLevel ();

	_processDoubleReplacing (inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------
// static
//-----------------------------------------------------------------------------
AudioEffect* Vst2Wrapper::create (IPluginFactory* factory, const TUID vst3ComponentID,
                                  VstInt32 vst2ID, audioMasterCallback audioMaster)
{
	if (!factory)
		return nullptr;

	BaseWrapper::SVST3Config config;
	config.factory = factory; 
	config.processor = nullptr;
	
	FReleaser factoryReleaser (factory);

	factory->createInstance (vst3ComponentID, IAudioProcessor::iid, (void**)&config.processor);
	if (config.processor)
	{
		config.controller = nullptr;
		if (config.processor->queryInterface (IEditController::iid, (void**)&config.controller) !=
		    kResultTrue)
		{
			FUnknownPtr<IComponent> component (config.processor);
			if (component)
			{
				TUID editorCID;
				if (component->getControllerClassId (editorCID) == kResultTrue)
				{
					factory->createInstance (editorCID, IEditController::iid,
					                         (void**)&config.controller);
				}
			}
		}

		config.vst3ComponentID = FUID::fromTUID (vst3ComponentID);

		auto* wrapper = new Vst2Wrapper (config, audioMaster, vst2ID);
		if (wrapper->init () == false)
		{
			wrapper->release ();
			return nullptr;
		}

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
						wrapper->setEffectVersion (classInfo2.version);

						if (classInfo2.vendor[0] != 0)
							wrapper->setVendorName (classInfo2.vendor);

						break;
					}
				}
			}
		}

		return wrapper;
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::getName (String128 name)
{
	char8 productString[128];
	if (getHostProductString (productString))
	{
		String str (productString);
		str.copyTo16 (name, 0, 127);

		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
// IComponentHandler
//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::beginEdit (ParamID tag)
{
	std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find (tag);
	if (iter != mParamIndexMap.end ())
		AudioEffectX::beginEdit ((*iter).second);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::performEdit (ParamID tag, ParamValue valueNormalized)
{
	std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find (tag);
	if (iter != mParamIndexMap.end () && audioMaster)
		audioMaster (&cEffect, audioMasterAutomate, (*iter).second, 0, nullptr,
		             (float)valueNormalized); // value is in opt

	mInputTransfer.addChange (tag, valueNormalized, 0);

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::endEdit (ParamID tag)
{
	std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find (tag);
	if (iter != mParamIndexMap.end ())
		AudioEffectX::endEdit ((*iter).second);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_ioChanged ()
{
	BaseWrapper::_ioChanged ();
	ioChanged ();
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_updateDisplay ()
{
	BaseWrapper::_updateDisplay ();
	updateDisplay ();
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_setNumInputs (int32 inputs)
{
	BaseWrapper::_setNumInputs (inputs);
	setNumInputs (inputs);
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_setNumOutputs (int32 outputs)
{
	BaseWrapper::_setNumOutputs (outputs);
	setNumOutputs (outputs);
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::_sizeWindow (int32 width, int32 height)
{
	return sizeWindow (width, height);
}

//-----------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

extern bool InitModule ();

//-----------------------------------------------------------------------------
extern "C" {

#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#define VST_EXPORT __attribute__ ((visibility ("default")))
#elif SMTG_OS_WINDOWS
#define VST_EXPORT __declspec (dllexport)
#else
#define VST_EXPORT
#endif

//-----------------------------------------------------------------------------
/** Prototype of the export function main */
//-----------------------------------------------------------------------------
VST_EXPORT AEffect* VSTPluginMain (audioMasterCallback audioMaster)
{
	// Get VST Version of the Host
	if (!audioMaster (nullptr, audioMasterVersion, 0, 0, nullptr, 0))
		return nullptr; // old version

	if (InitModule () == false)
		return nullptr;

	// Create the AudioEffect
	AudioEffect* effect = createEffectInstance (audioMaster);
	if (!effect)
		return nullptr;

	// Return the VST AEffect structure
	return effect->getAeffect ();
}

//-----------------------------------------------------------------------------
// support for old hosts not looking for VSTPluginMain
#if (TARGET_API_MAC_CARBON && __ppc__)
VST_EXPORT AEffect* main_macho (audioMasterCallback audioMaster)
{
	return VSTPluginMain (audioMaster);
}
#elif WIN32
VST_EXPORT AEffect* MAIN (audioMasterCallback audioMaster)
{
	return VSTPluginMain (audioMaster);
}
#elif BEOS
VST_EXPORT AEffect* main_plugin (audioMasterCallback audioMaster)
{
	return VSTPluginMain (audioMaster);
}
#endif

} // extern "C"
//-----------------------------------------------------------------------------

/// \endcond
