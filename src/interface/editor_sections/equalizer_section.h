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
#include "equalizer_response.h"
#include "synth_section.h"

class SynthSlider;
class Spectrogram;
class TabSelector;

class EqualizerSection : public SynthSection, public EqualizerResponse::Listener {
  public:
    EqualizerSection(String name, const vital::output_map& mono_modulations);
    virtual ~EqualizerSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void setActive(bool active) override;
    
    void lowBandSelected() override;
    void midBandSelected() override;
    void highBandSelected() override;

    void sliderValueChanged(Slider* slider) override;
    void buttonClicked(Button* slider) override;
    void setScrollWheelEnabled(bool enabled) override;

    void setGainActive();
    void parentHierarchyChanged() override;

    virtual void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;

  private:
    SynthGuiInterface* parent_;
    std::unique_ptr<SynthButton> on_;

    std::unique_ptr<OpenGlShapeButton> low_mode_;
    std::unique_ptr<OpenGlShapeButton> band_mode_;
    std::unique_ptr<OpenGlShapeButton> high_mode_;

    std::unique_ptr<EqualizerResponse> eq_response_;
    std::unique_ptr<Spectrogram> spectrogram_;

    std::unique_ptr<SynthSlider> low_cutoff_;
    std::unique_ptr<SynthSlider> low_resonance_;
    std::unique_ptr<SynthSlider> low_gain_;
    std::unique_ptr<SynthSlider> band_cutoff_;
    std::unique_ptr<SynthSlider> band_resonance_;
    std::unique_ptr<SynthSlider> band_gain_;
    std::unique_ptr<SynthSlider> high_cutoff_;
    std::unique_ptr<SynthSlider> high_resonance_;
    std::unique_ptr<SynthSlider> high_gain_;
    std::unique_ptr<TabSelector> selected_band_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerSection)
};

