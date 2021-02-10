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

#include "phaser_section.h"

#include "skin.h"
#include "shaders.h"
#include "fonts.h"
#include "synth_strings.h"
#include "synth_button.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "tempo_selector.h"
#include "text_look_and_feel.h"

PhaserResponse::PhaserResponse(const vital::output_map& mono_modulations) :
    OpenGlLineRenderer(kResolution), phaser_filter_(false) {
  parent_ = nullptr;
  active_ = true;
  mix_ = 1.0f;

  setFill(true);
  setFillCenter(-1.0f);

  cutoff_slider_ = nullptr;
  resonance_slider_ = nullptr;
  blend_slider_ = nullptr;
  mix_slider_ = nullptr;

  line_data_ = std::make_unique<float[]>(2 * kResolution);
  line_buffer_ = 0;
  response_buffer_ = 0;
  vertex_array_object_ = 0;

  for (int i = 0; i < kResolution; ++i) {
    float t = i / (kResolution - 1.0f);
    line_data_[2 * i] = 2.0f * t - 1.0f;
    line_data_[2 * i + 1] = 0.0f;
  }

  filter_mix_output_ = mono_modulations.at("phaser_dry_wet");
  resonance_output_ = mono_modulations.at("phaser_feedback");
  blend_output_ = mono_modulations.at("phaser_blend");

  blend_setting_ = 1.0f;

  phaser_filter_.setSampleRate(kDefaultVisualSampleRate);
}

PhaserResponse::~PhaserResponse() { }

void PhaserResponse::init(OpenGlWrapper& open_gl) {
  if (parent_ == nullptr)
    parent_ = findParentComponentOfClass<SynthGuiInterface>();

  if (parent_)
    phaser_cutoff_ = parent_->getSynth()->getStatusOutput("phaser_cutoff");

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

  OpenGLShaderProgram* shader = open_gl.shaders->getShaderProgram(Shaders::kPhaserFilterResponseVertex,
                                                                  Shaders::kColorFragment, varyings);
  response_shader_.shader = shader;

  shader->use();
  response_shader_.position = getAttribute(open_gl, *shader, "position");

  response_shader_.mix = getUniform(open_gl, *shader, "mix");
  response_shader_.midi_cutoff = getUniform(open_gl, *shader, "midi_cutoff");
  response_shader_.resonance = getUniform(open_gl, *shader, "resonance");
  response_shader_.db24 = getUniform(open_gl, *shader, "db24");

  for (int s = 0; s < FilterResponseShader::kMaxStages; ++s) {
    String stage = String("stage") + String(s);
    response_shader_.stages[s] = getUniform(open_gl, *shader, stage.toRawUTF8());
  }
}

void PhaserResponse::render(OpenGlWrapper& open_gl, bool animate) {
  drawFilterResponse(open_gl, animate);
  renderCorners(open_gl, animate);
}

void PhaserResponse::destroy(OpenGlWrapper& open_gl) {
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
  response_shader_.db24 = nullptr;

  for (int s = 0; s < FilterResponseShader::kMaxStages; ++s)
    response_shader_.stages[s] = nullptr;
}

vital::poly_float PhaserResponse::getOutputTotal(const vital::Output* output, vital::poly_float default_value) {
  if (output && output->owner->enabled())
    return output->trigger_value;
  return default_value;
}

void PhaserResponse::setupFilterState() {
  filter_state_.midi_cutoff = phaser_cutoff_->value();
  mix_ = getOutputTotal(filter_mix_output_, mix_slider_->getValue());
  filter_state_.resonance_percent = getOutputTotal(resonance_output_, resonance_slider_->getValue());
  filter_state_.pass_blend = getOutputTotal(blend_output_, blend_slider_->getValue());
}

void PhaserResponse::loadShader(int index) {
  phaser_filter_.setupFilter(filter_state_);
  response_shader_.shader->use();
  response_shader_.midi_cutoff->set(filter_state_.midi_cutoff[index]);
  response_shader_.resonance->set(phaser_filter_.getResonance()[index]);
  response_shader_.db24->set(filter_state_.style != vital::SynthFilter::k12Db ? 1.0f : 0.0f);

  response_shader_.stages[0]->set(phaser_filter_.getPeak1Amount()[index]);
  response_shader_.stages[1]->set(phaser_filter_.getPeak3Amount()[index]);
  response_shader_.stages[2]->set(phaser_filter_.getPeak5Amount()[index]);
  response_shader_.mix->set(mix_[index]);
}

void PhaserResponse::bind(OpenGLContext& open_gl_context) {
  open_gl_context.extensions.glBindVertexArray(vertex_array_object_);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);

  OpenGLShaderProgram::Attribute* position = response_shader_.position.get();
  open_gl_context.extensions.glVertexAttribPointer(position->attributeID, 2, GL_FLOAT,
                                                   GL_FALSE, 2 * sizeof(float), nullptr);
  open_gl_context.extensions.glEnableVertexAttribArray(position->attributeID);

  open_gl_context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, response_buffer_);
}

void PhaserResponse::unbind(OpenGLContext& open_gl_context) {
  OpenGLShaderProgram::Attribute* position = response_shader_.position.get();
  open_gl_context.extensions.glDisableVertexAttribArray(position->attributeID);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl_context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
}

void PhaserResponse::renderLineResponse(OpenGlWrapper& open_gl) {
  open_gl.context.extensions.glBeginTransformFeedback(GL_POINTS);
  glDrawArrays(GL_POINTS, 0, kResolution);
  open_gl.context.extensions.glEndTransformFeedback();

  void* buffer = open_gl.context.extensions.glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
                                                             kResolution * sizeof(float), GL_MAP_READ_BIT);

  float* response_data = (float*)buffer;
  float x_adjust = getWidth();
  float y_adjust = getHeight() / 2.0f;
  for (int i = 0; i < kResolution; ++i) {
    setXAt(i, x_adjust * i / (kResolution - 1.0f));
    setYAt(i, y_adjust * (1.0 - response_data[i]));
  }

  open_gl.context.extensions.glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
}

void PhaserResponse::drawFilterResponse(OpenGlWrapper& open_gl, bool animate) {
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
    renderLineResponse(open_gl);

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
  renderLineResponse(open_gl);

  setFillColors(color_fill_from, color_fill_to);
  setColor(color_line);
  OpenGlLineRenderer::render(open_gl, animate);

  unbind(open_gl.context);
  glDisable(GL_BLEND);
  checkGlError();
}

PhaserSection::PhaserSection(String name, const vital::output_map& mono_modulations) : SynthSection(name) {
  static const double TEMPO_DRAG_SENSITIVITY = 0.3;

  phase_offset_ = std::make_unique<SynthSlider>("phaser_phase_offset");
  addSlider(phase_offset_.get());
  phase_offset_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  mod_depth_ = std::make_unique<SynthSlider>("phaser_mod_depth");
  addSlider(mod_depth_.get());
  mod_depth_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  center_ = std::make_unique<SynthSlider>("phaser_center");
  addSlider(center_.get());
  center_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(center_.get());

  frequency_ = std::make_unique<SynthSlider>("phaser_frequency");
  addSlider(frequency_.get());
  frequency_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  frequency_->setLookAndFeel(TextLookAndFeel::instance());

  tempo_ = std::make_unique<SynthSlider>("phaser_tempo");
  addSlider(tempo_.get());
  tempo_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tempo_->setLookAndFeel(TextLookAndFeel::instance());
  tempo_->setSensitivity(TEMPO_DRAG_SENSITIVITY);

  sync_ = std::make_unique<TempoSelector>("phaser_sync");
  addSlider(sync_.get());
  sync_->setSliderStyle(Slider::LinearBar);
  sync_->setTempoSlider(tempo_.get());
  sync_->setFreeSlider(frequency_.get());

  feedback_ = std::make_unique<SynthSlider>("phaser_feedback");
  addSlider(feedback_.get());
  feedback_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  blend_ = std::make_unique<SynthSlider>("phaser_blend");
  addSlider(blend_.get());
  blend_->setSliderStyle(Slider::LinearBar);
  blend_->snapToValue(true, 1.0);
  blend_->setBipolar();
  blend_->setPopupPlacement(BubbleComponent::above);

  dry_wet_ = std::make_unique<SynthSlider>("phaser_dry_wet");
  addSlider(dry_wet_.get());
  dry_wet_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  phaser_response_ = std::make_unique<PhaserResponse>(mono_modulations);
  phaser_response_->setCutoffSlider(center_.get());
  phaser_response_->setResonanceSlider(feedback_.get());
  phaser_response_->setBlendSlider(blend_.get());
  phaser_response_->setMixSlider(dry_wet_.get());
  addOpenGlComponent(phaser_response_.get());

  on_ = std::make_unique<SynthButton>("phaser_on");
  addButton(on_.get());
  setActivator(on_.get());
  setSkinOverride(Skin::kPhaser);
}

PhaserSection::~PhaserSection() { }

void PhaserSection::paintBackground(Graphics& g) {
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

void PhaserSection::resized() {
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

  blend_->setBounds(widget_x - widget_margin, widget_margin - getSliderOverlap(),
                    widget_width + 2 * widget_margin, getSliderWidth());
  int widget_y = blend_->getBottom() - getSliderOverlapWithSpace();
  phaser_response_->setBounds(widget_x, widget_y, widget_width, getHeight() - widget_y - widget_margin);

  placeKnobsInArea(Rectangle<int>(knobs_x, 0, knobs_area.getWidth(), section_height),
                   { feedback_.get(), dry_wet_.get() });

  placeKnobsInArea(Rectangle<int>(knobs_x, knob_y2, knobs_area.getWidth(), section_height),
                   { center_.get(), mod_depth_.get() });

  SynthSection::resized();
}

void PhaserSection::setActive(bool active) {
  SynthSection::setActive(active);
  phaser_response_->setActive(active);
}
