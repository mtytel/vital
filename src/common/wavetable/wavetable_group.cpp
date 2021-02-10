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

#include "wavetable_group.h"
#include "synth_constants.h"
#include "wave_frame.h"
#include "wave_source.h"
#include "wavetable.h"

int WavetableGroup::getComponentIndex(WavetableComponent* component) {
  for (int i = 0; i < components_.size(); ++i) {
    if (components_[i].get() == component)
      return i;
  }
  return -1;
}

void WavetableGroup::moveUp(int index) {
  if (index <= 0)
    return;

  components_[index].swap(components_[index - 1]);
}

void WavetableGroup::moveDown(int index) {
  if (index < 0 || index >= components_.size() - 1)
    return;

  components_[index].swap(components_[index + 1]);
}

void WavetableGroup::removeComponent(int index) {
  if (index < 0 || index >= components_.size())
    return;

  std::unique_ptr<WavetableComponent> component = std::move(components_[index]);
  components_.erase(components_.begin() + index);
}

void WavetableGroup::reset() {
  components_.clear();
  loadDefaultGroup();
}

void WavetableGroup::prerender() {
  for (auto& component : components_)
    component->prerender();
}

bool WavetableGroup::isShepardTone() {
  for (auto& component : components_) {
    if (component->getType() != WavetableComponentFactory::kShepardToneSource)
      return false;
  }

  return true;
}

void WavetableGroup::render(vital::WaveFrame* wave_frame, float position) const {
  wave_frame->index = position;

  for (auto& component : components_)
    component->render(wave_frame, position);
}

void WavetableGroup::renderTo(vital::Wavetable* wavetable) {
  for (int i = 0; i < vital::kNumOscillatorWaveFrames; ++i) {
    compute_frame_.index = i;

    for (auto& component : components_)
      component->render(&compute_frame_, i);

    wavetable->loadWaveFrame(&compute_frame_);
  }
}

void WavetableGroup::loadDefaultGroup() {
  WaveSource* wave_source = new WaveSource();
  wave_source->insertNewKeyframe(0);
  vital::WaveFrame* wave_frame = wave_source->getWaveFrame(0);
  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    float t = i / (vital::WaveFrame::kWaveformSize - 1.0f);
    int half_shift = (i + vital::WaveFrame::kWaveformSize / 2) % vital::WaveFrame::kWaveformSize;
    wave_frame->time_domain[half_shift] = 1.0f - 2.0f * t;
  }
  wave_frame->toFrequencyDomain();

  addComponent(wave_source);
}

int WavetableGroup::getLastKeyframePosition() {
  int last_position = 0;
  for (auto& component : components_)
    last_position = std::max(last_position, component->getLastKeyframePosition());

  return last_position;
}

json WavetableGroup::stateToJson() {
  json json_components;
  for (auto& component : components_) {
    json json_component = component->stateToJson();
    json_components.push_back(json_component);
  }

  return { { "components", json_components } };
}

void WavetableGroup::jsonToState(json data) {
  components_.clear();

  json json_components = data["components"];
  for (json json_component : json_components) {
    std::string type = json_component["type"];
    WavetableComponent* component = WavetableComponentFactory::createComponent(type);
    component->jsonToState(json_component);
    addComponent(component);
  }
}
