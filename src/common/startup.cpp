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

#include "startup.h"
#include "load_save.h"
#include "JuceHeader.h"
#include "synth_base.h"

void Startup::doStartupChecks(MidiManager* midi_manager, vital::StringLayout* layout) {
  if (!LoadSave::isInstalled())
    return;

  if (LoadSave::wasUpgraded())
    LoadSave::saveVersionConfig();

  LoadSave::loadConfig(midi_manager, layout);
}

bool Startup::isComputerCompatible() {
  #if defined(__ARM_NEON__)
  return true;
  #else
  return SystemStats::hasSSE2() || SystemStats::hasAVX2();
  #endif
}
