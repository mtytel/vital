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

#include "modulation_button.h"
#include "modulation_button.h"

class SynthSection;

class ModulationTabSelector : public SynthSection, public ModulationButton::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void modulationSelected(ModulationTabSelector* selector, int index) = 0;
    };

    ModulationTabSelector(std::string prefix, int number);
    ModulationTabSelector(String name, int number, const char** names);
    virtual ~ModulationTabSelector();

    void paintBackground(Graphics& g) override;
    void paintTabShadow(Graphics& g) override;
    void resized() override;
    void checkNumShown(bool should_repaint);
    void reset() override;

    void modulationClicked(ModulationButton* source) override;
    void modulationConnectionChanged() override;
    void endModulationMap() override;
    void modulationCleared() override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void registerModulationButtons(SynthSection* synth_section);
    void setFontSize(float font_size);

    void setVertical(bool vertical) { vertical_ = vertical; }
    void enableSelections() { selections_enabled_ = true; modulation_buttons_[0]->select(true); }
    void setMinModulationsShown(int num) { min_modulations_shown_ = num; }

    void connectRight(bool connect) {
      for (auto& modulation_button : modulation_buttons_)
        modulation_button->setConnectRight(connect);
    }

    ModulationButton* getButton(int index) { return modulation_buttons_[index].get(); }

    void drawBorders(bool draw) {
      for (auto& button : modulation_buttons_)
        button->setDrawBorder(draw);
    }

  private:
    int getModulationIndex(String name);
    int getNumModulationsToShow();

    std::vector<std::unique_ptr<ModulationButton>> modulation_buttons_;
    std::vector<Listener*> listeners_;
    bool vertical_;
    bool selections_enabled_;
    int min_modulations_shown_;
    int num_shown_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationTabSelector)
};

