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

#include "reverb.h"

#include "futils.h"
#include "memory.h"
#include "synth_constants.h"

namespace vital {
  static constexpr float kMaxChorusDrift = 2500.0f;
  static constexpr float kMinDecayTime = 0.1f;
  static constexpr float kMaxDecayTime = 100.0f;
  static constexpr float kMaxChorusFrequency = 16.0f;
  static constexpr float kChorusShiftAmount = 0.9f;
  static constexpr float kSampleDelayMultiplier = 0.05f;
  static constexpr float kSampleIncrementMultiplier = 0.05f;

  const poly_int Reverb::kAllpassDelays[kNetworkContainers] = {
    { 1001, 799, 933, 876 },
    { 895, 807, 907, 853 },
    { 957, 1019, 711, 567 },
    { 833, 779, 663, 997 }
  };

  const poly_float Reverb::kFeedbackDelays[kNetworkContainers] = {
    { 6753.2f, 9278.4f, 7704.5f, 11328.5f },
    { 9701.12f, 5512.5f, 8480.45f, 5638.65f },
    { 3120.73f, 3429.5f, 3626.37f, 7713.52f },
    { 4521.54f, 6518.97f, 5265.56f, 5630.25f }
  };

  Reverb::Reverb() : Processor(kNumInputs, 1), chorus_phase_(0.0f), chorus_amount_(0.0f), feedback_(0.0f),
                     damping_(0.0f), dry_(0.0f), wet_(0.0f), write_index_(0),
                     max_allpass_size_(0), max_feedback_size_(0),
                     feedback_mask_(0), allpass_mask_(0), poly_allpass_mask_(0) {
    setupBuffersForSampleRate(kDefaultSampleRate);

    memory_ = std::make_unique<StereoMemory>(kMaxSampleRate);

    for (int i = 0; i < kNetworkContainers; ++i)
      decays_[i] = 0.0f;

    low_pre_coefficient_ = 0.1f;
    high_pre_coefficient_ = 0.1f;
    low_coefficient_ = 0.1f;
    high_coefficient_ = 0.1f;
    low_amplitude_ = 0.0f;
    high_amplitude_ = 0.0f;
    sample_delay_ = kMinDelay;
  }

  void Reverb::setupBuffersForSampleRate(int sample_rate) {
    int buffer_scale = getBufferScale(sample_rate);
    int max_feedback_size = buffer_scale * (1 << (kBaseFeedbackBits + kMaxSizePower));
    if (max_feedback_size_ == max_feedback_size)
      return;

    max_feedback_size_ = max_feedback_size;
    feedback_mask_ = max_feedback_size_ - 1;
    for (int i = 0; i < kNetworkSize; ++i) {
      feedback_memories_[i] = std::make_unique<mono_float[]>(max_feedback_size_ + kExtraLookupSample);
      feedback_lookups_[i] = feedback_memories_[i].get() + 1;
    }

    max_allpass_size_ = buffer_scale * (1 << kBaseAllpassBits);
    poly_allpass_mask_ = max_allpass_size_ - 1;
    allpass_mask_ = max_allpass_size_ * poly_float::kSize - 1;
    for (int i = 0; i < kNetworkContainers; ++i)
      allpass_lookups_[i] = std::make_unique<poly_float[]>(max_allpass_size_);

    write_index_ &= feedback_mask_;
  }

  void Reverb::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  void Reverb::processWithInput(const poly_float* audio_in, int num_samples) {
    for (int i = 0; i < kNetworkSize; ++i)
      wrapFeedbackBuffer(feedback_memories_[i].get());

    poly_float* audio_out = output()->buffer;
    mono_float tick_increment = 1.0f / num_samples;

    poly_float current_dry = dry_;
    poly_float current_wet = wet_;
    poly_float current_low_pre_coefficient = low_pre_coefficient_;
    poly_float current_high_pre_coefficient = high_pre_coefficient_;
    poly_float current_low_coefficient = low_coefficient_;
    poly_float current_low_amplitude = low_amplitude_;
    poly_float current_high_coefficient = high_coefficient_;
    poly_float current_high_amplitude = high_amplitude_;

    poly_float wet_in = utils::clamp(input(kWet)->at(0), 0.0f, 1.0f);
    wet_ = futils::equalPowerFade(wet_in);
    dry_ = futils::equalPowerFadeInverse(wet_in);
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;

    int sample_rate = getSampleRate();
    int buffer_scale = getBufferScale(sample_rate);
    float sample_rate_ratio = getSampleRateRatio(sample_rate);
    poly_float low_pre_cutoff_midi = utils::clamp(input(kPreLowCutoff)->at(0), 0.0f, 130.0f);
    poly_float low_pre_cutoff_frequency = utils::midiNoteToFrequency(low_pre_cutoff_midi);
    low_pre_coefficient_ = OnePoleFilter<>::computeCoefficient(low_pre_cutoff_frequency, sample_rate);

    poly_float high_pre_cutoff_midi = utils::clamp(input(kPreHighCutoff)->at(0), 0.0f, 130.0f);
    poly_float high_pre_cutoff_frequency = utils::midiNoteToFrequency(high_pre_cutoff_midi);
    high_pre_coefficient_ = OnePoleFilter<>::computeCoefficient(high_pre_cutoff_frequency, sample_rate);
    poly_float delta_low_pre_coefficient = (low_pre_coefficient_ - current_low_pre_coefficient) * tick_increment;
    poly_float delta_high_pre_coefficient = (high_pre_coefficient_ - current_high_pre_coefficient) * tick_increment;

    poly_float low_cutoff_midi = utils::clamp(input(kLowCutoff)->at(0), 0.0f, 130.0f);
    poly_float low_cutoff_frequency = utils::midiNoteToFrequency(low_cutoff_midi);
    low_coefficient_ = OnePoleFilter<>::computeCoefficient(low_cutoff_frequency, sample_rate);

    poly_float high_cutoff_midi = utils::clamp(input(kHighCutoff)->at(0), 0.0f, 130.0f);
    poly_float high_cutoff_frequency = utils::midiNoteToFrequency(high_cutoff_midi);
    high_coefficient_ = OnePoleFilter<>::computeCoefficient(high_cutoff_frequency, sample_rate);
    poly_float delta_low_coefficient = (low_coefficient_ - current_low_coefficient) * tick_increment;
    poly_float delta_high_coefficient = (high_coefficient_ - current_high_coefficient) * tick_increment;

    poly_float low_gain = utils::clamp(input(kLowGain)->at(0), -24.0f, 0.0f);
    low_amplitude_ = poly_float(1.0f) - utils::dbToMagnitude(low_gain);
    poly_float high_gain = utils::clamp(input(kHighGain)->at(0), -24.0f, 0.0f);
    high_amplitude_ = utils::dbToMagnitude(high_gain);
    poly_float delta_low_amplitude = (low_amplitude_ - current_low_amplitude) * tick_increment;
    poly_float delta_high_amplitude = (high_amplitude_ - current_high_amplitude) * tick_increment;

    const mono_float* allpass_lookup1 = (mono_float*)allpass_lookups_[0].get();
    const mono_float* allpass_lookup2 = (mono_float*)allpass_lookups_[1].get();
    const mono_float* allpass_lookup3 = (mono_float*)allpass_lookups_[2].get();
    const mono_float* allpass_lookup4 = (mono_float*)allpass_lookups_[3].get();

    mono_float* feedback_lookups1[] = { feedback_lookups_[0], feedback_lookups_[1],
                                        feedback_lookups_[2], feedback_lookups_[3] };
    mono_float* feedback_lookups2[] = { feedback_lookups_[4], feedback_lookups_[5],
                                        feedback_lookups_[6], feedback_lookups_[7] };
    mono_float* feedback_lookups3[] = { feedback_lookups_[8], feedback_lookups_[9],
                                        feedback_lookups_[10], feedback_lookups_[11] };
    mono_float* feedback_lookups4[] = { feedback_lookups_[12], feedback_lookups_[13],
                                        feedback_lookups_[14], feedback_lookups_[15] };

    poly_float size = utils::clamp(input(kSize)->at(0), 0.0f, 1.0f);
    poly_float size_mult = futils::pow(2.0f, size * kSizePowerRange + kMinSizePower);

    poly_float decay_samples = utils::clamp(input(kDecayTime)->at(0), kMinDecayTime, kMaxDecayTime) * kBaseSampleRate;
    poly_float decay_period = size_mult / decay_samples;
    poly_float current_decay1 = decays_[0];
    poly_float current_decay2 = decays_[1];
    poly_float current_decay3 = decays_[2];
    poly_float current_decay4 = decays_[3];
    decays_[0] = utils::pow(kT60Amplitude, kFeedbackDelays[0] * decay_period);
    decays_[1] = utils::pow(kT60Amplitude, kFeedbackDelays[1] * decay_period);
    decays_[2] = utils::pow(kT60Amplitude, kFeedbackDelays[2] * decay_period);
    decays_[3] = utils::pow(kT60Amplitude, kFeedbackDelays[3] * decay_period);
    poly_float delta_decay1 = (decays_[0] - current_decay1) * tick_increment;
    poly_float delta_decay2 = (decays_[1] - current_decay2) * tick_increment;
    poly_float delta_decay3 = (decays_[2] - current_decay3) * tick_increment;
    poly_float delta_decay4 = (decays_[3] - current_decay4) * tick_increment;

    poly_int delay_offset(0, -1, -2, -3);
    if (buffer_scale)
      delay_offset += poly_float::kSize;

    poly_int allpass_offset1 = utils::swapStereo(kAllpassDelays[0] * buffer_scale * poly_float::kSize + delay_offset);
    poly_int allpass_offset2 = utils::swapStereo(kAllpassDelays[1] * buffer_scale * poly_float::kSize + delay_offset);
    poly_int allpass_offset3 = utils::swapStereo(kAllpassDelays[2] * buffer_scale * poly_float::kSize + delay_offset);
    poly_int allpass_offset4 = utils::swapStereo(kAllpassDelays[3] * buffer_scale * poly_float::kSize + delay_offset);

    mono_float chorus_frequency = utils::clamp(input(kChorusFrequency)->at(0)[0], 0.0f, kMaxChorusFrequency);
    mono_float chorus_phase_increment = chorus_frequency / sample_rate;

    mono_float network_offset = 2.0f * kPi / kNetworkSize;
    poly_float phase_offset = poly_float(0.0f, 1.0f, 2.0f, 3.0f) * network_offset;
    poly_float container_phase = phase_offset + chorus_phase_ * 2.0f * kPi;
    chorus_phase_ += num_samples * chorus_phase_increment;
    chorus_phase_ -= std::floor(chorus_phase_);

    poly_float chorus_increment_real = utils::cos(chorus_phase_increment * (2.0f * kPi));
    poly_float chorus_increment_imaginary = utils::sin(chorus_phase_increment * (2.0f * kPi));
    poly_float current_chorus_real = utils::cos(container_phase);
    poly_float current_chorus_imaginary = utils::sin(container_phase);

    poly_float delay1 = size_mult * kFeedbackDelays[0] * sample_rate_ratio;
    poly_float delay2 = size_mult * kFeedbackDelays[1] * sample_rate_ratio;
    poly_float delay3 = size_mult * kFeedbackDelays[2] * sample_rate_ratio;
    poly_float delay4 = size_mult * kFeedbackDelays[3] * sample_rate_ratio;

    poly_float current_chorus_amount = chorus_amount_;
    chorus_amount_ = utils::clamp(input(kChorusAmount)->at(0)[0], 0.0f, 1.0f) * kMaxChorusDrift * sample_rate_ratio;
    chorus_amount_ = utils::min(chorus_amount_, delay1 - 8 * poly_float::kSize);
    chorus_amount_ = utils::min(chorus_amount_, delay2 - 8 * poly_float::kSize);
    chorus_amount_ = utils::min(chorus_amount_, delay3 - 8 * poly_float::kSize);
    chorus_amount_ = utils::min(chorus_amount_, delay4 - 8 * poly_float::kSize);
    poly_float delta_chorus_amount = (chorus_amount_ - current_chorus_amount) * tick_increment;
    current_chorus_amount = current_chorus_amount * size_mult;

    poly_float current_sample_delay = sample_delay_;
    poly_float current_delay_increment = sample_delay_increment_;
    poly_float end_target = current_sample_delay + current_delay_increment * num_samples;
    poly_float target_delay = utils::clamp(input(kDelay)->at(0) * getSampleRate(), kMinDelay, kMaxSampleRate);
    target_delay = utils::interpolate(sample_delay_, target_delay, kSampleDelayMultiplier);
    poly_float makeup_delay = target_delay - end_target;
    poly_float delta_delay_increment = makeup_delay / (0.5f * num_samples * num_samples) * kSampleIncrementMultiplier;

    for (int i = 0; i < num_samples; ++i) {
      current_chorus_amount += delta_chorus_amount;
      current_chorus_real = current_chorus_real * chorus_increment_real -
                            current_chorus_imaginary * chorus_increment_imaginary;
      current_chorus_imaginary = current_chorus_imaginary * chorus_increment_real +
                                 current_chorus_real * chorus_increment_imaginary;
      poly_float feedback_offset1 = delay1 + current_chorus_real * current_chorus_amount;
      poly_float feedback_offset2 = delay2 - current_chorus_real * current_chorus_amount;
      poly_float feedback_offset3 = delay3 + current_chorus_imaginary * current_chorus_amount;
      poly_float feedback_offset4 = delay4 - current_chorus_imaginary * current_chorus_amount;
      poly_float feedback_read1 = readFeedback(feedback_lookups1, feedback_offset1);
      poly_float feedback_read2 = readFeedback(feedback_lookups2, feedback_offset2);
      poly_float feedback_read3 = readFeedback(feedback_lookups3, feedback_offset3);
      poly_float feedback_read4 = readFeedback(feedback_lookups4, feedback_offset4);

      poly_float input = audio_in[i] & constants::kFirstMask;
      input += utils::swapVoices(input);
      poly_float filtered_input = high_pre_filter_.tickBasic(input, current_high_pre_coefficient);
      filtered_input = low_pre_filter_.tickBasic(input, current_low_pre_coefficient) - filtered_input;
      poly_float scaled_input = filtered_input * 0.25f;

      poly_float allpass_read1 = readAllpass(allpass_lookup1, allpass_offset1);
      poly_float allpass_read2 = readAllpass(allpass_lookup2, allpass_offset2);
      poly_float allpass_read3 = readAllpass(allpass_lookup3, allpass_offset3);
      poly_float allpass_read4 = readAllpass(allpass_lookup4, allpass_offset4);
      
      poly_float allpass_delay_input1 = feedback_read1 - allpass_read1 * kAllpassFeedback;
      poly_float allpass_delay_input2 = feedback_read2 - allpass_read2 * kAllpassFeedback;
      poly_float allpass_delay_input3 = feedback_read3 - allpass_read3 * kAllpassFeedback;
      poly_float allpass_delay_input4 = feedback_read4 - allpass_read4 * kAllpassFeedback;

      int allpass_write_index = write_index_ & poly_allpass_mask_;
      allpass_lookups_[0][allpass_write_index] = scaled_input + allpass_delay_input1;
      allpass_lookups_[1][allpass_write_index] = scaled_input + allpass_delay_input2;
      allpass_lookups_[2][allpass_write_index] = scaled_input + allpass_delay_input3;
      allpass_lookups_[3][allpass_write_index] = scaled_input + allpass_delay_input4;

      poly_float allpass_output1 = allpass_read1 + allpass_delay_input1 * kAllpassFeedback;
      poly_float allpass_output2 = allpass_read2 + allpass_delay_input2 * kAllpassFeedback;
      poly_float allpass_output3 = allpass_read3 + allpass_delay_input3 * kAllpassFeedback;
      poly_float allpass_output4 = allpass_read4 + allpass_delay_input4 * kAllpassFeedback;

      poly_float total_rows = allpass_output1 + allpass_output2 + allpass_output3 + allpass_output4;
      poly_float other_feedback = poly_float::mulAdd(total_rows.sum() * 0.25f, total_rows, -0.5f);

      poly_float write1 = other_feedback + allpass_output1;
      poly_float write2 = other_feedback + allpass_output2;
      poly_float write3 = other_feedback + allpass_output3;
      poly_float write4 = other_feedback + allpass_output4;

      poly_float::transpose(allpass_output1.value, allpass_output2.value,
                            allpass_output3.value, allpass_output4.value);
      poly_float adjacent_feedback = (allpass_output1 + allpass_output2 + allpass_output3 + allpass_output4) * -0.5f;

      write1 += adjacent_feedback[0];
      write2 += adjacent_feedback[1];
      write3 += adjacent_feedback[2];
      write4 += adjacent_feedback[3];
      poly_float high_filtered1 = high_shelf_filters_[0].tickBasic(write1, current_high_coefficient);
      poly_float high_filtered2 = high_shelf_filters_[1].tickBasic(write2, current_high_coefficient);
      poly_float high_filtered3 = high_shelf_filters_[2].tickBasic(write3, current_high_coefficient);
      poly_float high_filtered4 = high_shelf_filters_[3].tickBasic(write4, current_high_coefficient);
      write1 = high_filtered1 + current_high_amplitude * (write1 - high_filtered1);
      write2 = high_filtered2 + current_high_amplitude * (write2 - high_filtered2);
      write3 = high_filtered3 + current_high_amplitude * (write3 - high_filtered3);
      write4 = high_filtered4 + current_high_amplitude * (write4 - high_filtered4);

      poly_float low_filtered1 = low_shelf_filters_[0].tickBasic(write1, current_low_coefficient);
      poly_float low_filtered2 = low_shelf_filters_[1].tickBasic(write2, current_low_coefficient);
      poly_float low_filtered3 = low_shelf_filters_[2].tickBasic(write3, current_low_coefficient);
      poly_float low_filtered4 = low_shelf_filters_[3].tickBasic(write4, current_low_coefficient);
      write1 -= low_filtered1 * current_low_amplitude;
      write2 -= low_filtered2 * current_low_amplitude;
      write3 -= low_filtered3 * current_low_amplitude;
      write4 -= low_filtered4 * current_low_amplitude;

      current_decay1 += delta_decay1;
      current_decay2 += delta_decay2;
      current_decay3 += delta_decay3;
      current_decay4 += delta_decay4;
      poly_float store1 = current_decay1 * write1;
      poly_float store2 = current_decay2 * write2;
      poly_float store3 = current_decay3 * write3;
      poly_float store4 = current_decay4 * write4;
      feedback_lookups1[0][write_index_] = store1[0];
      feedback_lookups1[1][write_index_] = store1[1];
      feedback_lookups1[2][write_index_] = store1[2];
      feedback_lookups1[3][write_index_] = store1[3];
      feedback_lookups2[0][write_index_] = store2[0];
      feedback_lookups2[1][write_index_] = store2[1];
      feedback_lookups2[2][write_index_] = store2[2];
      feedback_lookups2[3][write_index_] = store2[3];
      feedback_lookups3[0][write_index_] = store3[0];
      feedback_lookups3[1][write_index_] = store3[1];
      feedback_lookups3[2][write_index_] = store3[2];
      feedback_lookups3[3][write_index_] = store3[3];
      feedback_lookups4[0][write_index_] = store4[0];
      feedback_lookups4[1][write_index_] = store4[1];
      feedback_lookups4[2][write_index_] = store4[2];
      feedback_lookups4[3][write_index_] = store4[3];

      write_index_ = (write_index_ + 1) & feedback_mask_;

      poly_float total_allpass = store1 + store2 + store3 + store4;
      poly_float other_feedback_allpass = poly_float::mulAdd(total_allpass.sum() * 0.25f, total_allpass, -0.5f);

      poly_float feed_forward1 = other_feedback_allpass + store1;
      poly_float feed_forward2 = other_feedback_allpass + store2;
      poly_float feed_forward3 = other_feedback_allpass + store3;
      poly_float feed_forward4 = other_feedback_allpass + store4;

      poly_float::transpose(store1.value, store2.value, store3.value, store4.value);
      poly_float adjacent_feedback_allpass = (store1 + store2 + store3 + store4) * -0.5f;

      feed_forward1 += adjacent_feedback_allpass[0];
      feed_forward2 += adjacent_feedback_allpass[1];
      feed_forward3 += adjacent_feedback_allpass[2];
      feed_forward4 += adjacent_feedback_allpass[3];

      poly_float total = write1 + write2 + write3 + write4;
      total += (feed_forward1 * current_decay1 + feed_forward2 * current_decay2 +
                feed_forward3 * current_decay3 + feed_forward4 * current_decay4) * 0.125f;

      memory_->push(total + utils::swapVoices(total));
      audio_out[i] = current_wet * memory_->get(current_sample_delay) + current_dry * input;

      current_delay_increment += delta_delay_increment;
      current_sample_delay += current_delay_increment;
      current_sample_delay = utils::clamp(current_sample_delay, kMinDelay, kMaxSampleRate);
      current_dry += delta_dry;
      current_wet += delta_wet;
      current_high_coefficient += delta_high_coefficient;
      current_high_amplitude += delta_high_amplitude;
    }

    sample_delay_increment_ = current_delay_increment;
    sample_delay_ = current_sample_delay;
  }

  void Reverb::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    setupBuffersForSampleRate(getSampleRate());
  }

  void Reverb::setOversampleAmount(int oversample_amount) {
    Processor::setOversampleAmount(oversample_amount);
    setupBuffersForSampleRate(getSampleRate());
  }

  void Reverb::hardReset() {
    wet_ = 0.0f;
    dry_ = 0.0f;
    low_pre_filter_.reset(constants::kFullMask);
    high_pre_filter_.reset(constants::kFullMask);
    chorus_amount_ = utils::clamp(input(kChorusAmount)->at(0)[0], 0.0f, 1.0f) * kMaxChorusDrift;

    for (int i = 0; i < kNetworkContainers; ++i) {
      low_shelf_filters_[i].reset(constants::kFullMask);
      high_shelf_filters_[i].reset(constants::kFullMask);
      decays_[i] = 0.0f;
    }

    for (int n = 0; n < kNetworkContainers; ++n) {
      for (int i = 0; i < max_allpass_size_; ++i)
        allpass_lookups_[n][i] = 0.0f;
    }

    for (int n = 0; n < kNetworkSize; ++n) {
      for (int i = 0; i < max_feedback_size_ + kExtraLookupSample; ++i)
        feedback_memories_[n][i] = 0.0f;
    }
  }
} // namespace vital
