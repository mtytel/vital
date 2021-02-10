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

#include "filter_response.h"

#include "comb_filter.h"
#include "skin.h"
#include "synth_constants.h"
#include "shaders.h"
#include "synth_slider.h"
#include "utils.h"

namespace {
  FilterResponse::FilterShader getShaderForModel(vital::constants::FilterModel model, int style) {
    switch (model) {
      case vital::constants::kAnalog:
        return FilterResponse::kAnalog;
      case vital::constants::kComb: {
        vital::CombFilter::FeedbackStyle feedback_style = vital::CombFilter::getFeedbackStyle(style);
        if (feedback_style == vital::CombFilter::kComb)
          return FilterResponse::kComb;
        if (feedback_style == vital::CombFilter::kPositiveFlange)
          return FilterResponse::kPositiveFlange;
        return FilterResponse::kNegativeFlange;
      }
      case vital::constants::kDiode:
        return FilterResponse::kDiode;
      case vital::constants::kDirty:
        return FilterResponse::kDirty;
      case vital::constants::kLadder:
        return FilterResponse::kLadder;
      case vital::constants::kPhase:
        return FilterResponse::kPhase;
      case vital::constants::kFormant:
        return FilterResponse::kFormant;
      case vital::constants::kDigital:
        return FilterResponse::kDigital;
      default:
        VITAL_ASSERT(false);
        return FilterResponse::kNumFilterShaders;
    }
  }

  Shaders::VertexShader getVertexShader(FilterResponse::FilterShader shader) {
    switch (shader) {
      case FilterResponse::kAnalog:
        return Shaders::kAnalogFilterResponseVertex;
      case FilterResponse::kComb:
        return Shaders::kCombFilterResponseVertex;
      case FilterResponse::kPositiveFlange:
        return Shaders::kPositiveFlangeFilterResponseVertex;
      case FilterResponse::kNegativeFlange:
        return Shaders::kNegativeFlangeFilterResponseVertex;
      case FilterResponse::kDiode:
        return Shaders::kDiodeFilterResponseVertex;
      case FilterResponse::kDirty:
        return Shaders::kDirtyFilterResponseVertex;
      case FilterResponse::kLadder:
        return Shaders::kLadderFilterResponseVertex;
      case FilterResponse::kPhase:
        return Shaders::kPhaserFilterResponseVertex;
      case FilterResponse::kFormant:
        return Shaders::kFormantFilterResponseVertex;
      case FilterResponse::kDigital:
        return Shaders::kDigitalFilterResponseVertex;
      default:
        VITAL_ASSERT(false);
        return Shaders::kNumVertexShaders;
    }
  }

  std::pair<vital::Output*, vital::Output*> getOutputs(const vital::output_map& mono_modulations,
                                                       const std::string& name) {
    return {
      mono_modulations.at(name),
      nullptr
    };
  }

  std::pair<vital::Output*, vital::Output*> getOutputs(const vital::output_map& mono_modulations,
                                                       const vital::output_map& poly_modulations,
                                                       const std::string& name) {
    return {
      mono_modulations.at(name),
      poly_modulations.at(name)
    };
  }
} // namespace

FilterResponse::FilterResponse() : OpenGlLineRenderer(kResolution), phaser_filter_(false) {
  setFill(true);
  setFillCenter(-1.0f);
  active_ = false;
  animate_ = false;
  mix_ = 1.0f;
  current_resonance_value_ = 0.0;
  current_cutoff_value_ = 0.0;
  current_formant_x_value_ = 0.0;
  current_formant_y_value_ = 0.0;

  cutoff_slider_ = nullptr;
  resonance_slider_ = nullptr;
  formant_x_slider_ = nullptr;
  formant_y_slider_ = nullptr;
  filter_mix_slider_ = nullptr;
  blend_slider_ = nullptr;
  transpose_slider_ = nullptr;
  formant_transpose_slider_ = nullptr;
  formant_resonance_slider_ = nullptr;
  formant_spread_slider_ = nullptr;
  last_filter_style_ = 0;
  last_filter_model_ = static_cast<vital::constants::FilterModel>(0);
  filter_model_ = static_cast<vital::constants::FilterModel>(0);

  line_data_ = std::make_unique<float[]>(2 * kResolution);
  line_buffer_ = 0;
  response_buffer_ = 0;
  vertex_array_object_ = 0;

  for (int i = 0; i < kResolution; ++i) {
    float t = i / (kResolution - 1.0f);
    line_data_[2 * i] = 2.0f * t - 1.0f;
    line_data_[2 * i + 1] = (i / kCombAlternatePeriod) % 2;
  }

  analog_filter_.setSampleRate(kDefaultVisualSampleRate);
  comb_filter_.setSampleRate(kDefaultVisualSampleRate);
  digital_filter_.setSampleRate(kDefaultVisualSampleRate);
  diode_filter_.setSampleRate(kDefaultVisualSampleRate);
  dirty_filter_.setSampleRate(kDefaultVisualSampleRate);
  formant_filter_.setSampleRate(kDefaultVisualSampleRate);
  ladder_filter_.setSampleRate(kDefaultVisualSampleRate);
  phaser_filter_.setSampleRate(kDefaultVisualSampleRate);
}

FilterResponse::FilterResponse(String suffix, const vital::output_map& mono_modulations) : FilterResponse() {
  std::string prefix = std::string("filter_") + suffix.toStdString() +"_";

  filter_mix_outputs_ = getOutputs(mono_modulations, prefix + "mix");
  midi_cutoff_outputs_ = getOutputs(mono_modulations, prefix + "cutoff");
  resonance_outputs_ = getOutputs(mono_modulations, prefix + "resonance");
  blend_outputs_ = getOutputs(mono_modulations, prefix + "blend");
  transpose_outputs_ = getOutputs(mono_modulations, prefix + "blend_transpose");
  interpolate_x_outputs_ = getOutputs(mono_modulations, prefix + "formant_x");
  interpolate_y_outputs_ = getOutputs(mono_modulations, prefix + "formant_y");
  formant_transpose_outputs_ = getOutputs(mono_modulations, prefix + "formant_transpose");
  formant_resonance_outputs_ = getOutputs(mono_modulations, prefix + "formant_resonance");
  formant_spread_outputs_ = getOutputs(mono_modulations, prefix + "formant_spread");
}

FilterResponse::FilterResponse(int index, const vital::output_map& mono_modulations,
                               const vital::output_map& poly_modulations) : FilterResponse() {
  std::string prefix = std::string("filter_") + std::to_string(index) + "_";

  filter_mix_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "mix");
  midi_cutoff_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "cutoff");
  resonance_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "resonance");
  blend_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "blend");
  transpose_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "blend_transpose");
  interpolate_x_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "formant_x");
  interpolate_y_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "formant_y");
  formant_transpose_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "formant_transpose");
  formant_resonance_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "formant_resonance");
  formant_spread_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "formant_spread");
}

FilterResponse::~FilterResponse() { }

void FilterResponse::init(OpenGlWrapper& open_gl) {
  OpenGlLineRenderer::init(open_gl);

  const GLchar* varyings[] = { "response_out" };
  open_gl.context.extensions.glGenVertexArrays(1, &vertex_array_object_);
  open_gl.context.extensions.glBindVertexArray(vertex_array_object_);

  GLsizeiptr data_size = static_cast<GLsizeiptr>(kResolution * sizeof(float));
  open_gl.context.extensions.glGenBuffers(1, &line_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, 2 * data_size, line_data_.get(), GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &response_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, response_buffer_);
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, data_size, nullptr, GL_STATIC_READ);

  for (int i = 0; i < kNumFilterShaders; ++i) {
    Shaders::VertexShader vertex_shader = getVertexShader(static_cast<FilterShader>(i));
    OpenGLShaderProgram* shader = open_gl.shaders->getShaderProgram(vertex_shader, Shaders::kColorFragment, varyings);
    shaders_[i].shader = shader;

    shader->use();
    shaders_[i].position = getAttribute(open_gl, *shader, "position");

    shaders_[i].mix = getUniform(open_gl, *shader, "mix");
    shaders_[i].midi_cutoff = getUniform(open_gl, *shader, "midi_cutoff");
    shaders_[i].resonance = getUniform(open_gl, *shader, "resonance");
    shaders_[i].drive = getUniform(open_gl, *shader, "drive");
    shaders_[i].db24 = getUniform(open_gl, *shader, "db24");

    shaders_[i].formant_cutoff = getUniform(open_gl, *shader, "formant_cutoff");
    shaders_[i].formant_resonance = getUniform(open_gl, *shader, "formant_resonance");
    shaders_[i].formant_spread = getUniform(open_gl, *shader, "formant_spread");
    shaders_[i].formant_low = getUniform(open_gl, *shader, "low");
    shaders_[i].formant_band = getUniform(open_gl, *shader, "band");
    shaders_[i].formant_high = getUniform(open_gl, *shader, "high");

    for (int s = 0; s < FilterResponseShader::kMaxStages; ++s) {
      String stage = String("stage") + String(s);
      shaders_[i].stages[s] = getUniform(open_gl, *shader, stage.toRawUTF8());
    }
  }
}

void FilterResponse::render(OpenGlWrapper& open_gl, bool animate) {
  animate_ = animate;
  drawFilterResponse(open_gl);
  renderCorners(open_gl, animate);
}

void FilterResponse::destroy(OpenGlWrapper& open_gl) {
  OpenGlLineRenderer::destroy(open_gl);

  open_gl.context.extensions.glDeleteBuffers(1, &line_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &response_buffer_);

  vertex_array_object_ = 0;
  line_buffer_ = 0;
  response_buffer_ = 0;

  for (int i = 0; i < kNumFilterShaders; ++i) {
    shaders_[i].shader = nullptr;
    shaders_[i].position = nullptr;

    shaders_[i].mix = nullptr;
    shaders_[i].midi_cutoff = nullptr;
    shaders_[i].resonance = nullptr;
    shaders_[i].drive = nullptr;
    shaders_[i].db24 = nullptr;

    shaders_[i].formant_cutoff = nullptr;
    shaders_[i].formant_resonance = nullptr;
    shaders_[i].formant_spread = nullptr;
    shaders_[i].formant_low = nullptr;
    shaders_[i].formant_band = nullptr;
    shaders_[i].formant_high = nullptr;

    for (int s = 0; s < FilterResponseShader::kMaxStages; ++s)
      shaders_[i].stages[s] = nullptr;
  }
}

void FilterResponse::paintBackground(Graphics& g) {
  g.fillAll(findColour(Skin::kWidgetBackground, true));

  line_left_color_ = findColour(Skin::kWidgetPrimary1, true);
  line_right_color_ = findColour(Skin::kWidgetPrimary2, true);
  line_disabled_color_ = findColour(Skin::kWidgetPrimaryDisabled, true);
  fill_left_color_ = findColour(Skin::kWidgetSecondary1, true);
  fill_right_color_ = findColour(Skin::kWidgetSecondary2, true);
  fill_disabled_color_ = findColour(Skin::kWidgetSecondaryDisabled, true);
}

void FilterResponse::setFilterSettingsFromPosition(Point<int> position) {
  Point<int> delta = position - last_mouse_position_;
  last_mouse_position_ = position;
  double width = getWidth();
  double height = getHeight();
  current_cutoff_value_ += delta.x * cutoff_slider_->getRange().getLength() / width;
  current_formant_x_value_ += delta.x * formant_x_slider_->getRange().getLength() / width;
  current_resonance_value_ -= delta.y * resonance_slider_->getRange().getLength() / height;
  current_formant_y_value_ -= delta.y * formant_y_slider_->getRange().getLength() / height;
  current_cutoff_value_ = cutoff_slider_->getRange().clipValue(current_cutoff_value_);
  current_formant_x_value_ = formant_x_slider_->getRange().clipValue(current_formant_x_value_);
  current_resonance_value_ = resonance_slider_->getRange().clipValue(current_resonance_value_);
  current_formant_y_value_ = formant_y_slider_->getRange().clipValue(current_formant_y_value_);

  if (filter_model_ == vital::constants::kFormant) {
    formant_x_slider_->setValue(current_formant_x_value_);
    formant_x_slider_->showPopup(true);
    formant_y_slider_->setValue(current_formant_y_value_);
    formant_y_slider_->showPopup(false);
  }
  else {
    cutoff_slider_->setValue(current_cutoff_value_);
    cutoff_slider_->showPopup(true);
    resonance_slider_->setValue(current_resonance_value_);
    resonance_slider_->showPopup(false);
  }
}

void FilterResponse::mouseDown(const MouseEvent& e) {
  last_mouse_position_ = e.getPosition();
  current_resonance_value_ = resonance_slider_->getValue();
  current_cutoff_value_ = cutoff_slider_->getValue();
  current_formant_x_value_ = formant_x_slider_->getValue();
  current_formant_y_value_ = formant_y_slider_->getValue();

  if (filter_model_ == vital::constants::kFormant) {
    formant_x_slider_->showPopup(true);
    formant_y_slider_->showPopup(false);
  }
  else {
    cutoff_slider_->showPopup(true);
    resonance_slider_->showPopup(false);
  }
}

void FilterResponse::mouseDrag(const MouseEvent& e) {
  setFilterSettingsFromPosition(e.getPosition());
}

void FilterResponse::mouseExit(const MouseEvent& e) {
  cutoff_slider_->hidePopup(true);
  resonance_slider_->hidePopup(false);
  OpenGlLineRenderer::mouseExit(e);
}

void FilterResponse::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  MouseWheelDetails horizontal_details = wheel;
  horizontal_details.deltaY = 0.0f;
  MouseWheelDetails vertical_details = wheel;
  vertical_details.deltaX = 0.0f;

  if (filter_model_ == vital::constants::kFormant) {
    formant_x_slider_->mouseWheelMove(e, horizontal_details);
    formant_y_slider_->mouseWheelMove(e, vertical_details);
  }
  else {
    cutoff_slider_->mouseWheelMove(e, horizontal_details);
    resonance_slider_->mouseWheelMove(e, vertical_details);
  }
}

inline vital::poly_float FilterResponse::getOutputsTotal(
    std::pair<vital::Output*, vital::Output*> outputs, vital::poly_float default_value) {
  if (!active_ || !animate_ || !outputs.first->owner->enabled())
    return default_value;
  if (outputs.second == nullptr || num_voices_readout_ == nullptr || num_voices_readout_->value()[0] <= 0.0f)
    return outputs.first->trigger_value;
  return outputs.first->trigger_value + outputs.second->trigger_value;
}

bool FilterResponse::setupFilterState(vital::constants::FilterModel model) {
  vital::poly_float midi_cutoff = getOutputsTotal(midi_cutoff_outputs_, cutoff_slider_->getValue());
  midi_cutoff = vital::utils::max(midi_cutoff, 0.0f);
  vital::poly_float mix = getOutputsTotal(filter_mix_outputs_, filter_mix_slider_->getValue());
  mix = vital::utils::clamp(mix, 0.0f, 1.0f);
  vital::poly_float resonance_percent = getOutputsTotal(resonance_outputs_, resonance_slider_->getValue());
  vital::poly_float pass_blend = getOutputsTotal(blend_outputs_, blend_slider_->getValue());
  pass_blend = vital::utils::clamp(pass_blend, 0.0f, 2.0f);
  vital::poly_float transpose = getOutputsTotal(transpose_outputs_, transpose_slider_->getValue());
  vital::poly_float interpolate_x = getOutputsTotal(interpolate_x_outputs_, formant_x_slider_->getValue());
  vital::poly_float interpolate_y = getOutputsTotal(interpolate_y_outputs_, formant_y_slider_->getValue());

  if (model == vital::constants::kFormant) {
    transpose = getOutputsTotal(formant_transpose_outputs_, formant_transpose_slider_->getValue());
    resonance_percent = getOutputsTotal(formant_resonance_outputs_, formant_resonance_slider_->getValue());
    pass_blend = getOutputsTotal(formant_spread_outputs_, formant_spread_slider_->getValue());
  }

  vital::poly_mask equal = vital::constants::kFullMask;
  equal = equal & vital::poly_float::equal(filter_state_.midi_cutoff, midi_cutoff);
  equal = equal & vital::poly_float::equal(mix_, mix);
  equal = equal & vital::poly_float::equal(filter_state_.resonance_percent, resonance_percent);
  equal = equal & vital::poly_float::equal(filter_state_.pass_blend, pass_blend);
  equal = equal & vital::poly_float::equal(filter_state_.transpose, transpose);
  equal = equal & vital::poly_float::equal(filter_state_.interpolate_x, interpolate_x);
  equal = equal & vital::poly_float::equal(filter_state_.interpolate_y, interpolate_y);

  filter_state_.midi_cutoff = midi_cutoff;
  mix_ = mix;
  filter_state_.resonance_percent = resonance_percent;
  filter_state_.pass_blend = pass_blend;
  filter_state_.transpose = transpose;
  filter_state_.interpolate_x = interpolate_x;
  filter_state_.interpolate_y = interpolate_y;

  bool new_type = last_filter_model_ != model || last_filter_style_ != filter_state_.style;
  last_filter_style_ = filter_state_.style;
  last_filter_model_ = model;

  return (~equal).anyMask() || new_type;
}

bool FilterResponse::isStereoState() {
  vital::poly_mask equal = vital::constants::kFullMask;
  equal = equal & vital::poly_float::equal(filter_state_.midi_cutoff,
                                           vital::utils::swapStereo(filter_state_.midi_cutoff));
  equal = equal & vital::poly_float::equal(mix_, vital::utils::swapStereo(mix_));
  equal = equal & vital::poly_float::equal(filter_state_.resonance_percent,
                                           vital::utils::swapStereo(filter_state_.resonance_percent));
  equal = equal & vital::poly_float::equal(filter_state_.pass_blend,
                                           vital::utils::swapStereo(filter_state_.pass_blend));
  equal = equal & vital::poly_float::equal(filter_state_.transpose,
                                           vital::utils::swapStereo(filter_state_.transpose));
  equal = equal & vital::poly_float::equal(filter_state_.interpolate_x,
                                           vital::utils::swapStereo(filter_state_.interpolate_x));
  equal = equal & vital::poly_float::equal(filter_state_.interpolate_y,
                                           vital::utils::swapStereo(filter_state_.interpolate_y));

  return (~equal).anyMask();
}

void FilterResponse::loadShader(FilterShader shader, vital::constants::FilterModel model, int index) {
  if (model == vital::constants::kAnalog) {
    analog_filter_.setupFilter(filter_state_);
    shaders_[shader].shader->use();
    float resonance = vital::utils::clamp(analog_filter_.getResonance()[index], 0.0f, 2.0f);
    shaders_[shader].midi_cutoff->set(filter_state_.midi_cutoff[index]);
    shaders_[shader].resonance->set(resonance);
    shaders_[shader].drive->set(analog_filter_.getDrive()[index]);
    shaders_[shader].db24->set(filter_state_.style != vital::SynthFilter::k12Db ? 1.0f : 0.0f);

    shaders_[shader].stages[0]->set(analog_filter_.getLowAmount()[index]);
    shaders_[shader].stages[1]->set(analog_filter_.getBandAmount()[index]);
    shaders_[shader].stages[2]->set(analog_filter_.getHighAmount()[index]);
    shaders_[shader].stages[3]->set(analog_filter_.getLowAmount24(filter_state_.style)[index]);
    shaders_[shader].stages[4]->set(analog_filter_.getHighAmount24(filter_state_.style)[index]);
  }
  else if (model == vital::constants::kComb) {
    comb_filter_.setupFilter(filter_state_);
    shaders_[shader].shader->use();
    float resonance = vital::utils::clamp(comb_filter_.getResonance()[index], -0.99f, 0.99f);
    shaders_[shader].midi_cutoff->set(filter_state_.midi_cutoff[index]);
    shaders_[shader].resonance->set(resonance);
    shaders_[shader].drive->set(comb_filter_.getDrive()[index]);

    shaders_[shader].stages[0]->set(comb_filter_.getLowAmount()[index]);
    shaders_[shader].stages[1]->set(comb_filter_.getHighAmount()[index]);
    shaders_[shader].stages[2]->set(comb_filter_.getFilterMidiCutoff()[index]);
    shaders_[shader].stages[3]->set(comb_filter_.getFilter2MidiCutoff()[index]);
  }
  else if (model == vital::constants::kDigital) {
    digital_filter_.setupFilter(filter_state_);
    shaders_[shader].shader->use();
    float resonance = vital::utils::clamp(digital_filter_.getResonance()[index], 0.0f, 2.0f);
    shaders_[shader].midi_cutoff->set(digital_filter_.getMidiCutoff()[index]);
    shaders_[shader].resonance->set(resonance);
    shaders_[shader].drive->set(digital_filter_.getDrive()[index]);
    shaders_[shader].db24->set(filter_state_.style != vital::SynthFilter::k12Db ? 1.0f : 0.0f);

    shaders_[shader].stages[0]->set(digital_filter_.getLowAmount()[index]);
    shaders_[shader].stages[1]->set(digital_filter_.getBandAmount()[index]);
    shaders_[shader].stages[2]->set(digital_filter_.getHighAmount()[index]);
    shaders_[shader].stages[3]->set(digital_filter_.getLowAmount24(filter_state_.style)[index]);
    shaders_[shader].stages[4]->set(digital_filter_.getHighAmount24(filter_state_.style)[index]);
  }
  else if (model == vital::constants::kDiode) {
    diode_filter_.setupFilter(filter_state_);
    shaders_[shader].shader->use();
    shaders_[shader].midi_cutoff->set(filter_state_.midi_cutoff[index]);
    shaders_[shader].resonance->set(diode_filter_.getResonance()[index]);
    shaders_[shader].drive->set(diode_filter_.getDrive()[index]);
    shaders_[shader].db24->set(diode_filter_.getHighPassAmount()[index]);
    shaders_[shader].stages[0]->set(diode_filter_.getHighPassRatio()[index]);
  }
  else if (model == vital::constants::kDirty) {
    dirty_filter_.setupFilter(filter_state_);
    shaders_[shader].shader->use();
    float resonance = vital::utils::clamp(dirty_filter_.getResonance()[index], 0.0f, 2.0f);
    shaders_[shader].midi_cutoff->set(filter_state_.midi_cutoff[index]);
    shaders_[shader].resonance->set(resonance);
    shaders_[shader].drive->set(dirty_filter_.getDrive()[index]);
    shaders_[shader].db24->set(filter_state_.style != vital::SynthFilter::k12Db ? 1.0f : 0.0f);

    shaders_[shader].stages[0]->set(dirty_filter_.getLowAmount()[index]);
    shaders_[shader].stages[1]->set(dirty_filter_.getBandAmount()[index]);
    shaders_[shader].stages[2]->set(dirty_filter_.getHighAmount()[index]);
    shaders_[shader].stages[3]->set(dirty_filter_.getLowAmount24(filter_state_.style)[index]);
    shaders_[shader].stages[4]->set(dirty_filter_.getHighAmount24(filter_state_.style)[index]);
  }
  else if (model == vital::constants::kFormant) {
    shaders_[shader].shader->use();

    vital::DigitalSvf* formant0 = formant_filter_.getFormant(0);
    vital::DigitalSvf* formant1 = formant_filter_.getFormant(1);
    vital::DigitalSvf* formant2 = formant_filter_.getFormant(2);
    vital::DigitalSvf* formant3 = formant_filter_.getFormant(3);

    formant_filter_.setupFilter(filter_state_);
    shaders_[shader].formant_cutoff->set(formant0->getMidiCutoff()[index], formant1->getMidiCutoff()[index],
                                         formant2->getMidiCutoff()[index], formant3->getMidiCutoff()[index]);
    shaders_[shader].formant_resonance->set(formant0->getResonance()[index], formant1->getResonance()[index],
                                            formant2->getResonance()[index], formant3->getResonance()[index]);
    vital::poly_float drive0 = formant0->getDrive();
    vital::poly_float drive1 = formant1->getDrive();
    vital::poly_float drive2 = formant2->getDrive();
    vital::poly_float drive3 = formant3->getDrive();
    vital::poly_float low0 = formant0->getLowAmount() * drive0;
    vital::poly_float low1 = formant1->getLowAmount() * drive1;
    vital::poly_float low2 = formant2->getLowAmount() * drive2;
    vital::poly_float low3 = formant3->getLowAmount() * drive3;
    vital::poly_float band0 = formant0->getBandAmount() * drive0;
    vital::poly_float band1 = formant1->getBandAmount() * drive1;
    vital::poly_float band2 = formant2->getBandAmount() * drive2;
    vital::poly_float band3 = formant3->getBandAmount() * drive3;
    vital::poly_float high0 = formant0->getHighAmount() * drive0;
    vital::poly_float high1 = formant1->getHighAmount() * drive1;
    vital::poly_float high2 = formant2->getHighAmount() * drive2;
    vital::poly_float high3 = formant3->getHighAmount() * drive3;

    shaders_[shader].formant_low->set(low0[index], low1[index], low2[index], low3[index]);
    shaders_[shader].formant_band->set(band0[index], band1[index], band2[index], band3[index]);
    shaders_[shader].formant_high->set(high0[index], high1[index], high2[index], high3[index]);
  }
  else if (model == vital::constants::kLadder) {
    ladder_filter_.setupFilter(filter_state_);
    shaders_[shader].shader->use();
    shaders_[shader].midi_cutoff->set(filter_state_.midi_cutoff[index]);
    shaders_[shader].resonance->set(ladder_filter_.getResonance()[index]);
    shaders_[shader].drive->set(ladder_filter_.getDrive()[index]);

    for (int s = 0; s < FilterResponseShader::kMaxStages; ++s)
      shaders_[shader].stages[s]->set(ladder_filter_.getStageScale(s)[index]);
  }
  else if (model == vital::constants::kPhase) {
    phaser_filter_.setupFilter(filter_state_);
    shaders_[shader].shader->use();
    shaders_[shader].midi_cutoff->set(filter_state_.midi_cutoff[index]);
    shaders_[shader].resonance->set(phaser_filter_.getResonance()[index]);
    shaders_[shader].db24->set(filter_state_.style != vital::SynthFilter::k12Db ? 1.0f : 0.0f);

    shaders_[shader].stages[0]->set(phaser_filter_.getPeak1Amount()[index]);
    shaders_[shader].stages[1]->set(phaser_filter_.getPeak3Amount()[index]);
    shaders_[shader].stages[2]->set(phaser_filter_.getPeak5Amount()[index]);
  }
  
  shaders_[shader].mix->set(mix_[index]);
}

void FilterResponse::bind(FilterShader shader, OpenGLContext& open_gl_context) {
  open_gl_context.extensions.glBindVertexArray(vertex_array_object_);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);

  OpenGLShaderProgram::Attribute* position = shaders_[shader].position.get();
  open_gl_context.extensions.glVertexAttribPointer(position->attributeID, 2, GL_FLOAT,
                                                  GL_FALSE, 2 * sizeof(float), nullptr);
  open_gl_context.extensions.glEnableVertexAttribArray(position->attributeID);

  open_gl_context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, response_buffer_);
}

void FilterResponse::unbind(FilterShader shader, OpenGLContext& open_gl_context) {
  OpenGLShaderProgram::Attribute* position = shaders_[shader].position.get();
  open_gl_context.extensions.glDisableVertexAttribArray(position->attributeID);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl_context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
}

void FilterResponse::drawFilterResponse(OpenGlWrapper& open_gl) {
  vital::constants::FilterModel model = filter_model_;
  bool new_response = setupFilterState(model);
  new_response = new_response || isStereoState();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  setViewPort(open_gl);

  Colour color_line = line_right_color_;
  Colour color_fill_to = fill_right_color_;
  float fill_fade = findValue(Skin::kWidgetFillFade);
  Colour color_fill_from = color_fill_to.withMultipliedAlpha(1.0f - fill_fade);

  setLineWidth(findValue(Skin::kWidgetLineWidth));
  setFillCenter(findValue(Skin::kWidgetFillCenter));

  FilterShader shader = getShaderForModel(model, filter_state_.style);

  if (active_) {
    if (new_response) {
      bind(shader, open_gl.context);
      loadShader(shader, model, 1);
      renderLineResponse(open_gl);
    }

    setFillColors(color_fill_from, color_fill_to);
    setColor(color_line);
    OpenGlLineRenderer::render(open_gl, animate_);
  }

  color_line = line_left_color_;
  color_fill_to = fill_left_color_;
  if (!active_) {
    color_line = line_disabled_color_;
    color_fill_to = fill_disabled_color_;
  }
  color_fill_from = color_fill_to.withMultipliedAlpha(1.0f - fill_fade);

  if (new_response) {
    bind(shader, open_gl.context);
    loadShader(shader, model, 0);
    renderLineResponse(open_gl);
  }

  setFillColors(color_fill_from, color_fill_to);
  setColor(color_line);
  OpenGlLineRenderer::render(open_gl, animate_);

  unbind(shader, open_gl.context);
  glDisable(GL_BLEND);
  checkGlError();
}

void FilterResponse::renderLineResponse(OpenGlWrapper& open_gl) {
  glEnable(GL_BLEND);
  open_gl.context.extensions.glBeginTransformFeedback(GL_POINTS);
  glDrawArrays(GL_POINTS, 0, kResolution);
  open_gl.context.extensions.glEndTransformFeedback();

  void* buffer = open_gl.context.extensions.glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
                                                             kResolution * sizeof(float), GL_MAP_READ_BIT);

  float* response_data = (float*)buffer;
  float width = getWidth();
  float y_adjust = getHeight() / 2.0f;
  for (int i = 0; i < kResolution; ++i) {
    setXAt(i, width * i / (kResolution - 1.0f));
    setYAt(i, y_adjust * (1.0f - response_data[i]));
  }

  open_gl.context.extensions.glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
  glDisable(GL_BLEND);
}
