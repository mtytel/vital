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

#include "random_section.h"

#include "skin.h"
#include "fonts.h"
#include "open_gl_line_renderer.h"
#include "synth_strings.h"
#include "tempo_selector.h"
#include "text_selector.h"
#include "text_look_and_feel.h"

class RandomViewer : public OpenGlLineRenderer {
  public:
    static constexpr int kResolution = 64;
    static constexpr float kBoostAmount = 1.0f;
    static constexpr float kDecayMult = 0.9f;

    RandomViewer(String name) : OpenGlLineRenderer(kResolution), stereo_line_(kResolution), random_value_(nullptr) {
      setName(name);
      setFill(true);
      setFillCenter(-1.0f);

      stereo_line_.setFill(true);
      stereo_line_.setFillCenter(-1.0f);
      addAndMakeVisible(stereo_line_);

      setFillBoostAmount(kBoostAmount);
      stereo_line_.setFillBoostAmount(kBoostAmount);
      setBoostAmount(1.0f);
      stereo_line_.setBoostAmount(1.0f);

      float boost = 1.0f;
      for (int i = 0; i < kResolution; ++i) {
        setYAt(i, 10000);
        stereo_line_.setYAt(i, 10000);
        setBoostLeft(i, 0.0f);
        stereo_line_.setBoostLeft(i, 0.0f);
        boost *= kDecayMult;
      }
    }

    void paintBackground(Graphics& g) override {
      OpenGlLineRenderer::paintBackground(g);
      g.setColour(findColour(Skin::kWidgetPrimaryDisabled, true));
      g.fillRect(0, getHeight() / 2, getWidth(), 1);
    }

    void init(OpenGlWrapper& open_gl) override {
      OpenGlLineRenderer::init(open_gl);
      stereo_line_.init(open_gl);
    }
    
    void destroy(OpenGlWrapper& open_gl) override {
      OpenGlLineRenderer::destroy(open_gl);
      stereo_line_.destroy(open_gl);
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      bool animating = animate;
      if (parent_)
        animating = animating && parent_->getSynth()->isModSourceEnabled(getName().toStdString());

      if (random_value_ == nullptr || !animating) {
        renderCorners(open_gl, animate);
        return;
      }

      float line_width = findValue(Skin::kWidgetLineWidth);
      setLineWidth(line_width);
      stereo_line_.setLineWidth(line_width);
      float fill_center = findValue(Skin::kWidgetFillCenter);
      setFillCenter(fill_center);
      stereo_line_.setFillCenter(fill_center);

      float width = getWidth();
      float height = getHeight();
      for (int i = kResolution - 1; i > 1; --i) {
        float x = i * width / (kResolution - 1);
        setXAt(i, x);
        setYAt(i, yAt(i - 1));
        stereo_line_.setXAt(i, x);
        stereo_line_.setYAt(i, stereo_line_.yAt(i - 1));

        setBoostLeft(i, boostLeftAt(i - 1));
        stereo_line_.setBoostLeft(i, stereo_line_.boostLeftAt(i - 1));
      }

      vital::poly_float random_value = (-random_value_->value() + 1.0f) * height;
      setXAt(1, 0.0f);
      setYAt(1, random_value[0]);
      setXAt(0, -1.0f);
      setYAt(0, random_value[0]);
      float boost = 0.0f;
      if (random_value[0] >= height || yAt(2) >= height || yAt(3) >= height)
        boost = -1.0f;

      setBoostLeft(0, boost);
      setBoostLeft(1, boost);
      setBoostLeft(2, boost);

      stereo_line_.setXAt(0, -1.0f);
      stereo_line_.setYAt(0, random_value[1]);
      stereo_line_.setXAt(1, 0.0f);
      stereo_line_.setYAt(1, random_value[1]);

      boost = 0.0f;
      if (random_value[1] >= height || stereo_line_.yAt(2) >= height || stereo_line_.yAt(3) >= height)
        boost = -1.0f;

      stereo_line_.setBoostLeft(0, boost);
      stereo_line_.setBoostLeft(1, boost);
      stereo_line_.setBoostLeft(2, boost);

      Colour fill_color = findColour(Skin::kWidgetSecondary1, true);
      float fill_fade = findValue(Skin::kWidgetFillFade);
      Colour fill_color_fade = fill_color.withMultipliedAlpha(1.0f - fill_fade);

      Colour fill_color_stereo = findColour(Skin::kWidgetSecondary2, true);
      Colour fill_color_stereo_fade = fill_color_stereo.withMultipliedAlpha(1.0f - fill_fade);

      stereo_line_.setColor(findColour(Skin::kWidgetPrimary2, true));
      stereo_line_.setFillColors(fill_color_stereo_fade, fill_color_stereo);
      stereo_line_.drawLines(open_gl, true);

      setColor(findColour(Skin::kWidgetPrimary1, true));
      setFillColors(fill_color_fade, fill_color);
      OpenGlLineRenderer::drawLines(open_gl, true);

      renderCorners(open_gl, animate);
    }

    void parentHierarchyChanged() override {
      parent_ = findParentComponentOfClass<SynthGuiInterface>();

      if (random_value_ == nullptr && parent_)
        random_value_ = parent_->getSynth()->getStatusOutput(getName().toStdString());

      OpenGlLineRenderer::parentHierarchyChanged();
    }

    void resized() override {
      OpenGlLineRenderer::resized();
      stereo_line_.setBounds(getLocalBounds());
    }

  private:
    SynthGuiInterface* parent_;
    OpenGlLineRenderer stereo_line_;
    const vital::StatusOutput* random_value_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomViewer)
};

RandomSection::RandomSection(String name, std::string value_prepend,
                             const vital::output_map& mono_modulations, const vital::output_map& poly_modulations) :
                             SynthSection(name) {
  static constexpr double kTempoDragSensitivity = 0.3;

  frequency_ = std::make_unique<SynthSlider>(value_prepend + "_frequency");
  addSlider(frequency_.get());
  frequency_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  frequency_->setLookAndFeel(TextLookAndFeel::instance());

  tempo_ = std::make_unique<SynthSlider>(value_prepend + "_tempo");
  addSlider(tempo_.get());
  tempo_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tempo_->setLookAndFeel(TextLookAndFeel::instance());
  tempo_->setSensitivity(kTempoDragSensitivity);

  keytrack_transpose_ = std::make_unique<SynthSlider>(value_prepend + "_keytrack_transpose");
  addSlider(keytrack_transpose_.get());
  keytrack_transpose_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  keytrack_transpose_->setLookAndFeel(TextLookAndFeel::instance());
  keytrack_transpose_->setSensitivity(kTransposeMouseSensitivity);
  keytrack_transpose_->setBipolar();

  keytrack_tune_ = std::make_unique<SynthSlider>(value_prepend + "_keytrack_tune");
  addSlider(keytrack_tune_.get());
  keytrack_tune_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  keytrack_tune_->setLookAndFeel(TextLookAndFeel::instance());
  keytrack_tune_->setBipolar();
  keytrack_tune_->setMaxDisplayCharacters(3);
  keytrack_tune_->setMaxDecimalPlaces(0);

  transpose_tune_divider_ = std::make_unique<OpenGlQuad>(Shaders::kColorFragment);
  addOpenGlComponent(transpose_tune_divider_.get());
  transpose_tune_divider_->setInterceptsMouseClicks(false, false);

  sync_ = std::make_unique<TempoSelector>(value_prepend + "_sync");
  addSlider(sync_.get());
  sync_->setSliderStyle(Slider::LinearBar);
  sync_->setTempoSlider(tempo_.get());
  sync_->setFreeSlider(frequency_.get());
  sync_->setKeytrackTransposeSlider(keytrack_transpose_.get());
  sync_->setKeytrackTuneSlider(keytrack_tune_.get());

  style_ = std::make_unique<TextSelector>(value_prepend + "_style");
  addSlider(style_.get());
  style_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  style_->setLookAndFeel(TextLookAndFeel::instance());
  style_->setLongStringLookup(strings::kRandomNames);

  viewer_ = std::make_unique<RandomViewer>(value_prepend);
  addOpenGlComponent(viewer_.get());
  addAndMakeVisible(viewer_.get());

  stereo_ = std::make_unique<SynthButton>(value_prepend + "_stereo");
  addButton(stereo_.get());
  stereo_->setButtonText("STEREO");
  stereo_->setLookAndFeel(TextLookAndFeel::instance());

  sync_type_ = std::make_unique<SynthButton>(value_prepend + "_sync_type");
  addButton(sync_type_.get());
  sync_type_->setButtonText("SYNC");
  sync_type_->setLookAndFeel(TextLookAndFeel::instance());

  setSkinOverride(Skin::kRandomLfo);
}

RandomSection::~RandomSection() { }

void RandomSection::paintBackground(Graphics& g) {
  setLabelFont(g);
  int widget_margin = findValue(Skin::kWidgetMargin);
  int frequency_right = getWidth() - widget_margin;
  int tempo_x = tempo_->getX();
  Rectangle<int> frequency_bounds(tempo_x, tempo_->getY(), frequency_right - tempo_x, tempo_->getHeight());

  drawTextComponentBackground(g, style_->getBounds(), true);
  drawTextComponentBackground(g, frequency_bounds, true);
  drawTempoDivider(g, sync_.get());

  drawLabel(g, TRANS("STYLE"), style_->getBounds(), true);
  drawLabel(g, TRANS("FREQUENCY"), frequency_bounds, true);

  transpose_tune_divider_->setColor(findColour(Skin::kLightenScreen, true));

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void RandomSection::resized() {
  int knob_section_height = getKnobSectionHeight();
  int text_button_height = findValue(Skin::kTextButtonHeight);

  int widget_margin = findValue(Skin::kWidgetMargin);
  int button_width = (getWidth() - 3 * widget_margin) / 2;
  sync_type_->setBounds(widget_margin, widget_margin, button_width, text_button_height);
  int stereo_x = sync_type_->getRight() + widget_margin;
  stereo_->setBounds(stereo_x, widget_margin, getWidth() - stereo_x - widget_margin, text_button_height);

  int viewer_y = text_button_height + 2 * widget_margin;
  int viewer_height = getHeight() - knob_section_height - viewer_y;
  viewer_->setBounds(widget_margin, viewer_y, getWidth() - 2 * widget_margin, viewer_height);

  int component_width = (getWidth() - 3 * widget_margin) / 2;
  int control_y = getHeight() - knob_section_height + widget_margin;
  style_->setBounds(widget_margin, control_y, component_width, knob_section_height - 2 * widget_margin);
  int frequency_x = style_->getRight() + widget_margin;
  placeTempoControls(frequency_x, control_y, getWidth() - widget_margin - frequency_x,
                     knob_section_height - 2 * widget_margin, frequency_.get(), sync_.get());
  tempo_->setBounds(frequency_->getBounds());
  tempo_->setModulationArea(frequency_->getModulationArea());

  Rectangle<int> divider_bounds = frequency_->getModulationArea() + frequency_->getBounds().getTopLeft();
  divider_bounds = divider_bounds.reduced(divider_bounds.getHeight() / 4);
  divider_bounds.setX(divider_bounds.getCentreX());
  divider_bounds.setWidth(1);
  transpose_tune_divider_->setBounds(divider_bounds);

  Rectangle<int> frequency_bounds = frequency_->getBounds();
  keytrack_transpose_->setBounds(frequency_bounds.withWidth(frequency_bounds.getWidth() / 2));
  keytrack_tune_->setBounds(frequency_bounds.withLeft(keytrack_transpose_->getRight()));
  keytrack_transpose_->setModulationArea(frequency_->getModulationArea().withWidth(keytrack_transpose_->getWidth()));
  keytrack_tune_->setModulationArea(frequency_->getModulationArea().withWidth(keytrack_tune_->getWidth()));

  SynthSection::resized();
}
