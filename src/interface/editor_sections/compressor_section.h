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
#include "text_selector.h"

class CompressorEditor;
class SynthButton;
class SynthSlider;

class CompressorSection : public SynthSection {
  public:
    CompressorSection(const String& name);
    ~CompressorSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void setAllValues(vital::control_map& controls) override;
    void setActive(bool active) override;
    void sliderValueChanged(Slider* changed_slider) override;

  private:
    void setCompressorActiveBands();

    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<SynthSlider> mix_;
    std::unique_ptr<SynthSlider> attack_;
    std::unique_ptr<SynthSlider> release_;
    std::unique_ptr<SynthSlider> low_gain_;
    std::unique_ptr<SynthSlider> band_gain_;
    std::unique_ptr<SynthSlider> high_gain_;
    std::unique_ptr<TextSelector> enabled_bands_;
    std::unique_ptr<CompressorEditor> compressor_editor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorSection)
};

