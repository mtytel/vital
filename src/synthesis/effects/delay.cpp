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

#include "delay.h"

#include "futils.h"
#include "memory.h"
#include "synth_constants.h"
namespace vital {

  namespace {
    force_inline poly_float saturate(poly_float value) {
      return futils::hardTanh(value);
    }

    force_inline poly_float saturateLarge(poly_float value) {
      static constexpr float kRatio = 8.0f;
      static constexpr float kMult = 1.0f / kRatio;
      return futils::hardTanh(value * kMult) * kRatio;
    }
  }

  template<class MemoryType>
  void Delay<MemoryType>::hardReset() {
    memory_->clearAll();

    filter_gain_ = 0.0f;
    low_pass_.reset(constants::kFullMask);
    high_pass_.reset(constants::kFullMask);
  }
  
  template<class MemoryType>
  void Delay<MemoryType>::setMaxSamples(int max_samples) {
    memory_ = std::make_unique<MemoryType>(max_samples);
    period_ = utils::min(period_, max_samples - 1);
  }
  
  template<class MemoryType>
  void Delay<MemoryType>::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  template<class MemoryType>
  void Delay<MemoryType>::processWithInput(const poly_float* audio_in, int num_samples) {
    VITAL_ASSERT(checkInputAndOutputSize(num_samples));

    poly_float current_wet = wet_;
    poly_float current_dry = dry_;
    poly_float current_feedback = feedback_;
    poly_float current_period = period_;
    poly_float current_filter_gain = filter_gain_;
    poly_float current_low_coefficient = low_coefficient_;
    poly_float current_high_coefficient = high_coefficient_;

    poly_float target_frequency = input(kFrequency)->at(0);

    Style style = static_cast<Style>(static_cast<int>(input(kStyle)->at(0)[0]));
    if (style == kStereo || style == kPingPong || style == kMidPingPong)
      target_frequency = utils::maskLoad(target_frequency, input(kFrequencyAux)->at(0), constants::kRightMask);

    poly_float decay = futils::exp_half(num_samples / (kDelayHalfLife * getSampleRate()));
    last_frequency_ = utils::interpolate(target_frequency, last_frequency_, decay);

    poly_float wet = utils::clamp(input(kWet)->at(0), 0.0f, 1.0f);
    wet_ = futils::equalPowerFade(wet);
    dry_ = futils::equalPowerFadeInverse(wet);
    feedback_ = utils::clamp(input(kFeedback)->at(0), -1.0f, 1.0f);

    poly_float samples = poly_float(getSampleRate()) / last_frequency_;
    if (style == kMidPingPong)
      samples += utils::swapStereo(samples) & constants::kLeftMask;
    if (style == kPingPong) {
      current_feedback = utils::maskLoad(current_feedback, 1.0f, constants::kRightMask);
      feedback_ = utils::maskLoad(feedback_, 1.0f, constants::kRightMask);
    }

    period_ = utils::clamp(samples, 3.0f, memory_->getMaxPeriod());
    period_ = utils::interpolate(current_period, period_, 0.5f);

    poly_float filter_cutoff = input(kFilterCutoff)->at(0);
    poly_float filter_radius = getFilterRadius(input(kFilterSpread)->at(0));

    poly_float low_frequency = utils::midiNoteToFrequency(filter_cutoff + filter_radius);
    float min_nyquist = getSampleRate() * kMinNyquistMult;
    low_frequency = utils::clamp(low_frequency, 1.0f, min_nyquist);
    low_coefficient_ = OnePoleFilter<>::computeCoefficient(low_frequency, getSampleRate());

    poly_float high_frequency = utils::midiNoteToFrequency(filter_cutoff - filter_radius);
    high_frequency = utils::clamp(high_frequency, 1.0f, min_nyquist);
    high_coefficient_ = OnePoleFilter<>::computeCoefficient(high_frequency, getSampleRate());

    filter_gain_ = high_frequency / low_frequency + 1.0f;
    poly_float damping = utils::clamp(input(kDamping)->at(0), 0.0f, 1.0f);
    poly_float damping_note = utils::interpolate(kMinDampNote, kMaxDampNote, damping);
    poly_float damping_frequency = utils::midiNoteToFrequency(damping_note);

    switch (style) {
      case kMono:
      case kStereo:
        process(audio_in, num_samples, current_period, current_feedback, current_filter_gain,
                current_low_coefficient, current_high_coefficient, current_wet, current_dry);
        break;
      case kPingPong:
        processMonoPingPong(audio_in, num_samples, current_period, current_feedback, current_filter_gain,
                            current_low_coefficient, current_high_coefficient, current_wet, current_dry);
        break;
      case kMidPingPong:
        processPingPong(audio_in, num_samples, current_period, current_feedback, current_filter_gain,
                        current_low_coefficient, current_high_coefficient, current_wet, current_dry);
        break;
      case kClampedDampened:
        damping_frequency = utils::clamp(damping_frequency, 1.0f, min_nyquist);
        low_coefficient_ = OnePoleFilter<>::computeCoefficient(damping_frequency, getSampleRate());
        processDamped(audio_in, num_samples, current_period, current_feedback,
                      current_low_coefficient, current_wet, current_dry);
        break;
      case kUnclampedUnfiltered:
        processCleanUnfiltered(audio_in, num_samples, current_period, current_feedback, current_wet, current_dry);
        break;
      default:
        processUnfiltered(audio_in, num_samples, current_period, current_feedback, current_wet, current_dry);
        break;
    }
  }

  template<class MemoryType>
  void Delay<MemoryType>::processCleanUnfiltered(const poly_float* audio_in, int num_samples,
                                                 poly_float current_period, poly_float current_feedback,
                                                 poly_float current_wet, poly_float current_dry) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;
    poly_float delta_feedback = (feedback_ - current_feedback) * tick_increment;
    poly_float delta_period = (period_ - current_period) * tick_increment;

    poly_float* dest = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      current_feedback += delta_feedback;
      current_wet += delta_wet;
      current_dry += delta_dry;

      dest[i] = tickCleanUnfiltered(audio_in[i], current_period, current_feedback, current_wet, current_dry);
      current_period += delta_period;
    }
  }

  template<class MemoryType>
  void Delay<MemoryType>::processUnfiltered(const poly_float* audio_in, int num_samples,
                                            poly_float current_period, poly_float current_feedback,
                                            poly_float current_wet, poly_float current_dry) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;
    poly_float delta_feedback = (feedback_ - current_feedback) * tick_increment;
    poly_float delta_period = (period_ - current_period) * tick_increment;

    poly_float* dest = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      current_feedback += delta_feedback;
      current_wet += delta_wet;
      current_dry += delta_dry;

      dest[i] = tickUnfiltered(audio_in[i], current_period, current_feedback, current_wet, current_dry);
      current_period += delta_period;
    }
  }

  template<class MemoryType>
  void Delay<MemoryType>::process(const poly_float* audio_in, int num_samples,
                                  poly_float current_period, poly_float current_feedback,
                                  poly_float current_filter_gain,
                                  poly_float current_low_coefficient, poly_float current_high_coefficient,
                                  poly_float current_wet, poly_float current_dry) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;
    poly_float delta_feedback = (feedback_ - current_feedback) * tick_increment;
    poly_float delta_period = (period_ - current_period) * tick_increment;
    poly_float delta_filter_gain = (filter_gain_ - current_filter_gain) * tick_increment;
    poly_float delta_low_coefficient = (low_coefficient_ - current_low_coefficient) * tick_increment;
    poly_float delta_high_coefficient = (high_coefficient_ - current_high_coefficient) * tick_increment;

    poly_float* dest = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      current_feedback += delta_feedback;
      current_wet += delta_wet;
      current_dry += delta_dry;
      current_filter_gain += delta_filter_gain;
      current_low_coefficient += delta_low_coefficient;
      current_high_coefficient += delta_high_coefficient;

      dest[i] = tick(audio_in[i], current_period, current_feedback, current_filter_gain,
                     current_low_coefficient, current_high_coefficient, current_wet, current_dry);

      current_period += delta_period;
    }
  }

  template<class MemoryType>
  void Delay<MemoryType>::processDamped(const poly_float* audio_in, int num_samples,
                                        poly_float current_period, poly_float current_feedback,
                                        poly_float current_low_coefficient,
                                        poly_float current_wet, poly_float current_dry) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;
    poly_float delta_feedback = (feedback_ - current_feedback) * tick_increment;
    poly_float delta_period = (period_ - current_period) * tick_increment;
    poly_float delta_low_coefficient = (low_coefficient_ - current_low_coefficient) * tick_increment;

    poly_float* dest = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      current_feedback += delta_feedback;
      current_wet += delta_wet;
      current_dry += delta_dry;
      current_low_coefficient += delta_low_coefficient;

      dest[i] = tickDamped(audio_in[i], current_period, current_feedback,
                           current_low_coefficient, current_wet, current_dry);

      current_period += delta_period;
    }
  }

  template<class MemoryType>
  void Delay<MemoryType>::processPingPong(const poly_float* audio_in, int num_samples,
                                          poly_float current_period, poly_float current_feedback,
                                          poly_float current_filter_gain,
                                          poly_float current_low_coefficient, poly_float current_high_coefficient,
                                          poly_float current_wet, poly_float current_dry) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;
    poly_float delta_feedback = (feedback_ - current_feedback) * tick_increment;
    poly_float delta_period = (period_ - current_period) * tick_increment;
    poly_float delta_filter_gain = (filter_gain_ - current_filter_gain) * tick_increment;
    poly_float delta_low_coefficient = (low_coefficient_ - current_low_coefficient) * tick_increment;
    poly_float delta_high_coefficient = (high_coefficient_ - current_high_coefficient) * tick_increment;

    poly_float* dest = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      current_feedback += delta_feedback;
      current_wet += delta_wet;
      current_dry += delta_dry;
      current_filter_gain += delta_filter_gain;
      current_low_coefficient += delta_low_coefficient;
      current_high_coefficient += delta_high_coefficient;

      dest[i] = tickPingPong(audio_in[i], current_period, current_feedback, current_filter_gain,
                             current_low_coefficient, current_high_coefficient, current_wet, current_dry);
    
      current_period += delta_period;
    }
  }

  template<class MemoryType>
  void Delay<MemoryType>::processMonoPingPong(const poly_float* audio_in, int num_samples,
                                              poly_float current_period, poly_float current_feedback,
                                              poly_float current_filter_gain,
                                              poly_float current_low_coefficient, poly_float current_high_coefficient,
                                              poly_float current_wet, poly_float current_dry) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;
    poly_float delta_feedback = (feedback_ - current_feedback) * tick_increment;
    poly_float delta_period = (period_ - current_period) * tick_increment;
    poly_float delta_filter_gain = (filter_gain_ - current_filter_gain) * tick_increment;
    poly_float delta_low_coefficient = (low_coefficient_ - current_low_coefficient) * tick_increment;
    poly_float delta_high_coefficient = (high_coefficient_ - current_high_coefficient) * tick_increment;

    poly_float* dest = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      current_feedback += delta_feedback;
      current_wet += delta_wet;
      current_dry += delta_dry;
      current_filter_gain += delta_filter_gain;
      current_low_coefficient += delta_low_coefficient;
      current_high_coefficient += delta_high_coefficient;

      dest[i] = tickMonoPingPong(audio_in[i], current_period, current_feedback, current_filter_gain,
                                 current_low_coefficient, current_high_coefficient, current_wet, current_dry);

      current_period += delta_period;
    }
  }

  template<class MemoryType>
  force_inline poly_float Delay<MemoryType>::tickCleanUnfiltered(poly_float audio_in, poly_float period,
                                                                 poly_float feedback,
                                                                 poly_float wet, poly_float dry) {
    poly_float read = memory_->get(period);
    memory_->push(audio_in + read * feedback);
    return dry * audio_in + wet * read;
  }

  template<class MemoryType>
  force_inline poly_float Delay<MemoryType>::tickUnfiltered(poly_float audio_in, poly_float period,
                                                            poly_float feedback,
                                                            poly_float wet, poly_float dry) {
    poly_float read = memory_->get(period);
    memory_->push(saturate(audio_in + read * feedback));
    return dry * audio_in + wet * read;
  }

  template<class MemoryType>
  force_inline poly_float Delay<MemoryType>::tick(poly_float audio_in, poly_float period, poly_float feedback,
                                                  poly_float filter_gain, poly_float low_coefficient,
                                                  poly_float high_coefficient,
                                                  poly_float wet, poly_float dry) {
    poly_float read = memory_->get(period);
    poly_float write_raw_value = saturateLarge(audio_in + read * feedback);
    poly_float low_pass_result = low_pass_.tickBasic(write_raw_value * filter_gain, low_coefficient);
    poly_float second_pass_result = high_pass_.tickBasic(low_pass_result, high_coefficient);
    memory_->push(low_pass_result - second_pass_result);
    return dry * audio_in + wet * read;
  }

  template<class MemoryType>
  force_inline poly_float Delay<MemoryType>::tickDamped(poly_float audio_in, poly_float period,
                                                        poly_float feedback, poly_float low_coefficient,
                                                        poly_float wet, poly_float dry) {
    poly_float read = memory_->get(period);
    poly_float write_raw_value = saturateLarge(audio_in + read * feedback);
    poly_float low_pass_result = low_pass_.tickBasic(write_raw_value, low_coefficient);
    memory_->push(low_pass_result);
    return dry * audio_in + wet * read;
  }

  template<class MemoryType>
  force_inline poly_float Delay<MemoryType>::tickPingPong(poly_float audio_in, poly_float period, poly_float feedback,
                                                          poly_float filter_gain, poly_float low_coefficient,
                                                          poly_float high_coefficient,
                                                          poly_float wet, poly_float dry) {
    poly_float read = memory_->get(period);
    poly_float write_raw_value = utils::swapStereo(saturateLarge(audio_in + read * feedback));
    poly_float low_pass_result = low_pass_.tickBasic(write_raw_value * filter_gain, low_coefficient);
    poly_float second_pass_result = high_pass_.tickBasic(low_pass_result, high_coefficient);
    memory_->push(low_pass_result - second_pass_result);
    return dry * audio_in + wet * read;
  }

  template<class MemoryType>
  force_inline poly_float Delay<MemoryType>::tickMonoPingPong(poly_float audio_in, poly_float period,
                                                              poly_float feedback,
                                                              poly_float filter_gain, poly_float low_coefficient,
                                                              poly_float high_coefficient,
                                                              poly_float wet, poly_float dry) {
    poly_float read = memory_->get(period);
    poly_float mono_in = (audio_in + utils::swapStereo(audio_in)) * (1.0f / kSqrt2) & constants::kLeftMask;
    poly_float write_raw_value = utils::swapStereo(saturateLarge(mono_in + read * feedback));
    poly_float low_pass_result = low_pass_.tickBasic(write_raw_value * filter_gain, low_coefficient);
    poly_float second_pass_result = high_pass_.tickBasic(low_pass_result, high_coefficient);
    memory_->push(low_pass_result - second_pass_result);
    return dry * audio_in + wet * read;
  }

  template class Delay<StereoMemory>;
  template class Delay<Memory>;
} // namespace vital
