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

#include "modulation_interface.h"

#include "envelope_section.h"
#include "extra_mod_section.h"
#include "lfo_section.h"
#include "random_section.h"
#include "synth_gui_interface.h"

namespace {
  const char* kKeyboardTopModulationStrings[] = {
    "note",
    "velocity",
    "lift",
    "note_in_octave"
  };

  const char* kKeyboardBottomModulationStrings[] = {
   "aftertouch",
   "slide",
   "stereo",
   "random",
  };
}

ModulationInterface::ModulationInterface(SynthGuiData* synth_data) : SynthSection("modulation") {
  for (int i = 0; i < vital::kNumEnvelopes; ++i) {
    std::string string_num = std::to_string(i + 1);
    std::string prefix = "env_" + string_num;
    envelopes_[i] = std::make_unique<EnvelopeSection>("ENVELOPE " + string_num, prefix,
                                                      synth_data->mono_modulations, synth_data->poly_modulations);
    addSubSection(envelopes_[i].get());
    envelopes_[i]->setVisible(i == 0);
  }

  envelope_tab_selector_ = std::make_unique<ModulationTabSelector>("env", vital::kNumEnvelopes);
  addSubSection(envelope_tab_selector_.get());
  envelope_tab_selector_->addListener(this);
  envelope_tab_selector_->registerModulationButtons(this);
  envelope_tab_selector_->enableSelections();
  envelope_tab_selector_->setMinModulationsShown(kMinEnvelopeModulationsToShow);
  envelope_tab_selector_->connectRight(true);
  envelope_tab_selector_->drawBorders(true);

  for (int i = 0; i < vital::kNumLfos; ++i) {
    std::string string_num = std::to_string(i + 1);
    std::string prefix = "lfo_" + string_num;
    lfos_[i] = std::make_unique<LfoSection>("LFO " + string_num, prefix, synth_data->synth->getLfoSource(i),
                                            synth_data->mono_modulations, synth_data->poly_modulations);
    addSubSection(lfos_[i].get());
    lfos_[i]->setVisible(i == 0);
  }

  lfo_tab_selector_ = std::make_unique<ModulationTabSelector>("lfo", vital::kNumLfos);
  addSubSection(lfo_tab_selector_.get());
  lfo_tab_selector_->addListener(this);
  lfo_tab_selector_->registerModulationButtons(this);
  lfo_tab_selector_->enableSelections();
  lfo_tab_selector_->setMinModulationsShown(kMinLfoModulationsToShow);
  lfo_tab_selector_->connectRight(true);
  lfo_tab_selector_->drawBorders(true);

  for (int i = 0; i < vital::kNumRandomLfos; ++i) {
    std::string string_num = std::to_string(i + 1);
    std::string prefix = "random_" + string_num;
    random_lfos_[i] = std::make_unique<RandomSection>("RANDOM " + string_num, prefix,
                                                      synth_data->mono_modulations, synth_data->poly_modulations);
    addSubSection(random_lfos_[i].get());
    random_lfos_[i]->setVisible(i == 0);
  }

  random_tab_selector_ = std::make_unique<ModulationTabSelector>("random", vital::kNumRandomLfos);
  addSubSection(random_tab_selector_.get());
  random_tab_selector_->addListener(this);
  random_tab_selector_->registerModulationButtons(this);
  random_tab_selector_->enableSelections();
  random_tab_selector_->setMinModulationsShown(kMinRandomModulationsToShow);
  random_tab_selector_->connectRight(true);
  random_tab_selector_->drawBorders(true);

  keyboard_modulations_top_ = std::make_unique<ModulationTabSelector>("top", 4, kKeyboardTopModulationStrings);
  addSubSection(keyboard_modulations_top_.get());
  keyboard_modulations_top_->registerModulationButtons(this);
  keyboard_modulations_top_->setVertical(false);
  keyboard_modulations_top_->getButton(3)->overrideText("OCT NOTE");
  keyboard_modulations_top_->drawBorders(true);

  keyboard_modulations_bottom_ = std::make_unique<ModulationTabSelector>("bottom", 4, kKeyboardBottomModulationStrings);
  addSubSection(keyboard_modulations_bottom_.get());
  keyboard_modulations_bottom_->registerModulationButtons(this);
  keyboard_modulations_bottom_->setVertical(false);
  keyboard_modulations_bottom_->getButton(0)->overrideText("PRESSURE");
  keyboard_modulations_bottom_->drawBorders(true);

  setOpaque(false);
}

ModulationInterface::~ModulationInterface() { }

void ModulationInterface::paintBackground(Graphics& g) {
  g.fillAll(findColour(Skin::kBackground, true));
  paintBackgroundShadow(g);

  int mod_x = lfo_tab_selector_->getX();
  int lfo_env_width = lfos_[0]->getRight() - mod_x;
  Rectangle<int> lfo_bounds(mod_x, lfo_tab_selector_->getY(), lfo_env_width, lfo_tab_selector_->getHeight());
  paintBody(g, lfo_bounds);
  Rectangle<int> env_bounds(mod_x, envelope_tab_selector_->getY(), lfo_env_width, envelope_tab_selector_->getHeight());
  paintBody(g, env_bounds);

  int random_width = random_lfos_[0]->getRight() - mod_x;
  Rectangle<int> random_bounds(mod_x, random_tab_selector_->getY(), random_width, random_tab_selector_->getHeight());
  paintBody(g, random_bounds);

  paintChildrenBackgrounds(g);
  g.saveState();
  int tabs_right = lfo_tab_selector_->getRight();
  g.reduceClipRegion(tabs_right, 0, getWidth() - tabs_right, getHeight());
  paintBorder(g, lfo_bounds);
  paintBorder(g, env_bounds);
  paintBorder(g, random_bounds);
  g.restoreState();
}

void ModulationInterface::paintBackgroundShadow(Graphics& g) {
  paintTabShadow(g, envelope_tab_selector_->getBounds());
  paintTabShadow(g, envelopes_[0]->getBounds());
  paintTabShadow(g, lfo_tab_selector_->getBounds());
  paintTabShadow(g, lfos_[0]->getBounds());
  paintTabShadow(g, random_tab_selector_->getBounds());
  paintTabShadow(g, random_lfos_[0]->getBounds());
  paintTabShadow(g, keyboard_modulations_top_->getBounds());
  paintTabShadow(g, keyboard_modulations_bottom_->getBounds());
}

void ModulationInterface::resized() {
  int padding = getPadding();
  int active_width = getWidth();
  int active_height = getHeight() - 2 * padding;
  int envelope_height = (active_height * kMinEnvelopeModulationsToShow * 1.0f) / kMinTotalModulations;
  int lfo_height = (active_height * kMinLfoModulationsToShow * 1.0f) / kMinTotalModulations;
  int mod_width = findValue(Skin::kModulationButtonWidth);

  envelope_tab_selector_->setBounds(0, 0, mod_width, envelope_height);
  Rectangle<int> envelope_bounds(mod_width, 0, active_width - mod_width, envelope_height);
  for (int i = 0; i < vital::kNumEnvelopes; ++i)
    envelopes_[i]->setBounds(envelope_bounds);

  int lfo_y = envelope_bounds.getBottom() + padding;
  lfo_tab_selector_->setBounds(0, lfo_y, mod_width, lfo_height);
  Rectangle<int> lfo_bounds(mod_width, lfo_y, active_width - mod_width, lfo_height);
  for (int i = 0; i < vital::kNumLfos; ++i)
    lfos_[i]->setBounds(lfo_bounds);

  int keyboard_width = mod_width * 4;
  int keyboard_x = getWidth() - keyboard_width;

  int random_y = lfo_bounds.getBottom() + padding;
  int random_height = getHeight() - random_y;
  random_tab_selector_->setBounds(0, random_y, mod_width, random_height);
  Rectangle<int> random_bounds(mod_width, random_y, keyboard_x - padding - mod_width, random_height);
  for (int i = 0; i < vital::kNumRandomLfos; ++i)
    random_lfos_[i]->setBounds(random_bounds);

  int keyboard_top_height = random_height / 2;
  keyboard_modulations_top_->setBounds(keyboard_x, random_y, keyboard_width, keyboard_top_height);

  int keyboard_bottom_y = random_y + keyboard_top_height + 1;
  int keyboard_bottom_height = getHeight() - keyboard_bottom_y;
  keyboard_modulations_bottom_->setBounds(keyboard_x, keyboard_bottom_y, keyboard_width, keyboard_bottom_height);

  envelope_tab_selector_->setFontSize(getModFontSize());
  lfo_tab_selector_->setFontSize(getModFontSize());
  random_tab_selector_->setFontSize(getModFontSize());
  keyboard_modulations_top_->setFontSize(getModFontSize());
  keyboard_modulations_bottom_->setFontSize(getModFontSize());

  SynthSection::resized();
}

void ModulationInterface::reset() {
  lfo_tab_selector_->reset();
  envelope_tab_selector_->reset();
  random_tab_selector_->reset();
  keyboard_modulations_top_->reset();
  keyboard_modulations_bottom_->reset();

  for (int i = 0; i < vital::kNumEnvelopes; ++i) {
    if (envelopes_[i]->isVisible())
      envelopes_[i]->reset();
  }
  for (int i = 0; i < vital::kNumLfos; ++i) {
    if (lfos_[i]->isVisible())
      lfos_[i]->reset();
  }
  for (int i = 0; i < vital::kNumRandomLfos; ++i) {
    if (random_lfos_[i]->isVisible())
      random_lfos_[i]->reset();
  }
}

void ModulationInterface::checkNumShown() {
  lfo_tab_selector_->checkNumShown(true);
  envelope_tab_selector_->checkNumShown(true);
  random_tab_selector_->checkNumShown(true);
  keyboard_modulations_top_->checkNumShown(true);
  keyboard_modulations_bottom_->checkNumShown(true);
}

void ModulationInterface::modulationSelected(ModulationTabSelector* selector, int index) {
  Image image(Image::ARGB, 1, 1, false);
  Graphics g(image);

  if (selector == envelope_tab_selector_.get()) {
    for (int i = 0; i < vital::kNumEnvelopes; ++i)
      envelopes_[i]->setVisible(i == index);
    envelopes_[index]->paintOpenGlChildrenBackgrounds(g);
    envelopes_[index]->reset();
  }
  else if (selector == lfo_tab_selector_.get()) {
    for (int i = 0; i < vital::kNumLfos; ++i)
      lfos_[i]->setVisible(i == index);
    lfos_[index]->paintOpenGlChildrenBackgrounds(g);
    lfos_[index]->reset();
  }
  else if (selector == random_tab_selector_.get()) {
    for (int i = 0; i < vital::kNumRandomLfos; ++i)
      random_lfos_[i]->setVisible(i == index);
    random_lfos_[index]->paintOpenGlChildrenBackgrounds(g);
    random_lfos_[index]->reset();
  }
}
