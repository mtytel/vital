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
#include "synth_slider.h"

class SynthGuiInterface;

class ControlWheel : public SynthSlider {
  public:
    static constexpr float kBufferWidthRatio = 0.05f;
    static constexpr float kWheelRoundAmountRatio = 0.25f;
    static constexpr float kContainerRoundAmountRatio = 0.15f;

    ControlWheel(String name);

    void paintLine(Graphics& g, float y_percent, Colour line_color, Colour fill_color);
    void paint(Graphics& g) override;
    void parentHierarchyChanged() override;

  protected:
    SynthGuiInterface* parent_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlWheel)
};

class ModWheel : public ControlWheel {
  public:
    ModWheel();
    virtual ~ModWheel() = default;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModWheel)
};

class PitchWheel : public ControlWheel {
  public:
    PitchWheel();
    virtual ~PitchWheel() = default;

    void mouseUp(const MouseEvent& e) override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchWheel)
};

class BendSection : public SynthSection {
  public:
    BendSection(const String& name);
    ~BendSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }
    void resized() override;
    void sliderValueChanged(Slider* changed_slider) override;

  private:
    std::unique_ptr<PitchWheel> pitch_wheel_;
    std::unique_ptr<ModWheel> mod_wheel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BendSection)
};

