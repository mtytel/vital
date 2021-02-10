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

#include "delay_section.h"

#include "skin.h"
#include "fonts.h"
#include "bar_renderer.h"
#include "synth_strings.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "tempo_selector.h"
#include "text_look_and_feel.h"
#include "text_selector.h"
#include "futils.h"

namespace {
  vital::poly_float getValue(const vital::Output* output, Slider* slider) {
    if (slider && !output->owner->enabled())
      return slider->getValue();
    return output->trigger_value;
  }
}

class DelayViewer : public BarRenderer {
  public:
    DelayViewer(int num_bars, const vital::output_map& mono_modulations) : BarRenderer(num_bars, true),
        active_(true), feedback_slider_(nullptr), mix_slider_(nullptr),
        tempo_slider_(nullptr), frequency_slider_(nullptr), sync_slider_(nullptr),
      aux_tempo_slider_(nullptr), aux_frequency_slider_(nullptr), aux_sync_slider_(nullptr), style_slider_(nullptr) {
      setBarWidth(0.3f);
      setScale(1.0f);
      setAdditiveBlending(false);

      feedback_ = mono_modulations.at("delay_feedback");
      mix_ = mono_modulations.at("delay_dry_wet");
      tempo_ = mono_modulations.at("delay_tempo");
      frequency_ = mono_modulations.at("delay_frequency");

      aux_tempo_ = mono_modulations.at("delay_aux_tempo");
      aux_frequency_ = mono_modulations.at("delay_aux_frequency");
    }

    float getFeedback(int index) {
      return std::max(-1.0f, std::min(1.0f, getValue(feedback_, feedback_slider_)[index]));
    }

    vital::poly_float getMix() {
      return getValue(mix_, mix_slider_);
    }

    float getRawFrequency(int index) {
      if (index && style_slider_->getValue() != vital::StereoDelay::kMono)
        return getValue(aux_frequency_, aux_frequency_slider_)[0];
      return getValue(frequency_, frequency_slider_)[index];
    }

    float getMultiplier(int index) {
      static constexpr float kDottedMultiplier = 3.0f / 2.0f;
      static constexpr float kTripletMultiplier = 2.0f / 3.0f;

      Slider* sync_slider = aux_sync_slider_;
      if (index == 0 || style_slider_->getValue() == vital::StereoDelay::kMono)
        sync_slider = sync_slider_;

      if (sync_slider->getValue() == vital::TempoChooser::kDottedMode)
        return kDottedMultiplier;
      if (sync_slider->getValue() == vital::TempoChooser::kTripletMode)
        return kTripletMultiplier;
      return 1.0f;
    }

    float getTempoFrequency(int index) {
      static constexpr float kDefaultPowerOffset = -6.0f;
      if (index && style_slider_->getValue() != vital::StereoDelay::kMono)
        return std::round(getValue(aux_tempo_, aux_tempo_slider_)[0]) + kDefaultPowerOffset;
      return std::round(getValue(tempo_, tempo_slider_)[index]) + kDefaultPowerOffset;
    }

    float getFrequency(int index) {
      bool frequency_mode = sync_slider_->getValue() == vital::TempoChooser::kFrequencyMode;
      bool aux_frequency_mode = aux_sync_slider_->getValue() == vital::TempoChooser::kFrequencyMode;
      bool mono = style_slider_->getValue() == vital::StereoDelay::kMono;
      if ((index == 0 && frequency_mode) || (index == 1 && aux_frequency_mode && !mono) ||
          (index == 1 && frequency_mode && mono)) {
        return getRawFrequency(index);
      }

      return getTempoFrequency(index);
    }

    void drawBars(OpenGlWrapper& open_gl, bool animate, int index) {
      static constexpr float kMaxSeconds = 4.0f;

      float feedback = std::abs(getFeedback(index));
      vital::poly_float mix_value = vital::utils::clamp(getMix(), 0.0f, 1.0f);
      float wet = vital::futils::equalPowerFade(mix_value)[index];
      float dry = vital::futils::equalPowerFade(-mix_value + 1.0f)[index];
      float increment = 2.0f * powf(2.0f, -getFrequency(index)) * getMultiplier(index) / kMaxSeconds;
      float other_increment = 2.0f * powf(2.0f, -getFrequency(1 - index)) * getMultiplier(1 - index) / kMaxSeconds;
      float even_increment = increment;
      float odd_increment = increment;
      float x = -1.0f;
      if (style_slider_->getValue() == vital::StereoDelay::kPingPong) {
        if (index)
          x -= other_increment;
        even_increment = increment + other_increment;
        odd_increment = even_increment;
      }
      else if (style_slider_->getValue() == vital::StereoDelay::kMidPingPong) {
        if (index == 0) {
          odd_increment = other_increment + increment;
          even_increment = other_increment;
        }
        else {
          odd_increment = increment;
          even_increment = other_increment + increment;
        }
      }

      setBarWidth(std::min(1.0f, num_points_ * increment / 2.0f));

      float mult_top = index ? 0.0f : 1.0f;
      float mult_bottom = index ? -1.0f : 0.0f;

      setY(0, mult_top * dry);
      setBottom(0, mult_bottom * dry);
      for (int i = 1; i < num_points_; ++i) {
        if (i % 2)
          x += odd_increment;
        else
          x += even_increment;
        setX(i, x);
        setY(i, mult_top * wet);
        setBottom(i, mult_bottom * wet);

        wet *= feedback;
      }

      BarRenderer::render(open_gl, animate);
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      if (active_)
        setColor(findColour(Skin::kWidgetPrimary1, true));
      else
        setColor(findColour(Skin::kWidgetPrimaryDisabled, true));

      drawBars(open_gl, animate, 0);

      if (active_)
        setColor(findColour(Skin::kWidgetPrimary2, true));
      else
        setColor(findColour(Skin::kWidgetPrimaryDisabled, true));

      drawBars(open_gl, animate, 1);
      renderCorners(open_gl, animate);
    }

    void mouseDown(const MouseEvent& e) override {
      last_mouse_position_ = e.getPosition();
    }

    void mouseDrag(const MouseEvent& e) override {
      Point<int> delta = e.getPosition() - last_mouse_position_;
      last_mouse_position_ = e.getPosition();

      float feedback_range = feedback_slider_->getMaximum() - feedback_slider_->getMinimum();
      feedback_slider_->setValue(feedback_slider_->getValue() - delta.y * feedback_range / getWidth());
    }

    void setFeedbackSlider(Slider* slider) { feedback_slider_ = slider; }
    void setMixSlider(Slider* slider) { mix_slider_ = slider; }
    void setTempoSlider(Slider* slider) { tempo_slider_ = slider; }
    void setFrequencySlider(Slider* slider) { frequency_slider_ = slider; }
    void setSyncSlider(Slider* slider) { sync_slider_ = slider; }
    void setAuxTempoSlider(Slider* slider) { aux_tempo_slider_ = slider; }
    void setAuxFrequencySlider(Slider* slider) { aux_frequency_slider_ = slider; }
    void setAuxSyncSlider(Slider* slider) { aux_sync_slider_ = slider; }
    void setStyleSlider(Slider* slider) { style_slider_ = slider; }

    void setActive(bool active) { active_ = active; }

  private:
    bool active_;
    Point<int> last_mouse_position_;

    vital::Output* feedback_;
    vital::Output* mix_;
    vital::Output* tempo_;
    vital::Output* frequency_;

    vital::Output* aux_tempo_;
    vital::Output* aux_frequency_;

    Slider* feedback_slider_;
    Slider* mix_slider_;
    Slider* tempo_slider_;
    Slider* frequency_slider_;
    Slider* sync_slider_;
    Slider* aux_tempo_slider_;
    Slider* aux_frequency_slider_;
    Slider* aux_sync_slider_;
    Slider* style_slider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayViewer)
};

DelaySection::DelaySection(const String& name, const vital::output_map& mono_modulations) : SynthSection(name) {
  static const double TEMPO_DRAG_SENSITIVITY = 0.3;
  static constexpr int kViewerResolution = 64;
  
  frequency_ = std::make_unique<SynthSlider>("delay_frequency");
  addSlider(frequency_.get());
  frequency_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  frequency_->setLookAndFeel(TextLookAndFeel::instance());

  tempo_ = std::make_unique<SynthSlider>("delay_tempo");
  addSlider(tempo_.get());
  tempo_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tempo_->setLookAndFeel(TextLookAndFeel::instance());
  tempo_->setSensitivity(TEMPO_DRAG_SENSITIVITY);

  sync_ = std::make_unique<TempoSelector>("delay_sync");
  addSlider(sync_.get());
  sync_->setSliderStyle(Slider::LinearBar);
  sync_->setTempoSlider(tempo_.get());
  sync_->setFreeSlider(frequency_.get());

  aux_frequency_ = std::make_unique<SynthSlider>("delay_aux_frequency");
  addSlider(aux_frequency_.get());
  aux_frequency_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  aux_frequency_->setLookAndFeel(TextLookAndFeel::instance());

  aux_tempo_ = std::make_unique<SynthSlider>("delay_aux_tempo");
  addSlider(aux_tempo_.get());
  aux_tempo_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  aux_tempo_->setLookAndFeel(TextLookAndFeel::instance());
  aux_tempo_->setSensitivity(TEMPO_DRAG_SENSITIVITY);

  aux_sync_ = std::make_unique<TempoSelector>("delay_aux_sync");
  addSlider(aux_sync_.get());
  aux_sync_->setSliderStyle(Slider::LinearBar);
  aux_sync_->setTempoSlider(aux_tempo_.get());
  aux_sync_->setFreeSlider(aux_frequency_.get());

  filter_cutoff_ = std::make_unique<SynthSlider>("delay_filter_cutoff");
  addSlider(filter_cutoff_.get());
  filter_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  filter_spread_ = std::make_unique<SynthSlider>("delay_filter_spread");
  addSlider(filter_spread_.get());
  filter_spread_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  feedback_ = std::make_unique<SynthSlider>("delay_feedback");
  addSlider(feedback_.get());
  feedback_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  feedback_->setBipolar();

  dry_wet_ = std::make_unique<SynthSlider>("delay_dry_wet");
  addSlider(dry_wet_.get());
  dry_wet_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  style_ = std::make_unique<TextSelector>("delay_style");
  addSlider(style_.get());
  style_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  style_->setLookAndFeel(TextLookAndFeel::instance());
  style_->setLongStringLookup(strings::kDelayStyleNames);

  delay_viewer_ = std::make_unique<DelayViewer>(50, mono_modulations);
  delay_viewer_->setFeedbackSlider(feedback_.get());
  delay_viewer_->setMixSlider(dry_wet_.get());
  delay_viewer_->setTempoSlider(tempo_.get());
  delay_viewer_->setFrequencySlider(frequency_.get());
  delay_viewer_->setAuxFrequencySlider(aux_frequency_.get());
  delay_viewer_->setAuxTempoSlider(aux_tempo_.get());
  delay_viewer_->setAuxSyncSlider(aux_sync_.get());
  delay_viewer_->setSyncSlider(sync_.get());
  delay_viewer_->setStyleSlider(style_.get());
  addOpenGlComponent(delay_viewer_.get());

  delay_filter_viewer_ = std::make_unique<DelayFilterViewer>("delay_filter", kViewerResolution, mono_modulations);
  delay_filter_viewer_->setCutoffSlider(filter_cutoff_.get());
  delay_filter_viewer_->setSpreadSlider(filter_spread_.get());
  delay_filter_viewer_->addListener(this);
  addOpenGlComponent(delay_filter_viewer_.get());

  on_ = std::make_unique<SynthButton>("delay_on");
  addButton(on_.get());
  setActivator(on_.get());
  setSkinOverride(Skin::kDelay);
}

DelaySection::~DelaySection() = default;

void DelaySection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  int section_height = getKnobSectionHeight();
  int frequency_x = tempo_->getX();
  int frequency_width = std::max(sync_->getRight(), aux_sync_->getRight()) - frequency_x;
  int widget_margin = findValue(Skin::kWidgetMargin);
  Rectangle<int> frequency_bounds(frequency_x, widget_margin, frequency_width, section_height - 2  * widget_margin);
  drawTextComponentBackground(g, frequency_bounds, true);
  drawTextComponentBackground(g, style_->getBounds(), true);

  setLabelFont(g);
  drawLabelForComponent(g, TRANS("FEEDBACK"), feedback_.get());
  drawLabelForComponent(g, TRANS("MIX"), dry_wet_.get());
  drawLabelForComponent(g, TRANS("CUTOFF"), filter_cutoff_.get());
  drawLabelForComponent(g, TRANS("SPREAD"), filter_spread_.get());
  drawLabelForComponent(g, TRANS("MODE"), style_.get(), true);
  drawLabel(g, TRANS("FREQUENCY"), frequency_bounds, true);
  drawTempoDivider(g, sync_.get());
  drawTempoDivider(g, aux_sync_.get());
}

void DelaySection::resized() {
  int title_width = getTitleWidth();
  int knob_section_height = getKnobSectionHeight();

  int widget_margin = findValue(Skin::kWidgetMargin);
  Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
  Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 3, 2, widget_margin);

  int knob_y2 = getHeight() - knob_section_height;

  Rectangle<int> text_area = getDividedAreaUnbuffered(bounds, 3, 0, widget_margin);
  style_->setBounds(text_area.getX(), knob_y2 + widget_margin, text_area.getWidth(),
                    knob_section_height - 2 * widget_margin);

  int widget_x = text_area.getRight() + widget_margin;
  int viewer_width = knobs_area.getX() - widget_x;
  int delay_height = (getHeight() - 3 * widget_margin) / 2;
  delay_viewer_->setBounds(widget_x, widget_margin, viewer_width, delay_height);

  int filter_y = delay_viewer_->getBottom() + widget_margin;
  delay_filter_viewer_->setBounds(widget_x, filter_y, viewer_width, getHeight() - filter_y - widget_margin);

  placeKnobsInArea(knobs_area.withBottom(knob_section_height), { feedback_.get(), dry_wet_.get() });
  placeKnobsInArea(knobs_area.withTop(knob_y2).withHeight(knob_section_height),
                   { filter_cutoff_.get(), filter_spread_.get() });

  resizeTempoControls();
  SynthSection::resized();
}

void DelaySection::setActive(bool active) {
  SynthSection::setActive(active);
  delay_filter_viewer_->setActive(active);
  delay_viewer_->setActive(active);
}

void DelaySection::resizeTempoControls() {
  int knob_section_height = getKnobSectionHeight();

  int widget_margin = findValue(Skin::kWidgetMargin);
  int text_component_width = style_->getWidth();
  int text_component_height = style_->getHeight();
  int text_control_x = style_->getX();

  if (style_->getValue() == vital::StereoDelay::kMono) {
    placeTempoControls(text_control_x, widget_margin, text_component_width,
                       text_component_height, frequency_.get(), sync_.get());
    tempo_->setBounds(frequency_->getBounds());
    tempo_->setModulationArea(frequency_->getModulationArea());

    aux_frequency_->setBounds(0, 0, 0, 0);
    aux_sync_->setBounds(0, 0, 0, 0);
    aux_tempo_->setBounds(0, 0, 0, 0);
  }
  else {
    int width = text_component_width / 2;
    placeTempoControls(text_control_x, widget_margin, width,
                       text_component_height, frequency_.get(), sync_.get());
    tempo_->setBounds(frequency_->getBounds());
    tempo_->setModulationArea(frequency_->getModulationArea());

    int width2 = text_component_width - width;
    placeTempoControls(text_control_x + width, widget_margin, width2,
                       knob_section_height - 2 * widget_margin, aux_frequency_.get(), aux_sync_.get());
    aux_tempo_->setBounds(aux_frequency_->getBounds());
    aux_tempo_->setModulationArea(aux_frequency_->getModulationArea());
  }
}

void DelaySection::sliderValueChanged(Slider* changed_slider) {
  SynthSection::sliderValueChanged(changed_slider);

  if (changed_slider == style_.get()) {
    if (aux_tempo_->getWidth() == 0) {
      aux_tempo_->setValue(tempo_->getValue());
      aux_sync_->setValue(sync_->getValue());
      aux_frequency_->setValue(frequency_->getValue());
    }
    resizeTempoControls();
    repaintBackground();
  }
}

void DelaySection::deltaMovement(float x, float y) {
  double x_range = filter_cutoff_->getMaximum() - filter_cutoff_->getMinimum();
  double y_range = filter_spread_->getMaximum() - filter_spread_->getMinimum();

  filter_cutoff_->setValue(filter_cutoff_->getValue() + x * x_range);
  filter_spread_->setValue(filter_spread_->getValue() + y * y_range);
}
