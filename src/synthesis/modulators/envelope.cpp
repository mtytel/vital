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

#include "envelope.h"
#include "futils.h"

namespace vital {

  Envelope::Envelope() :
      Processor(kNumInputs, kNumOutputs), current_value_(0.0f),
      position_(0.0f), value_(0.0f), poly_state_(0.0f), start_value_(0.0f),
      attack_power_(0.0f), decay_power_(0.0f), release_power_(0.0f), sustain_(0.0f) { }

  void Envelope::process(int num_samples) {
    if (isControlRate())
      processControlRate(num_samples);
    else
      processAudioRate(num_samples);
  }

  void Envelope::processControlRate(int num_samples) {
    poly_mask trigger_mask = input(kTrigger)->source->trigger_mask;
    poly_float trigger_value = input(kTrigger)->source->trigger_value;
    poly_float delay_time = utils::max(input(kDelay)->at(0), 0.0f);
    poly_mask has_delay_mask = poly_float::notEqual(delay_time, 0.0f);
    poly_mask note_on_mask = poly_float::equal(trigger_value, kVoiceOn);
    trigger_value = utils::maskLoad(trigger_value, kVoiceIdle, has_delay_mask & note_on_mask);
    
    poly_state_ = utils::maskLoad(poly_state_, trigger_value, trigger_mask);
    position_ = utils::maskLoad(position_, 0.0f, trigger_mask);

    poly_int triggered_remaining = poly_int(num_samples) - input(kTrigger)->source->trigger_offset;
    poly_int remaining_samples = utils::maskLoad(num_samples, triggered_remaining, trigger_mask);
    start_value_ = utils::maskLoad(start_value_, value_, trigger_mask);

    poly_mask delay_mask = poly_float::equal(poly_state_, kVoiceIdle);
    poly_mask attack_mask = poly_float::equal(poly_state_, kVoiceOn);
    poly_mask hold_mask = poly_float::equal(poly_state_, kVoiceHold);
    poly_mask decay_mask = poly_float::equal(poly_state_, kVoiceDecay);
    poly_mask release_mask = poly_float::equal(poly_state_, kVoiceOff);
    poly_mask kill_mask = poly_float::equal(poly_state_, kVoiceKill);

    poly_float delta_time = utils::toFloat(remaining_samples) * (1.0f / getSampleRate());

    poly_float delta_delay = delta_time / utils::max(delay_time, 0.0000001f);
    position_ += delta_delay & delay_mask;

    poly_float attack_time = utils::max(input(kAttack)->at(0), 0.000000001f);
    poly_float delta_attack = delta_time / attack_time;
    position_ += delta_attack & attack_mask;

    poly_float hold_time = utils::max(input(kHold)->at(0), 0.0f);
    poly_mask has_hold_mask = poly_float::notEqual(hold_time, 0.0f);
    poly_float delta_hold = delta_time / utils::max(hold_time, 0.0000001f);
    position_ += delta_hold & hold_mask;

    poly_float decay_time = utils::max(input(kDecay)->at(0), 0.000000001f);
    poly_float delta_decay = delta_time / decay_time;
    position_ += delta_decay & decay_mask;

    poly_float release_time = utils::max(input(kRelease)->at(0), 0.000000001f);
    poly_float delta_release = delta_time / release_time;
    position_ += delta_release & release_mask;

    poly_float delta_kill = delta_time * (1.0f / kVoiceKillTime);
    position_ += delta_kill & kill_mask;
    position_ = utils::clamp(position_, 0.0f, 1.0f);

    poly_float power = (-input(kAttackPower)->at(0) & attack_mask) +
                       (input(kDecayPower)->at(0) & decay_mask) +
                       (input(kReleasePower)->at(0) & release_mask);

    poly_float attack_value = futils::powerScale(position_, power);

    poly_float sustain = input(kSustain)->at(0);
    poly_float decay_value = poly_float(1.0f) - (poly_float(1.0f) - sustain) * attack_value;
    poly_float release_value = start_value_ * (poly_float(1.0f) - attack_value);
    poly_float kill_value = start_value_ * (poly_float(1.0f) - attack_value);

    value_ = (attack_value & attack_mask) +
             (poly_float(1.0f) & hold_mask) +
             (decay_value & decay_mask) +
             (release_value & release_mask) +
             (kill_value & kill_mask);

    value_ = utils::clamp(value_, 0.0f, 1.0f);
    output(kValue)->trigger_value = value_;
    output(kValue)->buffer[0] = value_;
    output(kPhase)->buffer[0] = poly_state_ + position_;

    poly_mask attack_transition_mask = delay_mask & poly_float::equal(position_, 1.0f);
    poly_mask hold_transition_mask = attack_mask & poly_float::equal(position_, 1.0f) & has_hold_mask;
    poly_mask decay_turn_mask = (attack_mask & ~has_hold_mask) | hold_mask;
    poly_mask decay_transition_mask = decay_turn_mask & poly_float::equal(position_, 1.0f);
    poly_state_ = utils::maskLoad(poly_state_, kVoiceOn, attack_transition_mask);
    poly_state_ = utils::maskLoad(poly_state_, kVoiceHold, hold_transition_mask);
    poly_state_ = utils::maskLoad(poly_state_, kVoiceDecay, decay_transition_mask);

    poly_mask transition_mask = attack_transition_mask | hold_transition_mask | decay_transition_mask;
    position_ = position_ & ~transition_mask;

    poly_mask dead_transition_mask = release_mask & poly_float::equal(position_, 1.0f);
    poly_state_ = utils::maskLoad(poly_state_, kVoiceKill, dead_transition_mask);
  }

  poly_float Envelope::processSection(poly_float* audio_out, int from, int to,
                                      poly_float power, poly_float delta_power,
                                      poly_float position, poly_float delta_position,
                                      poly_float start, poly_float end, poly_float delta_end) {
    int num_samples = to - from;
    poly_float power_change = delta_power * num_samples;
    poly_float position_change = delta_position * num_samples;

    poly_float current_power = power;
    poly_float current_position = position;
    poly_float current_end = end;
    for (int i = from; i < to; ++i) {
      poly_float t = futils::powerScale(current_position, current_power);
      audio_out[i] = utils::interpolate(start, current_end, t);

      current_power += delta_power;
      current_position = utils::clamp(current_position + delta_position, 0.0f, 1.0f);
      current_end += delta_end;
    }

    return utils::clamp(position + delta_position * num_samples, 0.0f, 1.0f);
  }

  void Envelope::processAudioRate(int num_samples) {
    poly_float delta_time = 1.0f / getSampleRate();
    mono_float delta_sample = 1.0f / num_samples;

    poly_float sustain_end = utils::clamp(input(kSustain)->at(0), 0.0f, 1.0f);
    
    poly_float delay_time = utils::max(input(kDelay)->at(0), 0.0f);
    poly_float delta_delay = delta_time / utils::max(delay_time, 0.0000001f);
    
    poly_float attack_time = utils::max(input(kAttack)->at(0), 0.000000001f);
    poly_float delta_attack = delta_time / attack_time;
    poly_float attack_power_end = -input(kAttackPower)->at(0);

    poly_float hold_time = utils::max(input(kHold)->at(0), 0.0f);
    poly_mask has_hold_mask = poly_float::notEqual(hold_time, 0.0f);
    poly_float delta_hold = delta_time / utils::max(hold_time, 0.0000001f);

    poly_float decay_time = utils::max(input(kDecay)->at(0), 0.000000001f);
    poly_float delta_decay = delta_time / decay_time;
    poly_float decay_power_end = input(kDecayPower)->at(0);

    poly_float release_time = utils::max(input(kRelease)->at(0), 0.000000001f);
    poly_float delta_release = delta_time / release_time;
    poly_float release_power_end = input(kReleasePower)->at(0);

    poly_float delta_kill = delta_time * (1.0f / kVoiceKillTime);

    poly_mask trigger_mask = input(kTrigger)->source->trigger_mask;
    poly_float trigger_value = input(kTrigger)->source->trigger_value;
    poly_mask has_delay_mask = poly_float::notEqual(delay_time, 0.0f);
    poly_mask note_on_mask = poly_float::equal(trigger_value, kVoiceOn);
    trigger_value = utils::maskLoad(trigger_value, kVoiceIdle, has_delay_mask & note_on_mask);
    
    poly_int triggered_position = input(kTrigger)->source->trigger_offset;
    triggered_position = utils::maskLoad(num_samples, triggered_position, trigger_mask);

    poly_float current_position = position_;
    poly_float* audio_out = output(kValue)->buffer;

    for (int i = 0; i < num_samples;) {
      poly_mask triggering = trigger_mask & poly_int::equal(i, triggered_position);
      triggered_position = utils::maskLoad(triggered_position, num_samples, triggering);
      poly_state_ = utils::maskLoad(poly_state_, trigger_value, triggering);
      current_position = utils::maskLoad(current_position, 0.0f, triggering);

      poly_mask dying = poly_float::equal(trigger_value, kVoiceOff) | poly_float::equal(trigger_value, kVoiceKill);
      start_value_ = utils::maskLoad(start_value_, value_, triggering);
      attack_power_ = utils::maskLoad(attack_power_, attack_power_end, triggering);
      decay_power_ = utils::maskLoad(decay_power_, decay_power_end, triggering);
      release_power_ = utils::maskLoad(release_power_, release_power_end, triggering);
      sustain_ = utils::maskLoad(sustain_, sustain_end, triggering);

      poly_mask delay_mask = poly_float::equal(poly_state_, kVoiceIdle);
      poly_mask attack_mask = poly_float::equal(poly_state_, kVoiceOn);
      poly_mask hold_mask = poly_float::equal(poly_state_, kVoiceHold);
      poly_mask decay_mask = poly_float::equal(poly_state_, kVoiceDecay);
      poly_mask release_mask = poly_float::equal(poly_state_, kVoiceOff);
      poly_mask kill_mask = poly_float::equal(poly_state_, kVoiceKill);

      poly_float delta_position = (delta_delay & delay_mask) + (delta_attack & attack_mask) +
                                  (delta_hold & hold_mask) + (delta_decay & decay_mask) +
                                  (delta_release & release_mask) + (delta_kill & kill_mask);

      poly_float from_power = (attack_power_ & attack_mask) +
                              (decay_power_ & decay_mask) +
                              (release_power_ & release_mask);
      poly_float to_power = (attack_power_end & attack_mask) +
                            (decay_power_end & decay_mask) +
                            (release_power_end & release_mask);

      poly_float power = utils::interpolate(from_power, to_power, i / (1.0f * num_samples));
      poly_float delta_power = (to_power - from_power) * delta_sample;

      poly_float cycles_remaining = (utils::ceil(current_position) - current_position) / delta_position;
      poly_float end_cycle = utils::maskLoad(num_samples, cycles_remaining + i, attack_mask);
      end_cycle = utils::min(end_cycle, utils::toFloat(triggered_position));
      int last_cycle = std::max((int)utils::minFloat(end_cycle), i + 1);

      poly_float current_sustain = utils::interpolate(sustain_, sustain_end, i / (1.0f * num_samples));
      poly_float start = utils::maskLoad(start_value_, 1.0f, decay_mask | hold_mask);
      poly_float end = (poly_float(1.0f) & (attack_mask | hold_mask)) + (current_sustain & decay_mask);
      poly_float delta_end = ((sustain_end - sustain_) * delta_sample) & decay_mask;

      current_position = processSection(audio_out, i, last_cycle, power, delta_power,
                                        current_position, delta_position, start, end, delta_end);
      i = last_cycle;

      value_ = audio_out[i - 1];
      
      poly_mask attack_transition_mask = delay_mask & poly_float::equal(current_position, 1.0f);      
      poly_mask hold_transition_mask = attack_mask & poly_float::equal(current_position, 1.0f) & has_hold_mask;
      poly_mask decay_turn_mask = (attack_mask & ~has_hold_mask) | hold_mask;
      poly_mask decay_transition_mask = decay_turn_mask & poly_float::equal(current_position, 1.0f);

      poly_state_ = utils::maskLoad(poly_state_, kVoiceOn, attack_transition_mask);
      poly_state_ = utils::maskLoad(poly_state_, kVoiceHold, hold_transition_mask);
      poly_state_ = utils::maskLoad(poly_state_, kVoiceDecay, decay_transition_mask);

      poly_mask transition_mask = attack_transition_mask | hold_transition_mask | decay_transition_mask;
      current_position = current_position & ~transition_mask;

      poly_mask dead_transition_mask = release_mask & poly_float::equal(current_position, 1.0f);
      poly_state_ = utils::maskLoad(poly_state_, kVoiceKill, dead_transition_mask);
    }

    output(kValue)->trigger_value = audio_out[0];

    position_ = current_position;
    attack_power_ = attack_power_end;
    decay_power_ = decay_power_end;
    release_power_ = release_power_end;
    sustain_ = sustain_end;
    output(kPhase)->buffer[0] = poly_state_ + position_;
  }
} // namespace vital
