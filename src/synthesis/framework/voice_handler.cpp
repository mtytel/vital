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

#include "voice_handler.h"

#include "synth_constants.h"
#include "utils.h"

namespace vital {

  namespace {
    constexpr int kParallelVoices = poly_float::kSize / 2;
    constexpr int kChannelShift = 8;
    constexpr int kNoteMask = (1 << kChannelShift) - 1;

    force_inline int combineNoteChannel(int note, int channel) {
      return (channel << kChannelShift) + note;
    }

    force_inline int getChannel(int value) {
      return value >> kChannelShift;
    }

    force_inline int getNote(int value) {
      return value & kNoteMask;
    }

    force_inline int voiceCompareNewestFirst(Voice* left, Voice* right) {
      int left_count = left->state().note_count;
      int right_count = right->state().note_count;
      return left_count - right_count;
    }

    force_inline int voiceCompareLowestFirst(Voice* left, Voice* right) {
      return right->state().midi_note - left->state().midi_note;
    }

    force_inline int voiceCompareHighestFirst(Voice* left, Voice* right) {
      return left->state().midi_note - right->state().midi_note;
    }

    force_inline int pressedCompareLowestFirst(int left, int right) {
      return getNote(right) - getNote(left);
    }

    force_inline int pressedCompareHighestFirst(int left, int right) {
      return getNote(left) - getNote(right);
    }
  } // namespace

  Voice::Voice(AggregateVoice* parent) : voice_index_(0), voice_mask_(0), event_sample_(-1),
      aftertouch_sample_(-1), aftertouch_(0.0f), slide_sample_(-1), slide_(0.0f), parent_(parent) {
    state_.event = kVoiceOff;
    state_.midi_note = 0;
    state_.tuned_note = 0;
    state_.velocity = 0;
    state_.last_note = 0;
    state_.note_pressed = 0;
    state_.channel = 0;
    key_state_ = kDead;
    last_key_state_ = kDead;
  }

  VoiceHandler::VoiceHandler(int num_outputs, int polyphony, bool control_rate) :
      SynthModule(kNumInputs, num_outputs, control_rate), polyphony_(0), legato_(false),
      voice_killer_(nullptr), last_num_voices_(0), last_played_note_(-1.0f),
      sustain_(), sostenuto_(), mod_wheel_values_(), pitch_wheel_values_(), zoned_pitch_wheel_values_(),
      pressure_values_(), slide_values_(), tuning_(nullptr),
      voice_priority_(kRoundRobin), voice_override_(kKill), total_notes_(0) {
    pressed_notes_.reserve(kMidiSize);
    all_voices_.reserve(kMaxPolyphony + kParallelVoices);
    free_voices_.reserve(kMaxPolyphony + kParallelVoices);
    active_voices_.reserve(kMaxPolyphony + kParallelVoices);
    all_aggregate_voices_.reserve(kMaxPolyphony / kParallelVoices + kParallelVoices);
    active_aggregate_voices_.reserve(kMaxPolyphony / kParallelVoices + kParallelVoices);

    voice_midi_ = &note_;

    voice_event_.owner = &voice_router_;
    retrigger_.owner = &voice_router_;
    reset_.owner = &voice_router_;
    note_.owner = &voice_router_;
    last_note_.owner = &voice_router_;
    note_pressed_.owner = &voice_router_;
    note_count_.owner = &voice_router_;
    note_in_octave_.owner = &voice_router_;
    channel_.owner = &voice_router_;
    velocity_.owner = &voice_router_;
    lift_.owner = &voice_router_;
    aftertouch_.owner = &voice_router_;
    slide_.owner = &voice_router_;
    active_mask_.owner = &voice_router_;
    mod_wheel_.owner = &voice_router_;
    pitch_wheel_.owner = &voice_router_;
    pitch_wheel_percent_.owner = &voice_router_;
    local_pitch_bend_.owner = &voice_router_;

    setPolyphony(polyphony);
    voice_router_.router(this);
    global_router_.router(this);
  }

  VoiceHandler::~VoiceHandler() { }

  void VoiceHandler::prepareVoiceTriggers(AggregateVoice* aggregate_voice, int num_samples) {
    note_.clearTrigger();
    last_note_.clearTrigger();
    channel_.clearTrigger();
    velocity_.clearTrigger();
    lift_.clearTrigger();
    voice_event_.clearTrigger();
    retrigger_.clearTrigger();
    reset_.clearTrigger();
    aftertouch_.clearTrigger();
    slide_.clearTrigger();

    for (Voice* voice : aggregate_voice->voices) {
      if (voice->hasNewEvent()) {
        int offset = voice->event_sample() * getOversampleAmount();
        if (num_samples <= offset)
          voice->shiftVoiceEvent(num_samples / getOversampleAmount());
        else {
          poly_mask mask = voice->voice_mask();
          voice_event_.trigger(mask, voice->state().event, offset);

          if (voice->state().event == kVoiceOn) {
            note_.trigger(mask, voice->state().tuned_note, offset);
            last_note_.trigger(mask, voice->state().last_note, offset);
            velocity_.trigger(mask, voice->state().velocity, offset);
            channel_.trigger(mask, voice->state().channel, offset);

            if (voice->last_key_state() == Voice::kDead)
              reset_.trigger(mask, kVoiceOn, offset);
          }
          else if (voice->state().event == kVoiceOff)
            lift_.trigger(mask, voice->state().lift, offset);

          if (!legato_ || voice->last_key_state() != Voice::kHeld || voice->state().event != kVoiceOn)
            retrigger_.trigger(mask, voice->state().event, offset);

          voice->completeVoiceEvent();
        }

        if (voice->hasNewAftertouch()) {
          int aftertouch_sample = voice->aftertouch_sample() * getOversampleAmount();
          if (num_samples <= aftertouch_sample)
            voice->shiftAftertouchEvent(num_samples / getOversampleAmount());
          else {
            aftertouch_.trigger(voice->voice_mask(), voice->aftertouch(), aftertouch_sample);
            voice->clearAftertouchEvent();
          }
        }

        if (voice->hasNewSlide()) {
          int slide_sample = voice->slide_sample() * getOversampleAmount();
          if (num_samples <= slide_sample)
            voice->shiftSlideEvent(num_samples / getOversampleAmount());
          else {
            slide_.trigger(voice->voice_mask(), voice->slide(), slide_sample);
            voice->clearSlideEvent();
          }
        }
      }
    }
  }

  void VoiceHandler::prepareVoiceValues(AggregateVoice* aggregate_voice) {
    for (Voice* voice : aggregate_voice->voices) {
      poly_mask mask = voice->voice_mask();
      int channel = voice->state().channel;
      poly_float note = utils::maskLoad(note_.trigger_value, voice->state().tuned_note, mask);
      note_.trigger_value = note;
      last_note_.trigger_value = utils::maskLoad(last_note_.trigger_value, voice->state().last_note, mask);

      note_pressed_.trigger_value = utils::maskLoad(note_pressed_.trigger_value, voice->state().note_pressed, mask);
      note_count_.trigger_value = utils::maskLoad(note_count_.trigger_value, voice->state().note_count, mask);
      note_in_octave_.trigger_value = utils::mod(note * (1.0f / kNotesPerOctave));
      channel_.trigger_value = utils::maskLoad(channel_.trigger_value, channel, mask);
      velocity_.trigger_value = utils::maskLoad(velocity_.trigger_value, voice->state().velocity, mask);

      mono_float lift = 0.0f;
      if (voice->released())
        lift = voice->state().lift;
      lift_.trigger_value = utils::maskLoad(lift_.trigger_value, lift, mask);

      aftertouch_.trigger_value = utils::maskLoad(aftertouch_.trigger_value, voice->aftertouch(), mask);
      slide_.trigger_value = utils::maskLoad(slide_.trigger_value, voice->slide(), mask);

      bool dead = voice->key_state() == Voice::kDead;
      poly_float active_value = dead ? 0.0f : 1.0f;
      active_mask_.trigger_value = utils::maskLoad(active_mask_.trigger_value, active_value, mask);

      mono_float mod_wheel = mod_wheel_values_[channel];
      mod_wheel_.trigger_value = utils::maskLoad(mod_wheel_.trigger_value, mod_wheel, mask);

      mono_float pitch_wheel = zoned_pitch_wheel_values_[channel];
      pitch_wheel_.trigger_value = utils::maskLoad(pitch_wheel_.trigger_value, pitch_wheel, mask);

      mono_float pitch_wheel_percent = pitch_wheel * 0.5f + 0.5f;
      pitch_wheel_percent_.trigger_value = utils::maskLoad(pitch_wheel_percent_.trigger_value,
                                                           pitch_wheel_percent, mask);

      mono_float local_pitch_bend = voice->state().local_pitch_bend * kLocalPitchBendRange;
      local_pitch_bend_.trigger_value = utils::maskLoad(local_pitch_bend_.trigger_value, local_pitch_bend, mask);
    }
  }

  void VoiceHandler::processVoice(AggregateVoice* voice, int num_samples) {
    voice->processor->process(num_samples);
  }

  void VoiceHandler::clearAccumulatedOutputs() {
    for (auto& output : accumulated_outputs_)
      utils::zeroBuffer(output.second->buffer, output.second->buffer_size);
  }

  void VoiceHandler::clearNonaccumulatedOutputs() {
    for (auto& outputs : nonaccumulated_outputs_)
      utils::zeroBuffer(outputs.second->buffer, outputs.second->buffer_size);
  }

  void VoiceHandler::accumulateOutputs(int num_samples) {
    for (auto& output : accumulated_outputs_) {
      int buffer_size = std::min(num_samples, output.second->buffer_size);
      poly_float* dest = output.second->buffer;
      const poly_float* source = output.first->buffer;

      for (int i = 0; i < buffer_size; ++i)
        dest[i] += source[i];
    }
  }

  void VoiceHandler::combineAccumulatedOutputs(int num_samples) {
    for (auto& output : accumulated_outputs_) {
      int buffer_size = std::min(num_samples, output.second->buffer_size);
      poly_float* dest = output.second->buffer;

      for (int i = 0; i < buffer_size; ++i)
        dest[i] += utils::swapVoices(dest[i]);
    }
  }

  void VoiceHandler::writeNonaccumulatedOutputs(poly_mask voice_mask, int num_samples) {
    for (auto& outputs : nonaccumulated_outputs_) {
      int buffer_size = std::min(num_samples, outputs.second->buffer_size);
      poly_float* dest = outputs.second->buffer;
      const poly_float* source = outputs.first->buffer;

      VITAL_ASSERT(buffer_size == 1);

      for (int i = 0; i < buffer_size; ++i) {
        poly_float masked = source[i] & voice_mask;
        dest[i] = masked + utils::swapVoices(masked);
      }
    }
  }

  bool VoiceHandler::shouldAccumulate(Output* output) {
    return output->buffer_size > 1 || (output->owner && !output->owner->isControlRate());
  }

  void VoiceHandler::process(int num_samples) {
    global_router_.process(num_samples);

    int num_voices = active_voices_.size();
    if (num_voices == 0) {
      if (last_num_voices_)
        clearAccumulatedOutputs();

      last_num_voices_ = num_voices;
      return;
    }

    int polyphony = static_cast<int>(std::roundf(input(kPolyphony)->at(0)[0]));
    setPolyphony(utils::iclamp(polyphony, 1, kMaxActivePolyphony));

    int priority = utils::roundToInt(input(kVoicePriority)->at(0))[0];
    voice_priority_ = static_cast<VoicePriority>(priority);

    int voice_override = utils::roundToInt(input(kVoiceOverride)->at(0))[0];
    voice_override_ = static_cast<VoiceOverride>(voice_override);

    clearAccumulatedOutputs();

    active_aggregate_voices_.clear();
    AggregateVoice* last_aggregate_voice = nullptr;
    int last_aggregate_index = 0;
    for (Voice* active_voice : active_voices_) {
      if (active_aggregate_voices_.count(active_voice->parent()) == 0)
        active_aggregate_voices_.push_back(active_voice->parent());
      last_aggregate_voice = active_voice->parent();
      last_aggregate_index = active_voice->voice_index();
    }

    if (last_aggregate_voice) {
      active_aggregate_voices_.remove(last_aggregate_voice);
      active_aggregate_voices_.push_back(last_aggregate_voice);
    }

    for (AggregateVoice* aggregate_voice : active_aggregate_voices_) {
      prepareVoiceTriggers(aggregate_voice, num_samples);
      prepareVoiceValues(aggregate_voice);
      processVoice(aggregate_voice, num_samples);
      accumulateOutputs(num_samples);

      // Remove voice if the right processor has a full silent buffer.
      poly_mask alive_mask = constants::kFullMask;
      if (voice_killer_)
        alive_mask = ~utils::getSilentMask(voice_killer_->buffer, num_samples);
      for (Voice* single_voice : aggregate_voice->voices) {
        bool released = single_voice->state().event == kVoiceOff || single_voice->state().event == kVoiceKill;
        bool alive = (single_voice->voice_mask() & alive_mask).sum();
        bool active = active_voices_.count(single_voice);
        if (released && !alive && active) {
          active_voices_.remove(single_voice);
          free_voices_.push_back(single_voice);
          single_voice->markDead();
        }
      }
    }

    combineAccumulatedOutputs(num_samples);

    if (active_voices_.size()) {
      poly_mask voice_mask = constants::kFirstMask;
      if (last_aggregate_index)
        voice_mask = ~voice_mask;

      writeNonaccumulatedOutputs(voice_mask, num_samples);

      last_played_note_ = voice_midi_->trigger_value & voice_mask;
      last_played_note_ += utils::swapVoices(last_played_note_);
    }

    last_num_voices_ = num_voices;
  }

  void VoiceHandler::init() {
    voice_router_.init();
    global_router_.init();
    ProcessorRouter::init();
  }

  void VoiceHandler::setSampleRate(int sample_rate) {
    ProcessorRouter::setSampleRate(sample_rate);
    voice_router_.setSampleRate(sample_rate);
    global_router_.setSampleRate(sample_rate);
    for (auto& aggregate_voice : all_aggregate_voices_)
      aggregate_voice->processor->setSampleRate(sample_rate);
  }

  int VoiceHandler::getNumActiveVoices() {
    return active_voices_.size();
  }

  bool VoiceHandler::isNotePlaying(int note) {
    for (Voice* voice : active_voices_) {
      if (voice->state().event != kVoiceKill && voice->state().midi_note == note)
        return true;
    }
    return false;
  }

  bool VoiceHandler::isNotePlaying(int note, int channel) {
    for (Voice* voice : active_voices_) {
      if (voice->state().event != kVoiceKill && voice->state().midi_note == note && voice->state().channel == channel)
        return true;
    }
    return false;
  }

  void VoiceHandler::sustainOn(int channel) {
    sustain_[channel] = true;
  }

  void VoiceHandler::sustainOff(int sample, int channel) {
    sustain_[channel] = false;
    for (Voice* voice : active_voices_) {
      if (voice->sustained() && !voice->sostenuto() && voice->state().channel == channel)
        voice->deactivate(sample);
    }
  }

  void VoiceHandler::sostenutoOn(int channel) {
    sostenuto_[channel] = true;
    for (Voice* voice : active_voices_) {
      if (voice->state().channel == channel)
        voice->setSostenuto(true);
    }
  }

  void VoiceHandler::sostenutoOff(int sample, int channel) {
    sostenuto_[channel] = false;
    for (Voice* voice : active_voices_) {
      if (voice->state().channel == channel) {
        voice->setSostenuto(false);

        if (voice->sustained() && !sustain_[channel])
          voice->deactivate(sample);
      }
    }
  }

  void VoiceHandler::sustainOnRange(int from_channel, int to_channel) {
    for (int i = from_channel; i <= to_channel; ++i)
      sustain_[i] = true;
  }

  void VoiceHandler::sustainOffRange(int sample, int from_channel, int to_channel) {
    for (int i = from_channel; i <= to_channel; ++i)
      sustain_[i] = false;

    for (Voice* voice : active_voices_) {
      int channel = voice->state().channel;
      if (voice->sustained() && !voice->sostenuto() && channel >= from_channel && channel <= to_channel)
        voice->deactivate(sample);
    }
  }

  void VoiceHandler::sostenutoOnRange(int from_channel, int to_channel) {
    for (int i = from_channel; i <= to_channel; ++i)
      sostenuto_[i] = true;
    for (Voice* voice : active_voices_) {
      int channel = voice->state().channel;

      if (channel >= from_channel && channel <= to_channel)
        voice->setSostenuto(true);
    }
  }

  void VoiceHandler::sostenutoOffRange(int sample, int from_channel, int to_channel) {
    for (int i = from_channel; i <= to_channel; ++i)
      sostenuto_[i] = false;

    for (Voice* voice : active_voices_) {
      int channel = voice->state().channel;
      if (channel >= from_channel && channel <= to_channel) {
        voice->setSostenuto(false);

        if (voice->sustained() && !sustain_[channel])
          voice->deactivate(sample);
      }
    }
  }

  void VoiceHandler::allSoundsOff() {
    pressed_notes_.clear();

    for (Voice* voice : active_voices_) {
      voice->kill(0);
      voice->markDead();
      free_voices_.push_back(voice);
    }

    active_voices_.clear();
  }
  
  void VoiceHandler::allNotesOff(int sample) {
    pressed_notes_.clear();

    for (Voice* voice : active_voices_)
      voice->deactivate(sample);
  }

  void VoiceHandler::allNotesOff(int sample, int channel) {
    pressed_notes_.clear();

    for (Voice* voice : active_voices_) {
      if (voice->state().channel == channel)
        voice->deactivate(sample);
    }
  }

  void VoiceHandler::allNotesOffRange(int sample, int from_channel, int to_channel) {
    pressed_notes_.clear();

    for (Voice* voice : active_voices_) {
      int channel = voice->state().channel;
      if (channel >= from_channel && channel <= to_channel)
        voice->deactivate(sample);
    }
  }

  poly_mask VoiceHandler::getCurrentVoiceMask() {
    if (active_voices_.size()) {
      if (active_voices_.back()->voice_index())
        return ~constants::kFirstMask;
      return constants::kFirstMask;
    }

    return 0;
  }

  Voice* VoiceHandler::grabVoice() {
    Voice* voice = nullptr;
    if (active_voices_.size() < polyphony() || (voice_override_ == kKill && !legato_)) {
      voice = grabFreeParallelVoice();
      if (voice == nullptr)
        voice = grabFreeVoice();
    }

    if (voice == nullptr)
      voice = grabVoiceOfType(Voice::kReleased);

    if (voice == nullptr)
      voice = grabVoiceOfType(Voice::kSustained);

    if (voice == nullptr)
      voice = grabVoiceOfType(Voice::kHeld);

    if (voice == nullptr)
      voice = grabVoiceOfType(Voice::kTriggering);

    return voice;
  }

  Voice* VoiceHandler::grabFreeVoice() {
    Voice* voice = nullptr;
    if (free_voices_.size()) {
      voice = free_voices_.front();
      free_voices_.pop_front();
    }
    return voice;
  }

  Voice* VoiceHandler::grabFreeParallelVoice() {
    for (auto& aggregate_voice : all_aggregate_voices_) {
      Voice* dead_voice = nullptr;
      bool has_active_voice = false;

      for (Voice* single_voice : aggregate_voice->voices) {
        if (single_voice->key_state() == Voice::kDead)
          dead_voice = single_voice;
        else
          has_active_voice = true;
      }

      if (has_active_voice && dead_voice) {
        VITAL_ASSERT(free_voices_.count(dead_voice));
        free_voices_.remove(dead_voice);
        return dead_voice;
      }
    }
    return nullptr;
  }

  Voice* VoiceHandler::grabVoiceOfType(Voice::KeyState key_state) {
    for (auto iter = active_voices_.begin(); iter != active_voices_.end(); ++iter) {
      Voice* voice = *iter;
      if (voice->key_state() == key_state) {
        active_voices_.erase(iter);
        return voice;
      }
    }
    return nullptr;
  }

  Voice* VoiceHandler::getVoiceToKill(int max_voices) {
    int excess_voices = active_voices_.size() - max_voices;
    Voice* released = nullptr;
    Voice* sustained = nullptr;
    Voice* held = nullptr;

    for (Voice* voice : active_voices_) {
      if (voice->state().event == kVoiceKill)
        excess_voices--;
      else if (released == nullptr && voice->key_state() == Voice::kReleased)
        released = voice;
      else if (sustained == nullptr && voice->key_state() == Voice::kSustained)
        sustained = voice;
      else if (held == nullptr)
        held = voice;
    }

    // Return null if we've killed enough voices.
    if (excess_voices <= 0)
      return nullptr;

    if (released)
      return released;

    if (sustained)
      return sustained;

    if (held)
      return held;

    return nullptr;
  }

  int VoiceHandler::grabNextUnplayedPressedNote() {
    auto iter = pressed_notes_.begin();

    if (voice_priority_ == kNewest) {
      iter = pressed_notes_.end();

      while (iter != pressed_notes_.begin()) {
        iter--;
        if (!isNotePlaying(getNote(*iter), getChannel(*iter)))
          break;
      }
    }
    else {
      for (; iter != pressed_notes_.end(); ++iter) {
        if (!isNotePlaying(getNote(*iter), getChannel(*iter)))
          break;
      }
    }

    int old_note_value = *iter;
    if (voice_priority_ == kRoundRobin) {
      pressed_notes_.erase(iter);
      pressed_notes_.push_back(old_note_value);
    }
    return old_note_value;
  }

  void VoiceHandler::sortVoicePriority() {
    switch (voice_priority_) {
      case kHighest:
        active_voices_.sort<voiceCompareLowestFirst>();
        pressed_notes_.sort<pressedCompareHighestFirst>();
        break;
      case kLowest:
        active_voices_.sort<voiceCompareHighestFirst>();
        pressed_notes_.sort<pressedCompareLowestFirst>();
        break;
      case kOldest:
        active_voices_.sort<voiceCompareNewestFirst>();
        break;
      default:
        break;
    }
  }

  void VoiceHandler::noteOn(int note, mono_float velocity, int sample, int channel) {
    VITAL_ASSERT(channel >= 0 && channel < kNumMidiChannels);

    Voice* voice = grabVoice();
    if (voice == nullptr)
      return;

    mono_float tuned_note = note;
    if (tuning_)
      tuned_note = tuning_->convertMidiNote(note);

    poly_float last_note = tuned_note;
    if (last_played_note_[0] >= 0.0f)
      last_note = last_played_note_;
    last_played_note_ = tuned_note;

    int note_value = combineNoteChannel(note, channel);
    pressed_notes_.remove(note_value);
    pressed_notes_.push_back(note_value);

    total_notes_++;
    voice->activate(note, tuned_note, velocity, last_note, pressed_notes_.size(), total_notes_, sample, channel);
    voice->setLocalPitchBend(pitch_wheel_values_[channel]);
    voice->setAftertouch(pressure_values_[channel]);
    voice->setSlide(slide_values_[channel]);
    active_voices_.push_back(voice);

    sortVoicePriority();
  }

  void VoiceHandler::noteOff(int note, mono_float lift, int sample, int channel) {
    pressed_notes_.removeAll(combineNoteChannel(note, channel));

    for (Voice* voice : active_voices_) {
      if (voice->state().midi_note == note && voice->state().channel == channel) {
        if (sustain_[channel]) {
          voice->sustain();
          voice->setLiftVelocity(lift);
        }
        else {
          if (polyphony_ <= pressed_notes_.size() && voice->state().event != kVoiceKill) {
            Voice* new_voice = voice;
            if (voice_override_ == kKill) {
              voice->kill();
              new_voice = grabVoice();
            }
            else
              active_voices_.remove(voice);

            if (voice_priority_ == kNewest)
              active_voices_.push_front(new_voice);
            else
              active_voices_.push_back(new_voice);

            int old_note_value = grabNextUnplayedPressedNote();

            mono_float old_note = getNote(old_note_value);
            int old_channel = getChannel(old_note_value);
            mono_float tuned_note = old_note;
            if (tuning_)
              tuned_note = tuning_->convertMidiNote(old_note);

            total_notes_++;
            new_voice->activate(old_note, tuned_note, voice->state().velocity,
                                last_played_note_, pressed_notes_.size() + 1, total_notes_, sample, old_channel);
            voice->setLocalPitchBend(pitch_wheel_values_[channel]);
            voice->setAftertouch(pressure_values_[channel]);
            voice->setSlide(slide_values_[channel]);
          }
          else {
            voice->deactivate(sample);
            voice->setLiftVelocity(lift);
          }
        }
      }
    }

    sortVoicePriority();
  }

  void VoiceHandler::setAftertouch(int note, mono_float aftertouch, int sample, int channel) {
    for (Voice* voice : active_voices_) {
      if (voice->state().midi_note == note && voice->state().channel == channel)
        voice->setAftertouch(aftertouch, sample);
    }
  }

  void VoiceHandler::setChannelAftertouch(int channel, mono_float aftertouch, int sample) {
    pressure_values_[channel] = aftertouch;
    for (Voice* voice : active_voices_) {
      if (voice->state().channel == channel && voice->held())
        voice->setAftertouch(aftertouch, sample);
    }
  }

  void VoiceHandler::setChannelRangeAftertouch(int from_channel, int to_channel, mono_float aftertouch, int sample) {
    for (Voice* voice : active_voices_) {
      int channel = voice->state().channel;
      if (channel >= from_channel && channel <= to_channel)
        voice->setAftertouch(aftertouch, sample);
    }
  }

  void VoiceHandler::setChannelSlide(int channel, mono_float slide, int sample) {
    slide_values_[channel] = slide;
    for (Voice* voice : active_voices_) {
      if (voice->state().channel == channel && voice->held())
        voice->setSlide(slide, sample);
    }
  }

  void VoiceHandler::setChannelRangeSlide(int from_channel, int to_channel, mono_float slide, int sample) {
    for (Voice* voice : active_voices_) {
      int channel = voice->state().channel;
      if (channel >= from_channel && channel <= to_channel)
        voice->setSlide(slide, sample);
    }
  }

  void VoiceHandler::setPolyphony(int polyphony) {
    while (all_voices_.size() < polyphony)
      addParallelVoices();

    int num_voices_to_kill = active_voices_.size() - polyphony;
    for (int i = 0; i < num_voices_to_kill; ++i) {
      Voice* sacrifice = getVoiceToKill(polyphony);
      if (sacrifice)
        sacrifice->kill();
    }

    polyphony_ = polyphony;
  }

  mono_float VoiceHandler::getLastActiveNote() const {
    if (active_voices_.size())
      return active_voices_.back()->state().tuned_note;
    return 0.0f;
  }

  void VoiceHandler::addProcessor(Processor* processor) {
    processor->setSampleRate(getSampleRate());
    voice_router_.addProcessor(processor);
  }

  void VoiceHandler::addIdleProcessor(Processor* processor) {
    processor->setSampleRate(getSampleRate());
    voice_router_.addIdleProcessor(processor);
  }

  void VoiceHandler::removeProcessor(Processor* processor) {
    voice_router_.removeProcessor(processor);
  }

  void VoiceHandler::addGlobalProcessor(Processor* processor) {
    global_router_.addProcessor(processor);
  }

  void VoiceHandler::removeGlobalProcessor(Processor* processor) {
    global_router_.removeProcessor(processor);
  }

  void VoiceHandler::resetFeedbacks(poly_mask reset_mask) {
    voice_router_.resetFeedbacks(reset_mask);
  }

  Output* VoiceHandler::registerOutput(Output* output) {
    VITAL_ASSERT(accumulated_outputs_.count(output) == 0);
    VITAL_ASSERT(last_voice_outputs_.count(output) == 0);

    Output* new_output = new Output(output->buffer_size);
    new_output->owner = this;
    ProcessorRouter::registerOutput(new_output);

    if (shouldAccumulate(output))
      accumulated_outputs_[output] = std::unique_ptr<Output>(new_output);
    else {
      last_voice_outputs_[output] = std::unique_ptr<Output>(new_output);
      nonaccumulated_outputs_.ensureCapacity(static_cast<int>(last_voice_outputs_.size()));
    }

    return new_output;
  }

  Output* VoiceHandler::registerControlRateOutput(Output* output, bool active) {
    VITAL_ASSERT(accumulated_outputs_.count(output) == 0);
    VITAL_ASSERT(last_voice_outputs_.count(output) == 0);

    cr::Output* new_output = new cr::Output();
    new_output->owner = this;
    ProcessorRouter::registerOutput(new_output);
    last_voice_outputs_[output] = std::unique_ptr<Output>(new_output);
    nonaccumulated_outputs_.ensureCapacity(static_cast<int>(last_voice_outputs_.size()));
    if (active)
      nonaccumulated_outputs_.push_back({ output, new_output });

    return new_output;
  }

  Output* VoiceHandler::registerOutput(Output* output, int index) {
    VITAL_ASSERT(false);
    return output;
  }

  bool VoiceHandler::isPolyphonic(const Processor* processor) const {
    return processor == &voice_router_;
  }

  void VoiceHandler::setActiveNonaccumulatedOutput(Output* output) {
    if (last_voice_outputs_.count(output) == 0)
      return;

    std::pair<Output*, Output*> pair(output, last_voice_outputs_[output].get());
    if (!nonaccumulated_outputs_.contains(pair))
      nonaccumulated_outputs_.push_back(pair);
  }

  void VoiceHandler::setInactiveNonaccumulatedOutput(Output* output) {
    if (last_voice_outputs_.count(output) == 0)
      return;

    std::pair<Output*, Output*> pair(output, last_voice_outputs_[output].get());
    utils::zeroBuffer(pair.second->buffer, pair.second->buffer_size);
    nonaccumulated_outputs_.remove(pair);
  }

  void VoiceHandler::addParallelVoices() {
    poly_float voice_value = 0.0f;
    for (int i = 0; i < kParallelVoices; ++i) {
      voice_value.set(2 * i, i);
      voice_value.set(2 * i + 1, i);
    }

    std::unique_ptr<AggregateVoice> aggregate_voice = std::make_unique<AggregateVoice>();
    aggregate_voice->processor = std::unique_ptr<Processor>(voice_router_.clone());
    aggregate_voice->processor->process(1);
    aggregate_voice->voices.reserve(kParallelVoices);

    for (int i = 0; i < kParallelVoices; ++i) {
      std::unique_ptr<Voice> single_voice = std::make_unique<Voice>(aggregate_voice.get());
      single_voice->setVoiceInfo(i, poly_float::equal(voice_value, i));

      aggregate_voice->voices.push_back(single_voice.get());
      free_voices_.push_back(single_voice.get());
      all_voices_.push_back(std::move(single_voice));
    }

    all_aggregate_voices_.push_back(std::move(aggregate_voice));
  }
} // namespace vital
