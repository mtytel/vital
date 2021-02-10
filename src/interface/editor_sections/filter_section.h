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
#include "preset_selector.h"

class FilterResponse;
class OpenGlToggleButton;
class SynthButton;
class SynthSlider;
class TextSelector;
class TextSlider;

class FilterSection : public SynthSection, public PresetSelector::Listener {
  public:
    static constexpr int kBlendLabelPaddingY = 4;

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void filterSerialSelected(FilterSection* section) = 0;
        virtual void oscInputToggled(FilterSection* section, int index, bool on) = 0;
        virtual void sampleInputToggled(FilterSection* section, bool on) = 0;
    };

    FilterSection(String suffix, const vital::output_map& mono_modulations);
    FilterSection(int index, const vital::output_map& mono_modulations, const vital::output_map& poly_modulations);
    ~FilterSection();

    void setFilterResponseSliders();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void positionTopBottom();
    void positionLeftRight();
    void resized() override;
    void buttonClicked(Button* clicked_button) override;
    void setAllValues(vital::control_map& controls) override;

    void prevClicked() override;
    void nextClicked() override;
    void textMouseDown(const MouseEvent& e) override;

    Path getLeftMorphPath();
    Path getRightMorphPath();

    void setActive(bool active) override;
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void setFilterSelected(int menu_id);
    void clearFilterInput() { filter_input_->setToggleState(false, sendNotification); }
    void setOscillatorInput(int oscillator_index, bool input);
    void setSampleInput(bool input);

  private:
    FilterSection(String name, String suffix);

    void showModelKnobs();
    void setFilterText();
    void setLabelText();
    void notifyFilterChange();

    std::vector<Listener*> listeners_;

    std::string model_name_;
    std::string style_name_;
    int current_model_;
    int current_style_;
    bool specify_input_;

    std::unique_ptr<SynthButton> filter_on_;
    std::unique_ptr<PresetSelector> preset_selector_;
    std::unique_ptr<FilterResponse> filter_response_;
    std::unique_ptr<SynthSlider> mix_;
    std::unique_ptr<SynthSlider> cutoff_;
    std::unique_ptr<SynthSlider> resonance_;
    std::unique_ptr<SynthSlider> blend_;
    std::unique_ptr<SynthSlider> keytrack_;
    std::unique_ptr<SynthSlider> drive_;

    std::unique_ptr<SynthSlider> formant_x_;
    std::unique_ptr<SynthSlider> formant_y_;
    std::unique_ptr<SynthSlider> formant_transpose_;
    std::unique_ptr<SynthSlider> formant_resonance_;
    std::unique_ptr<SynthSlider> formant_spread_;

    std::unique_ptr<OpenGlToggleButton> osc1_input_;
    std::unique_ptr<OpenGlToggleButton> osc2_input_;
    std::unique_ptr<OpenGlToggleButton> osc3_input_;
    std::unique_ptr<OpenGlToggleButton> sample_input_;
    std::unique_ptr<SynthButton> filter_input_;

    std::unique_ptr<PlainTextComponent> filter_label_1_;
    std::unique_ptr<PlainTextComponent> filter_label_2_;

    std::unique_ptr<SynthSlider> blend_transpose_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSection)
};

