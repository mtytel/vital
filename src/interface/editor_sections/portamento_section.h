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

class SynthButton;
class SynthSlider;
class TextSlider;

class PortamentoSection : public SynthSection {
  public:
    PortamentoSection(String name);
    virtual ~PortamentoSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }
    void resized() override;
    void sliderValueChanged(Slider* changed_slider) override;
    void setAllValues(vital::control_map& controls) override;

  private:
    std::unique_ptr<SynthSlider> portamento_;
    std::unique_ptr<SynthSlider> portamento_slope_;
    std::unique_ptr<SynthButton> portamento_scale_;
    std::unique_ptr<SynthButton> portamento_force_;
    std::unique_ptr<SynthButton> legato_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PortamentoSection)
};

