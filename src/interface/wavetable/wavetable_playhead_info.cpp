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

#include "wavetable_playhead_info.h"

#include "skin.h"

void WavetablePlayheadInfo::paint(Graphics& g) {
  g.fillAll(findColour(Skin::kBody, true));
  String position_text(playhead_position_);
  g.setColour(findColour(Skin::kBodyText, true));
  Rectangle<int> bounds = getLocalBounds();
  bounds.setWidth(bounds.getWidth() - bounds.getHeight() * 0.5f);
  g.drawText(position_text, bounds, Justification::centredRight );
}

void WavetablePlayheadInfo::resized() {
  repaint();
}

void WavetablePlayheadInfo::playheadMoved(int position) {
  playhead_position_ = position;
  repaint();
}
