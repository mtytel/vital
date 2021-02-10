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

#include "line_map_editor.h"

#include "skin.h"
#include "shaders.h"
#include "synth_gui_interface.h"
#include "utils.h"

LineMapEditor::LineMapEditor(LineGenerator* line_source, String name) : LineEditor(line_source) {
  animate_ = true;
  raw_input_ = nullptr;
  last_phase_ = 0.0f;

  setFill(true);
  setFillCenter(-1.0f);
  setLoop(false);
  setName(name);
  setBoostAmount(0.0f);
  setFillBoostAmount(0.0f);
}

LineMapEditor::~LineMapEditor() { }

void LineMapEditor::parentHierarchyChanged() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();

  if (parent && raw_input_ == nullptr)
    raw_input_ = parent->getSynth()->getStatusOutput(getName().toStdString());

  LineEditor::parentHierarchyChanged();
}

void LineMapEditor::render(OpenGlWrapper& open_gl, bool animate) {
  setGlPositions();
  renderGrid(open_gl, animate);

  setLineWidth(findValue(Skin::kWidgetLineWidth));

  Colour envelope_graph_fill = findColour(Skin::kWidgetSecondary1, true);
  Colour envelope_graph_fill_stereo = findColour(Skin::kWidgetSecondary2, true);

  float fill_fade = findValue(Skin::kWidgetFillFade);
  if (!active_) {
    envelope_graph_fill = findColour(Skin::kWidgetSecondaryDisabled, true);
    envelope_graph_fill_stereo = envelope_graph_fill;
  }
  Colour envelope_graph_fill_fade = envelope_graph_fill.withMultipliedAlpha(1.0f - fill_fade);
  Colour envelope_graph_fill_stereo_fade = envelope_graph_fill_stereo.withMultipliedAlpha(1.0f - fill_fade);

  Colour position_color = findColour(Skin::kWidgetPrimary1, true);
  Colour position_color_stereo = findColour(Skin::kWidgetPrimary2, true);
  Colour center = findColour(Skin::kWidgetCenterLine, true);
  if (!active_) {
    position_color = findColour(Skin::kWidgetPrimaryDisabled, true);
    center = position_color;
    position_color_stereo = position_color;
  }

  if (animate && animate_) {
    decayBoosts(kTailDecay);

    vital::poly_float phase = raw_input_->value();
    if (!raw_input_->isClearValue(phase)) {
      vital::poly_float adjusted_phase = adjustBoostPhase(phase);
      boostRange(last_phase_, adjusted_phase, kNumWrapPoints, kTailDecay);
      last_phase_ = adjusted_phase;
    }

    setFill(true);
    setBoostAmount(findValue(Skin::kWidgetLineBoost));
    setFillBoostAmount(findValue(Skin::kWidgetFillBoost));

    setIndex(1);
    setColor(position_color_stereo);
    setFillColors(envelope_graph_fill_stereo_fade, envelope_graph_fill_stereo);
    drawLines(open_gl, false);

    setIndex(0);
    setColor(position_color);
    setFillColors(envelope_graph_fill_fade, envelope_graph_fill);
    drawLines(open_gl, true);

    setFill(false);
    setBoostAmount(0.0f);
    setFillBoostAmount(0.0f);
    setColor(center);
    LineEditor::render(open_gl, false);

    setViewPort(open_gl);
    drawPosition(open_gl, position_color_stereo, phase[1]);
    drawPosition(open_gl, position_color, phase[0]);
  }
  else {
    setBoostAmount(0.0f);
    setFillBoostAmount(0.0f);
    decayBoosts(0.0f);

    setFill(true);
    setColor(position_color_stereo);
    setFillColors(envelope_graph_fill_stereo_fade, envelope_graph_fill_stereo);
    drawLines(open_gl, false);

    setColor(position_color);
    setFillColors(envelope_graph_fill_fade, envelope_graph_fill);
    drawLines(open_gl, true);

    setFill(false);
    setColor(center);
    drawLines(open_gl, true);
  }

  renderPoints(open_gl, animate);
  renderCorners(open_gl, animate);
}
