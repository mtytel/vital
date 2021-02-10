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

class SynthSlider;
class TextSlider;

class VoiceSection : public SynthSection {
  public:
    VoiceSection(String name);
    virtual ~VoiceSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }
    void resized() override;
    void buttonClicked(Button* clicked_button) override;
    void setAllValues(vital::control_map& controls) override;
    void setStereoModeSelected(int selection);

  private:
    std::unique_ptr<SynthSlider> polyphony_;
    std::unique_ptr<SynthSlider> velocity_track_;
    std::unique_ptr<SynthSlider> pitch_bend_range_;
    std::unique_ptr<SynthSlider> stereo_routing_;

    std::unique_ptr<PlainTextComponent> stereo_mode_text_;
    std::unique_ptr<ShapeButton> stereo_mode_type_selector_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceSection)
};

