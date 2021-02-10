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

#include "digital_svf.h"

#include "futils.h"

namespace vital {
  const DigitalSvf::SvfCoefficientLookup DigitalSvf::svf_coefficient_lookup_;

  DigitalSvf::DigitalSvf() : Processor(DigitalSvf::kNumInputs, 1),
                             min_resonance_(kDefaultMinResonance), max_resonance_(kDefaultMaxResonance),
                             basic_(false), drive_compensation_(true) {
    hardReset();
  }

  void DigitalSvf::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));
    VITAL_ASSERT(inputMatchesBufferSize(kMidiCutoff));
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  void DigitalSvf::processWithInput(const poly_float* audio_in, int num_samples) {
    FilterValues blends1 = blends1_;
    FilterValues blends2 = blends2_;
    poly_float current_resonance = resonance_;
    poly_float current_drive = drive_;
    poly_float current_post_multiply = post_multiply_;

    filter_state_.loadSettings(this);
    setupFilter(filter_state_);

    poly_mask reset_mask = getResetMask(kReset);

    if (reset_mask.anyMask()) {
      reset(reset_mask);
      blends1.reset(reset_mask, blends1_);
      blends2.reset(reset_mask, blends2_);
      current_resonance = utils::maskLoad(current_resonance, resonance_, reset_mask);
      current_drive = utils::maskLoad(current_drive, drive_, reset_mask);
      current_post_multiply = utils::maskLoad(current_post_multiply, post_multiply_, reset_mask);
    }

    if (filter_state_.style == kShelving || basic_)
      processBasic12(audio_in, num_samples, current_resonance, current_drive, current_post_multiply, blends1);
    else if (filter_state_.style == kDualNotchBand) {
      processDual(audio_in, num_samples, current_resonance, current_drive, current_post_multiply, blends1, blends2);
    }
    else if (filter_state_.style == k12Db)
      process12(audio_in, num_samples, current_resonance, current_drive, current_post_multiply, blends1);
    else
      process24(audio_in, num_samples, current_resonance, current_drive, current_post_multiply, blends1);
  }

  void DigitalSvf::process12(const poly_float* audio_in, int num_samples,
                             poly_float current_resonance, poly_float current_drive,
                             poly_float current_post_multiply, FilterValues& blends) {
    mono_float sample_inc = 1.0f / num_samples;
    FilterValues delta_blends = blends.getDelta(blends1_, sample_inc);
    poly_float delta_resonance = (resonance_ - current_resonance) * sample_inc;
    poly_float delta_drive = (drive_ - current_drive) * sample_inc;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * sample_inc;

    poly_float* audio_out = output()->buffer;
    const SvfCoefficientLookup* coefficient_lookup = getSvfCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = utils::max(midi_cutoff_buffer[i], 0.0f) - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      blends.increment(delta_blends);
      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;

      audio_out[i] = tick(audio_in[i], coefficient, current_resonance, current_drive, blends) * current_post_multiply;
      VITAL_ASSERT(utils::isFinite(audio_out[i]));
    }
  }

  void DigitalSvf::processBasic12(const poly_float* audio_in, int num_samples,
                                  poly_float current_resonance, poly_float current_drive,
                                  poly_float current_post_multiply, FilterValues& blends) {
    mono_float sample_inc = 1.0f / num_samples;
    FilterValues delta_blends = blends.getDelta(blends1_, sample_inc);
    poly_float delta_resonance = (resonance_ - current_resonance) * sample_inc;
    poly_float delta_drive = (drive_ - current_drive) * sample_inc;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * sample_inc;

    poly_float* audio_out = output()->buffer;
    const SvfCoefficientLookup* coefficient_lookup = getSvfCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      blends.increment(delta_blends);
      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;

      audio_out[i] = tickBasic(audio_in[i], coefficient, current_resonance, current_drive, blends) * current_post_multiply;
      VITAL_ASSERT(utils::isFinite(audio_out[i]));
    }
  }

  void DigitalSvf::process24(const poly_float* audio_in, int num_samples,
                             poly_float current_resonance, poly_float current_drive,
                             poly_float current_post_multiply, FilterValues& blends) {
    mono_float sample_inc = 1.0f / num_samples;
    FilterValues delta_blends = blends.getDelta(blends1_, sample_inc);
    poly_float delta_resonance = (resonance_ - current_resonance) * sample_inc;
    poly_float delta_drive = (drive_ - current_drive) * sample_inc;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * sample_inc;

    poly_float* audio_out = output()->buffer;
    const SvfCoefficientLookup* coefficient_lookup = getSvfCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      blends.increment(delta_blends);
      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;
      
      poly_float result = tick24(audio_in[i], coefficient, current_resonance, current_drive, blends);
      audio_out[i] = result * current_post_multiply;
      VITAL_ASSERT(utils::isFinite(audio_out[i]));
    }
  }

  void DigitalSvf::processBasic24(const poly_float* audio_in, int num_samples,
                                  poly_float current_resonance, poly_float current_drive,
                                  poly_float current_post_multiply, FilterValues& blends) {
    mono_float sample_inc = 1.0f / num_samples;
    FilterValues delta_blends = blends.getDelta(blends1_, sample_inc);
    poly_float delta_resonance = (resonance_ - current_resonance) * sample_inc;
    poly_float delta_drive = (drive_ - current_drive) * sample_inc;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * sample_inc;

    poly_float* audio_out = output()->buffer;
    const SvfCoefficientLookup* coefficient_lookup = getSvfCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      blends.increment(delta_blends);
      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;

      poly_float result = tickBasic24(audio_in[i], coefficient, current_resonance, current_drive, blends);
      audio_out[i] = result * current_post_multiply;
      VITAL_ASSERT(utils::isFinite(audio_out[i]));
    }
  }

  void DigitalSvf::processDual(const poly_float* audio_in, int num_samples,
                               poly_float current_resonance, poly_float current_drive,
                               poly_float current_post_multiply,
                               FilterValues& blends1, FilterValues& blends2) {
    mono_float sample_inc = 1.0f / num_samples;
    FilterValues delta_blends1 = blends1.getDelta(blends1_, sample_inc);
    FilterValues delta_blends2 = blends2.getDelta(blends2_, sample_inc);
    poly_float delta_resonance = (resonance_ - current_resonance) * sample_inc;
    poly_float delta_drive = (drive_ - current_drive) * sample_inc;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * sample_inc;

    poly_float* audio_out = output()->buffer;
    const SvfCoefficientLookup* coefficient_lookup = getSvfCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());
    
    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      blends1.increment(delta_blends1);
      blends2.increment(delta_blends2);
      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;

      poly_float result = tickDual(audio_in[i], coefficient, current_resonance, current_drive, blends1, blends2);
      audio_out[i] = result * current_post_multiply;
      VITAL_ASSERT(utils::isFinite(audio_out[i]));
    }
  }

  void DigitalSvf::setupFilter(const FilterState& filter_state) {
    midi_cutoff_ = filter_state.midi_cutoff;
    poly_float cutoff = utils::midiNoteToFrequency(filter_state.midi_cutoff);
    float min_nyquist = getSampleRate() * kMinNyquistMult;
    cutoff = utils::clamp(cutoff, kMinCutoff, min_nyquist);

    poly_float gain_decibels = utils::clamp(filter_state.gain, kMinGain, kMaxGain);
    poly_float gain_amplitude = utils::dbToMagnitude(gain_decibels);

    poly_float resonance_percent = utils::clamp(filter_state.resonance_percent, 0.0f, 1.0f);
    poly_float resonance_adjust = resonance_percent * resonance_percent * resonance_percent;
    poly_float resonance = utils::interpolate(min_resonance_, max_resonance_, resonance_adjust);
    if (drive_compensation_)
      drive_ = filter_state.drive / (resonance_adjust * 2.0f + 1.0f);
    else
      drive_ = filter_state.drive;

    post_multiply_ = gain_amplitude / utils::sqrt(filter_state.drive);
    resonance_ = poly_float(1.0f) / resonance;

    poly_float blend = utils::clamp(filter_state.pass_blend - 1.0f, -1.0f, 1.0f);

    if (filter_state.style == kDualNotchBand) {
      poly_float t = blend * 0.5f + 0.5f;
      poly_float drive_t = poly_float::min(-blend + 1.0f, 1.0f);
      poly_float drive_mult = -t + 2.0f;
      drive_ = utils::interpolate(filter_state.drive, drive_ * drive_mult, drive_t);

      low_amount_ = t;
      band_amount_ = 0.0f;
      high_amount_ = 1.0f;
    }
    else if (filter_state.style == kNotchPassSwap) {
      poly_float drive_t = poly_float::abs(blend);
      drive_ = utils::interpolate(filter_state.drive, drive_, drive_t);

      low_amount_ = utils::min(-blend + 1.0f, 1.0f);
      band_amount_ = 0.0f;
      high_amount_ = utils::min(blend + 1.0f, 1.0f);
    }
    else if (filter_state.style == kBandPeakNotch) {
      poly_float drive_t = poly_float::min(-blend + 1.0f, 1.0f);
      drive_ = utils::interpolate(filter_state.drive, drive_, drive_t);

      poly_float drive_inv_t = -drive_t + 1.0f;
      poly_float mult = utils::sqrt((drive_inv_t * drive_inv_t) * 0.5f + 0.5f);
      poly_float peak_band_value = -utils::max(-blend, 0.0f);
      low_amount_ = mult * (peak_band_value + 1.0f);
      band_amount_ = mult * (peak_band_value - blend + 1.0f) * 2.0f;
      high_amount_ = low_amount_;
    }
    else if (filter_state.style == kShelving) {
      drive_ = 1.0f;
      post_multiply_ = 1.0f;
      poly_float low_bell_t = utils::clamp(blend + 1.0f, 0.0f, 1.0f);
      poly_float bell_high_t = utils::clamp(blend, 0.0f, 1.0f);
      poly_float band_t = poly_float(1.0f) - blend * blend;

      poly_float amplitude_sqrt = utils::sqrt(gain_amplitude);
      poly_float amplitude_quartic = utils::sqrt(amplitude_sqrt);
      poly_float mult_adjust = futils::pow(amplitude_quartic, blend);

      low_amount_ = utils::interpolate(gain_amplitude, 1.0f, low_bell_t);
      high_amount_ = utils::interpolate(1.0f, gain_amplitude, bell_high_t);
      band_amount_ = resonance_ * amplitude_sqrt * utils::interpolate(1.0f, amplitude_sqrt, band_t);
      midi_cutoff_ += utils::ratioToMidiTranspose(mult_adjust);
    }
    else {
      band_amount_ = utils::sqrt(-blend * blend + 1.0f);
      poly_mask blend_mask = poly_float::lessThan(blend, 0.0f);
      low_amount_ = (-blend) & blend_mask;
      high_amount_ = blend & ~blend_mask;
    }

    blends1_.v0 = 0.0f;
    blends1_.v1 = band_amount_;
    blends1_.v2 = low_amount_;

    blends2_.v0 = 0.0f;
    blends2_.v1 = band_amount_;
    blends2_.v2 = high_amount_;

    blends1_.v0 += high_amount_;
    blends1_.v1 += -resonance_ * high_amount_;
    blends1_.v2 += -high_amount_;

    blends2_.v0 += low_amount_;
    blends2_.v1 += -resonance_ * low_amount_;
    blends2_.v2 += -low_amount_;
  }

  void DigitalSvf::setResonanceBounds(mono_float min, mono_float max) {
    min_resonance_ = min;
    max_resonance_ = max;
  }

  force_inline poly_float DigitalSvf::tick(poly_float audio_in, poly_float coefficient, 
                                           poly_float resonance, poly_float drive, FilterValues& blends) {
    return futils::hardTanh(tickBasic(audio_in, coefficient, resonance, drive, blends));
  }

  force_inline poly_float DigitalSvf::tickBasic(poly_float audio_in, poly_float coefficient,
                                                poly_float resonance, poly_float drive, FilterValues& blends) {
    poly_float coefficient_squared = coefficient * coefficient;
    poly_float coefficient_0 = poly_float(1.0f) / (coefficient_squared + coefficient * resonance + 1.0f);
    poly_float coefficient_1 = coefficient_0 * coefficient;
    poly_float coefficient_2 = coefficient_0 * coefficient_squared;
    poly_float in = drive * audio_in;

    poly_float v3 = in - ic2eq_;
    poly_float v1 = utils::mulAdd(coefficient_0 * ic1eq_, coefficient_1, v3);
    poly_float v2 = utils::mulAdd(utils::mulAdd(ic2eq_, coefficient_1, ic1eq_), coefficient_2, v3);
    ic1eq_ = v1 * 2.0f - ic1eq_;
    ic2eq_ = v2 * 2.0f - ic2eq_;

    return utils::mulAdd(utils::mulAdd(blends.v0 * in, blends.v1, v1), blends.v2, v2);
  }

  force_inline poly_float DigitalSvf::tick24(poly_float audio_in, poly_float coefficient,
                                             poly_float resonance, poly_float drive, FilterValues& blends) {
    poly_float coefficient_squared = coefficient * coefficient;
    poly_float pre_coefficient_0 = poly_float(1.0f) / (coefficient_squared + coefficient + 1.0f);
    poly_float pre_coefficient_1 = pre_coefficient_0 * coefficient;
    poly_float pre_coefficient_2 = pre_coefficient_0 * coefficient_squared;

    poly_float in = drive * audio_in;

    poly_float v3_pre = in - ic2eq_pre_;
    poly_float v1_pre = utils::mulAdd(pre_coefficient_0 * ic1eq_pre_, pre_coefficient_1, v3_pre);
    poly_float v2_pre = utils::mulAdd(utils::mulAdd(ic2eq_pre_, pre_coefficient_1, ic1eq_pre_),
                                      pre_coefficient_2, v3_pre);
    ic1eq_pre_ = v1_pre * 2.0f - ic1eq_pre_;
    ic2eq_pre_ = v2_pre * 2.0f - ic2eq_pre_;
    poly_float out_pre = utils::mulAdd(utils::mulAdd(blends.v0 * in, blends.v1, v1_pre), blends.v2, v2_pre);

    poly_float distort = futils::hardTanh(out_pre);

    return tick(distort, coefficient, resonance, 1.0f, blends);
  }

  force_inline poly_float DigitalSvf::tickBasic24(poly_float audio_in, poly_float coefficient,
                                                  poly_float resonance, poly_float drive, FilterValues& blends) {
    poly_float coefficient_squared = coefficient * coefficient;
    poly_float pre_coefficient_0 = poly_float(1.0f) / (coefficient_squared + coefficient + 1.0f);
    poly_float pre_coefficient_1 = pre_coefficient_0 * coefficient;
    poly_float pre_coefficient_2 = pre_coefficient_0 * coefficient_squared;

    poly_float v3_pre = audio_in - ic2eq_pre_;
    poly_float v1_pre = utils::mulAdd(pre_coefficient_0 * ic1eq_pre_, pre_coefficient_1, v3_pre);
    poly_float v2_pre = utils::mulAdd(utils::mulAdd(ic2eq_pre_, pre_coefficient_1, ic1eq_pre_),
                                      pre_coefficient_2, v3_pre);
    ic1eq_pre_ = v1_pre * 2.0f - ic1eq_pre_;
    ic2eq_pre_ = v2_pre * 2.0f - ic2eq_pre_;
    poly_float out_pre = utils::mulAdd(utils::mulAdd(blends.v0 * audio_in, blends.v1, v1_pre), blends.v2, v2_pre);

    return tickBasic(out_pre, coefficient, resonance, drive, blends);
  }

  force_inline poly_float DigitalSvf::tickDual(poly_float audio_in, poly_float coefficient,
                                               poly_float resonance, poly_float drive,
                                               FilterValues& blends1, FilterValues& blends2) {
    poly_float coefficient_squared = coefficient * coefficient;
    poly_float pre_coefficient_0 = poly_float(1.0f) / (coefficient_squared + coefficient + 1.0f);
    poly_float pre_coefficient_1 = pre_coefficient_0 * coefficient;
    poly_float pre_coefficient_2 = pre_coefficient_0 * coefficient_squared;
    poly_float coefficient_0 = poly_float(1.0f) / (coefficient_squared + coefficient * resonance + 1.0f);
    poly_float coefficient_1 = coefficient_0 * coefficient;
    poly_float coefficient_2 = coefficient_0 * coefficient_squared;

    poly_float in = drive * audio_in;

    poly_float v3_pre = in - ic2eq_pre_;
    poly_float v1_pre = utils::mulAdd(pre_coefficient_0 * ic1eq_pre_, pre_coefficient_1, v3_pre);
    poly_float v2_pre = utils::mulAdd(utils::mulAdd(ic2eq_pre_, pre_coefficient_1, ic1eq_pre_),
                                      pre_coefficient_2, v3_pre);
    ic1eq_pre_ = v1_pre * 2.0f - ic1eq_pre_;
    ic2eq_pre_ = v2_pre * 2.0f - ic2eq_pre_;
    poly_float out_pre = utils::mulAdd(utils::mulAdd(blends1.v0 * in, blends1.v1, v1_pre), blends1.v2, v2_pre);

    poly_float distort = futils::hardTanh(out_pre);

    poly_float v3 = distort - ic2eq_;
    poly_float v1 = utils::mulAdd(coefficient_0 * ic1eq_, coefficient_1, v3);
    poly_float v2 = utils::mulAdd(utils::mulAdd(ic2eq_, coefficient_1, ic1eq_), coefficient_2, v3);
    ic1eq_ = v1 * 2.0f - ic1eq_;
    ic2eq_ = v2 * 2.0f - ic2eq_;

    return futils::hardTanh(utils::mulAdd(utils::mulAdd(blends2.v0 * distort, blends2.v1, v1), blends2.v2, v2));
  }

  void DigitalSvf::reset(poly_mask reset_mask) {
    ic1eq_pre_ = utils::maskLoad(ic1eq_pre_, 0.0f, reset_mask);
    ic2eq_pre_ = utils::maskLoad(ic2eq_pre_, 0.0f, reset_mask);
    ic1eq_ = utils::maskLoad(ic1eq_, 0.0f, reset_mask);
    ic2eq_ = utils::maskLoad(ic2eq_, 0.0f, reset_mask);
  }

  void DigitalSvf::hardReset() {
    reset(constants::kFullMask);
    resonance_ = 1.0f;
    blends1_.hardReset();
    blends2_.hardReset();

    low_amount_ = 0.0f;
    band_amount_ = 0.0f;
    high_amount_ = 0.0f;

    drive_ = 0.0f;
    post_multiply_ = 0.0f;
  }
} // namespace vital
