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

#include "chorus_section.h"

#include "chorus_module.h"
#include "delay_section.h"
#include "skin.h"
#include "fonts.h"
#include "synth_strings.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "tempo_selector.h"
#include "text_look_and_feel.h"

class ChorusViewer : public BarRenderer {
  public:
    static constexpr int kDelays = vital::ChorusModule::kMaxDelayPairs;
    static constexpr int kNumBars = kDelays * vital::poly_float::kSize;
  
    ChorusViewer() : BarRenderer(kNumBars, true), delays_() {
      active_ = true;
      parent_ = nullptr;
      num_voices_ = nullptr;

      setBarWidth(0.3f);
      setScale(1.0f);
      setAdditiveBlending(true);
    }

    void parentHierarchyChanged() override {
      if (delays_[0])
        return;

      SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
      if (parent == nullptr)
        return;

      for (int i = 0; i < kDelays; ++i)
        delays_[i] = parent->getSynth()->getStatusOutput("chorus_delays" + std::to_string(i + 1));
    }

    void drawBars(OpenGlWrapper& open_gl, bool animate) {
      static constexpr float kMaxDelay = 0.07f;
      static constexpr float kMinDelay = 0.000f;
      static constexpr float kDelayRange = kMaxDelay - kMinDelay;
      if (delays_[0] == nullptr)
        return;
      
      int num_voices = num_voices_->getValue() * vital::poly_float::kSize;
      for (int i = 0; i < num_voices; ++i) {
        vital::poly_float delay_frequency = delays_[i / vital::poly_float::kSize]->value();
        float delay = 1.0f / delay_frequency[i % vital::poly_float::kSize];
        setX(i, 2.0f * (delay - kMinDelay) / kDelayRange - 1.0f);
        setY(i, 0.5f);
        setBottom(i, -0.5f);
      }
      
      for (int i = num_voices; i < num_points_; ++i)
        setX(i , -2.0f);

      BarRenderer::render(open_gl, animate);
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      if (active_) {
        Colour color = findColour(Skin::kWidgetSecondary1, true);
        float alpha = color.getFloatAlpha();
        setColor(color.withAlpha(alpha + (1.0f - alpha) * alpha));
      }
      else
        setColor(findColour(Skin::kWidgetSecondaryDisabled, true));

      drawBars(open_gl, animate);
      renderCorners(open_gl, animate);
    }
  
    void setActive(bool active) { active_ = active; }
    void setNumVoicesSlider(SynthSlider* num_voices) { num_voices_ = num_voices; }

  private:
    bool active_;
    const vital::StatusOutput* delays_[4];
    SynthSlider* num_voices_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusViewer)
};

ChorusSection::ChorusSection(const String& name, const vital::output_map& mono_modulations) : SynthSection(name) {
  static const double kTempoDragSensitivity = 0.5;
  static constexpr int kViewerResolution = 64;

  voices_ = std::make_unique<SynthSlider>("chorus_voices");
  addSlider(voices_.get());
  voices_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  voices_->setLookAndFeel(TextLookAndFeel::instance());
  voices_->setSensitivity(kTempoDragSensitivity);

  delay_1_ = std::make_unique<SynthSlider>("chorus_delay_1");
  addSlider(delay_1_.get());
  delay_1_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  delay_2_ = std::make_unique<SynthSlider>("chorus_delay_2");
  addSlider(delay_2_.get());
  delay_2_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  mod_depth_ = std::make_unique<SynthSlider>("chorus_mod_depth");
  addSlider(mod_depth_.get());
  mod_depth_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  frequency_ = std::make_unique<SynthSlider>("chorus_frequency");
  addSlider(frequency_.get());
  frequency_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  frequency_->setLookAndFeel(TextLookAndFeel::instance());

  tempo_ = std::make_unique<SynthSlider>("chorus_tempo");
  addSlider(tempo_.get());
  tempo_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tempo_->setLookAndFeel(TextLookAndFeel::instance());
  tempo_->setSensitivity(kTempoDragSensitivity);

  sync_ = std::make_unique<TempoSelector>("chorus_sync");
  addSlider(sync_.get());
  sync_->setSliderStyle(Slider::LinearBar);
  sync_->setTempoSlider(tempo_.get());
  sync_->setFreeSlider(frequency_.get());

  feedback_ = std::make_unique<SynthSlider>("chorus_feedback");
  addSlider(feedback_.get());
  feedback_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  feedback_->setBipolar();
  feedback_->snapToValue(true);

  dry_wet_ = std::make_unique<SynthSlider>("chorus_dry_wet");
  addSlider(dry_wet_.get());
  dry_wet_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  filter_cutoff_ = std::make_unique<SynthSlider>("chorus_cutoff");
  addSlider(filter_cutoff_.get());
  filter_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  filter_spread_ = std::make_unique<SynthSlider>("chorus_spread");
  addSlider(filter_spread_.get());
  filter_spread_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  
  chorus_viewer_ = std::make_unique<ChorusViewer>();
  addOpenGlComponent(chorus_viewer_.get());
  chorus_viewer_->setNumVoicesSlider(voices_.get());
  
  filter_viewer_ = std::make_unique<DelayFilterViewer>("chorus", kViewerResolution, mono_modulations);
  filter_viewer_->setCutoffSlider(filter_cutoff_.get());
  filter_viewer_->setSpreadSlider(filter_spread_.get());
  filter_viewer_->addListener(this);
  addOpenGlComponent(filter_viewer_.get());

  on_ = std::make_unique<SynthButton>("chorus_on");
  addButton(on_.get());
  setActivator(on_.get());
  setSkinOverride(Skin::kChorus);
}

ChorusSection::~ChorusSection() = default;

void ChorusSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);
  
  Rectangle<int> frequency_bounds(tempo_->getX(), tempo_->getY(),
                                  sync_->getRight() - tempo_->getX(), tempo_->getHeight());
  drawTextComponentBackground(g, frequency_bounds, true);
  drawTextComponentBackground(g, voices_->getBounds(), true);

  setLabelFont(g);
  drawLabel(g, TRANS("FREQUENCY"), frequency_bounds, true);
  drawLabelForComponent(g, TRANS("VOICES"), voices_.get(), true);
  drawLabelForComponent(g, TRANS("FEEDBACK"), feedback_.get());
  drawLabelForComponent(g, TRANS("MIX"), dry_wet_.get());
  drawLabelForComponent(g, TRANS("DEPTH"), mod_depth_.get());
  drawLabelForComponent(g, TRANS("DELAY 1"), delay_1_.get());
  drawLabelForComponent(g, TRANS("DELAY 2"), delay_2_.get());
  drawLabelForComponent(g, TRANS("CUTOFF"), filter_cutoff_.get());
  drawLabelForComponent(g, TRANS("SPREAD"), filter_spread_.get());

  drawTempoDivider(g, sync_.get());
}

void ChorusSection::resized() {
  int widget_margin = findValue(Skin::kWidgetMargin);
  int title_width = getTitleWidth();
  int section_height = getKnobSectionHeight();
  
  Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
  Rectangle<int> delay_area = getDividedAreaBuffered(bounds, 3, 0, widget_margin);
  Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 3, 2, widget_margin);

  placeKnobsInArea(Rectangle<int>(delay_area.getX(), 0, delay_area.getWidth(), section_height),
                   { voices_.get(), tempo_.get()});
  voices_->setBounds(voices_->getBounds().withTop(widget_margin));
  placeTempoControls(tempo_->getX(), widget_margin, tempo_->getWidth(),
                     section_height - 2 * widget_margin, frequency_.get(), sync_.get());
  tempo_->setBounds(frequency_->getBounds());
  tempo_->setModulationArea(frequency_->getModulationArea());

  int delay_y = section_height - widget_margin;
  placeKnobsInArea(Rectangle<int>(delay_area.getX(), delay_y, delay_area.getWidth(), section_height),
                   { mod_depth_.get(), delay_1_.get(), delay_2_.get() });

  int widget_x = delay_2_->getRight() + widget_margin;
  int viewer_width = knobs_area.getX() - widget_x;
  int delay_height = (getHeight() - 3 * widget_margin) / 2;
  int filter_y = delay_height + 2 * widget_margin;
  chorus_viewer_->setBounds(widget_x, widget_margin, viewer_width, delay_height);
  filter_viewer_->setBounds(widget_x, filter_y, viewer_width, getHeight() - filter_y - widget_margin);

  placeKnobsInArea(Rectangle<int>(knobs_area.getX(), 0, knobs_area.getWidth(), section_height),
                   { feedback_.get(),  dry_wet_.get() });

  int knob_y2 = section_height - widget_margin;
  placeKnobsInArea(Rectangle<int>(knobs_area.getX(), knob_y2, knobs_area.getWidth(), section_height),
                   { filter_cutoff_.get(), filter_spread_.get() });

  SynthSection::resized();
}

void ChorusSection::setActive(bool active) {
  SynthSection::setActive(active);
  chorus_viewer_->setActive(active);
  filter_viewer_->setActive(active);
}

void ChorusSection::deltaMovement(float x, float y) {
  double x_range = filter_cutoff_->getMaximum() - filter_cutoff_->getMinimum();
  double y_range = filter_spread_->getMaximum() - filter_spread_->getMinimum();

  filter_cutoff_->setValue(filter_cutoff_->getValue() + x * x_range);
  filter_spread_->setValue(filter_spread_->getValue() + y * y_range);
}
