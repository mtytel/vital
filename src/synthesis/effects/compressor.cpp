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

#include "compressor.h"
#include "futils.h"

namespace vital {
  
  namespace {
    constexpr mono_float kRmsTime = 0.025f;
    constexpr mono_float kMaxExpandMult = 32.0f;
    constexpr mono_float kLowAttackMs = 2.8f;
    constexpr mono_float kBandAttackMs = 1.4f;
    constexpr mono_float kHighAttackMs = 0.7f;
    constexpr mono_float kLowReleaseMs = 40.0f;
    constexpr mono_float kBandReleaseMs = 28.0f;
    constexpr mono_float kHighReleaseMs = 15.0f;

    constexpr mono_float kMinGain = -30.0f;
    constexpr mono_float kMaxGain = 30.0f;
    constexpr mono_float kMinThreshold = -100.0f;
    constexpr mono_float kMaxThreshold = 12.0f;
    constexpr mono_float kMinSampleEnvelope = 5.0f;
  } // namespace

  Compressor::Compressor(mono_float base_attack_ms_first, mono_float base_release_ms_first,
                         mono_float base_attack_ms_second, mono_float base_release_ms_second) :
      Processor(kNumInputs, kNumOutputs), input_mean_squared_(0.0f) {
    base_attack_ms_ = utils::maskLoad(poly_float(base_attack_ms_second), base_attack_ms_first, constants::kFirstMask);
    poly_float second_release = base_release_ms_second;
    base_release_ms_ = utils::maskLoad(second_release, base_release_ms_first, constants::kFirstMask);
    output_mult_ = 0.0f;
    mix_ = 0.0f;
  }

  void Compressor::process(int num_samples) {
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  void Compressor::processWithInput(const poly_float* audio_in, int num_samples) {
    processRms(audio_in, num_samples);

    input_mean_squared_ = computeMeanSquared(audio_in, num_samples, input_mean_squared_);
    output_mean_squared_ = computeMeanSquared(output(kAudioOut)->buffer, num_samples, output_mean_squared_);
    scaleOutput(audio_in, num_samples);
  }

  void Compressor::processRms(const poly_float* audio_in, int num_samples) {
    poly_float* audio_out = output(kAudioOut)->buffer;

    mono_float samples_per_ms = (1.0f * getSampleRate()) / kMsPerSec;
    poly_float attack_mult = base_attack_ms_ * samples_per_ms;
    poly_float release_mult = base_release_ms_ * samples_per_ms;
    poly_float attack_exponent = utils::clamp(input(kAttack)->at(0), 0.0f, 1.0f) * 8.0f - 4.0f;
    poly_float release_exponent = utils::clamp(input(kRelease)->at(0), 0.0f, 1.0f) * 8.0f - 4.0f;
    poly_float envelope_attack_samples = futils::exp(attack_exponent) * attack_mult;
    poly_float envelope_release_samples = futils::exp(release_exponent) * release_mult;
    envelope_attack_samples = utils::max(envelope_attack_samples, kMinSampleEnvelope);
    envelope_release_samples = utils::max(envelope_release_samples, kMinSampleEnvelope);

    poly_float attack_scale = poly_float(1.0f) / (envelope_attack_samples + 1.0f);
    poly_float release_scale = poly_float(1.0f) / (envelope_release_samples + 1.0f);

    poly_float upper_threshold = utils::clamp(input(kUpperThreshold)->at(0), kMinThreshold, kMaxThreshold);
    upper_threshold = futils::dbToMagnitude(upper_threshold);
    upper_threshold *= upper_threshold;
    poly_float lower_threshold = utils::clamp(input(kLowerThreshold)->at(0), kMinThreshold, kMaxThreshold);
    lower_threshold = futils::dbToMagnitude(lower_threshold);
    lower_threshold *= lower_threshold;

    poly_float upper_ratio = utils::clamp(input(kUpperRatio)->at(0), 0.0f, 1.0f) * 0.5f;
    poly_float lower_ratio = utils::clamp(input(kLowerRatio)->at(0), -1.0f, 1.0f) * 0.5f;

    poly_float low_enveloped_mean_squared = low_enveloped_mean_squared_;
    poly_float high_enveloped_mean_squared = high_enveloped_mean_squared_;

    for (int i = 0; i < num_samples; ++i) {
      poly_float sample = audio_in[i];
      poly_float sample_squared = sample * sample;

      poly_mask high_attack_mask = poly_float::greaterThan(sample_squared, high_enveloped_mean_squared);
      poly_float high_samples = utils::maskLoad(envelope_release_samples, envelope_attack_samples, high_attack_mask);
      poly_float high_scale = utils::maskLoad(release_scale, attack_scale, high_attack_mask);

      high_enveloped_mean_squared = (sample_squared + high_enveloped_mean_squared * high_samples) * high_scale;
      high_enveloped_mean_squared = utils::max(high_enveloped_mean_squared, upper_threshold);

      poly_float upper_mag_delta = upper_threshold / high_enveloped_mean_squared;
      poly_float upper_mult = futils::pow(upper_mag_delta, upper_ratio);

      poly_mask low_attack_mask = poly_float::greaterThan(sample_squared, low_enveloped_mean_squared);
      poly_float low_samples = utils::maskLoad(envelope_release_samples, envelope_attack_samples, low_attack_mask);
      poly_float low_scale = utils::maskLoad(release_scale, attack_scale, low_attack_mask);

      low_enveloped_mean_squared = (sample_squared + low_enveloped_mean_squared * low_samples) * low_scale;
      low_enveloped_mean_squared = utils::min(low_enveloped_mean_squared, lower_threshold);
      
      poly_float lower_mag_delta = lower_threshold / low_enveloped_mean_squared;
      poly_float lower_mult = futils::pow(lower_mag_delta, lower_ratio);

      poly_float gain_compression = utils::clamp(upper_mult * lower_mult, 0.0f, kMaxExpandMult);
      audio_out[i] = gain_compression * sample;
      VITAL_ASSERT(utils::isContained(audio_out[i]));
    }

    low_enveloped_mean_squared_ = low_enveloped_mean_squared;
    high_enveloped_mean_squared_ = high_enveloped_mean_squared;
  }

  void Compressor::scaleOutput(const poly_float* audio_input, int num_samples) {
    poly_float* audio_out = output(kAudioOut)->buffer;

    poly_float current_output_mult = output_mult_;
    poly_float gain = utils::clamp(input(kOutputGain)->at(0), kMinGain, kMaxGain);
    output_mult_ = futils::dbToMagnitude(gain);
    poly_float delta_output_mult = (output_mult_ - current_output_mult) * (1.0f / num_samples);

    poly_float current_mix = mix_;
    mix_ = utils::clamp(input(kMix)->at(0), 0.0f, 1.0f);
    poly_float delta_mix = (mix_ - current_mix) * (1.0f / num_samples);

    for (int i = 0; i < num_samples; ++i) {
      current_output_mult += delta_output_mult;
      current_mix += delta_mix;
      audio_out[i] = utils::interpolate(audio_input[i], audio_out[i] * current_output_mult, current_mix);
      VITAL_ASSERT(utils::isContained(audio_out[i]));
    }
  }

  void Compressor::reset(poly_mask reset_mask) {
    input_mean_squared_ = 0.0f;
    output_mean_squared_ = 0.0f;
    output_mult_ = 0.0f;
    mix_ = 0.0f;

    high_enveloped_mean_squared_ = 0.0f;
    low_enveloped_mean_squared_ = 0.0f;
  }

  poly_float Compressor::computeMeanSquared(const poly_float* audio_in, int num_samples, poly_float mean_squared) {
    int rms_samples = kRmsTime * getSampleRate();
    float rms_adjusted = rms_samples - 1.0f;
    mono_float input_scale = 1.0f / rms_samples;

    for (int i = 0; i < num_samples; ++i) {
      poly_float sample = audio_in[i];
      poly_float sample_squared = sample * sample;
      mean_squared = (mean_squared * rms_adjusted + sample_squared) * input_scale;
    }
    return mean_squared;
  }

  MultibandCompressor::MultibandCompressor() :
      Processor(kNumInputs, kNumOutputs), low_band_filter_(120.0f), band_high_filter_(2500.0f),
      low_band_compressor_(kLowAttackMs, kLowReleaseMs, kBandAttackMs, kBandReleaseMs),
      band_high_compressor_(kBandAttackMs, kBandReleaseMs, kHighAttackMs, kHighReleaseMs) {
    was_low_enabled_ = false;
    was_high_enabled_ = false;

    low_band_compressor_.plug(&low_band_upper_threshold_, Compressor::kUpperThreshold);
    low_band_compressor_.plug(&low_band_lower_threshold_, Compressor::kLowerThreshold);
    low_band_compressor_.plug(&low_band_upper_ratio_, Compressor::kUpperRatio);
    low_band_compressor_.plug(&low_band_lower_ratio_, Compressor::kLowerRatio);
    low_band_compressor_.plug(&low_band_output_gain_, Compressor::kOutputGain);
    low_band_compressor_.useInput(input(kAttack), Compressor::kAttack);
    low_band_compressor_.useInput(input(kRelease), Compressor::kRelease);
    low_band_compressor_.useInput(input(kMix), Compressor::kMix);

    band_high_compressor_.plug(&band_high_upper_threshold_, Compressor::kUpperThreshold);
    band_high_compressor_.plug(&band_high_lower_threshold_, Compressor::kLowerThreshold);
    band_high_compressor_.plug(&band_high_upper_ratio_, Compressor::kUpperRatio);
    band_high_compressor_.plug(&band_high_lower_ratio_, Compressor::kLowerRatio);
    band_high_compressor_.plug(&band_high_output_gain_, Compressor::kOutputGain);
    band_high_compressor_.useInput(input(kAttack), Compressor::kAttack);
    band_high_compressor_.useInput(input(kRelease), Compressor::kRelease);
    band_high_compressor_.useInput(input(kMix), Compressor::kMix);
  }

  void MultibandCompressor::setOversampleAmount(int oversample) {
    Processor::setOversampleAmount(oversample);
    low_band_filter_.setOversampleAmount(oversample);
    band_high_filter_.setOversampleAmount(oversample);
    low_band_compressor_.setOversampleAmount(oversample);
    band_high_compressor_.setOversampleAmount(oversample);
  }

  void MultibandCompressor::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    low_band_filter_.setSampleRate(sample_rate);
    band_high_filter_.setSampleRate(sample_rate);
    low_band_compressor_.setSampleRate(sample_rate);
    band_high_compressor_.setSampleRate(sample_rate);
  }

  void MultibandCompressor::reset(poly_mask reset_mask) {
    low_band_filter_.reset(reset_mask);
    band_high_filter_.reset(reset_mask);
    low_band_compressor_.reset(reset_mask);
    band_high_compressor_.reset(reset_mask);

    output(kLowInputMeanSquared)->buffer[0] = 0.0f;
    output(kLowOutputMeanSquared)->buffer[0] = 0.0f;
    output(kBandInputMeanSquared)->buffer[0] = 0.0f;
    output(kBandOutputMeanSquared)->buffer[0] = 0.0f;
    output(kHighInputMeanSquared)->buffer[0] = 0.0f;
    output(kHighOutputMeanSquared)->buffer[0] = 0.0f;
  }

  void MultibandCompressor::process(int num_samples) {
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  void MultibandCompressor::packFilterOutput(LinkwitzRileyFilter* filter, int num_samples, poly_float* dest) {
    const poly_float* low_output = filter->output(LinkwitzRileyFilter::kAudioLow)->buffer;
    const poly_float* high_output = filter->output(LinkwitzRileyFilter::kAudioHigh)->buffer;

    for (int i = 0; i < num_samples; ++i) {
      poly_float low_sample = low_output[i];
      poly_float high_sample = utils::swapVoices(high_output[i]);
      dest[i] = utils::maskLoad(high_sample, low_sample, constants::kFirstMask);
    }
  }

  void MultibandCompressor::packLowBandCompressor(int num_samples, poly_float* dest) {
    const poly_float* low_output = band_high_filter_.output(LinkwitzRileyFilter::kAudioLow)->buffer;
    const poly_float* high_output = band_high_filter_.output(LinkwitzRileyFilter::kAudioHigh)->buffer;

    for (int i = 0; i < num_samples; ++i) {
      poly_float low_band_sample = low_output[i];
      poly_float low_high_sample = high_output[i] & constants::kFirstMask;
      dest[i] = low_band_sample + low_high_sample;
    }
  }

  void MultibandCompressor::writeAllCompressorOutputs(int num_samples, poly_float* dest) {
    const poly_float* low_band_output = low_band_compressor_.output(Compressor::kAudioOut)->buffer;
    const poly_float* high_output = band_high_compressor_.output(Compressor::kAudioOut)->buffer;

    for (int i = 0; i < num_samples; ++i) {
      poly_float low_band_sample = low_band_output[i];
      low_band_sample += utils::swapVoices(low_band_sample);
      poly_float high_sample = utils::swapVoices(high_output[i]);
      dest[i] = low_band_sample + high_sample;
    }
  }

  void MultibandCompressor::writeCompressorOutputs(Compressor* compressor, int num_samples, poly_float* dest) {
    const poly_float* compressor_output = compressor->output(Compressor::kAudioOut)->buffer;

    for (int i = 0; i < num_samples; ++i) {
      poly_float sample = compressor_output[i];
      dest[i] = sample + utils::swapVoices(sample);
    }
  }

  void MultibandCompressor::processWithInput(const poly_float* audio_in, int num_samples) {
    int enabled_bands = input(kEnabledBands)->at(0)[0];
    bool low_enabled = enabled_bands == kMultiband || enabled_bands == kLowBand;
    bool high_enabled = enabled_bands == kMultiband || enabled_bands == kHighBand;

    low_band_upper_threshold_.buffer[0] = utils::maskLoad(input(kBandUpperThreshold)->at(0),
                                                          input(kLowUpperThreshold)->at(0), constants::kFirstMask);
    band_high_upper_threshold_.buffer[0] = utils::maskLoad(input(kHighUpperThreshold)->at(0),
                                                           input(kBandUpperThreshold)->at(0), constants::kFirstMask);
    low_band_lower_threshold_.buffer[0] = utils::maskLoad(input(kBandLowerThreshold)->at(0),
                                                          input(kLowLowerThreshold)->at(0), constants::kFirstMask);
    band_high_lower_threshold_.buffer[0] = utils::maskLoad(input(kHighLowerThreshold)->at(0),
                                                           input(kBandLowerThreshold)->at(0), constants::kFirstMask);
    low_band_upper_ratio_.buffer[0] = utils::maskLoad(input(kBandUpperRatio)->at(0),
                                                      input(kLowUpperRatio)->at(0), constants::kFirstMask);
    band_high_upper_ratio_.buffer[0] = utils::maskLoad(input(kHighUpperRatio)->at(0),
                                                       input(kBandUpperRatio)->at(0), constants::kFirstMask);
    low_band_lower_ratio_.buffer[0] = utils::maskLoad(input(kBandLowerRatio)->at(0),
                                                      input(kLowLowerRatio)->at(0), constants::kFirstMask);
    band_high_lower_ratio_.buffer[0] = utils::maskLoad(input(kHighLowerRatio)->at(0),
                                                       input(kBandLowerRatio)->at(0), constants::kFirstMask);

    low_band_output_gain_.buffer[0] = utils::maskLoad(input(kBandOutputGain)->at(0),
                                                      input(kLowOutputGain)->at(0), constants::kFirstMask);
    band_high_output_gain_.buffer[0] = utils::maskLoad(input(kHighOutputGain)->at(0),
                                                       input(kBandOutputGain)->at(0), constants::kFirstMask);

    if (low_enabled != was_low_enabled_ || high_enabled != was_high_enabled_) {
      low_band_filter_.reset(constants::kFullMask);
      band_high_filter_.reset(constants::kFullMask);
      low_band_compressor_.reset(constants::kFullMask);
      band_high_compressor_.reset(constants::kFullMask);
      was_low_enabled_ = low_enabled;
      was_high_enabled_ = high_enabled;
    }

    poly_float* audio_out = output(kAudioOut)->buffer;

    if (low_enabled && high_enabled) {
      low_band_filter_.processWithInput(audio_in, num_samples);
      packFilterOutput(&low_band_filter_, num_samples, audio_out);
      band_high_filter_.processWithInput(audio_out, num_samples);
      packLowBandCompressor(num_samples, audio_out);

      low_band_compressor_.processWithInput(audio_out, num_samples);
      const poly_float* band_high_buffer = band_high_filter_.output(LinkwitzRileyFilter::kAudioHigh)->buffer;
      band_high_compressor_.processWithInput(band_high_buffer, num_samples);
      writeAllCompressorOutputs(num_samples, audio_out);
    }
    else if (low_enabled) {
      low_band_filter_.processWithInput(audio_in, num_samples);
      packFilterOutput(&low_band_filter_, num_samples, audio_out);
      low_band_compressor_.processWithInput(audio_out, num_samples);
      writeCompressorOutputs(&low_band_compressor_, num_samples, audio_out);
    }
    else if (high_enabled) {
      band_high_filter_.processWithInput(audio_in, num_samples);
      packFilterOutput(&band_high_filter_, num_samples, audio_out);
      band_high_compressor_.processWithInput(audio_out, num_samples);
      writeCompressorOutputs(&band_high_compressor_, num_samples, audio_out);
    }
    else {
      band_high_compressor_.processWithInput(audio_in, num_samples);
      utils::copyBuffer(audio_out, band_high_compressor_.output(Compressor::kAudioOut)->buffer, num_samples);
    }

    poly_float low_band_input_ms = low_band_compressor_.getInputMeanSquared();
    poly_float band_high_input_ms = band_high_compressor_.getInputMeanSquared();
    poly_float low_band_output_ms = low_band_compressor_.getOutputMeanSquared();
    poly_float band_high_output_ms = band_high_compressor_.getOutputMeanSquared();

    output(kLowInputMeanSquared)->buffer[0] = low_band_input_ms;
    output(kLowOutputMeanSquared)->buffer[0] = low_band_output_ms;

    if (low_enabled) {
      output(kBandInputMeanSquared)->buffer[0] = utils::swapVoices(low_band_input_ms);
      output(kBandOutputMeanSquared)->buffer[0] = utils::swapVoices(low_band_output_ms);
    }
    else {
      output(kBandInputMeanSquared)->buffer[0] = band_high_input_ms;
      output(kBandOutputMeanSquared)->buffer[0] = band_high_output_ms;
    }

    output(kHighInputMeanSquared)->buffer[0] = utils::swapVoices(band_high_input_ms);
    output(kHighOutputMeanSquared)->buffer[0] = utils::swapVoices(band_high_output_ms);
  }

} // namespace vital
