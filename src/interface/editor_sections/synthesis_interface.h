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

#include "filter_section.h"
#include "oscillator_section.h"
#include "sample_section.h"
#include "synth_section.h"

class SynthesisInterface : public SynthSection, public OscillatorSection::Listener,
                           public SampleSection::Listener, public FilterSection::Listener {
  public:
    SynthesisInterface(Authentication* auth,
                       const vital::output_map& mono_modulations,
                       const vital::output_map& poly_modulations);
    virtual ~SynthesisInterface();

    void paintBackground(Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    void setFocus() { grabKeyboardFocus(); }
    void distortionTypeChanged(OscillatorSection* section, int type) override;
    void oscillatorDestinationChanged(OscillatorSection* section, int destination) override;

    void sampleDestinationChanged(SampleSection* section, int destination) override;

    void filterSerialSelected(FilterSection* section) override;
    void oscInputToggled(FilterSection* section, int index, bool on) override;
    void sampleInputToggled(FilterSection* section, bool on) override;

    Slider* getWaveFrameSlider(int index) { return oscillators_[index]->getWaveFrameSlider(); }
    Rectangle<int> getOscillatorBounds(int index) { return oscillators_[index]->getBounds(); }
    const OscillatorSection* getOscillatorSection(int index) const { return oscillators_[index].get(); }

    void setWavetableName(int index, String name) { oscillators_[index]->setName(name); }

    FilterSection* getFilterSection1() { return filter_section_1_.get(); }
    FilterSection* getFilterSection2() { return filter_section_2_.get(); }
    OscillatorSection* getOscillatorSection(int index) { return oscillators_[index].get(); }

  private:
    std::unique_ptr<FilterSection> filter_section_1_;
    std::unique_ptr<FilterSection> filter_section_2_;
    std::unique_ptr<OscillatorSection> oscillators_[vital::kNumOscillators];
    std::unique_ptr<SampleSection> sample_section_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthesisInterface)
};

