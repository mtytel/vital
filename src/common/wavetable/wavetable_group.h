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
#include "wave_frame.h"
#include "wavetable_component.h"

namespace vital {
  class Wavetable;
} // namespace vital

class WavetableGroup {
  public:
    WavetableGroup() { }

    int getComponentIndex(WavetableComponent* component);
    void addComponent(WavetableComponent* component) {
      components_.push_back(std::unique_ptr< WavetableComponent>(component));
    }
    void removeComponent(int index);
    void moveUp(int index);
    void moveDown(int index);
    void reset();
    void prerender();

    int numComponents() const { return static_cast<int>(components_.size()); }
    WavetableComponent* getComponent(int index) const { return components_[index].get(); }
    bool isShepardTone();
    void render(vital::WaveFrame* wave_frame, float position) const;
    void renderTo(vital::Wavetable* wavetable);
    void loadDefaultGroup();
    int getLastKeyframePosition();

    json stateToJson();
    void jsonToState(json data);

  protected:
    vital::WaveFrame compute_frame_;
    std::vector<std::unique_ptr<WavetableComponent>> components_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableGroup)
};

