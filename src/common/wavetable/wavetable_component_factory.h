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

class WavetableComponent;

class WavetableComponentFactory {
  public:
    enum ComponentType {
      kWaveSource,
      kLineSource,
      kFileSource,
      kNumSourceTypes,
      kShepardToneSource = kNumSourceTypes, // Deprecated

      kBeginModifierTypes = kNumSourceTypes + 1,
      kPhaseModifier = kBeginModifierTypes,
      kWaveWindow,
      kFrequencyFilter,
      kSlewLimiter,
      kWaveFolder,
      kWaveWarp,
      kNumComponentTypes
    };

    static int numComponentTypes() { return kNumComponentTypes; }
    static int numSourceTypes() { return kNumSourceTypes; }
    static int numModifierTypes() { return kNumComponentTypes - kBeginModifierTypes; }

    static WavetableComponent* createComponent(ComponentType type);
    static WavetableComponent* createComponent(const std::string& type);
    static std::string getComponentName(ComponentType type);
    static ComponentType getSourceType(int type) { return static_cast<ComponentType>(type); }
    static ComponentType getModifierType(int type) {
      return (ComponentType)(type + kBeginModifierTypes);
    }

  private:
    WavetableComponentFactory() { }
};

