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

class RandomViewer;
class SynthSlider;
class TempoSelector;
class TextSelector;

class RandomSection : public SynthSection {
  public:
    RandomSection(String name, std::string value_prepend,
                  const vital::output_map& mono_modulations,
                  const vital::output_map& poly_modulations);
    ~RandomSection();

    void paintBackground(Graphics& g) override;
    void resized() override;

    void setAllValues(vital::control_map& controls) override {
      SynthSection::setAllValues(controls);
      transpose_tune_divider_->setVisible(sync_->isKeytrack());
    }

    void sliderValueChanged(Slider* changed_slider) override {
      SynthSection::sliderValueChanged(changed_slider);
      transpose_tune_divider_->setVisible(sync_->isKeytrack());
    }

  private:
    std::unique_ptr<RandomViewer> viewer_;

    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<SynthButton> stereo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<SynthButton> sync_type_;
    std::unique_ptr<TextSelector> style_;

    std::unique_ptr<SynthSlider> keytrack_transpose_;
    std::unique_ptr<SynthSlider> keytrack_tune_;
    std::unique_ptr<OpenGlQuad> transpose_tune_divider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomSection)
};

