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

#include "tempo_selector.h"

#include "paths.h"
#include "skin.h"
#include "default_look_and_feel.h"

TempoSelector::TempoSelector(String name) : SynthSlider(name), free_slider_(nullptr),
                                            tempo_slider_(nullptr), keytrack_transpose_slider_(nullptr),
                                            keytrack_tune_slider_(nullptr) {
  paintToImage(true);
}

void TempoSelector::mouseDown(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    SynthSlider::mouseDown(e);
    return;
  }

  PopupItems options;
  options.addItem(kSeconds, "Seconds");
  options.addItem(kTempo, "Tempo");
  options.addItem(kTempoDotted, "Tempo Dotted");
  options.addItem(kTempoTriplet, "Tempo Triplets");
  if (getMaximum() >= kKeytrack)
    options.addItem(kKeytrack, "Keytrack");

  parent_->showPopupSelector(this, Point<int>(0, getHeight()), options, [=](int value) { setValue(value); });
}

void TempoSelector::mouseUp(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    SynthSlider::mouseDown(e);
    return;
  }
}

void TempoSelector::valueChanged() {
  int menu_value = getValue();

  free_slider_->setVisible(menu_value == kSeconds);
  tempo_slider_->setVisible(menu_value != kSeconds && menu_value != kKeytrack);

  if (keytrack_transpose_slider_)
    keytrack_transpose_slider_->setVisible(menu_value == kKeytrack);
  if (keytrack_tune_slider_)
    keytrack_tune_slider_->setVisible(menu_value == kKeytrack);

  SynthSlider::valueChanged();
}

void TempoSelector::paint(Graphics& g) {
  g.setColour(findColour(Skin::kIconSelectorIcon, true));

  Path path;
  int value = getValue();
  if (value == kSeconds)
    path = Paths::clock();
  else if (value == kTempo || value == kTempoDotted)
    path = Paths::note();
  else if (value == kTempoTriplet)
    path = Paths::tripletNotes();
  else if (value == kKeytrack)
    path = Paths::keyboardBordered();

  g.fillPath(path, path.getTransformToScaleToFit(getLocalBounds().toFloat(), true));
  if (value == kTempoDotted) {
    float dot_width = getWidth() / 8.0f;
    g.fillEllipse(3.0f * getWidth() / 4.0f - dot_width / 2.0f, getHeight() / 2.0f, dot_width, dot_width);
  }
}

void TempoSelector::setFreeSlider(Slider* slider) {
  bool free_slider = getValue() == kSeconds;

  free_slider_ = slider;
  free_slider_->setVisible(free_slider);
}

void TempoSelector::setTempoSlider(Slider* slider) {
  bool visible = (getValue() != kSeconds) && (getValue() != kKeytrack);

  tempo_slider_ = slider;
  tempo_slider_->setVisible(visible);
}

void TempoSelector::setKeytrackTransposeSlider(Slider* slider) {
  bool visible = getValue() == kKeytrack;

  keytrack_transpose_slider_ = slider;
  keytrack_transpose_slider_->setVisible(visible);
}

void TempoSelector::setKeytrackTuneSlider(Slider* slider) {
  bool visible = getValue() == kKeytrack;

  keytrack_tune_slider_ = slider;
  keytrack_tune_slider_->setVisible(visible);
}
