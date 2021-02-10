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

#include "extra_mod_section.h"

#include "line_map_editor.h"
#include "macro_knob_section.h"
#include "skin.h"
#include "fonts.h"
#include "modulation_button.h"
#include "synth_gui_interface.h"

namespace {
  const char* kModulationStrings[] = {
     "pitch_wheel",
     "mod_wheel"
  };
}

ExtraModSection::ExtraModSection(String name, SynthGuiData* synth_data) : SynthSection(name) {
  other_modulations_ = std::make_unique<ModulationTabSelector>("OTHER", 2, kModulationStrings);
  other_modulations_->getButton(0)->overrideText("PITCH WHL");
  other_modulations_->getButton(1)->overrideText("MOD WHL");
  other_modulations_->drawBorders(true);
  addSubSection(other_modulations_.get());
  other_modulations_->registerModulationButtons(this);
  other_modulations_->setVertical(true);

  macro_knobs_ = std::make_unique<MacroKnobSection>("MACRO");
  addSubSection(macro_knobs_.get());
}

ExtraModSection::~ExtraModSection() { }

void ExtraModSection::paintBackground(Graphics& g) {
  g.saveState();
  Rectangle<int> bounds = getLocalArea(other_modulations_.get(), other_modulations_->getLocalBounds());
  g.reduceClipRegion(bounds);
  g.setOrigin(bounds.getTopLeft());
  other_modulations_->paintBackground(g);
  g.restoreState();

  paintChildrenBackgrounds(g);
}

void ExtraModSection::paintBackgroundShadow(Graphics& g) {
  paintTabShadow(g, other_modulations_->getBounds());
  SynthSection::paintBackgroundShadow(g);
}

void ExtraModSection::resized() {
  int padding = getPadding();
  int knob_section_height = getKnobSectionHeight();
  int widget_margin = getWidgetMargin();
  int macro_height = 4 * (2 * knob_section_height - widget_margin + padding) - padding;
  int mod_height = getHeight() - macro_height - padding;

  macro_knobs_->setBounds(0, 0, getWidth(), macro_height);
  other_modulations_->setBounds(0, macro_height + padding, getWidth(), mod_height);

  SynthSection::resized();
  other_modulations_->setFontSize(getModFontSize());
}
