/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "circular_queue.h"
#include "synth_module.h"
#include "note_handler.h"

class LineGenerator;
class Tuning;

namespace vital {
  class Decimator;
  class PeakMeter;
  class Sample;
  class ReorderableEffectChain;
  class StereoMemory;
  class SynthVoiceHandler;
  class SynthLfo;
  class Value;
  class ValueSwitch;
  class WaveFrame;
  class Wavetable;

  class SoundEngine : public SynthModule, public NoteHandler {
    public:
      static constexpr int kDefaultOversamplingAmount = 2;
      static constexpr int kDefaultSampleRate = 44100;

      SoundEngine();
      virtual ~SoundEngine();

      void init() override;
      void process(int num_samples) override;
      void correctToTime(double seconds) override;

      int getNumPressedNotes();
      void connectModulation(const modulation_change& change);
      void disconnectModulation(const modulation_change& change);
      int getNumActiveVoices();
      ModulationConnectionBank& getModulationBank();
      mono_float getLastActiveNote() const;

      void setTuning(const Tuning* tuning);

      void allSoundsOff() override;
      void allNotesOff(int sample) override;
      void allNotesOff(int sample, int channel) override;
      void allNotesOffRange(int sample, int from_channel, int to_channel);

      void noteOn(int note, mono_float velocity, int sample, int channel) override;
      void noteOff(int note, mono_float lift, int sample, int channel) override;
      void setModWheel(mono_float value, int channel);
      void setModWheelAllChannels(mono_float value);
      void setPitchWheel(mono_float value, int channel);
      void setZonedPitchWheel(mono_float value, int from_channel, int to_channel);
      void disableUnnecessaryModSources();
      void enableModSource(const std::string& source);
      void disableModSource(const std::string& source);
      bool isModSourceEnabled(const std::string& source);
      const StereoMemory* getEqualizerMemory();

      void setBpm(mono_float bpm);
      void setAftertouch(mono_float note, mono_float value, int sample, int channel);
      void setChannelAftertouch(int channel, mono_float value, int sample);
      void setChannelRangeAftertouch(int from_channel, int to_channel, mono_float value, int sample);
      void setChannelSlide(int channel, mono_float value, int sample);
      void setChannelRangeSlide(int from_channel, int to_channel, mono_float value, int sample);
      Wavetable* getWavetable(int index);
      Sample* getSample();
      LineGenerator* getLfoSource(int index);

      void sustainOn(int channel);
      void sustainOff(int sample, int channel);
      void sostenutoOn(int channel);
      void sostenutoOff(int sample, int channel);

      void sustainOnRange(int from_channel, int to_channel);
      void sustainOffRange(int sample, int from_channel, int to_channel);
      void sostenutoOnRange(int from_channel, int to_channel);
      void sostenutoOffRange(int sample, int from_channel, int to_channel);
      force_inline int getOversamplingAmount() const { return last_oversampling_amount_; }

      void checkOversampling();

    private:
      void setOversamplingAmount(int oversampling_amount, int sample_rate);
    
      SynthVoiceHandler* voice_handler_;
      ReorderableEffectChain* effect_chain_;
      Add* output_total_;

      int last_oversampling_amount_;
      int last_sample_rate_;
      Value* oversampling_;
      Value* bps_;
      Value* legato_;
      Decimator* decimator_;
      PeakMeter* peak_meter_;

      CircularQueue<Processor*> modulation_processors_;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundEngine)
  };
} // namespace vital

