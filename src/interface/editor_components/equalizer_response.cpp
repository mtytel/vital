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

#include "equalizer_response.h"

#include "shaders.h"
#include "skin.h"
#include "utils.h"

EqualizerResponse::EqualizerResponse() : OpenGlLineRenderer(kResolution),
                                         unselected_points_(2, Shaders::kRingFragment),
                                         selected_point_(Shaders::kCircleFragment),
                                         dragging_point_(Shaders::kCircleFragment), shader_(nullptr) {
  unselected_points_.setThickness(1.0f);
  setFill(true);

  addAndMakeVisible(unselected_points_);
  addAndMakeVisible(selected_point_);
  addAndMakeVisible(dragging_point_);

  low_cutoff_ = nullptr;
  low_resonance_ = nullptr;
  low_gain_ = nullptr;
  band_cutoff_ = nullptr;
  band_resonance_ = nullptr;
  band_gain_ = nullptr;
  high_cutoff_ = nullptr;
  high_resonance_ = nullptr;
  high_gain_ = nullptr;
  
  low_cutoff_output_ = nullptr;
  low_resonance_output_ = nullptr;
  low_gain_output_ = nullptr;
  band_cutoff_output_ = nullptr;
  band_resonance_output_ = nullptr;
  band_gain_output_ = nullptr;
  high_cutoff_output_ = nullptr;
  high_resonance_output_ = nullptr;
  high_gain_output_ = nullptr;

  current_cutoff_ = nullptr;
  current_gain_ = nullptr;

  low_filter_.setSampleRate(kViewSampleRate);
  band_filter_.setSampleRate(kViewSampleRate);
  high_filter_.setSampleRate(kViewSampleRate);
  low_filter_.setDriveCompensation(false);
  high_filter_.setDriveCompensation(false);

  active_ = false;
  high_pass_ = false;
  notch_ = false;
  low_pass_ = false;
  animate_ = true;
  draw_frequency_lines_ = true;
  selected_band_ = 0;

  line_data_ = std::make_unique<float[]>(kResolution);
  line_buffer_ = 0;
  response_buffer_ = 0;
  vertex_array_object_ = 0;

  for (int i = 0; i < kResolution; ++i) {
    float t = i / (kResolution - 1.0f);
    line_data_[i] = 2.0f * t - 1.0f;
  }
  
  db_buffer_ratio_ = kDefaultDbBufferRatio;
  min_db_ = 0.0f;
  max_db_ = 1.0f;
}

EqualizerResponse::~EqualizerResponse() = default;

void EqualizerResponse::initEq(const vital::output_map& mono_modulations) {
  low_cutoff_output_ = mono_modulations.at("eq_low_cutoff");
  low_resonance_output_ = mono_modulations.at("eq_low_resonance");
  low_gain_output_ = mono_modulations.at("eq_low_gain");
  band_cutoff_output_ = mono_modulations.at("eq_band_cutoff");
  band_resonance_output_ = mono_modulations.at("eq_band_resonance");
  band_gain_output_ = mono_modulations.at("eq_band_gain");
  high_cutoff_output_ = mono_modulations.at("eq_high_cutoff");
  high_resonance_output_ = mono_modulations.at("eq_high_resonance");
  high_gain_output_ = mono_modulations.at("eq_high_gain");
}

void EqualizerResponse::initReverb(const vital::output_map& mono_modulations) {
  low_cutoff_output_ = mono_modulations.at("reverb_low_shelf_cutoff");
  low_gain_output_ = mono_modulations.at("reverb_low_shelf_gain");
  high_cutoff_output_ = mono_modulations.at("reverb_high_shelf_cutoff");
  high_gain_output_ = mono_modulations.at("reverb_high_shelf_gain");
}

void EqualizerResponse::init(OpenGlWrapper& open_gl) {
  OpenGlLineRenderer::init(open_gl);

  unselected_points_.init(open_gl);
  selected_point_.init(open_gl);
  dragging_point_.init(open_gl);

  open_gl.context.extensions.glGenVertexArrays(1, &vertex_array_object_);
  open_gl.context.extensions.glBindVertexArray(vertex_array_object_);

  GLsizeiptr vert_size = static_cast<GLsizeiptr>(kResolution * sizeof(float));
  open_gl.context.extensions.glGenBuffers(1, &line_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, line_data_.get(), GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &response_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, response_buffer_);
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, nullptr, GL_STATIC_READ);

  const GLchar* varyings[] = { "response_out" };
  shader_ = open_gl.shaders->getShaderProgram(Shaders::kEqFilterResponseVertex, Shaders::kColorFragment, varyings);
  shader_->use();

  position_attribute_ = getAttribute(open_gl, *shader_, "position");
  midi_cutoff_uniform_ = getUniform(open_gl, *shader_, "midi_cutoff");
  resonance_uniform_ = getUniform(open_gl, *shader_, "resonance");
  low_amount_uniform_ = getUniform(open_gl, *shader_, "low_amount");
  band_amount_uniform_ = getUniform(open_gl, *shader_, "band_amount");
  high_amount_uniform_ = getUniform(open_gl, *shader_, "high_amount");
}

void EqualizerResponse::drawResponse(OpenGlWrapper& open_gl, int index) {
  glEnable(GL_BLEND);
  setLineWidth(findValue(Skin::kWidgetLineWidth));
  setFillCenter(1.0f - 2.0f * max_db_ / (max_db_ - min_db_));

  Colour color_line = findColour(Skin::kWidgetPrimary1, true);
  Colour color_fill_to = findColour(Skin::kWidgetSecondary1, true);

  if (!active_) {
    color_line = findColour(Skin::kWidgetPrimaryDisabled, true);
    color_fill_to = findColour(Skin::kWidgetSecondaryDisabled, true);
  }
  else if (index) {
    color_line = findColour(Skin::kWidgetPrimary2, true);
    color_fill_to = findColour(Skin::kWidgetSecondary2, true);
  }

  setColor(color_line);
  float fill_fade = findValue(Skin::kWidgetFillFade);
  setFillColors(color_fill_to.withMultipliedAlpha(1.0f - fill_fade), color_fill_to);

  shader_->use();
  open_gl.context.extensions.glBindVertexArray(vertex_array_object_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);
  open_gl.context.extensions.glVertexAttribPointer(position_attribute_->attributeID, 1, GL_FLOAT, GL_FALSE,
                                                  sizeof(float), nullptr);
  open_gl.context.extensions.glEnableVertexAttribArray(position_attribute_->attributeID);
  open_gl.context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, response_buffer_);

  midi_cutoff_uniform_->set(low_filter_.getMidiCutoff()[index],
                            band_filter_.getMidiCutoff()[index],
                            high_filter_.getMidiCutoff()[index]);
  resonance_uniform_->set(low_filter_.getResonance()[index],
                          band_filter_.getResonance()[index],
                          high_filter_.getResonance()[index]);

  low_amount_uniform_->set(low_filter_.getLowAmount()[index],
                           band_filter_.getLowAmount()[index],
                           high_filter_.getLowAmount()[index]);
  band_amount_uniform_->set(low_filter_.getBandAmount()[index],
                            band_filter_.getBandAmount()[index],
                            high_filter_.getBandAmount()[index]);
  high_amount_uniform_->set(low_filter_.getHighAmount()[index],
                            band_filter_.getHighAmount()[index],
                            high_filter_.getHighAmount()[index]);

  open_gl.context.extensions.glBeginTransformFeedback(GL_POINTS);
  glDrawArrays(GL_POINTS, 0, kResolution);
  open_gl.context.extensions.glEndTransformFeedback();

  void* buffer = open_gl.context.extensions.glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
                                                             kResolution * sizeof(float), GL_MAP_READ_BIT);

  float* response_data = (float*)buffer;
  float width = getWidth();
  float range = max_db_ - min_db_;
  float y_mult = getHeight() / range;
  for (int i = 0; i < kResolution; ++i) {
    setXAt(i, i * width / (kResolution - 1.0f));
    setYAt(i, (max_db_ - response_data[i]) * y_mult);
  }

  open_gl.context.extensions.glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

  OpenGlLineRenderer::render(open_gl, animate_);
}

void EqualizerResponse::render(OpenGlWrapper& open_gl, bool animate) {
  animate_ = animate;
  computeFilterCoefficients();
  if (active_ && animate_)
    drawResponse(open_gl, 1);
  drawResponse(open_gl, 0);

  open_gl.context.extensions.glDisableVertexAttribArray(position_attribute_->attributeID);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl.context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

  checkGlError();

  drawControlPoints(open_gl);
  renderCorners(open_gl, animate);
}

void EqualizerResponse::destroy(OpenGlWrapper& open_gl) {
  OpenGlLineRenderer::destroy(open_gl);

  unselected_points_.destroy(open_gl);
  selected_point_.destroy(open_gl);
  dragging_point_.destroy(open_gl);

  open_gl.context.extensions.glDeleteBuffers(1, &line_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &response_buffer_);
  line_buffer_ = 0;
  response_buffer_ = 0;

  shader_ = nullptr;
  position_attribute_ = nullptr;
  midi_cutoff_uniform_ = nullptr;
  resonance_uniform_ = nullptr;
  low_amount_uniform_ = nullptr;
  band_amount_uniform_ = nullptr;
  high_amount_uniform_ = nullptr;
}

void EqualizerResponse::setControlPointBounds(float selected_x, float selected_y,
                                              float unselected_x1, float unselected_y1,
                                              float unselected_x2, float unselected_y2) {
  static constexpr float kHandleRadius = 0.06f;
  static constexpr float kDraggingRadius = 0.18f;

  float width = getWidth();
  float height = getHeight();
  float handle_radius = kHandleRadius * height;
  float handle_width = handle_radius * 4.0f / width;
  float handle_height = handle_radius * 4.0f / height;

  float dragging_radius = kDraggingRadius * height;
  float dragging_width = dragging_radius * 4.0f / width;
  float dragging_height = dragging_radius * 4.0f / height;

  selected_point_.setQuad(0, selected_x - handle_width * 0.5f, selected_y - handle_height * 0.5f,
                          handle_width, handle_height);
  dragging_point_.setQuad(0, selected_x - dragging_width * 0.5f, selected_y - dragging_height * 0.5f,
                          dragging_width, dragging_height);
  unselected_points_.setQuad(0, unselected_x1 - handle_width * 0.5f, unselected_y1 - handle_height * 0.5f,
                             handle_width, handle_height);
  unselected_points_.setQuad(1, unselected_x2 - handle_width * 0.5f, unselected_y2 - handle_height * 0.5f,
                             handle_width, handle_height);
}

void EqualizerResponse::drawControlPoints(OpenGlWrapper& open_gl) {

  Point<float> low_position = getLowPosition();
  Point<float> band_position = getBandPosition();
  Point<float> high_position = getHighPosition();

  float width = getWidth();
  float height = getHeight();

  float low_x = 2.0f * low_position.x / width - 1.0f;
  float band_x = 2.0f * band_position.x / width - 1.0f;
  float high_x = 2.0f * high_position.x / width - 1.0f;
  float low_y = 1.0f - 2.0f * low_position.y / height;
  float band_y = 1.0f - 2.0f * band_position.y / height;
  float high_y = 1.0f - 2.0f * high_position.y / height;

  if (band_cutoff_output_ == nullptr)
    band_x = -2.0f;

  if (selected_band_ == 0)
    setControlPointBounds(low_x, low_y, band_x, band_y, high_x, high_y);
  else if (band_cutoff_output_ && selected_band_ == 1)
    setControlPointBounds(band_x, band_y, low_x, low_y, high_x, high_y);
  else if (selected_band_ == 2)
    setControlPointBounds(high_x, high_y, low_x, low_y, band_x, band_y);

  dragging_point_.setColor(findColour(Skin::kLightenScreen, true));
  if (current_cutoff_ && current_gain_)
    dragging_point_.render(open_gl, true);
  selected_point_.setColor(findColour(Skin::kWidgetPrimary1, true));
  selected_point_.render(open_gl, true);
  unselected_points_.setColor(findColour(Skin::kWidgetPrimary1, true));
  unselected_points_.render(open_gl, true);
}

void EqualizerResponse::paintBackground(Graphics& g) {
  static constexpr int kLineSpacing = 10;

  OpenGlLineRenderer::paintBackground(g);
  if (!draw_frequency_lines_)
    return;

  float min_frequency = vital::utils::midiNoteToFrequency(low_cutoff_->getMinimum());
  float max_frequency = vital::utils::midiNoteToFrequency(low_cutoff_->getMaximum());

  int height = getHeight();
  float max_octave = log2f(max_frequency / min_frequency);
  g.setColour(findColour(Skin::kLightenScreen, true).withMultipliedAlpha(0.5f));
  float frequency = 0.0f;
  float increment = 1.0f;

  int x = 0;
  while (frequency < max_frequency) {
    for (int i = 0; i < kLineSpacing; ++i) {
      frequency += increment;
      float t = log2f(frequency / min_frequency) / max_octave;
      x = std::round(t * getWidth());
      g.fillRect(x, 0, 1, height);
    }
    g.fillRect(x, 0, 1, height);
    increment *= kLineSpacing;
  }
}

void EqualizerResponse::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  int band = getHoveredBand(e);

  if (band == 0 && low_resonance_)
    low_resonance_->mouseWheelMove(e, wheel);
  else if (band == 1 && band_resonance_)
    band_resonance_->mouseWheelMove(e, wheel);
  else if (band == 2 && high_resonance_)
    high_resonance_->mouseWheelMove(e, wheel);
  else
    OpenGlComponent::mouseWheelMove(e, wheel);
}

void EqualizerResponse::mouseDown(const MouseEvent& e) {
  selected_band_ = getHoveredBand(e);

  if (selected_band_ == 0) {
    current_cutoff_ = low_cutoff_;
    current_gain_ = low_gain_;
    for (Listener* listener : listeners_)
      listener->lowBandSelected();
  }
  else if (selected_band_ == 1) {
    current_cutoff_ = band_cutoff_;
    current_gain_ = band_gain_;
    for (Listener* listener : listeners_)
      listener->midBandSelected();
  }
  else if (selected_band_ == 2) {
    current_cutoff_ = high_cutoff_;
    current_gain_ = high_gain_;
    for (Listener* listener : listeners_)
      listener->highBandSelected();
  }

  OpenGlLineRenderer::mouseDown(e);
}

void EqualizerResponse::mouseDrag(const MouseEvent& e) {
  moveFilterSettings(e.position);
  OpenGlLineRenderer::mouseDrag(e);
}

void EqualizerResponse::mouseUp(const MouseEvent& e) {
  if (current_cutoff_ == nullptr && current_gain_ == nullptr) {
    OpenGlLineRenderer::mouseUp(e);
    return;
  }

  current_cutoff_ = nullptr;
  current_gain_ = nullptr;
  OpenGlLineRenderer::mouseUp(e);
}

void EqualizerResponse::mouseExit(const MouseEvent& e) {
  if (low_cutoff_) {
    low_cutoff_->hidePopup(true);
    low_cutoff_->hidePopup(false);
  }
  OpenGlLineRenderer::mouseExit(e);
}

int EqualizerResponse::getHoveredBand(const MouseEvent& e) {
  const float grab_distance = 0.06f * getWidth();
  
  float delta_low = e.position.getDistanceSquaredFrom(getLowPosition());
  float delta_band = e.position.getDistanceSquaredFrom(getBandPosition());
  float delta_high = e.position.getDistanceSquaredFrom(getHighPosition());

  float min_sqr_dist = grab_distance * grab_distance;
  float min = std::min(std::min(min_sqr_dist, delta_low), delta_high);
  if (band_cutoff_output_)
    min = std::min(min, delta_band);

  if (delta_low <= min)
    return 0;
  if (band_cutoff_output_ && delta_band <= min)
    return 1;
  if (delta_high <= min)
    return 2;

  return -1;
}

Point<float> EqualizerResponse::getLowPosition() {
  float cutoff_range = low_cutoff_->getMaximum() - low_cutoff_->getMinimum();
  float min_cutoff = low_cutoff_->getMinimum();
  float gain_range = max_db_ - min_db_;

  float low_x = getWidth() * (low_cutoff_->getValue() - min_cutoff) / cutoff_range;
  float low_y = getHeight() * (max_db_ - low_gain_->getValue()) / gain_range;
  return Point<float>(low_x, low_y);
}

Point<float> EqualizerResponse::getBandPosition() {
  if (band_cutoff_ == nullptr)
    return Point<float>(0.0f, 0.0f);
  
  float cutoff_range = band_cutoff_->getMaximum() - band_cutoff_->getMinimum();
  float min_cutoff = band_cutoff_->getMinimum();
  float gain_range = max_db_ - min_db_;

  float band_x = getWidth() * (band_cutoff_->getValue() - min_cutoff) / cutoff_range;
  float band_y = getHeight() * (max_db_ - band_gain_->getValue()) / gain_range;
  return Point<float>(band_x, band_y);
}

Point<float> EqualizerResponse::getHighPosition() {
  float cutoff_range = high_cutoff_->getMaximum() - high_cutoff_->getMinimum();
  float min_cutoff = high_cutoff_->getMinimum();
  float gain_range = max_db_ - min_db_;

  float high_x = getWidth() * (high_cutoff_->getValue() - min_cutoff) / cutoff_range;
  float high_y = getHeight() * (max_db_ - high_gain_->getValue()) / gain_range;
  return Point<float>(high_x, high_y);
}

void EqualizerResponse::computeFilterCoefficients() {
  low_filter_state_.midi_cutoff = getOutputTotal(low_cutoff_output_, low_cutoff_);
  low_filter_state_.resonance_percent = getOutputTotal(low_resonance_output_, low_resonance_);
  low_filter_state_.gain = getOutputTotal(low_gain_output_, low_gain_);
  if (high_pass_) {
    low_filter_state_.style = vital::DigitalSvf::k12Db;
    low_filter_state_.pass_blend = 2.0f;
  }
  else {
    low_filter_state_.style = vital::DigitalSvf::kShelving;
    low_filter_state_.pass_blend = 0.0f;
  }
  low_filter_.setupFilter(low_filter_state_);

  band_filter_state_.midi_cutoff = getOutputTotal(band_cutoff_output_, band_cutoff_);
  band_filter_state_.resonance_percent = getOutputTotal(band_resonance_output_, band_resonance_);
  band_filter_state_.gain = getOutputTotal(band_gain_output_, band_gain_);
  band_filter_state_.pass_blend = 1.0f;
  if (notch_)
    band_filter_state_.style = vital::DigitalSvf::kNotchPassSwap;
  else
    band_filter_state_.style = vital::DigitalSvf::kShelving;
  band_filter_.setupFilter(band_filter_state_);
  
  high_filter_state_.midi_cutoff = getOutputTotal(high_cutoff_output_, high_cutoff_);
  high_filter_state_.resonance_percent = getOutputTotal(high_resonance_output_, high_resonance_);
  high_filter_state_.gain = getOutputTotal(high_gain_output_, high_gain_);
  if (low_pass_) {
    high_filter_state_.style = vital::DigitalSvf::k12Db;
    high_filter_state_.pass_blend = 0.0f;
  }
  else {
    high_filter_state_.style = vital::DigitalSvf::kShelving;
    high_filter_state_.pass_blend = 2.0f;
  }

  high_filter_.setupFilter(high_filter_state_);
}

void EqualizerResponse::moveFilterSettings(Point<float> position) {
  if (current_cutoff_) {
    float ratio = vital::utils::clamp(position.x / getWidth(), 0.0f, 1.0f);
    float min = current_cutoff_->getMinimum();
    float max = current_cutoff_->getMaximum();
    float new_cutoff = ratio * (max - min) + min;
    current_cutoff_->showPopup(true);
    current_cutoff_->setValue(new_cutoff);
  }
  if (current_gain_) {
    float local_position = position.y - 0.5f * db_buffer_ratio_ * getHeight();
    float ratio = vital::utils::clamp(local_position / ((1.0f - db_buffer_ratio_) * getHeight()), 0.0f, 1.0f);
    float min = current_gain_->getMinimum();
    float max = current_gain_->getMaximum();
    float new_db = ratio * (min - max) + max;
    current_gain_->setValue(new_db);
    current_gain_->showPopup(false);
  }
  else
    low_gain_->hidePopup(false);
}

void EqualizerResponse::setLowSliders(SynthSlider *cutoff,
                                      SynthSlider *resonance, SynthSlider *gain) {
  float buffer = gain->getRange().getLength() * db_buffer_ratio_;
  min_db_ = gain->getMinimum() - buffer;
  max_db_ = gain->getMaximum() + buffer;
  low_cutoff_ = cutoff;
  low_resonance_ = resonance;
  low_gain_ = gain;
  low_cutoff_->addSliderListener(this);
  if (low_resonance_)
    low_resonance_->addSliderListener(this);
  low_gain_->addSliderListener(this);
  repaint();
}

void EqualizerResponse::setBandSliders(SynthSlider *cutoff,
                                       SynthSlider *resonance, SynthSlider *gain) {
  band_cutoff_ = cutoff;
  band_resonance_ = resonance;
  band_gain_ = gain;
  band_cutoff_->addSliderListener(this);
  if (band_resonance_)
    band_resonance_->addSliderListener(this);
  band_gain_->addSliderListener(this);
  repaint();
}

void EqualizerResponse::setHighSliders(SynthSlider *cutoff,
                                       SynthSlider *resonance, SynthSlider *gain) {
  high_cutoff_ = cutoff;
  high_resonance_ = resonance;
  high_gain_ = gain;
  high_cutoff_->addSliderListener(this);
  if (high_resonance_)
    high_resonance_->addSliderListener(this);
  high_gain_->addSliderListener(this);
  repaint();
}

void EqualizerResponse::setSelectedBand(int selected_band) {
  selected_band_ = selected_band;
  repaint();
}

void EqualizerResponse::setActive(bool active) {
  active_ = active;
  repaint();
}

void EqualizerResponse::setHighPass(bool high_pass) {
  high_pass_ = high_pass;
  repaint();
}

void EqualizerResponse::setNotch(bool notch) {
  notch_ = notch;
  repaint();
}

void EqualizerResponse::setLowPass(bool low_pass) {
  low_pass_ = low_pass;
  repaint();
}

vital::poly_float EqualizerResponse::getOutputTotal(vital::Output* output, Slider* slider) {
  if (output == nullptr || slider == nullptr)
    return 0.0f;
  if (!active_ || !animate_ || !output->owner->enabled())
    return slider->getValue();
  return output->trigger_value;
}
