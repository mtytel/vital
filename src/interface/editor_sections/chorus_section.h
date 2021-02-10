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
#include "delay_section.h"
#include "synth_section.h"

class ChorusViewer;
class SynthButton;
class TempoSelector;

class ChorusSection : public SynthSection, DelayFilterViewer::Listener {
  public:
    ChorusSection(const String& name, const vital::output_map& mono_modulations);
    ~ChorusSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void setActive(bool active) override;
    void deltaMovement(float x, float y) override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<SynthSlider> voices_;
    std::unique_ptr<ChorusViewer> chorus_viewer_;
    std::unique_ptr<DelayFilterViewer> filter_viewer_;

    std::unique_ptr<SynthSlider> feedback_;
    std::unique_ptr<SynthSlider> mod_depth_;
    std::unique_ptr<SynthSlider> delay_1_;
    std::unique_ptr<SynthSlider> delay_2_;
    std::unique_ptr<SynthSlider> dry_wet_;
    std::unique_ptr<SynthSlider> filter_cutoff_;
    std::unique_ptr<SynthSlider> filter_spread_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusSection)
};

