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

#include "chorus_module.h"

#include "delay.h"
#include "memory.h"
#include "synth_constants.h"

namespace vital {

  ChorusModule::ChorusModule(const Output* beats_per_second) :
      SynthModule(0, 1), beats_per_second_(beats_per_second),
      frequency_(nullptr), delay_time_1_(nullptr), delay_time_2_(nullptr),
      mod_depth_(nullptr), phase_(0.0f) {
    wet_ = 0.0f;
    dry_ = 0.0f;
    last_num_voices_ = 0;
    int max_samples = kMaxChorusDelay * kMaxSampleRate + 1;
        
    for (int i = 0; i < kMaxDelayPairs; ++i) {
      registerOutput(&delay_status_outputs_[i]);
      delays_[i] = new MultiDelay(max_samples);
      addIdleProcessor(delays_[i]);
    }
  }

  void ChorusModule::init() {
    static const cr::Value kDelayStyle(MultiDelay::kMono);

    voices_ = createBaseControl("chorus_voices");

    Output* free_frequency = createMonoModControl("chorus_frequency");
    frequency_ = createTempoSyncSwitch("chorus", free_frequency->owner, beats_per_second_, false);
    Output* feedback = createMonoModControl("chorus_feedback");
    wet_output_ = createMonoModControl("chorus_dry_wet");
    Output* cutoff = createMonoModControl("chorus_cutoff");
    Output* spread = createMonoModControl("chorus_spread");
    mod_depth_ = createMonoModControl("chorus_mod_depth");

    delay_time_1_ = createMonoModControl("chorus_delay_1");
    delay_time_2_ = createMonoModControl("chorus_delay_2");

    for (int i = 0; i < kMaxDelayPairs; ++i) {
      delays_[i]->plug(&delay_frequencies_[i], MultiDelay::kFrequency);
      delays_[i]->plug(feedback, MultiDelay::kFeedback);
      delays_[i]->plug(&constants::kValueOne, MultiDelay::kWet);
      delays_[i]->plug(cutoff, MultiDelay::kFilterCutoff);
      delays_[i]->plug(spread, MultiDelay::kFilterSpread);
      delays_[i]->plug(&kDelayStyle, MultiDelay::kStyle);
    }

    SynthModule::init();
  }

  void ChorusModule::enable(bool enable) {
    SynthModule::enable(enable);
    process(1);
    if (enable) {
      wet_ = 0.0f;
      dry_ = 0.0f;

      for (int i = 0; i < kMaxDelayPairs; ++i)
        delays_[i]->hardReset();
    }
  }

  int ChorusModule::getNextNumVoicePairs() {
    int num_voice_pairs = voices_->value();

    for (int i = last_num_voices_; i < num_voice_pairs; ++i)
      delays_[i]->reset(constants::kFullMask);

    last_num_voices_ = num_voice_pairs;
    return num_voice_pairs;
  }


  void ChorusModule::processWithInput(const poly_float* audio_in, int num_samples) {
    SynthModule::process(num_samples);
    poly_float frequency = frequency_->buffer[0];
    poly_float delta_phase = (frequency * num_samples) * (1.0f / getSampleRate());
    phase_ = utils::mod(phase_ + delta_phase);

    poly_float* audio_out = output()->buffer;
    for (int s = 0; s < num_samples; ++s) {
      poly_float sample = audio_in[s] & constants::kFirstMask;
      audio_out[s] = sample + utils::swapVoices(sample);
    }

    int num_voices = getNextNumVoicePairs();

    poly_float delay1 = delay_time_1_->buffer[0];
    poly_float delay2 = delay_time_2_->buffer[0];
    poly_float delay_time = utils::maskLoad(delay2, delay1, constants::kFirstMask);
    poly_float average_delay = (delay_time + utils::swapVoices(delay_time)) * 0.5f;
    for (int i = 0; i < num_voices; ++i) {
      float pair_offset = i * 0.25f / num_voices;
      poly_float right_offset = (poly_float(0.25f) & constants::kRightMask);
      poly_float phase = phase_ + right_offset + (poly_float(0.5f) & ~constants::kFirstMask) + pair_offset;

      poly_float mod_depth = mod_depth_->buffer[0] * kMaxChorusModulation;
      poly_float mod = utils::sin(phase * vital::kPi * 2.0f) * 0.5f + 1.0f;
      float delay_t = 0.0f;
      if (i > 0)
        delay_t = i / (num_voices - 1.0f);
      poly_float delay = mod * mod_depth + utils::interpolate(delay_time, average_delay, delay_t);

      vital::poly_float delay_frequency = poly_float(1.0f) / utils::max(0.00001f, delay);
      delay_frequencies_[i].set(delay_frequency);
      delays_[i]->processWithInput(audio_out, num_samples);
      
      delay_status_outputs_[i].buffer[0] = delay_frequency;
    }

    poly_float current_wet = wet_;
    poly_float current_dry = dry_;

    poly_float wet_value = utils::clamp(wet_output_->buffer[0], 0.0f, 1.0f);
    wet_ = futils::equalPowerFade(wet_value);
    dry_ = futils::equalPowerFadeInverse(wet_value);

    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;

    utils::zeroBuffer(audio_out, num_samples);

    for (int i = 0; i < num_voices; ++i) {
      const poly_float* delay_out = delays_[i]->output()->buffer;

      for (int s = 0; s < num_samples; ++s) {
        poly_float sample_out = delay_out[s] * 0.5f;
        audio_out[s] += sample_out + utils::swapVoices(sample_out);
      }
    }

    for (int s = 0; s < num_samples; ++s) {
      current_dry += delta_dry;
      current_wet += delta_wet;
      audio_out[s] = current_dry * audio_in[s] + current_wet * audio_out[s];
    }
  }

  void ChorusModule::correctToTime(double seconds) {
    phase_ = utils::getCycleOffsetFromSeconds(seconds, frequency_->buffer[0]);
  }
} // namespace vital
