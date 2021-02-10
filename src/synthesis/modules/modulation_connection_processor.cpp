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

#include "modulation_connection_processor.h"
#include "futils.h"

namespace vital {

  ModulationConnectionProcessor::ModulationConnectionProcessor(int index) :
      SynthModule(kNumInputs, kNumOutputs), index_(index), polyphonic_(true), current_value_(nullptr),
      bipolar_(nullptr), stereo_(nullptr) {
    setControlRate(true);

    modulation_amount_ = 0.0f;

    destination_scale_ = std::make_shared<mono_float>();
    *destination_scale_ = 0.0f;
    last_destination_scale_ = 0.0f;

    power_ = 0.0f;

    map_generator_ = std::make_shared<LineGenerator>();
    map_generator_->initLinear();
  }

  void ModulationConnectionProcessor::init() {
    std::string bipolar_name = "modulation_" + std::to_string(index_ + 1) + "_bipolar";
    bipolar_ = createBaseControl(bipolar_name);

    std::string stereo_name = "modulation_" + std::to_string(index_ + 1) + "_stereo";
    stereo_ = createBaseControl(stereo_name);

    std::string bypass_name = "modulation_" + std::to_string(index_ + 1) + "_bypass";
    bypass_ = createBaseControl(bypass_name);

    SynthModule::init();
  }

  void ModulationConnectionProcessor::process(int num_samples) {
    const Output* source = input(kModulationInput)->source;
    poly_float modulation_input = source->trigger_value;
    output(kModulationSource)->buffer[0] = modulation_input;

    if (last_destination_scale_ != *destination_scale_)
      modulation_amount_ = 0.0f;
    last_destination_scale_ = *destination_scale_;

    if (isControlRate() || source->isControlRate())
      processControlRate(source);
    else 
      processAudioRate(num_samples, source);
  }

  void ModulationConnectionProcessor::processAudioRate(int num_samples, const Output* source) {
    if (bypass_->value()) {
      output(kModulationOutput)->clearBuffer();
      output(kModulationOutput)->trigger_value = 0.0f;
      return;
    }

    poly_float power = -input(kModulationPower)->at(0);
    bool using_power = (poly_float::notEqual(0.0f, power) | poly_float::notEqual(0.0f, power_)).anyMask();
    bool using_map = !map_generator_->linear();

    if (using_power && using_map)
      processAudioRateRemappedAndMorphed(num_samples, source, power);
    else if (using_power)
      processAudioRateMorphed(num_samples, source, power);
    else if (using_map)
      processAudioRateRemapped(num_samples, source);
    else
      processAudioRateLinear(num_samples, source);

    power_ = power;
  }

  void ModulationConnectionProcessor::processAudioRateLinear(int num_samples, const Output* source) {
    poly_float* dest = output(kModulationOutput)->buffer;
    const poly_float* modulation_source = source->buffer;

    poly_float bipolar_offset = -bipolar_->value() * 0.5f;
    poly_float current_amount = modulation_amount_;
    poly_float stereo_scale = poly_float(1.0f) - (constants::kRightOne * 2.0f * stereo_->value());
    poly_float modulation_amount = utils::clamp(input(kModulationAmount)->at(0), -1.0f, 1.0f) * stereo_scale;
    modulation_amount_ = modulation_amount * (*destination_scale_);
    current_amount = utils::maskLoad(current_amount, modulation_amount_, getResetMask(kReset));
    poly_float delta_amount = (modulation_amount_ - current_amount) * (1.0f / num_samples);

    for (int i = 0; i < num_samples; ++i) {
      current_amount += delta_amount;
      poly_float modulation_value = modulation_source[i];
      dest[i] = (modulation_value + bipolar_offset) * current_amount;
    }

    output(kModulationPreScale)->buffer[0] = (modulation_source[0] + bipolar_offset) * modulation_amount;
    output(kModulationOutput)->trigger_value = dest[0];
  }

  void ModulationConnectionProcessor::processAudioRateMorphed(int num_samples, const Output* source,
                                                              poly_float power) {
    poly_float* dest = output(kModulationOutput)->buffer;
    const poly_float* modulation_source = source->buffer;
    poly_float bipolar_offset = -bipolar_->value() * 0.5f;

    poly_float bipolar = bipolar_->value();
    poly_float polarity_pre_scale = bipolar + 1.0f;
    poly_float polarity_post_scale = -bipolar * 0.5f + 1.0f;
    polarity_post_scale *= poly_float(1.0f) - (constants::kRightOne * 2.0f * stereo_->value());

    poly_float current_amount = modulation_amount_;
    poly_float current_power = power_;

    poly_float modulation_amount = utils::clamp(input(kModulationAmount)->at(0), -1.0f, 1.0f);
    modulation_amount_ = modulation_amount * (*destination_scale_);

    poly_mask reset_mask = getResetMask(kReset);
    current_amount = utils::maskLoad(current_amount, modulation_amount_, reset_mask);
    current_power = utils::maskLoad(current_power, power, reset_mask);

    float sample_inc = 1.0f / num_samples;
    poly_float delta_amount = (modulation_amount_ - current_amount) * sample_inc;
    poly_float delta_power = (power - current_power) * sample_inc;

    for (int i = 0; i < num_samples; ++i) {
      current_amount += delta_amount;
      current_power += delta_power;

      poly_float modulation_value = modulation_source[i];
      poly_float modulation_shift = modulation_value * polarity_pre_scale - bipolar;
      poly_float modulation_abs = poly_float::abs(modulation_shift);
      poly_mask sign_mask = poly_float::sign_mask(modulation_shift);

      poly_float shifted_modulation = futils::powerScale(modulation_abs, current_power);
      poly_float pre_modulation = current_amount * shifted_modulation;
      dest[i] = (pre_modulation ^ sign_mask) * polarity_post_scale;
    }

    output(kModulationPreScale)->buffer[0] = dest[0] * (1.0f / (*destination_scale_));
    output(kModulationOutput)->trigger_value = dest[0];
  }

  void ModulationConnectionProcessor::processAudioRateRemappedAndMorphed(int num_samples, const Output* source,
                                                                         poly_float power) {
    poly_float* dest = output(kModulationOutput)->buffer;
    const poly_float* modulation_source = source->buffer;
    poly_float bipolar_offset = -bipolar_->value() * 0.5f;

    poly_float bipolar = bipolar_->value();
    poly_float polarity_pre_scale = bipolar + 1.0f;
    poly_float polarity_post_scale = -bipolar * 0.5f + 1.0f;
    polarity_post_scale *= poly_float(1.0f) - (constants::kRightOne * 2.0f * stereo_->value());

    poly_float current_amount = modulation_amount_;
    poly_float current_power = power_;

    poly_float modulation_amount = utils::clamp(input(kModulationAmount)->at(0), -1.0f, 1.0f);
    modulation_amount_ = modulation_amount * (*destination_scale_);

    poly_mask reset_mask = getResetMask(kReset);
    current_amount = utils::maskLoad(current_amount, modulation_amount_, reset_mask);
    current_power = utils::maskLoad(current_power, power, reset_mask);

    float sample_inc = 1.0f / num_samples;
    poly_float delta_amount = (modulation_amount_ - current_amount) * sample_inc;
    poly_float delta_power = (power - current_power) * sample_inc;

    mono_float* buffer = map_generator_->getCubicInterpolationBuffer();
    for (int i = 0; i < num_samples; ++i) {
      current_amount += delta_amount;
      current_power += delta_power;
      poly_float modulation_value = modulation_source[i];

      mono_float resolution = map_generator_->resolution();
      poly_float boost = utils::clamp(modulation_value * resolution, 0.0f, resolution);
      poly_int indices = utils::clamp(utils::toInt(boost), 0, resolution - 1);
      poly_float t = boost - utils::toFloat(indices);

      matrix interpolation_matrix = utils::getCatmullInterpolationMatrix(t);
      matrix value_matrix = utils::getValueMatrix(buffer, indices);

      value_matrix.transpose();
      modulation_value = utils::clamp(interpolation_matrix.multiplyAndSumRows(value_matrix), -1.0f, 1.0f);

      poly_float modulation_shift = modulation_value * polarity_pre_scale - bipolar;
      poly_float modulation_abs = poly_float::abs(modulation_shift);
      poly_mask sign_mask = poly_float::sign_mask(modulation_shift);

      poly_float shifted_modulation = futils::powerScale(modulation_abs, current_power);
      poly_float pre_modulation = current_amount * shifted_modulation;
      dest[i] = (pre_modulation ^ sign_mask) * polarity_post_scale;
    }

    output(kModulationPreScale)->buffer[0] = dest[0] * (1.0f / (*destination_scale_));
    output(kModulationOutput)->trigger_value = dest[0];
  }

  void ModulationConnectionProcessor::processAudioRateRemapped(int num_samples, const Output* source) {
    poly_float* dest = output(kModulationOutput)->buffer;
    const poly_float* modulation_source = source->buffer;

    poly_float bipolar_offset = -bipolar_->value() * 0.5f;
    poly_float current_amount = modulation_amount_;
    poly_float stereo_scale = poly_float(1.0f) - (constants::kRightOne * 2.0f * stereo_->value());
    poly_float modulation_amount = utils::clamp(input(kModulationAmount)->at(0), -1.0f, 1.0f) * stereo_scale;
    modulation_amount_ = modulation_amount * (*destination_scale_);
    current_amount = utils::maskLoad(current_amount, modulation_amount_, getResetMask(kReset));
    poly_float delta_amount = (modulation_amount_ - current_amount) * (1.0f / num_samples);

    mono_float* buffer = map_generator_->getCubicInterpolationBuffer();
    for (int i = 0; i < num_samples; ++i) {
      current_amount += delta_amount;
      poly_float modulation_value = modulation_source[i];

      mono_float resolution = map_generator_->resolution();
      poly_float boost = utils::clamp(modulation_value * resolution, 0.0f, resolution);
      poly_int indices = utils::clamp(utils::toInt(boost), 0, resolution - 1);
      poly_float t = boost - utils::toFloat(indices);

      matrix interpolation_matrix = utils::getCatmullInterpolationMatrix(t);
      matrix value_matrix = utils::getValueMatrix(buffer, indices);

      value_matrix.transpose();

      modulation_value = utils::clamp(interpolation_matrix.multiplyAndSumRows(value_matrix), -1.0f, 1.0f);
      dest[i] = (modulation_value + bipolar_offset) * current_amount;
    }

    output(kModulationPreScale)->buffer[0] = (modulation_source[0] + bipolar_offset) * modulation_amount;
    output(kModulationOutput)->trigger_value = dest[0];
  }

  void ModulationConnectionProcessor::processControlRate(const Output* source) {
    if (bypass_->value()) {
      output(kModulationOutput)->buffer[0] = 0.0f;
      output(kModulationOutput)->trigger_value = 0.0f;
      return;
    }

    poly_float modulation_input = utils::clamp(source->buffer[0], 0.0f, 1.0f);

    if (!map_generator_->linear()) {
      mono_float* buffer = map_generator_->getCubicInterpolationBuffer();
      mono_float resolution = map_generator_->resolution();
      poly_float boost = utils::clamp(modulation_input * resolution, 0.0f, resolution);
      poly_int indices = utils::clamp(utils::toInt(boost), 0, resolution - 1);
      poly_float t = boost - utils::toFloat(indices);

      matrix interpolation_matrix = utils::getCatmullInterpolationMatrix(t);
      matrix value_matrix = utils::getValueMatrix(buffer, indices);

      value_matrix.transpose();

      modulation_input = utils::clamp(interpolation_matrix.multiplyAndSumRows(value_matrix), -1.0f, 1.0f);
    }

    poly_float bipolar = bipolar_->value();
    poly_float polarity_pre_scale = bipolar + 1.0f;
    poly_float polarity_post_scale = -bipolar * 0.5f + 1.0f;
    polarity_post_scale *= poly_float(1.0f) - (constants::kRightOne * 2.0f * stereo_->value());

    poly_float modulation_shift = modulation_input * polarity_pre_scale - bipolar;
    poly_float modulation_abs = poly_float::abs(modulation_shift);
    poly_mask sign_mask = poly_float::sign_mask(modulation_shift);

    poly_float power = -input(kModulationPower)->at(0);
    poly_float shifted_modulation = futils::powerScale(modulation_abs, power);
    poly_float modulation_amount = utils::clamp(input(kModulationAmount)->at(0), -1.0f, 1.0f);
    poly_float pre_modulation = modulation_amount * shifted_modulation;
    poly_float raw_modulation = (pre_modulation ^ sign_mask) * polarity_post_scale;
    output(kModulationPreScale)->buffer[0] = raw_modulation;
    output(kModulationOutput)->buffer[0] = raw_modulation * (*destination_scale_);
    VITAL_ASSERT(utils::isFinite(output()->buffer[0]));
  }
} // namespace vital
