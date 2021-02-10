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

#include "modulation_tab_selector.h"
#include "modulation_button.h"
#include "synth_section.h"
#include "skin.h"

ModulationTabSelector::ModulationTabSelector(std::string prefix, int number) :
    SynthSection(prefix), vertical_(true), selections_enabled_(false), min_modulations_shown_(0), num_shown_(0) {
  for (int i = 0; i < number; ++i) {
    std::string name = prefix + "_" + std::to_string(i + 1);
    modulation_buttons_.push_back(std::make_unique<ModulationButton>(name));
    addOpenGlComponent(modulation_buttons_.back().get());
    modulation_buttons_.back()->addListener(this);
  }
}

ModulationTabSelector::ModulationTabSelector(String name, int number, const char** names) :
  SynthSection(name), vertical_(true), selections_enabled_(false), min_modulations_shown_(0), num_shown_(0) {
  for (int i = 0; i < number; ++i) {
    modulation_buttons_.push_back(std::make_unique<ModulationButton>(names[i]));
    addOpenGlComponent(modulation_buttons_.back().get());
    modulation_buttons_.back()->addListener(this);
  }
}

ModulationTabSelector::~ModulationTabSelector() = default;

void ModulationTabSelector::paintBackground(Graphics& g) {
  int num_to_show = getNumModulationsToShow();
  if (num_shown_ != num_to_show) {
    checkNumShown(false);
    num_shown_ = num_to_show;
  }

  g.fillAll(findColour(Skin::kBackground, true));
  paintTabShadow(g);

  for (auto& button : modulation_buttons_) {
    if (button->isVisible()) {
      g.saveState();
      Rectangle<int> bounds = getLocalArea(button.get(), button->getLocalBounds());
      g.reduceClipRegion(bounds);
      g.setOrigin(bounds.getTopLeft());
      button->paintBackground(g);
      g.restoreState();
    }
  }
}

void ModulationTabSelector::paintTabShadow(Graphics& g) {
  SynthSection* parent = findParentComponentOfClass<SynthSection>();
  if (parent == nullptr)
    return;

  int rounding_amount = parent->findValue(Skin::kBodyRounding);
  g.setColour(findColour(Skin::kShadow, true));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), rounding_amount);
}

void ModulationTabSelector::resized() {
  checkNumShown(false);
}

void ModulationTabSelector::checkNumShown(bool should_repaint) {
  int num_to_show = getNumModulationsToShow();

  if (vertical_) {
    float cell_height = float(getHeight() + 1) / num_to_show;
    int y = 0;

    for (int i = 0; i < num_to_show; ++i) {
      int last_y = y;
      y = std::round((i + 1) * cell_height);
      modulation_buttons_[i]->setBounds(0.0f, last_y, getWidth(), y - last_y - 1);
      modulation_buttons_[i]->setVisible(true);
    }
  }
  else {
    float cell_width = float(getWidth() + 1) / num_to_show;
    int x = 0;

    for (int i = 0; i < num_to_show; ++i) {
      int last_x = x;
      x = std::round((i + 1) * cell_width);
      modulation_buttons_[i]->setBounds(last_x, 0, x - last_x - 1, getHeight());
      modulation_buttons_[i]->setVisible(true);
    }
  }

  for (int i = num_to_show; i < modulation_buttons_.size(); ++i)
    modulation_buttons_[i]->setVisible(false);

  if (num_to_show != num_shown_ && should_repaint)
    repaintBackground();
}

void ModulationTabSelector::reset() {
  for (auto& button : modulation_buttons_) {
    button->select(false);
    button->setActiveModulation(false);
  }

  modulation_buttons_[0]->select(selections_enabled_);

  if (getNumModulationsToShow() != num_shown_)
    checkNumShown(true);

  modulation_buttons_[0]->select(selections_enabled_);
  for (Listener *listener : listeners_)
    listener->modulationSelected(this, 0);
}

void ModulationTabSelector::modulationClicked(ModulationButton* source) {
  int index = getModulationIndex(source->getName());

  if (selections_enabled_) {
    for (int i = 0; i < modulation_buttons_.size(); ++i)
      modulation_buttons_[i]->select(index == i);
  }

  repaintBackground();

  for (Listener *listener : listeners_)
    listener->modulationSelected(this, index);
}

void ModulationTabSelector::endModulationMap() {
  if (getNumModulationsToShow() != num_shown_)
    checkNumShown(true);
}

void ModulationTabSelector::modulationConnectionChanged() {
  if (getNumModulationsToShow() != num_shown_)
    checkNumShown(true);
}

void ModulationTabSelector::modulationCleared() {
  if (getNumModulationsToShow() != num_shown_)
    checkNumShown(true);
}

void ModulationTabSelector::registerModulationButtons(SynthSection* synth_section) {
  for (auto& button : modulation_buttons_)
    synth_section->addModulationButton(button.get(), false);
}

void ModulationTabSelector::setFontSize(float font_size) {
  for (auto& button : modulation_buttons_)
    button->setFontSize(font_size);
}

int ModulationTabSelector::getNumModulationsToShow() {
  int num_to_show = static_cast<int>(modulation_buttons_.size());
  if (min_modulations_shown_ > 0) {
    num_to_show = min_modulations_shown_;
    for (int i = min_modulations_shown_ - 1; i < modulation_buttons_.size(); ++i) {
      if (modulation_buttons_[i]->hasAnyModulation())
        num_to_show = i + 2;
    }
  }
  return std::min(num_to_show, static_cast<int>(modulation_buttons_.size()));
}

int ModulationTabSelector::getModulationIndex(String name) {
  for (int i = 0; i < modulation_buttons_.size(); ++i) {
    if (name == modulation_buttons_[i]->getName())
      return i;
  }
  return 0;
}
