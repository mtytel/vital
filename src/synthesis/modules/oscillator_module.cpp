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

#include "oscillator_module.h"

#include "synth_oscillator.h"
#include "wavetable.h"

namespace vital {

  OscillatorModule::OscillatorModule(std::string prefix) :
      SynthModule(kNumInputs, kNumOutputs), prefix_(std::move(prefix)), on_(nullptr), distortion_type_(nullptr) {
    wavetable_ = std::make_shared<Wavetable>(kNumOscillatorWaveFrames);
    was_on_ = std::make_shared<bool>(true);
  }

  void OscillatorModule::init() {
    oscillator_ = new SynthOscillator(wavetable_.get());

    createBaseControl(prefix_ + "_view_2d");
    on_ = createBaseControl(prefix_ + "_on");
    Value* midi_track = createBaseControl(prefix_ + "_midi_track");
    Value* smooth_interpolation = createBaseControl(prefix_ + "_smooth_interpolation");
    Value* spectral_unison = createBaseControl(prefix_ + "_spectral_unison");
    Value* stack_style = createBaseControl(prefix_ + "_stack_style");
    Value* transpose_quantize = createBaseControl(prefix_ + "_transpose_quantize");

    Input* reset = input(kReset);

    Output* wave_frame = createPolyModControl(prefix_ + "_wave_frame");
    Output* transpose = createPolyModControl(prefix_ + "_transpose", true, false, nullptr, reset);
    Output* tune = createPolyModControl(prefix_ + "_tune", true, false, nullptr, reset);
    Output* unison_voices = createPolyModControl(prefix_ + "_unison_voices");
    Output* unison_detune = createPolyModControl(prefix_ + "_unison_detune");
    Output* detune_power = createPolyModControl(prefix_ + "_detune_power");
    Output* detune_range = createPolyModControl(prefix_ + "_detune_range");
    Output* amplitude = createPolyModControl(prefix_ + "_level", true, true, nullptr, reset);
    Output* pan = createPolyModControl(prefix_ + "_pan");
    Output* phase = createPolyModControl(prefix_ + "_phase", true, true, nullptr, reset);
    Output* distortion_phase = createPolyModControl(prefix_ + "_distortion_phase");
    Output* rand_phase = createPolyModControl(prefix_ + "_random_phase");
    Output* blend = createPolyModControl(prefix_ + "_unison_blend");
    Output* stereo_spread = createPolyModControl(prefix_ + "_stereo_spread");
    Output* frame_spread = createPolyModControl(prefix_ + "_frame_spread");
    Output* distortion_spread = createPolyModControl(prefix_ + "_distortion_spread");
    distortion_type_ = createBaseControl(prefix_ + "_distortion_type");
    Output* distortion_amount = createPolyModControl(prefix_ + "_distortion_amount");
    Output* spectral_morph_spread = createPolyModControl(prefix_ + "_spectral_morph_spread");
    Value* spectral_morph_type = createBaseControl(prefix_ + "_spectral_morph_type");
    Output* spectral_morph_amount = createPolyModControl(prefix_ + "_spectral_morph_amount");

    oscillator_->useInput(input(kReset), SynthOscillator::kReset);
    oscillator_->useInput(input(kRetrigger), SynthOscillator::kRetrigger);
    oscillator_->useInput(input(kActiveVoices), SynthOscillator::kActiveVoices);
    oscillator_->useInput(input(kMidi), SynthOscillator::kMidiNote);
    oscillator_->plug(wave_frame, SynthOscillator::kWaveFrame);
    oscillator_->plug(midi_track, SynthOscillator::kMidiTrack);
    oscillator_->plug(smooth_interpolation, SynthOscillator::kSmoothlyInterpolate);
    oscillator_->plug(spectral_unison, SynthOscillator::kSpectralUnison);
    oscillator_->plug(transpose_quantize, SynthOscillator::kTransposeQuantize);
    oscillator_->plug(transpose, SynthOscillator::kTranspose);
    oscillator_->plug(tune, SynthOscillator::kTune);
    oscillator_->plug(stack_style, SynthOscillator::kStackStyle);
    oscillator_->plug(unison_detune, SynthOscillator::kUnisonDetune);
    oscillator_->plug(unison_voices, SynthOscillator::kUnisonVoices);
    oscillator_->plug(phase, SynthOscillator::kPhase);
    oscillator_->plug(distortion_phase, SynthOscillator::kDistortionPhase);
    oscillator_->plug(rand_phase, SynthOscillator::kRandomPhase);
    oscillator_->plug(blend, SynthOscillator::kBlend);
    oscillator_->plug(amplitude, SynthOscillator::kAmplitude);
    oscillator_->plug(pan, SynthOscillator::kPan);
    oscillator_->plug(detune_power, SynthOscillator::kDetunePower);
    oscillator_->plug(detune_range, SynthOscillator::kDetuneRange);
    oscillator_->plug(stereo_spread, SynthOscillator::kStereoSpread);
    oscillator_->plug(frame_spread, SynthOscillator::kUnisonFrameSpread);
    oscillator_->plug(distortion_spread, SynthOscillator::kUnisonDistortionSpread);
    oscillator_->plug(distortion_type_, SynthOscillator::kDistortionType);
    oscillator_->plug(distortion_amount, SynthOscillator::kDistortionAmount);
    oscillator_->plug(spectral_morph_spread, SynthOscillator::kUnisonSpectralMorphSpread);
    oscillator_->plug(spectral_morph_type, SynthOscillator::kSpectralMorphType);
    oscillator_->plug(spectral_morph_amount, SynthOscillator::kSpectralMorphAmount);
    oscillator_->useOutput(output(kRaw), SynthOscillator::kRaw);
    oscillator_->useOutput(output(kLevelled), SynthOscillator::kLevelled);

    addProcessor(oscillator_);
    SynthModule::init();
  }

  void OscillatorModule::process(int num_samples) {
    bool on = on_->value();

    if (on)
      SynthModule::process(num_samples);
    else if (*was_on_) {
      output(kRaw)->clearBuffer();
      output(kLevelled)->clearBuffer();
    }

    *was_on_ = on;
  }
} // namespace vital
