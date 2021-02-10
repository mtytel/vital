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
#include "modulation_tab_selector.h"
#include "synth_constants.h"
#include "synth_section.h"

class EnvelopeSection;
class RandomSection;
class LineGenerator;
class LfoSection;
struct SynthGuiData;

class ModulationInterface  : public SynthSection, public ModulationTabSelector::Listener {
  public:
    static constexpr int kMinEnvelopeModulationsToShow = 3;
    static constexpr int kMinLfoModulationsToShow = 4;
    static constexpr int kMinRandomModulationsToShow = 2;
    static constexpr int kMinTotalModulations =
        kMinEnvelopeModulationsToShow + kMinLfoModulationsToShow + kMinRandomModulationsToShow;

    ModulationInterface(SynthGuiData* synth_data);
    ~ModulationInterface();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override;
    void resized() override;
    void reset() override;

    void checkNumShown();
    void modulationSelected(ModulationTabSelector* selector, int index) override;

    void setFocus() { grabKeyboardFocus(); }

  private:
    std::unique_ptr<EnvelopeSection> envelopes_[vital::kNumEnvelopes];
    std::unique_ptr<ModulationTabSelector> envelope_tab_selector_;

    std::unique_ptr<LfoSection> lfos_[vital::kNumLfos];
    std::unique_ptr<ModulationTabSelector> lfo_tab_selector_;

    std::unique_ptr<RandomSection> random_lfos_[vital::kNumRandomLfos];
    std::unique_ptr<ModulationTabSelector> random_tab_selector_;

    std::unique_ptr<ModulationTabSelector> keyboard_modulations_top_;
    std::unique_ptr<ModulationTabSelector> keyboard_modulations_bottom_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationInterface)
};

