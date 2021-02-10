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

#include "comb_filter.h"

#include "futils.h"
#include "memory.h"

namespace vital {

  namespace {
    constexpr float kFlangeScale = 0.70710678119f;

    force_inline poly_float getLowGain(poly_float blend) {
      return utils::clamp(-blend + 2.0f, 0.0f, 1.0f);
    }

    force_inline poly_float getHighGain(poly_float blend) {
      return utils::clamp(blend, 0.0f, 1.0f);
    }

    force_inline poly_float tickComb(poly_float audio_in, Memory* memory,
                                     OnePoleFilter<>& filter1, OnePoleFilter<>& filter2,
                                     poly_float period, poly_float feedback, poly_float scale,
                                     poly_float filter_coefficient, poly_float filter2_coefficient,
                                     poly_float low_gain, poly_float high_gain) {
      poly_float read = memory->get(period);
      poly_float combine = utils::mulAdd(scale * audio_in, read, feedback);

      poly_float low_output = filter1.tickBasic(combine, filter_coefficient);
      poly_float high_output = combine - low_output;
      poly_float stage1_output = utils::mulAdd(low_gain * low_output, high_gain, high_output);
      poly_float stage2_output = filter2.tickBasic(stage1_output, filter2_coefficient);
      poly_float result = stage1_output - stage2_output;
      memory->push(futils::hardTanh(result));

      VITAL_ASSERT(utils::isFinite(result));
      return result;
    }

    force_inline poly_float tickPositiveFlange(poly_float audio_in, Memory* memory,
                                               OnePoleFilter<>& filter1, OnePoleFilter<>& filter2,
                                               poly_float period, poly_float feedback, poly_float scale,
                                               poly_float filter_coefficient, poly_float filter2_coefficient,
                                               poly_float low_gain, poly_float high_gain) {
      poly_float read = memory->get(period);
      poly_float low_output = filter1.tickBasic(read, filter_coefficient);
      poly_float high_output = read - low_output;
      poly_float stage1_output = utils::mulAdd(low_gain * low_output, high_gain, high_output);
      poly_float stage2_output = filter2.tickBasic(stage1_output, filter2_coefficient);
      poly_float filter_output = stage1_output - stage2_output;
      VITAL_ASSERT(utils::isFinite(filter_output));
      
      poly_float scaled_input = audio_in * kFlangeScale;
      memory->push(scaled_input + futils::hardTanh(filter_output * feedback));

      return scaled_input * scale + filter_output;
    }

    force_inline poly_float tickNegativeFlange(poly_float audio_in, Memory* memory,
                                               OnePoleFilter<>& filter1, OnePoleFilter<>& filter2,
                                               poly_float period, poly_float feedback, poly_float scale,
                                               poly_float filter_coefficient, poly_float filter2_coefficient,
                                               poly_float low_gain, poly_float high_gain) {
      poly_float read = memory->get(period * 0.5f);
      poly_float low_output = filter1.tickBasic(read, filter_coefficient);
      poly_float high_output = read - low_output;
      poly_float stage1_output = utils::mulAdd(low_gain * low_output, high_gain, high_output);
      poly_float stage2_output = filter2.tickBasic(stage1_output, filter2_coefficient);
      poly_float filter_output = stage1_output - stage2_output;
      VITAL_ASSERT(utils::isFinite(filter_output));

      poly_float scaled_input = audio_in * kFlangeScale;
      memory->push(scaled_input - futils::hardTanh(filter_output * feedback));
      return scaled_input * scale - filter_output;
    }
  } // namespace

  CombFilter::CombFilter(int size) : Processor(CombFilter::kNumInputs, 1) {
    feedback_style_ = kComb;
    memory_ = std::make_unique<Memory>(size);
    feedback_ = 0.0f;
    max_period_ = Memory::kMinPeriod;
    scale_ = 0.0f;
    low_gain_ = 0.0f;
    high_gain_ = 0.0f;
    filter_midi_cutoff_ = 0.0f;
    filter2_midi_cutoff_ = 0.0f;

    filter_coefficient_ = 0.0f;
    filter2_coefficient_ = 0.0f;
  }

  CombFilter::CombFilter(const CombFilter& other) : Processor(other), SynthFilter(other) {
    this->feedback_style_ = other.feedback_style_;
    this->memory_ = std::make_unique<Memory>(*other.memory_);
    this->feedback_ = 0.0f;
    this->max_period_ = Memory::kMinPeriod;
    this->filter_coefficient_ = 0.0f;
    this->filter2_coefficient_ = 0.0f;
    this->scale_ = 0.0f;
    this->filter_midi_cutoff_ = 0.0f;
    this->filter2_midi_cutoff_ = 0.0f;
    this->feedback_filter_.reset(constants::kFullMask);
  }

  CombFilter::~CombFilter() { }
  
  void CombFilter::reset(poly_mask reset_mask) {
    mono_float max_period = max_period_[0];
    for (int i = 1; i < poly_float::kSize; ++i)
      max_period = utils::max(max_period, max_period_[i]);

    int clear_samples = std::min(memory_->getSize() - 1, ((int)max_period) + 1);
    memory_->clearMemory(clear_samples, reset_mask);

    scale_ = utils::maskLoad(scale_, 0.0f, reset_mask);
    low_gain_ = utils::maskLoad(low_gain_, 0.0f, reset_mask);
    high_gain_ = utils::maskLoad(high_gain_, 0.0f, reset_mask);

    feedback_filter_.reset(reset_mask);
    feedback_filter2_.reset(reset_mask);
  }

  void CombFilter::hardReset() {
    reset(constants::kFullMask);
  }

  void CombFilter::setupFilter(const FilterState& filter_state) {
    feedback_style_ = getFeedbackStyle(filter_state.style);
    poly_float resonance = utils::clamp(filter_state.resonance_percent, 0.0f, 1.0f);

    if (feedback_style_ == kComb) {
      feedback_ = utils::interpolate(-kMaxFeedback, kMaxFeedback, resonance);
      feedback_ /= utils::sqrt(poly_float::abs(feedback_) + 0.00001f);
      scale_ = -feedback_ * feedback_ * kInputScale + 1.0f;
    }
    else {
      feedback_ = utils::interpolate(0.0f, kMaxFeedback, resonance);
      scale_ = poly_float(1.0f) / (feedback_ + 1.0f);
    }

    poly_float midi_cutoff = filter_state.midi_cutoff;
    float min_nyquist = getSampleRate() * kMinNyquistMult;

    poly_float blend = filter_state.pass_blend;
    poly_float min_cutoff = midi_cutoff - 4 * kNotesPerOctave;
    
    FilterStyle filter_style = getFilterStyle(filter_state.style);
    if (filter_style == kBandSpread) {
      poly_float midi_blend_transpose = filter_state.transpose;
      poly_float center_midi_cutoff = midi_cutoff + midi_blend_transpose;
      poly_float midi_band_range = (blend * 0.5f * kBandOctaveRange + kBandOctaveMin) * kNotesPerOctave;

      filter_midi_cutoff_ = center_midi_cutoff + midi_band_range;
      filter2_midi_cutoff_ = utils::max(min_cutoff, center_midi_cutoff - midi_band_range);
      poly_float filter1_cutoff = utils::midiNoteToFrequency(filter_midi_cutoff_);
      poly_float filter2_cutoff = utils::midiNoteToFrequency(filter2_midi_cutoff_);

      filter1_cutoff = utils::clamp(filter1_cutoff, 1.0f, getSampleRate() / 2.1f);
      filter2_cutoff = utils::clamp(filter2_cutoff, 1.0f, getSampleRate() / 2.1f);
      filter_midi_cutoff_ = utils::frequencyToMidiNote(filter1_cutoff);
      filter2_midi_cutoff_ = utils::frequencyToMidiNote(filter2_cutoff);
      low_gain_ = filter2_cutoff / filter1_cutoff + 1.0f;
      high_gain_ = 0.0f;

      filter_coefficient_ = OnePoleFilter<>::computeCoefficient(filter1_cutoff, getSampleRate());
      filter2_coefficient_ = OnePoleFilter<>::computeCoefficient(filter2_cutoff, getSampleRate());
    }
    else {
      low_gain_ = getLowGain(blend);
      high_gain_ = getHighGain(blend);

      poly_float midi_blend_transpose = filter_state.transpose;
      filter_midi_cutoff_ = midi_cutoff + midi_blend_transpose;
      filter2_midi_cutoff_ = min_cutoff;

      poly_float filter_cutoff = utils::midiNoteToFrequency(filter_midi_cutoff_);
      poly_float filter2_cutoff = utils::midiNoteToFrequency(filter2_midi_cutoff_);
      filter_cutoff = utils::clamp(filter_cutoff, 1.0f, min_nyquist);
      filter2_cutoff = utils::clamp(filter2_cutoff, 1.0f, min_nyquist);

      filter_coefficient_ = OnePoleFilter<>::computeCoefficient(filter_cutoff, getSampleRate());
      filter2_coefficient_ = OnePoleFilter<>::computeCoefficient(filter2_cutoff, getSampleRate());
    }
  }

  void CombFilter::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));

    filter_state_.loadSettings(this);
    FeedbackStyle style = getFeedbackStyle(filter_state_.style);

    if (style == kComb)
      processFilter<tickComb>(num_samples);
    else if (style == kPositiveFlange)
      processFilter<tickPositiveFlange>(num_samples);
    else if (style == kNegativeFlange)
      processFilter<tickNegativeFlange>(num_samples);
    else
      VITAL_ASSERT(false);
  }

  template<poly_float(*tick)(poly_float, Memory*, OnePoleFilter<>&, OnePoleFilter<>&,
                             poly_float, poly_float, poly_float, poly_float, poly_float, poly_float, poly_float)>
  void CombFilter::processFilter(int num_samples) {
    poly_float current_feedback = feedback_;
    poly_float current_filter_coefficient = filter_coefficient_;
    poly_float current_filter2_coefficient = filter2_coefficient_;
    poly_float current_scale = scale_;
    poly_float current_low_gain = low_gain_;
    poly_float current_high_gain = high_gain_;

    setupFilter(filter_state_);

    poly_float min_midi_cutoff = utils::min(filter_state_.midi_cutoff_buffer[0],
                                            filter_state_.midi_cutoff_buffer[num_samples - 1]);
    poly_float min_frequency = utils::midiNoteToFrequency(min_midi_cutoff);
    float min_nyquist = getSampleRate() * kMinNyquistMult;
    max_period_ = poly_float(getSampleRate()) / utils::clamp(min_frequency, 1.0f, min_nyquist);
    poly_float min_period = Memory::kMinPeriod;
    if (feedback_style_ == kNegativeFlange)
      min_period *= 2.0f;
    max_period_ = utils::clamp(max_period_, min_period, memory_->getMaxPeriod() - 5.0f);

    poly_mask reset_mask = getResetMask(kReset);
    if (reset_mask.anyMask()) {
      reset(reset_mask);

      current_feedback = utils::maskLoad(current_feedback, feedback_, reset_mask);
      current_filter_coefficient = utils::maskLoad(current_filter_coefficient, filter_coefficient_, reset_mask);
      current_filter2_coefficient = utils::maskLoad(current_filter2_coefficient, filter2_coefficient_, reset_mask);
      current_scale = utils::maskLoad(current_scale, scale_, reset_mask);
      current_low_gain = utils::maskLoad(current_low_gain, low_gain_, reset_mask);
      current_high_gain = utils::maskLoad(current_high_gain, high_gain_, reset_mask);
    }

    const poly_float* audio_in = input(kAudio)->source->buffer;
    poly_float* audio_out = output()->buffer;

    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_feedback = (feedback_ - current_feedback) * tick_increment;
    poly_float delta_coefficient = (filter_coefficient_ - current_filter_coefficient) * tick_increment;
    poly_float delta_scale = (scale_ - current_scale) * tick_increment;
    poly_float delta_coefficient2 = (filter2_coefficient_ - current_filter2_coefficient) * tick_increment;
    poly_float delta_low_gain = (low_gain_ - current_low_gain) * tick_increment;
    poly_float delta_high_gain = (high_gain_ - current_high_gain) * tick_increment;

    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi);

    poly_float sample_rate = getSampleRate();
    poly_float max_period = memory_->getMaxPeriod() - 5.0f;
    Memory* memory = memory_.get();
    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_offset = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = base_frequency * futils::midiOffsetToRatio(midi_offset);
      poly_float period = sample_rate / frequency;
      period = utils::clamp(period, min_period, max_period);

      current_feedback += delta_feedback;
      current_filter_coefficient += delta_coefficient;
      current_filter2_coefficient += delta_coefficient2;
      current_scale += delta_scale;
      current_low_gain += delta_low_gain;
      current_high_gain += delta_high_gain;

      audio_out[i] = tick(audio_in[i], memory, feedback_filter_, feedback_filter2_, 
                          period, current_feedback, current_scale,
                          current_filter_coefficient, current_filter2_coefficient,
                          current_low_gain, current_high_gain);
    }
  }

} // namespace vital
