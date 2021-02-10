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

class SynthButton;

class ReverbSection : public SynthSection, public EqualizerResponse::Listener {
  public:
    static constexpr float kFeedbackFilterBuffer = 0.4f;

    ReverbSection(String name, const vital::output_map& mono_modulations);
    ~ReverbSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void setActive(bool active) override;
    void sliderValueChanged(Slider* slider) override;

    void lowBandSelected() override;
    void midBandSelected() override { }
    void highBandSelected() override;
  
  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<EqualizerResponse> feedback_eq_response_;
    std::unique_ptr<TabSelector> selected_eq_band_;
    std::unique_ptr<SynthSlider> decay_time_;
    std::unique_ptr<SynthSlider> low_pre_cutoff_;
    std::unique_ptr<SynthSlider> high_pre_cutoff_;
    std::unique_ptr<SynthSlider> low_cutoff_;
    std::unique_ptr<SynthSlider> low_gain_;
    std::unique_ptr<SynthSlider> high_cutoff_;
    std::unique_ptr<SynthSlider> high_gain_;
    std::unique_ptr<SynthSlider> chorus_amount_;
    std::unique_ptr<SynthSlider> chorus_frequency_;
    std::unique_ptr<SynthSlider> size_;
    std::unique_ptr<SynthSlider> delay_;
    std::unique_ptr<SynthSlider> dry_wet_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbSection)
};

