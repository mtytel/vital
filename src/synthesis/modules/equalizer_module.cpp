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

#include "equalizer_module.h"

#include "digital_svf.h"
#include "value.h"

namespace vital {

  EqualizerModule::EqualizerModule() :
      SynthModule(0, 1),
      low_mode_(nullptr), band_mode_(nullptr), high_mode_(nullptr),
      high_pass_(nullptr), low_shelf_(nullptr),
      notch_(nullptr), band_shelf_(nullptr),
      low_pass_(nullptr), high_shelf_(nullptr) {
    audio_memory_ = std::make_shared<vital::StereoMemory>(vital::kAudioMemorySamples);
  }

  void EqualizerModule::init() {
    static const cr::Value kPass(DigitalSvf::k12Db);
    static const cr::Value kNotch(DigitalSvf::kNotchPassSwap);
    static const cr::Value kShelving(DigitalSvf::kShelving);

    high_pass_ = new DigitalSvf();
    low_shelf_ = new DigitalSvf();
    band_shelf_ = new DigitalSvf();
    notch_ = new DigitalSvf();
    low_pass_ = new DigitalSvf();
    high_shelf_ = new DigitalSvf();

    high_pass_->setDriveCompensation(false);
    high_pass_->setBasic(true);
    notch_->setDriveCompensation(false);
    notch_->setBasic(true);
    low_pass_->setDriveCompensation(false);
    low_pass_->setBasic(true);

    addIdleProcessor(high_pass_);
    addIdleProcessor(low_shelf_);
    addIdleProcessor(notch_);
    addIdleProcessor(band_shelf_);
    addIdleProcessor(low_pass_);
    addIdleProcessor(high_shelf_);
    low_pass_->useOutput(output());
    high_shelf_->useOutput(output());

    low_mode_ = createBaseControl("eq_low_mode");
    band_mode_ = createBaseControl("eq_band_mode");
    high_mode_ = createBaseControl("eq_high_mode");

    Output* low_cutoff_midi = createMonoModControl("eq_low_cutoff", true, true);
    Output* band_cutoff_midi = createMonoModControl("eq_band_cutoff", true, true);
    Output* high_cutoff_midi = createMonoModControl("eq_high_cutoff", true, true);

    Output* low_resonance = createMonoModControl("eq_low_resonance");
    Output* band_resonance = createMonoModControl("eq_band_resonance");
    Output* high_resonance = createMonoModControl("eq_high_resonance");

    Output* low_decibels = createMonoModControl("eq_low_gain");
    Output* band_decibels = createMonoModControl("eq_band_gain");
    Output* high_decibels = createMonoModControl("eq_high_gain");

    high_pass_->plug(&kPass, DigitalSvf::kStyle);
    high_pass_->plug(&constants::kValueTwo, DigitalSvf::kPassBlend);
    high_pass_->plug(low_cutoff_midi, DigitalSvf::kMidiCutoff);
    high_pass_->plug(low_resonance, DigitalSvf::kResonance);

    low_shelf_->plug(&kShelving, DigitalSvf::kStyle);
    low_shelf_->plug(&constants::kValueZero, DigitalSvf::kPassBlend);
    low_shelf_->plug(low_cutoff_midi, DigitalSvf::kMidiCutoff);
    low_shelf_->plug(low_resonance, DigitalSvf::kResonance);
    low_shelf_->plug(low_decibels, DigitalSvf::kGain);

    band_shelf_->plug(&kShelving, DigitalSvf::kStyle);
    band_shelf_->plug(&constants::kValueOne, DigitalSvf::kPassBlend);
    band_shelf_->plug(band_cutoff_midi, DigitalSvf::kMidiCutoff);
    band_shelf_->plug(band_resonance, DigitalSvf::kResonance);
    band_shelf_->plug(band_decibels, DigitalSvf::kGain);

    notch_->plug(&kNotch, DigitalSvf::kStyle);
    notch_->plug(&constants::kValueOne, DigitalSvf::kPassBlend);
    notch_->plug(band_cutoff_midi, DigitalSvf::kMidiCutoff);
    notch_->plug(band_resonance, DigitalSvf::kResonance);

    low_pass_->plug(&kPass, DigitalSvf::kStyle);
    low_pass_->plug(&constants::kValueZero, DigitalSvf::kPassBlend);
    low_pass_->plug(high_cutoff_midi, DigitalSvf::kMidiCutoff);
    low_pass_->plug(high_resonance, DigitalSvf::kResonance);

    high_shelf_->plug(&kShelving, DigitalSvf::kStyle);
    high_shelf_->plug(&constants::kValueTwo, DigitalSvf::kPassBlend);
    high_shelf_->plug(high_cutoff_midi, DigitalSvf::kMidiCutoff);
    high_shelf_->plug(high_resonance, DigitalSvf::kResonance);
    high_shelf_->plug(high_decibels, DigitalSvf::kGain);
    
    SynthModule::init();
  }

  void EqualizerModule::hardReset() {
    high_pass_->reset(constants::kFullMask);
    low_shelf_->reset(constants::kFullMask);
    band_shelf_->reset(constants::kFullMask);
    notch_->reset(constants::kFullMask);
    low_pass_->reset(constants::kFullMask);
    high_shelf_->reset(constants::kFullMask);
  }

  void EqualizerModule::enable(bool enable) {
    SynthModule::enable(enable);
    process(1);
    if (enable) {
      high_pass_->reset(constants::kFullMask);
      low_shelf_->reset(constants::kFullMask);
      band_shelf_->reset(constants::kFullMask);
      notch_->reset(constants::kFullMask);
      low_pass_->reset(constants::kFullMask);
      high_shelf_->reset(constants::kFullMask);
    }
  }

  void EqualizerModule::setSampleRate(int sample_rate) {
    high_pass_->setSampleRate(sample_rate);
    low_shelf_->setSampleRate(sample_rate);
    band_shelf_->setSampleRate(sample_rate);
    notch_->setSampleRate(sample_rate);
    low_pass_->setSampleRate(sample_rate);
    high_shelf_->setSampleRate(sample_rate);
  }

  void EqualizerModule::processWithInput(const poly_float* audio_in, int num_samples) {
    SynthModule::process(num_samples);

    Processor* low_processor = low_mode_->value() ? high_pass_ : low_shelf_;
    Processor* band_processor = band_mode_->value() ? notch_ : band_shelf_;
    Processor* high_processor = high_mode_->value() ? low_pass_ : high_shelf_;

    low_processor->processWithInput(audio_in, num_samples);
    band_processor->processWithInput(low_processor->output()->buffer, num_samples);
    high_processor->processWithInput(band_processor->output()->buffer, num_samples);

    const poly_float* output_buffer = high_processor->output()->buffer;
    for (int i = 0; i < num_samples; ++i)
      audio_memory_->push(output_buffer[i]);
  }
} // namespace vital
