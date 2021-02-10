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

#include "distortion_section.h"

#include "open_gl_line_renderer.h"
#include "distortion.h"
#include "fonts.h"
#include "synth_strings.h"
#include "synth_button.h"
#include "tab_selector.h"
#include "shaders.h"
#include "synth_slider.h"
#include "text_selector.h"
#include "text_look_and_feel.h"

class DistortionViewer : public OpenGlLineRenderer {
  public:
    static constexpr float kDrawPercent = 0.9f;

    DistortionViewer(int resolution, const vital::output_map& mono_modulations) :
        OpenGlLineRenderer(resolution), type_slider_(nullptr), drive_slider_(nullptr) {
      drive_ = mono_modulations.at("distortion_drive");
      active_ = true;
      setFill(true);
      setFillCenter(0.0f);
    }

    vital::poly_float getDrive() {
      if (drive_slider_ && !drive_->owner->enabled())
        return drive_slider_->getValue();
      return drive_->trigger_value;
    }

    void drawDistortion(OpenGlWrapper& open_gl, bool animate, int index) {
      vital::poly_float drive = vital::Distortion::getDriveValue(type_slider_->getValue(), getDrive());

      float width = getWidth();
      float height = getHeight();
      float y_scale = height / 2.0f;
      int num_points = numPoints();
      for (int i = 0; i < num_points; ++i) {
        float t = i / (num_points - 1.0f);
        float val = 2.0f * t - 1.0f;
        setXAt(i, t * width);
        float result = kDrawPercent * vital::Distortion::getDrivenValue(type_slider_->getValue(), val, drive)[index];
        setYAt(i, (1.0f - result) * y_scale);
      }

      OpenGlLineRenderer::render(open_gl, animate);
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      setLineWidth(findValue(Skin::kWidgetLineWidth));

      if (active_) {
        setColor(findColour(Skin::kWidgetPrimary2, true));
        float fill_fade = findValue(Skin::kWidgetFillFade);
        Colour color_fill_to = findColour(Skin::kWidgetSecondary2, true);
        Colour color_fill_from = color_fill_to.withMultipliedAlpha(1.0f - fill_fade);
        setFillColors(color_fill_from, color_fill_to);
        drawDistortion(open_gl, animate, 1);

        setColor(findColour(Skin::kWidgetPrimary1, true));
        color_fill_to = findColour(Skin::kWidgetSecondary1, true);
        color_fill_from = color_fill_to.withMultipliedAlpha(1.0f - fill_fade);
        setFillColors(color_fill_from, color_fill_to);
        drawDistortion(open_gl, animate, 0);
      }
      else {
        setColor(findColour(Skin::kWidgetPrimaryDisabled, true));
        setFillColor(findColour(Skin::kWidgetSecondaryDisabled, true));
        drawDistortion(open_gl, animate, 0);
      }

      renderCorners(open_gl, animate);
    }

    void mouseDown(const MouseEvent& e) override {
      last_mouse_position_ = e.getPosition();
    }

    void mouseDrag(const MouseEvent& e) override {
      Point<int> delta = e.getPosition() - last_mouse_position_;
      last_mouse_position_ = e.getPosition();

      float drive_range = drive_slider_->getMaximum() - drive_slider_->getMinimum();
      drive_slider_->setValue(drive_slider_->getValue() - delta.y * drive_range / getWidth());
    }

    void setTypeSlider(Slider* slider) { type_slider_ = slider; }
    void setDriveSlider(Slider* slider) { drive_slider_ = slider; }
    void setActive(bool active) { active_ = active; }

  private:
    bool active_;
    Point<int> last_mouse_position_;

    vital::Output* drive_;
    Slider* type_slider_;
    Slider* drive_slider_;
};

DistortionFilterResponse::DistortionFilterResponse(const vital::output_map& mono_modulations) :
    OpenGlLineRenderer(kResolution) {
  active_ = true;

  setFill(true);
  setFillCenter(-1.0f);

  filter_.setDriveCompensation(false);

  cutoff_slider_ = nullptr;
  resonance_slider_ = nullptr;
  blend_slider_ = nullptr;

  cutoff_output_ = mono_modulations.at("distortion_filter_cutoff");
  resonance_output_ = mono_modulations.at("distortion_filter_resonance");
  blend_output_ = mono_modulations.at("distortion_filter_blend");

  line_data_ = std::make_unique<float[]>(2 * kResolution);
  line_buffer_ = 0;
  response_buffer_ = 0;
  vertex_array_object_ = 0;

  for (int i = 0; i < kResolution; ++i) {
    float t = i / (kResolution - 1.0f);
    line_data_[2 * i] = 2.0f * t - 1.0f;
    line_data_[2 * i + 1] = 0.0f;
  }
}

DistortionFilterResponse::~DistortionFilterResponse() = default;

void DistortionFilterResponse::init(OpenGlWrapper& open_gl) {
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

  OpenGLShaderProgram* shader = open_gl.shaders->getShaderProgram(Shaders::kDigitalFilterResponseVertex,
                                                                  Shaders::kColorFragment, varyings);
  response_shader_.shader = shader;

  shader->use();
  response_shader_.position = getAttribute(open_gl, *shader, "position");

  response_shader_.mix = getUniform(open_gl, *shader, "mix");
  response_shader_.midi_cutoff = getUniform(open_gl, *shader, "midi_cutoff");
  response_shader_.resonance = getUniform(open_gl, *shader, "resonance");
  response_shader_.drive = getUniform(open_gl, *shader, "drive");
  response_shader_.db24 = getUniform(open_gl, *shader, "db24");

  for (int s = 0; s < FilterResponseShader::kMaxStages; ++s) {
    String stage = String("stage") + String(s);
    response_shader_.stages[s] = getUniform(open_gl, *shader, stage.toRawUTF8());
  }
}

void DistortionFilterResponse::render(OpenGlWrapper& open_gl, bool animate) {
  drawFilterResponse(open_gl, animate);
  renderCorners(open_gl, animate);
}

void DistortionFilterResponse::destroy(OpenGlWrapper& open_gl) {
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
  response_shader_.db24 = nullptr;

  for (auto& stage : response_shader_.stages)
    stage = nullptr;
}

vital::poly_float DistortionFilterResponse::getOutputTotal(vital::Output* output, vital::poly_float default_value) {
  if (output && output->owner->enabled())
    return output->trigger_value;
  return default_value;
}

void DistortionFilterResponse::setupFilterState() {
  filter_state_.midi_cutoff = getOutputTotal(cutoff_output_, cutoff_slider_->getValue());
  filter_state_.resonance_percent = getOutputTotal(resonance_output_, resonance_slider_->getValue());
  filter_state_.pass_blend = getOutputTotal(blend_output_, blend_slider_->getValue());
}

void DistortionFilterResponse::loadShader(int index) {
  filter_.setupFilter(filter_state_);
  response_shader_.shader->use(); 
  float min_cutoff = cutoff_slider_->getMinimum() + 0.001;
  float cutoff = std::max(min_cutoff, filter_state_.midi_cutoff[index]);
  response_shader_.midi_cutoff->set(cutoff);

  float resonance = vital::utils::clamp(filter_.getResonance()[index], 0.0f, 2.0f);
  response_shader_.resonance->set(resonance);
  response_shader_.mix->set(1.0f);

  response_shader_.drive->set(filter_.getDrive()[index]);
  response_shader_.db24->set(filter_state_.style != vital::SynthFilter::k12Db ? 1.0f : 0.0f);

  response_shader_.stages[0]->set(filter_.getLowAmount()[index]);
  response_shader_.stages[1]->set(filter_.getBandAmount()[index]);
  response_shader_.stages[2]->set(filter_.getHighAmount()[index]);
  response_shader_.stages[3]->set(filter_.getLowAmount24(filter_state_.style)[index]);
  response_shader_.stages[4]->set(filter_.getHighAmount24(filter_state_.style)[index]);
}

void DistortionFilterResponse::bind(OpenGLContext& open_gl_context) {
  open_gl_context.extensions.glBindVertexArray(vertex_array_object_);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);

  OpenGLShaderProgram::Attribute* position = response_shader_.position.get();
  open_gl_context.extensions.glVertexAttribPointer(position->attributeID, 2, GL_FLOAT,
                                                   GL_FALSE, 2 * sizeof(float), nullptr);
  open_gl_context.extensions.glEnableVertexAttribArray(position->attributeID);

  open_gl_context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, response_buffer_);
}

void DistortionFilterResponse::unbind(OpenGLContext& open_gl_context) {
  OpenGLShaderProgram::Attribute* position = response_shader_.position.get();
  open_gl_context.extensions.glDisableVertexAttribArray(position->attributeID);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl_context.extensions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
}

void DistortionFilterResponse::renderLineResponse(OpenGlWrapper& open_gl) {
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

void DistortionFilterResponse::drawFilterResponse(OpenGlWrapper& open_gl, bool animate) {
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

DistortionSection::DistortionSection(String name, const vital::output_map& mono_modulations) : SynthSection(name) {
  type_ = std::make_unique<TextSelector>("distortion_type");
  addSlider(type_.get());
  type_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  type_->setLookAndFeel(TextLookAndFeel::instance());
  type_->setLongStringLookup(strings::kDistortionTypeNames);

  filter_order_ = std::make_unique<TextSelector>("distortion_filter_order");
  addSlider(filter_order_.get());
  filter_order_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  filter_order_->setLookAndFeel(TextLookAndFeel::instance());
  filter_order_->setLongStringLookup(strings::kDistortionFilterOrderNames);

  filter_cutoff_ = std::make_unique<SynthSlider>("distortion_filter_cutoff");
  addSlider(filter_cutoff_.get());
  filter_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(filter_cutoff_.get());

  filter_resonance_ = std::make_unique<SynthSlider>("distortion_filter_resonance");
  addSlider(filter_resonance_.get());
  filter_resonance_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  filter_blend_ = std::make_unique<SynthSlider>("distortion_filter_blend");
  addSlider(filter_blend_.get());
  filter_blend_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  filter_blend_->setBipolar();

  int resolution = kViewerResolution;
  distortion_viewer_ = std::make_unique<DistortionViewer>(resolution, mono_modulations);
  addOpenGlComponent(distortion_viewer_.get());
  distortion_viewer_->setTypeSlider(type_.get());

  drive_ = std::make_unique<SynthSlider>("distortion_drive");
  addSlider(drive_.get());
  drive_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  distortion_viewer_->setDriveSlider(drive_.get());

  mix_ = std::make_unique<SynthSlider>("distortion_mix");
  addSlider(mix_.get());
  mix_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  filter_response_ = std::make_unique<DistortionFilterResponse>(mono_modulations);
  addOpenGlComponent(filter_response_.get());
  filter_response_->setCutoffSlider(filter_cutoff_.get());
  filter_response_->setResonanceSlider(filter_resonance_.get());
  filter_response_->setBlendSlider(filter_blend_.get());

  on_ = std::make_unique<SynthButton>("distortion_on");
  addButton(on_.get());
  setActivator(on_.get());
  setSkinOverride(Skin::kDistortion);
}

DistortionSection::~DistortionSection() = default;

void DistortionSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);
  setLabelFont(g);

  drawTextComponentBackground(g, type_->getBounds(), true);
  drawTextComponentBackground(g, filter_order_->getBounds(), true);
  drawLabelForComponent(g, TRANS("DRIVE"), drive_.get());
  drawLabelForComponent(g, TRANS("MIX"), mix_.get());
  drawLabelForComponent(g, TRANS("CUTOFF"), filter_cutoff_.get());
  drawLabelForComponent(g, TRANS("RESONANCE"), filter_resonance_.get());
  drawLabelForComponent(g, TRANS("BLEND"), filter_blend_.get());
  drawLabelForComponent(g, TRANS("TYPE"), type_.get(), true);
  drawLabelForComponent(g, TRANS("FILTER"), filter_order_.get(), true);
}

void DistortionSection::resized() {
  int widget_margin = findValue(Skin::kWidgetMargin);
  int title_width = getTitleWidth();
  int section_height = getKnobSectionHeight();
  
  Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
  Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 2, 1, widget_margin);
  Rectangle<int> settings_area = getDividedAreaUnbuffered(bounds, 4, 0, widget_margin);

  int widget_x = settings_area.getRight() + widget_margin;
  int widget_width = knobs_area.getX() - widget_x;
  
  int knob_y2 = section_height - widget_margin;
  type_->setBounds(settings_area.getX(), widget_margin, settings_area.getWidth(), section_height - 2 * widget_margin);
  filter_order_->setBounds(settings_area.getX(), knob_y2 + widget_margin,
                           settings_area.getWidth(), section_height - 2 * widget_margin);

  int distortion_viewer_height = (getHeight() - 3 * widget_margin) / 2;
  distortion_viewer_->setBounds(widget_x, widget_margin, widget_width, distortion_viewer_height);

  int response_height = getHeight() - distortion_viewer_height - 3 * widget_margin;
  int filter_y = getHeight() - response_height - widget_margin;
  filter_response_->setBounds(widget_x, filter_y, widget_width, response_height);

  placeKnobsInArea(Rectangle<int>(knobs_area.getX(), 0, knobs_area.getWidth(), section_height),
                   { drive_.get(), mix_.get() });

  placeKnobsInArea(Rectangle<int>(knobs_area.getX(), knob_y2, knobs_area.getWidth(), section_height),
                   { filter_cutoff_.get(), filter_resonance_.get(), filter_blend_.get() });
  setFilterActive(filter_order_->getValue() != 0.0f && isActive());

  SynthSection::resized();
}

void DistortionSection::setActive(bool active) {
  setFilterActive(active && filter_order_->getValue() != 0.0f);
  distortion_viewer_->setActive(active);
  SynthSection::setActive(active);
}

void DistortionSection::sliderValueChanged(Slider* changed_slider) {
  if (changed_slider == filter_order_.get())
    setFilterActive(filter_order_->getValue() != 0.0f && isActive());

  SynthSection::sliderValueChanged(changed_slider);
}

void DistortionSection::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  setFilterActive(filter_order_->getValue() != 0.0f && isActive());
}

void DistortionSection::setFilterActive(bool active) {
  filter_response_->setActive(active);
  filter_cutoff_->setActive(active);
  filter_resonance_->setActive(active);
  filter_blend_->setActive(active);
}
