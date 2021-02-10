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

#include "JuceHeader.h"
#include "concurrentqueue/concurrentqueue.h"
#include "line_generator.h"
#include "synth_constants.h"
#include "synth_types.h"
#include "midi_manager.h"
#include "tuning.h"
#include "wavetable_creator.h"

#include <set>
#include <string>

namespace vital {
  class SoundEngine;
  struct Output;
  class StatusOutput;
  class StereoMemory;
  class Sample;
  class WaveFrame;
  class Wavetable;
}

class SynthGuiInterface;

class SynthBase : public MidiManager::Listener {
  public:
    static constexpr float kOutputWindowMinNote = 16.0f;
    static constexpr float kOutputWindowMaxNote = 128.0f;

    SynthBase();
    virtual ~SynthBase();

    void valueChanged(const std::string& name, vital::mono_float value);
    void valueChangedThroughMidi(const std::string& name, vital::mono_float value) override;
    void pitchWheelMidiChanged(vital::mono_float value) override;
    void modWheelMidiChanged(vital::mono_float value) override;
    void pitchWheelGuiChanged(vital::mono_float value);
    void modWheelGuiChanged(vital::mono_float value);
    void presetChangedThroughMidi(File preset) override;
    void valueChangedExternal(const std::string& name, vital::mono_float value);
    void valueChangedInternal(const std::string& name, vital::mono_float value);
    bool connectModulation(const std::string& source, const std::string& destination);
    void connectModulation(vital::ModulationConnection* connection);
    void disconnectModulation(const std::string& source, const std::string& destination);
    void disconnectModulation(vital::ModulationConnection* connection);
    void clearModulations();
    void forceShowModulation(const std::string& source, bool force);
    bool isModSourceEnabled(const std::string& source);
    int getNumModulations(const std::string& destination);
    int getConnectionIndex(const std::string& source, const std::string& destination);
    vital::CircularQueue<vital::ModulationConnection*> getModulationConnections() { return mod_connections_; }
    std::vector<vital::ModulationConnection*> getSourceConnections(const std::string& source);
    bool isSourceConnected(const std::string& source);
    std::vector<vital::ModulationConnection*> getDestinationConnections(const std::string& destination);

    const vital::StatusOutput* getStatusOutput(const std::string& name);

    vital::Wavetable* getWavetable(int index);
    WavetableCreator* getWavetableCreator(int index);
    vital::Sample* getSample();
    LineGenerator* getLfoSource(int index);

    int getSampleRate();
    void initEngine();
    void loadTuningFile(const File& file);
    void loadInitPreset();
    bool loadFromFile(File preset, std::string& error);
    void renderAudioToFile(File file, float seconds, float bpm, std::vector<int> notes, bool render_images);
    void renderAudioForResynthesis(float* data, int samples, int note);
    bool saveToFile(File preset);
    bool saveToActiveFile();
    void clearActiveFile() { active_file_ = File(); }
    File getActiveFile() { return active_file_; }

    void setMpeEnabled(bool enabled);
    virtual void beginChangeGesture(const std::string& name) { }
    virtual void endChangeGesture(const std::string& name) { }
    virtual void setValueNotifyHost(const std::string& name, vital::mono_float value) { }

    void armMidiLearn(const std::string& name);
    void cancelMidiLearn();
    void clearMidiLearn(const std::string& name);
    bool isMidiMapped(const std::string& name);

    void setAuthor(const String& author);
    void setComments(const String& comments);
    void setStyle(const String& comments);
    void setPresetName(const String& preset_name);
    void setMacroName(int index, const String& macro_name);
    String getAuthor();
    String getComments();
    String getStyle();
    String getPresetName();
    String getMacroName(int index);

    vital::control_map& getControls() { return controls_; }
    vital::SoundEngine* getEngine() { return engine_.get(); }
    MidiKeyboardState* getKeyboardState() { return keyboard_state_.get(); }
    const vital::poly_float* getOscilloscopeMemory() { return oscilloscope_memory_; }
    const vital::StereoMemory* getAudioMemory() { return audio_memory_.get(); }
    const vital::StereoMemory* getEqualizerMemory();
    vital::ModulationConnectionBank& getModulationBank();
    void notifyOversamplingChanged();
    void checkOversampling();
    virtual const CriticalSection& getCriticalSection() = 0;
    virtual void pauseProcessing(bool pause) = 0;
    Tuning* getTuning() { return &tuning_; }

    struct ValueChangedCallback : public CallbackMessage {
      ValueChangedCallback(std::shared_ptr<SynthBase*> listener, std::string name, vital::mono_float val) :
          listener(listener), control_name(std::move(name)), value(val) { }

      void messageCallback() override;

      std::weak_ptr<SynthBase*> listener;
      std::string control_name;
      vital::mono_float value;
    };

  protected:
    vital::modulation_change createModulationChange(vital::ModulationConnection* connection);
    bool isInvalidConnection(const vital::modulation_change& change);
    virtual SynthGuiInterface* getGuiInterface() = 0;
    json saveToJson();
    bool loadFromJson(const json& state);
    vital::ModulationConnection* getConnection(const std::string& source, const std::string& destination);

    inline bool getNextModulationChange(vital::modulation_change& change) {
      return modulation_change_queue_.try_dequeue_non_interleaved(change);
    }
  
    inline void clearModulationQueue() {
      vital::modulation_change change;
      while (modulation_change_queue_.try_dequeue_non_interleaved(change))
        ;
    }

    void processAudio(AudioSampleBuffer* buffer, int channels, int samples, int offset);
    void processAudioWithInput(AudioSampleBuffer* buffer, const vital::poly_float* input_buffer,
                               int channels, int samples, int offset);
    void writeAudio(AudioSampleBuffer* buffer, int channels, int samples, int offset);
    void processMidi(MidiBuffer& buffer, int start_sample = 0, int end_sample = 0);
    void processKeyboardEvents(MidiBuffer& buffer, int num_samples);
    void processModulationChanges();
    void updateMemoryOutput(int samples, const vital::poly_float* audio);

    std::unique_ptr<vital::SoundEngine> engine_;
    std::unique_ptr<MidiManager> midi_manager_;
    std::unique_ptr<MidiKeyboardState> keyboard_state_;

    std::unique_ptr<WavetableCreator> wavetable_creators_[vital::kNumOscillators];
    std::shared_ptr<SynthBase*> self_reference_;

    File active_file_;
    vital::poly_float oscilloscope_memory_[2 * vital::kOscilloscopeMemoryResolution];
    vital::poly_float oscilloscope_memory_write_[2 * vital::kOscilloscopeMemoryResolution];
    std::unique_ptr<vital::StereoMemory> audio_memory_;
    vital::mono_float last_played_note_;
    int last_num_pressed_;
    vital::mono_float memory_reset_period_;
    vital::mono_float memory_input_offset_;
    int memory_index_;
    bool expired_;

    std::map<std::string, String> save_info_;
    vital::control_map controls_;
    vital::CircularQueue<vital::ModulationConnection*> mod_connections_;
    moodycamel::ConcurrentQueue<vital::control_change> value_change_queue_;
    moodycamel::ConcurrentQueue<vital::modulation_change> modulation_change_queue_;
    Tuning tuning_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthBase)
};

class HeadlessSynth : public SynthBase {
  public:
    virtual const CriticalSection& getCriticalSection() override {
      return critical_section_;
    }

    virtual void pauseProcessing(bool pause) override {
      if (pause)
        critical_section_.enter();
      else
        critical_section_.exit();
    }

  protected:
    virtual SynthGuiInterface* getGuiInterface() override { return nullptr; }

  private:
    CriticalSection critical_section_;
};
