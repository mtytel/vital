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
#include "note_handler.h"
#include "processor_router.h"
#include "synth_module.h"
#include "tuning.h"

#include <map>
#include <list>

namespace vital {

  struct VoiceState {
    VoiceState() : event(kInvalid), midi_note(0), tuned_note(0.0f), last_note(0.0f), velocity(0.0f),
                   lift(0.0f), local_pitch_bend(0.0f), note_pressed(0), note_count(0),
                   channel(0), sostenuto_pressed(false) { }

    VoiceEvent event;
    int midi_note;
    mono_float tuned_note;
    poly_float last_note;
    mono_float velocity;
    mono_float lift;
    mono_float local_pitch_bend;
    int note_pressed;
    int note_count;
    int channel;
    bool sostenuto_pressed;
  };

  struct AggregateVoice;

  class Voice {
    public:
      static constexpr mono_float kDefaultLiftVelocity = 0.5f;

      enum KeyState {
        kTriggering,
        kHeld,
        kSustained,
        kReleased,
        kDead,
        kNumStates
      };

      Voice(AggregateVoice* parent);
      Voice() = delete;
      virtual ~Voice() { }

      force_inline AggregateVoice* parent() { return parent_; }
      force_inline const VoiceState& state() { return state_; }
      force_inline const KeyState last_key_state() { return last_key_state_; }
      force_inline const KeyState key_state() { return key_state_; }
      force_inline int event_sample() { return event_sample_; }
      force_inline int voice_index() { return voice_index_; }
      force_inline poly_mask voice_mask() { return voice_mask_; }

      force_inline mono_float aftertouch() { return aftertouch_; }
      force_inline mono_float aftertouch_sample() { return aftertouch_sample_; }

      force_inline mono_float slide() { return slide_; }
      force_inline mono_float slide_sample() { return slide_sample_; }

      force_inline void activate(int midi_note, mono_float tuned_note, mono_float velocity,
                                 poly_float last_note, int note_pressed, int note_count,
                                 int sample, int channel) {
        event_sample_ = sample;
        state_.event = kVoiceOn;
        state_.midi_note = midi_note;
        state_.tuned_note = tuned_note;
        state_.velocity = velocity;
        state_.lift = kDefaultLiftVelocity;
        state_.local_pitch_bend = 0.0f;
        state_.last_note = last_note;
        state_.note_pressed = note_pressed;
        state_.note_count = note_count;
        state_.channel = channel;
        state_.sostenuto_pressed = false;
        aftertouch_ = 0.0f;
        aftertouch_sample_ = 0;
        slide_ = 0.0f;
        slide_sample_ = 0;
        setKeyState(kTriggering);
      }

      force_inline void setKeyState(KeyState key_state) {
        last_key_state_ = key_state_;
        key_state_ = key_state;
      }

      force_inline void sustain() {
        last_key_state_ = key_state_;
        key_state_ = kSustained;
      }

      force_inline bool sustained() {
        return key_state_ == kSustained;
      }

      force_inline bool held() {
        return key_state_ == kHeld;
      }

      force_inline bool released() {
        return key_state_ == kReleased;
      }

      force_inline bool sostenuto() {
        return state_.sostenuto_pressed;
      }

      force_inline void setSostenuto(bool sostenuto) {
        state_.sostenuto_pressed = sostenuto;
      }

      force_inline void setLocalPitchBend(mono_float bend) {
        state_.local_pitch_bend = bend;
      }

      force_inline void setLiftVelocity(mono_float lift) {
        state_.lift = lift;
      }

      force_inline void deactivate(int sample = 0) {
        event_sample_ = sample;
        state_.event = kVoiceOff;
        setKeyState(kReleased);
      }

      force_inline void kill(int sample = 0) {
        event_sample_ = sample;
        state_.event = kVoiceKill;
      }

      force_inline void markDead() {
        setKeyState(kDead);
      }

      force_inline bool hasNewEvent() {
        return event_sample_ >= 0;
      }

      force_inline void setAftertouch(mono_float aftertouch, int sample = 0) {
        aftertouch_ = aftertouch;
        aftertouch_sample_ = sample;
      }

      force_inline void setSlide(mono_float slide, int sample = 0) {
        slide_ = slide;
        slide_sample_ = sample;
      }

      force_inline bool hasNewAftertouch() {
        return aftertouch_sample_ >= 0;
      }

      force_inline bool hasNewSlide() {
        return slide_sample_ >= 0;
      }

      force_inline void completeVoiceEvent() {
        event_sample_ = -1;
        if (key_state_ == kTriggering)
          setKeyState(kHeld);
      }

      force_inline void shiftVoiceEvent(int num_samples) {
        event_sample_ -= num_samples;
        VITAL_ASSERT(event_sample_ >= 0);
      }

      force_inline void shiftAftertouchEvent(int num_samples) {
        aftertouch_sample_ -= num_samples;
        VITAL_ASSERT(aftertouch_sample_ >= 0);
      }

      force_inline void shiftSlideEvent(int num_samples) {
        slide_sample_ -= num_samples;
        VITAL_ASSERT(slide_sample_ >= 0);
      }

      force_inline void clearAftertouchEvent() {
        aftertouch_sample_ = -1;
      }

      force_inline void clearSlideEvent() {
        aftertouch_sample_ = -1;
      }

      force_inline void clearEvents() {
        event_sample_ = -1;
        aftertouch_sample_ = -1;
      }

      force_inline void setSharedVoices(std::vector<Voice*> shared_voices) {
        for (Voice* voice : shared_voices) {
          if (voice != this)
            shared_voices_.push_back(voice);
        }
      }

      force_inline void setVoiceInfo(int voice_index, poly_mask voice_mask) {
        voice_index_ = voice_index;
        voice_mask_ = voice_mask;
      }

    private:
      int voice_index_;
      poly_mask voice_mask_;
      std::vector<Voice*> shared_voices_;
      int event_sample_;
      VoiceState state_;
      KeyState last_key_state_;
      KeyState key_state_;

      int aftertouch_sample_;
      mono_float aftertouch_;

      int slide_sample_;
      mono_float slide_;

      AggregateVoice* parent_;
  };

  struct AggregateVoice {
    CircularQueue<Voice*> voices;
    std::unique_ptr<Processor> processor;
  };

  class VoiceHandler : public SynthModule, public NoteHandler {
    public:
      static constexpr mono_float kLocalPitchBendRange = 48.0f;

      enum {
        kPolyphony,
        kVoicePriority,
        kVoiceOverride,
        kNumInputs
      };

      enum VoiceOverride {
        kKill,
        kSteal,
        kNumVoiceOverrides
      };

      enum VoicePriority {
        kNewest,
        kOldest,
        kHighest,
        kLowest,
        kRoundRobin,
        kNumVoicePriorities
      };

      VoiceHandler(int num_outputs, int polyphony, bool control_rate = false);
      VoiceHandler() = delete;

      virtual ~VoiceHandler();

      virtual Processor* clone() const override {
        VITAL_ASSERT(false);
        return nullptr;
      }

      virtual void process(int num_samples) override;
      virtual void init() override;
      virtual void setSampleRate(int sample_rate) override;
      void setTuning(const Tuning* tuning) { tuning_ = tuning; }

      int getNumActiveVoices();
      force_inline int getNumPressedNotes() { return pressed_notes_.size(); }
      bool isNotePlaying(int note);
      bool isNotePlaying(int note, int channel);

      void allSoundsOff() override;
      void allNotesOff(int sample) override;
      void allNotesOff(int sample, int channel) override;
      void allNotesOffRange(int sample, int from_channel, int to_channel);

      virtual void noteOn(int note, mono_float velocity, int sample, int channel) override;
      virtual void noteOff(int note, mono_float velocity, int sample, int channel) override;

      void setAftertouch(int note, mono_float aftertouch, int sample, int channel);
      void setChannelAftertouch(int channel, mono_float aftertouch, int sample);
      void setChannelRangeAftertouch(int from_channel, int to_channel, mono_float aftertouch, int sample);
      void setChannelSlide(int channel, mono_float aftertouch, int sample);
      void setChannelRangeSlide(int from_channel, int to_channel, mono_float aftertouch, int sample);

      void sustainOn(int channel);
      void sustainOff(int sample, int channel);
      void sostenutoOn(int channel);
      void sostenutoOff(int sample, int channel);
      void sustainOnRange(int from_channel, int to_channel);
      void sustainOffRange(int sample, int from_channel, int to_channel);
      void sostenutoOnRange(int from_channel, int to_channel);
      void sostenutoOffRange(int sample, int from_channel, int to_channel);

      poly_mask getCurrentVoiceMask();

      force_inline void setModWheel(mono_float value, int channel = 0) {
        VITAL_ASSERT(channel < kNumMidiChannels && channel >= 0);
        mod_wheel_values_[channel] = value;
      }

      force_inline void setModWheelAllChannels(mono_float value) {
        for (int i = 0; i < kNumMidiChannels; ++i)
          mod_wheel_values_[i] = value;
      }

      force_inline void setPitchWheel(mono_float value, int channel = 0) {
        VITAL_ASSERT(channel < kNumMidiChannels && channel >= 0);
        pitch_wheel_values_[channel] = value;
        for (Voice* voice : active_voices_) {
          if (voice->state().channel == channel && voice->held())
            voice->setLocalPitchBend(value);
        }
      }

      force_inline void setZonedPitchWheel(mono_float value, int from_channel, int to_channel) {
        VITAL_ASSERT(from_channel < kNumMidiChannels && from_channel >= 0);
        VITAL_ASSERT(to_channel < kNumMidiChannels && to_channel >= 0);
        VITAL_ASSERT(to_channel >= from_channel);
        for (int i = from_channel; i <= to_channel; ++i)
          zoned_pitch_wheel_values_[i] = value;
      }

      force_inline Output* voice_event() { return &voice_event_; }
      force_inline Output* retrigger() { return &retrigger_; }
      force_inline Output* reset() { return &reset_; }
      force_inline Output* note() { return &note_; }
      force_inline Output* last_note() { return &last_note_; }
      force_inline Output* note_pressed() { return &note_pressed_; }
      force_inline Output* note_count() { return &note_count_; }
      force_inline Output* note_in_octave() { return &note_in_octave_; }
      force_inline Output* channel() { return &channel_; }
      force_inline Output* velocity() { return &velocity_; }
      force_inline Output* lift() { return &lift_; }
      force_inline Output* aftertouch() { return &aftertouch_; }
      force_inline Output* slide() { return &slide_; }
      force_inline Output* active_mask() { return &active_mask_; }
      force_inline Output* pitch_wheel() { return &pitch_wheel_; }
      force_inline Output* pitch_wheel_percent() { return &pitch_wheel_percent_; }
      force_inline Output* local_pitch_bend() { return &local_pitch_bend_; }
      force_inline Output* mod_wheel() { return &mod_wheel_; }
      force_inline Output* getAccumulatedOutput(Output* output) { return accumulated_outputs_[output].get(); }
      force_inline int polyphony() { return polyphony_; }
    
      mono_float getLastActiveNote() const;

      virtual ProcessorRouter* getMonoRouter() override { return &global_router_; }
      virtual ProcessorRouter* getPolyRouter() override { return &voice_router_; }

      void addProcessor(Processor* processor) override;
      void addIdleProcessor(Processor* processor) override;
      void removeProcessor(Processor* processor) override;
      void addGlobalProcessor(Processor* processor);
      void removeGlobalProcessor(Processor* processor);
      void resetFeedbacks(poly_mask reset_mask) override;
      Output* registerOutput(Output* output) override;
      Output* registerControlRateOutput(Output* output, bool active);
      Output* registerOutput(Output* output, int index) override;

      void setPolyphony(int polyphony);

      force_inline void setVoiceKiller(const Output* killer) {
        voice_killer_ = killer;
      }

      force_inline void setVoiceKiller(const Processor* killer) {
        setVoiceKiller(killer->output());
      }

      force_inline void setVoiceMidi(const Output* midi) {
        voice_midi_ = midi;
      }

      force_inline void setLegato(bool legato) {
        legato_ = legato;
      }

      force_inline bool legato() {
        return legato_;
      }

      bool isPolyphonic(const Processor* processor) const override;

      virtual void setOversampleAmount(int oversample) override {
        SynthModule::setOversampleAmount(oversample);
        voice_router_.setOversampleAmount(oversample);
        global_router_.setOversampleAmount(oversample);
      }

      void setActiveNonaccumulatedOutput(Output* output);
      void setInactiveNonaccumulatedOutput(Output* output);

    protected:
      virtual bool shouldAccumulate(Output* output);

    private:
      Voice* grabVoice();
      Voice* grabFreeVoice();
      Voice* grabFreeParallelVoice();
      Voice* grabVoiceOfType(Voice::KeyState key_state);
      Voice* getVoiceToKill(int max_voices);
      int grabNextUnplayedPressedNote();
      void sortVoicePriority();
      void addParallelVoices();
      void prepareVoiceTriggers(AggregateVoice* aggregate_voice, int num_samples);
      void prepareVoiceValues(AggregateVoice* aggregate_voice);
      void processVoice(AggregateVoice* aggregate_voice, int num_samples);
      void clearAccumulatedOutputs();
      void clearNonaccumulatedOutputs();
      void accumulateOutputs(int num_samples);
      void combineAccumulatedOutputs(int num_samples);
      void writeNonaccumulatedOutputs(poly_mask voice_mask, int num_samples);

      int polyphony_;
      bool legato_;
      std::map<Output*, std::unique_ptr<Output>> last_voice_outputs_;
      CircularQueue<std::pair<Output*, Output*>> nonaccumulated_outputs_;
      std::map<Output*, std::unique_ptr<Output>> accumulated_outputs_;
      const Output* voice_killer_;
      const Output* voice_midi_;
      int last_num_voices_;
      poly_float last_played_note_;

      cr::Output voice_event_;
      cr::Output retrigger_;
      cr::Output reset_;
      cr::Output note_;
      cr::Output last_note_;
      cr::Output note_pressed_;
      cr::Output note_count_;
      cr::Output note_in_octave_;
      cr::Output channel_;
      cr::Output velocity_;
      cr::Output lift_;
      cr::Output aftertouch_;
      cr::Output slide_;
      cr::Output active_mask_;
      cr::Output mod_wheel_;
      cr::Output pitch_wheel_;
      cr::Output pitch_wheel_percent_;
      cr::Output local_pitch_bend_;

      bool sustain_[kNumMidiChannels];
      bool sostenuto_[kNumMidiChannels];
      mono_float mod_wheel_values_[kNumMidiChannels];
      mono_float pitch_wheel_values_[kNumMidiChannels];
      mono_float zoned_pitch_wheel_values_[kNumMidiChannels];
      mono_float pressure_values_[kNumMidiChannels];
      mono_float slide_values_[kNumMidiChannels];

      const Tuning* tuning_;
      VoicePriority voice_priority_;
      VoiceOverride voice_override_;

      int total_notes_;
      CircularQueue<int> pressed_notes_;
      CircularQueue<std::unique_ptr<Voice>> all_voices_;

      CircularQueue<Voice*> free_voices_;
      CircularQueue<Voice*> active_voices_;
      CircularQueue<std::unique_ptr<AggregateVoice>> all_aggregate_voices_;
      CircularQueue<AggregateVoice*> active_aggregate_voices_;

      ProcessorRouter voice_router_;
      ProcessorRouter global_router_;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceHandler)
  };
} // namespace vital

