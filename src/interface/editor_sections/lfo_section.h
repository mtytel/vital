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

#include "synth_section.h"
#include "lfo_editor.h"
#include "preset_selector.h"

class RetriggerSelector;
class SynthSlider;
class TempoSelector;
class TextSelector;
class LineGenerator;
class PaintPatternSelector;

class LfoSection : public SynthSection, public PresetSelector::Listener, public LineEditor::Listener {
  public:
    enum PaintPattern {
      kStep,
      kHalf,
      kDown,
      kUp,
      kTri,
      kNumPaintPatterns
    };

    static std::vector<std::pair<float, float>> getPaintPattern(int pattern) {
      if (pattern == LfoSection::kHalf) {
        return {
            { 0.0f, 1.0f },
            { 0.5f, 1.0f },
            { 0.5f, 0.0f },
            { 1.0f, 0.0f }
        };
      }
      if (pattern == LfoSection::kDown) {
        return {
            { 0.0f, 1.0f },
            { 1.0f, 0.0f }
        };
      }
      if (pattern == LfoSection::kUp) {
        return {
            { 0.0f, 0.0f },
            { 1.0f, 1.0f }
        };
      }
      if (pattern == LfoSection::kTri) {
        return {
            { 0.0f, 0.0f },
            { 0.5f, 1.0f },
            { 1.0f, 0.0f }
        };
      }
      return {
          { 0.0f, 1.0f },
          { 1.0f, 1.0f }
      };
    }

    LfoSection(String name, std::string value_prepend,
               LineGenerator* lfo_source,
               const vital::output_map& mono_modulations,
               const vital::output_map& poly_modulations);
    ~LfoSection();

    void paintBackground(Graphics& g) override;
    void resized() override;
    void reset() override;
    void setAllValues(vital::control_map& controls) override;
    void sliderValueChanged(Slider* changed_slider) override;
    void buttonClicked(Button* clicked_button) override;

    void setPhase(float phase) override { phase_->setValue(phase); }
    void lineEditorScrolled(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void togglePaintMode(bool enabled, bool temporary_switch) override;
    void importLfo() override;
    void exportLfo() override;
    void fileLoaded() override;

    void loadFile(const File& file) override;
    File getCurrentFile() override { return current_file_; }

    void prevClicked() override;
    void nextClicked() override;
    void textMouseDown(const MouseEvent& e) override;

    void setSmoothModeSelected(int result);

  private:
    File current_file_;

    std::unique_ptr<LfoEditor> editor_;
    std::unique_ptr<PresetSelector> preset_selector_;

    std::unique_ptr<SynthSlider> phase_;
    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<SynthSlider> keytrack_transpose_;
    std::unique_ptr<SynthSlider> keytrack_tune_;

    std::unique_ptr<SynthSlider> fade_;
    std::unique_ptr<SynthSlider> smooth_;
    std::string smooth_mode_control_name_;
    std::unique_ptr<PlainTextComponent> smooth_mode_text_;
    std::unique_ptr<ShapeButton> smooth_mode_type_selector_;

    std::unique_ptr<SynthSlider> delay_;
    std::unique_ptr<SynthSlider> stereo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<TextSelector> sync_type_;

    std::unique_ptr<PaintPatternSelector> paint_pattern_;
    std::unique_ptr<OpenGlQuad> transpose_tune_divider_;

    std::unique_ptr<SynthSlider> grid_size_x_;
    std::unique_ptr<SynthSlider> grid_size_y_;
    std::unique_ptr<OpenGlShapeButton> paint_;
    std::unique_ptr<OpenGlShapeButton> lfo_smooth_;

    int current_preset_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoSection)
};

