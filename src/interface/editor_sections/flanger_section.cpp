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

#include "flanger_section.h"

#include "skin.h"
#include "fonts.h"
#include "shaders.h"
#include "synth_strings.h"
#include "synth_button.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "tempo_selector.h"
#include "text_look_and_feel.h"

FlangerResponse::FlangerResponse(const vital::output_map& mono_modulations) : OpenGlLineRenderer(kResolution) {
  parent_ = nullptr;
  active_ = true;
  mix_ = 1.0f;

  setFill(true);
  setFillCenter(-1.0f);

  center_slider_ = nullptr;
  feedback_slider_ = nullptr;
  mix_slider_ = nullptr;

  mix_output_ = mono_modulations.at("flanger_dry_wet");
  feedback_output_ = mono_modulations.at("flanger_feedback");

  line_data_ = std::make_unique<float[]>(2 * kResolution);
  line_buffer_ = 0;
  response_buffer_ = 0;
  vertex_array_object_ = 0;

  for (int i = 0; i < kResolution; ++i) {
    float t = i / (kResolution - 1.0f);
    line_data_[2 * i] = 6.0f * t - 4.0f;
    line_data_[2 * i + 1] = (i / kCombAlternatePeriod) % 2;
  }
}

FlangerResponse::~FlangerResponse() { }

void FlangerResponse::init(OpenGlWrapper& open_gl) {
  if (parent_ == nullptr)
    parent_ = findParentComponentOfClass<SynthGuiInterface>();

  if (parent_)
    flanger_frequency_ = parent_->getSynth()->getStatusOutput("flanger_delay_frequency");

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

  OpenGLShaderProgram* shader = open_gl.shaders->getShaderProgram(Shaders::kCombFilterResponseVertex,
                                                                  Shaders::kColorFragment, varyings);
  response_shader_.shader = shader;

  shader->use();
  response_shader_.position = getAttribute(open_gl, *shader, "position");

  response_shader_.mix = getUniform(open_gl, *shader, "mix");
  response_shader_.midi_cutoff = getUniform(open_gl, *shader, "midi_cutoff");
  response_shader_.resonance = getUniform(open_gl, *shader, "resonance");
  response_shader_.drive = getUniform(open_gl, *shader, "drive");

  for (int s = 0; s < FilterResponseShader::kMaxStages; ++s) {
    String stage = String("stage") + String(s);
    response_shader_.stages[s] = getUniform(open_gl, *shader, stage.toRawUTF8());
  }
}

void FlangerResponse::render(OpenGlWrapper& open_gl, bool animate) {
  drawFilterResponse(open_gl, animate);
  renderCorners(open_gl, animate);
}

void FlangerResponse::destroy(OpenGlWrapper& open_gl) {
  OpenGlLineRenderer::destroy(open_gl);

  open_gl.context.extensions.glDeleteBuffers(1, &line_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &response_buffer_);

  vertex_array_object_ = 0;
  line_buffer_ = 0;
  response_buffer_ = 0;

  response_shader_.shader = nullptr;
  response_shader_.position = nullptr;

  response_shader_.mix = nullptr;
  response_shader_.midi_cutoff = nullptr;
  response_shader_.resonance = nullptr;
  response_shader_.drive = nullptr;

  for (int s = 0; s < FilterResponseShader::kMaxStages; ++s)
    response_shader_.stages[s] = nullptr;
}

vital::poly_float FlangerResponse::getOutputTotal(vital::Output* output, vital::poly_float default_value) {
  if (output && output->owner->enabled())
    return output->trigger_value;
  return default_value;
}

void FlangerResponse::setupFilterState() {
  filter_state_.midi_cutoff = 0.0f;
  mix_ = getOutputTotal(mix_output_, mix_slider_->getValue());
  filter_state_.resonance_percent = getOutputTotal(feedback_output_, feedback_slider_->getValue()) * 0.5f + 0.5f;
  filter_state_.pass_blend = 1.0f;
}

void FlangerResponse::loadShader(int index) {
  comb_filter_.setupFilter(filter_state_);
  response_shader_.shader->use();
  float resonance = vital::utils::clamp(comb_filter_.getResonance()[index], -0.99f, 0.99f);
  response_shader_.midi_cutoff->set(filter_state_.midi_cutoff[index]);
  response_shader_.resonance->set(resonance);
  response_shader_.drive->set(1.0f);

  response_shader_.stages[0]->set(comb_filter_.getLowAmount()[index]);
  response_shader_.stages[1]->set(comb_filter_.getHighAmount()[index]);
  response_shader_.stages[2]->set(comb_filter_.getFilterMidiCutoff()[index]);
  response_shader_.stages[3]->set(comb_filter_.getFilter2MidiCutoff()[index]);
  response_shader_.mix->set(mix_[index]);
}

void FlangerResponse::bind(OpenGLContext& open_gl_context) {
  open_gl_context.extensions.glBindVertexArray(vertex_array_object_);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);

  OpenGLShaderProgram::Attribute* position = response_shader_.position.get();
  open_gl_context.extensions.glVertexAttribPointer(position->attributeID, 2, GL_FLOAT,
                                                   GL_FALSE, 2 * sizeof(float), nullptr);
  open_gl_context.extensions.glEnableVertexAttribArray(position->attributeID);

  open_gl_context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, response_buffer_);
}

void FlangerResponse::unbind(OpenGLContext& open_gl_context) {
  OpenGLShaderProgram::Attribute* position = response_shader_.position.get();
  open_gl_context.extensions.glDisableVertexAttribArray(position->attributeID);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl_context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
}

void FlangerResponse::renderLineResponse(OpenGlWrapper& open_gl, int index) {
  static constexpr float kMaxMidi = 128.0f;

  open_gl.context.extensions.glBeginTransformFeedback(GL_POINTS);
  glDrawArrays(GL_POINTS, 0, kResolution);
  open_gl.context.extensions.glEndTransformFeedback();

  void* buffer = open_gl.context.extensions.glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
                                                             kResolution * sizeof(float), GL_MAP_READ_BIT);
  vital::poly_float midi = vital::utils::frequencyToMidiNote(flanger_frequency_->value());
  float offset = midi[index] * (getWidth() / kMaxMidi) - getWidth() * 1.5f;
  float* response_data = (float*)buffer;
  float x_adjust = getWidth() * 3.0f;
  float y_adjust = getHeight() / 2.0f;
  for (int i = 0; i < kResolution; ++i) {
    setXAt(i, x_adjust * i / (kResolution - 1.0f) + offset);
    setYAt(i, y_adjust * (1.0 - response_data[i]));
  }

  open_gl.context.extensions.glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
}

void FlangerResponse::drawFilterResponse(OpenGlWrapper& open_gl, bool animate) {
  setupFilterState();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  setViewPort(open_gl);

  Colour color_line = findColour(Skin::kWidgetPrimary2, true);
  Colour color_fill_to = findColour(Skin::kWidgetSecondary2, true);
  float fill_fade = findValue(Skin::kWidgetFillFade);
  Colour color_fill_from = color_fill_to.withMultipliedAlpha(1.0f - fill_fade);

  setLineWidth(findValue(Skin::kWidgetLineWidth));
  setFillCenter(findValue(Skin::kWidgetFillCenter));

  if (active_) {
    bind(open_gl.context);
    loadShader(1);
    renderLineResponse(open_gl, 1);

    setFillColors(color_fill_from, color_fill_to);
    setColor(color_line);
    OpenGlLineRenderer::render(open_gl, animate);
  }

  glEnable(GL_BLEND);
  color_line = findColour(Skin::kWidgetPrimary1, true);
  color_fill_to = findColour(Skin::kWidgetSecondary1, true);
  if (!active_) {
    color_line = findColour(Skin::kWidgetPrimaryDisabled, true);
    color_fill_to = findColour(Skin::kWidgetSecondaryDisabled, true);
  }
  color_fill_from = color_fill_to.withMultipliedAlpha(1.0f - fill_fade);

  bind(open_gl.context);
  loadShader(0);
  renderLineResponse(open_gl, 0);

  setFillColors(color_fill_from, color_fill_to);
  setColor(color_line);
  OpenGlLineRenderer::render(open_gl, animate);

  unbind(open_gl.context);
  glDisable(GL_BLEND);
  checkGlError();
}

FlangerSection::FlangerSection(String name, const vital::output_map& mono_modulations) : SynthSection(name) {
  static const double TEMPO_DRAG_SENSITIVITY = 0.3;

  phase_offset_ = std::make_unique<SynthSlider>("flanger_phase_offset");
  addSlider(phase_offset_.get());
  phase_offset_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  mod_depth_ = std::make_unique<SynthSlider>("flanger_mod_depth");
  addSlider(mod_depth_.get());
  mod_depth_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  center_ = std::make_unique<SynthSlider>("flanger_center");
  addSlider(center_.get());
  center_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(center_.get());

  frequency_ = std::make_unique<SynthSlider>("flanger_frequency");
  addSlider(frequency_.get());
  frequency_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  frequency_->setLookAndFeel(TextLookAndFeel::instance());

  tempo_ = std::make_unique<SynthSlider>("flanger_tempo");
  addSlider(tempo_.get());
  tempo_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tempo_->setLookAndFeel(TextLookAndFeel::instance());
  tempo_->setSensitivity(TEMPO_DRAG_SENSITIVITY);

  sync_ = std::make_unique<TempoSelector>("flanger_sync");
  addSlider(sync_.get());
  sync_->setSliderStyle(Slider::LinearBar);
  sync_->setTempoSlider(tempo_.get());
  sync_->setFreeSlider(frequency_.get());

  feedback_ = std::make_unique<SynthSlider>("flanger_feedback");
  addSlider(feedback_.get());
  feedback_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  feedback_->setBipolar();
  feedback_->snapToValue(true);

  dry_wet_ = std::make_unique<SynthSlider>("flanger_dry_wet");
  addSlider(dry_wet_.get());
  dry_wet_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  flanger_response_ = std::make_unique<FlangerResponse>(mono_modulations);
  flanger_response_->setCenterSlider(center_.get());
  flanger_response_->setFeedbackSlider(feedback_.get());
  flanger_response_->setMixSlider(dry_wet_.get());
  addOpenGlComponent(flanger_response_.get());

  on_ = std::make_unique<SynthButton>("flanger_on");
  addButton(on_.get());
  setActivator(on_.get());
  setSkinOverride(Skin::kFlanger);
}

FlangerSection::~FlangerSection() { }

void FlangerSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  Rectangle<int> frequency_bounds(tempo_->getX(), tempo_->getY(), 
                                  sync_->getRight() - tempo_->getX(), tempo_->getHeight());
  drawTextComponentBackground(g, frequency_bounds, true);
  drawTempoDivider(g, sync_.get());

  setLabelFont(g);
  drawLabel(g, TRANS("FREQUENCY"), frequency_bounds, true);
  drawLabelForComponent(g, TRANS("FEEDBACK"), feedback_.get());
  drawLabelForComponent(g, TRANS("MIX"), dry_wet_.get());
  drawLabelForComponent(g, TRANS("CENTER"), center_.get());
  drawLabelForComponent(g, TRANS("DEPTH"), mod_depth_.get());
  drawLabelForComponent(g, TRANS("OFFSET"), phase_offset_.get());
}

void FlangerSection::resized() {
  int title_width = getTitleWidth();
  int widget_margin = findValue(Skin::kWidgetMargin);
  Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
  Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 3, 2, widget_margin);
  Rectangle<int> tempo_area = getDividedAreaUnbuffered(bounds, 4, 0, widget_margin);

  int section_height = getKnobSectionHeight();

  int knobs_x = knobs_area.getX();
  int knob_y2 = section_height - widget_margin;
  int tempo_x = tempo_area.getX();
  int tempo_width = tempo_area.getWidth();

  int widget_x = tempo_x + tempo_width + widget_margin;
  int widget_width = knobs_x - widget_x;

  placeTempoControls(tempo_x, widget_margin, tempo_width, section_height - 2 * widget_margin,
                     frequency_.get(), sync_.get());
  tempo_->setBounds(frequency_->getBounds());
  tempo_->setModulationArea(frequency_->getModulationArea());

  phase_offset_->setBounds(title_width + widget_margin, knob_y2, tempo_width, section_height - widget_margin);

  flanger_response_->setBounds(widget_x, widget_margin, widget_width, getHeight() - 2 * widget_margin);

  placeKnobsInArea(Rectangle<int>(knobs_x, 0, knobs_area.getWidth(), section_height),
                   { feedback_.get(), dry_wet_.get() });

  placeKnobsInArea(Rectangle<int>(knobs_x, knob_y2, knobs_area.getWidth(), section_height),
                   { center_.get(), mod_depth_.get() });

  SynthSection::resized();
}

void FlangerSection::setActive(bool active) {
  flanger_response_->setActive(active);
  SynthSection::setActive(active);
}
