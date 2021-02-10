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

#pragma once

#include "JuceHeader.h"
#include "delay.h"
#include "open_gl_line_renderer.h"
#include "synth_section.h"

class DelayViewer;
class SynthButton;
class SynthSlider;
class TempoSelector;
class TextSelector;
  
class DelayFilterViewer : public OpenGlLineRenderer {
  public:
    static constexpr float kMidiDrawStart = 8.0f;
    static constexpr float kMidiDrawEnd = 132.0f;
    static constexpr float kMinDb = -18.0f;
    static constexpr float kMaxDb = 6.0f;

    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void deltaMovement(float x, float y) = 0;
    };

    DelayFilterViewer(const std::string& prefix, int resolution, const vital::output_map& mono_modulations) :
        OpenGlLineRenderer(resolution), active_(true), cutoff_slider_(nullptr), spread_slider_(nullptr) {
      setFill(true);
      setFillCenter(-1.0f);

      cutoff_ = mono_modulations.at(prefix + "_cutoff");
      spread_ = mono_modulations.at(prefix + "_spread");
    }

    vital::poly_float getCutoff() {
      if (cutoff_slider_ && !cutoff_->owner->enabled())
        return cutoff_slider_->getValue();
      return cutoff_->trigger_value;
    }

    vital::poly_float getSpread() {
      if (spread_slider_ && !spread_->owner->enabled())
        return spread_slider_->getValue();
      return spread_->trigger_value;
    }

    void drawLines(OpenGlWrapper& open_gl, bool animate, float high_midi_cutoff, float low_midi_cutoff) {
      float midi_increment = (kMidiDrawEnd - kMidiDrawStart) / (numPoints() - 1);
      float mult_increment = vital::utils::centsToRatio(midi_increment * vital::kCentsPerNote);

      float high_midi_offset_start = kMidiDrawStart - high_midi_cutoff;
      float high_ratio = vital::utils::centsToRatio(high_midi_offset_start * vital::kCentsPerNote);
      float low_midi_offset_start = kMidiDrawStart - low_midi_cutoff;
      float low_ratio = vital::utils::centsToRatio(low_midi_offset_start * vital::kCentsPerNote);

      float gain = vital::utils::centsToRatio((high_midi_cutoff - low_midi_cutoff) * vital::kCentsPerNote) + 1.0f;

      float width = getWidth();
      float height = getHeight();
      int num_points = numPoints();
      for (int i = 0; i < num_points; ++i) {
        float high_response = high_ratio / sqrtf(1 + high_ratio * high_ratio);
        float low_response = 1.0f / sqrtf(1 + low_ratio * low_ratio);
        float response = gain * low_response * high_response;
        float db = vital::utils::magnitudeToDb(response);
        float y = (db - kMinDb) / (kMaxDb - kMinDb);

        setXAt(i, width * i / (num_points - 1.0f));
        setYAt(i, (1.0f - y) * height);

        high_ratio *= mult_increment;
        low_ratio *= mult_increment;
      }

      OpenGlLineRenderer::render(open_gl, animate);
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      vital::poly_float cutoff = getCutoff();
      vital::poly_float radius = vital::StereoDelay::getFilterRadius(getSpread());
      vital::poly_float high_midi_cutoff = cutoff - radius;
      vital::poly_float low_midi_cutoff = cutoff + radius;

      setLineWidth(findValue(Skin::kWidgetLineWidth));
      setFillCenter(findValue(Skin::kWidgetFillCenter));

      float fill_fade = findValue(Skin::kWidgetFillFade);
      Colour color_fill_to;
      if (active_) {
        setColor(findColour(Skin::kWidgetPrimary1, true));
        color_fill_to = findColour(Skin::kWidgetSecondary1, true);
      }
      else {
        setColor(findColour(Skin::kWidgetPrimaryDisabled, true));
        color_fill_to = findColour(Skin::kWidgetSecondaryDisabled, true);
      }

      Colour color_fill_from = color_fill_to.withMultipliedAlpha(1.0f - fill_fade);
      setFillColors(color_fill_from, color_fill_to);
      drawLines(open_gl, animate, high_midi_cutoff[0], low_midi_cutoff[0]);

      if (active_) {
        setColor(findColour(Skin::kWidgetPrimary2, true));
        color_fill_to = findColour(Skin::kWidgetSecondary2, true);
      }
      else {
        setColor(findColour(Skin::kWidgetPrimaryDisabled, true));
        color_fill_to = findColour(Skin::kWidgetSecondaryDisabled, true);
      }
      color_fill_from = color_fill_to.withMultipliedAlpha(1.0f - fill_fade);
      setFillColors(color_fill_from, color_fill_to);
      drawLines(open_gl, animate, high_midi_cutoff[1], low_midi_cutoff[1]);

      renderCorners(open_gl, animate);
    }

    void mouseDown(const MouseEvent& e) override {
      last_mouse_position_ = e.getPosition();
    }

    void mouseDrag(const MouseEvent& e) override {
      Point<int> delta = e.getPosition() - last_mouse_position_;
      last_mouse_position_ = e.getPosition();

      for (Listener* listener : listeners_)
        listener->deltaMovement(delta.x * 1.0f / getWidth(), -delta.y * 1.0f / getHeight());
    }

    void setCutoffSlider(Slider* slider) { cutoff_slider_ = slider; }
    void setSpreadSlider(Slider* slider) { spread_slider_ = slider; }
    void setActive(bool active) { active_ = active; }
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    bool active_;
    std::vector<Listener*> listeners_;
    Point<int> last_mouse_position_;

    vital::Output* cutoff_;
    vital::Output* spread_;
    Slider* cutoff_slider_;
    Slider* spread_slider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayFilterViewer)
};

class DelaySection : public SynthSection, DelayFilterViewer::Listener {
  public:
    DelaySection(const String& name, const vital::output_map& mono_modulations);
    virtual ~DelaySection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void setActive(bool active) override;
    void resizeTempoControls();

    void setAllValues(vital::control_map& controls) override {
      SynthSection::setAllValues(controls);
      resizeTempoControls();
    }
    void sliderValueChanged(Slider* changed_slider) override;
    void deltaMovement(float x, float y) override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<SynthSlider> aux_frequency_;
    std::unique_ptr<SynthSlider> aux_tempo_;
    std::unique_ptr<TempoSelector> aux_sync_;
    std::unique_ptr<SynthSlider> feedback_;
    std::unique_ptr<SynthSlider> dry_wet_;
    std::unique_ptr<SynthSlider> filter_cutoff_;
    std::unique_ptr<SynthSlider> filter_spread_;
    std::unique_ptr<TextSelector> style_;

    std::unique_ptr<DelayFilterViewer> delay_filter_viewer_;
    std::unique_ptr<DelayViewer> delay_viewer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelaySection)
};

