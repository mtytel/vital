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

#include "lfo_editor.h"

#include "default_look_and_feel.h"
#include "skin.h"
#include "shaders.h"
#include "synth_gui_interface.h"
#include "synth_section.h"
#include "utils.h"

LfoEditor::LfoEditor(LineGenerator* lfo_source, String prefix,
                     const vital::output_map& mono_modulations,
                     const vital::output_map& poly_modulations) : LineEditor(lfo_source) {
  parent_ = nullptr;
  wave_phase_ = nullptr;
  frequency_ = nullptr;
  last_phase_ = 0.0f;

  setFill(true);
  setFillCenter(-1.0f);
  setName(prefix);

  last_voice_ = -1.0f;
}

LfoEditor::~LfoEditor() { }

void LfoEditor::parentHierarchyChanged() {
  parent_ = findParentComponentOfClass<SynthGuiInterface>();

  if (wave_phase_ == nullptr && parent_)
    wave_phase_ = parent_->getSynth()->getStatusOutput(getName().toStdString() + "_phase");

  if (frequency_ == nullptr && parent_)
    frequency_ = parent_->getSynth()->getStatusOutput(getName().toStdString() + "_frequency");

  LineEditor::parentHierarchyChanged();
}

void LfoEditor::mouseDown(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    PopupItems options;

    int active_point = getActivePoint();
    if (active_point >= 0) {
      options.addItem(kSetPhaseToPoint, "Set Start Point");
      if (active_point >= 1 && active_point < getModel()->getNumPoints() - 1) {
        options.addItem(kRemovePoint, "Remove Point");
        options.addItem(kEnterPhase, "Enter Point Phase");
      }

      options.addItem(kEnterValue, "Enter Point Value");
      options.addItem(-1, "");
    }
    else if (getActivePower() >= 0) {
      options.addItem(kSetPhaseToPower, "Set Start Point");
      options.addItem(kResetPower, "Reset Power");
      options.addItem(-1, "");
    }
    else if (getActiveGridSection() >= 0)
      options.addItem(kSetPhaseToGrid, "Set Start Point");

    options.addItem(kCopy, "Copy");
    if (hasMatchingSystemClipboard())
      options.addItem(kPaste, "Paste");

    options.addItem(kSave, "Save to LFOs");

    options.addItem(kFlipHorizontal, "Flip Horizontal");
    options.addItem(kFlipVertical, "Flip Vertical");

    options.addItem(kImportLfo, "Import LFO");
    options.addItem(kExportLfo, "Export LFO");

    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    int point = active_point;
    int power = getActivePower();
    parent->showPopupSelector(this, e.getPosition(), options,
                              [=](int selection) { respondToCallback(point, power, selection); });
  }
  else
    LineEditor::mouseDown(e);
}

void LfoEditor::mouseDoubleClick(const MouseEvent& e) {
  if (!e.mods.isPopupMenu())
    LineEditor::mouseDoubleClick(e);
}

void LfoEditor::mouseUp(const MouseEvent& e) {
  if (!e.mods.isPopupMenu())
    LineEditor::mouseUp(e);
}

void LfoEditor::respondToCallback(int point, int power, int result) {
  if (result == kSetPhaseToPoint) {
    if (point >= 0 && point < numPoints())
      setPhase(getModel()->getPoint(point).first);
  }
  else if (result == kSetPhaseToPower) {
    if (power >= 0 && power < numPoints() - 1) {
      float from = getModel()->getPoint(power).first;
      float to = getModel()->getPoint(power + 1).first;
      setPhase((from + to) / 2.0f);
    }
  }
  else if (result == kSetPhaseToGrid) {
    int section = getActiveGridSection();
    int grid_size = getGridSizeX();
    if (section >= 0 && grid_size > 0)
      setPhase(section * 1.0f / grid_size);
  }
  else if (result == kImportLfo) {
    for (Listener* listener : listeners_)
      listener->importLfo();
  }
  else if (result == kExportLfo) {
    for (Listener* listener : listeners_)
      listener->exportLfo();
  }
  else
    LineEditor::respondToCallback(point, power, result);

  clearActiveMouseActions();
}

void LfoEditor::setPhase(float phase) {
  for (Listener* listener : listeners_)
    listener->setPhase(phase);
}

void LfoEditor::render(OpenGlWrapper& open_gl, bool animate) {
  static constexpr float kBackupTime = 1.0f / 50.0f;

  setGlPositions();
  renderGrid(open_gl, animate);

  vital::poly_float encoded_phase = wave_phase_->value();
  vital::poly_mask inactive_mask = 0;
  if (wave_phase_->isClearValue(encoded_phase)) {
    encoded_phase = 0.0f;
    inactive_mask = vital::constants::kFullMask;
  }

  vital::poly_float frequency = frequency_->value();
  if (frequency_->isClearValue(frequency))
    frequency = 0.0f;

  std::pair<vital::poly_float, vital::poly_float> decoded = vital::utils::decodePhaseAndVoice(encoded_phase);
  vital::poly_float phase = decoded.first;
  vital::poly_float voice = decoded.second;

  vital::poly_float phase_delta = vital::poly_float::abs(phase - last_phase_);
  vital::poly_float decay = vital::poly_float(1.0f) - phase_delta * kSpeedDecayMult;
  decay = vital::utils::clamp(decay, kBoostDecay, 1.0f);
  decay = vital::utils::maskLoad(decay, kBoostDecay, inactive_mask);
  decayBoosts(decay);

  vital::poly_mask switch_mask = vital::poly_float::notEqual(voice, last_voice_) | inactive_mask;
  vital::poly_float phase_reset = vital::utils::max(0.0f, phase - frequency * kBackupTime);
  last_phase_ = vital::utils::maskLoad(last_phase_, phase_reset, switch_mask);

  bool animating = animate;
  if (parent_)
    animating = animating && parent_->getSynth()->isModSourceEnabled(getName().toStdString());

  if (animating)
    boostRange(adjustBoostPhase(last_phase_), adjustBoostPhase(phase), kNumWrapPoints, decay);
  else 
    decayBoosts(0.0f);

  last_phase_ = phase;
  last_voice_ = voice;

  setLineWidth(findValue(Skin::kWidgetLineWidth));
  setFillCenter(findValue(Skin::kWidgetFillCenter));

  Colour fill_color = findColour(Skin::kWidgetSecondary1, true);
  float fill_fade = findValue(Skin::kWidgetFillFade);
  Colour fill_color_fade = fill_color.withMultipliedAlpha(1.0f - fill_fade);
  Colour position_color = findColour(Skin::kWidgetPrimary1, true);

  Colour fill_color_stereo = findColour(Skin::kWidgetSecondary2, true);
  Colour fill_color_stereo_fade = fill_color_stereo.withMultipliedAlpha(1.0f - fill_fade);
  Colour position_color_stereo = findColour(Skin::kWidgetPrimary2, true);

  if (animating) {
    setFill(true);
    setBoostAmount(findValue(Skin::kWidgetLineBoost));
    setFillBoostAmount(findValue(Skin::kWidgetFillBoost));

    setIndex(1);
    setColor(findColour(Skin::kWidgetPrimary2, true));
    setFillColors(fill_color_stereo_fade, fill_color_stereo);
    drawLines(open_gl, false);

    setIndex(0);
    setColor(findColour(Skin::kWidgetPrimary1, true));
    setFillColors(fill_color_fade, fill_color);
    drawLines(open_gl, anyBoostValue());

    setBoostAmount(0.0f);
    setFill(false);
    setColor(findColour(Skin::kWidgetCenterLine, true));
    drawLines(open_gl, anyBoostValue());

    setViewPort(open_gl);
    if (switch_mask.sum() == 0) {
      drawPosition(open_gl, position_color_stereo, phase[1]);
      drawPosition(open_gl, position_color, phase[0]);
    }
  }
  else {
    setBoostAmount(0.0f);
    setFillBoostAmount(0.0f);
    setFill(true);

    setColor(findColour(Skin::kWidgetPrimary2, true));
    setFillColors(fill_color_stereo_fade, fill_color_stereo);
    drawLines(open_gl, false);

    setColor(findColour(Skin::kWidgetPrimary1, true));
    setFillColors(fill_color_fade, fill_color);
    drawLines(open_gl, false);

    setFill(false);
    setColor(findColour(Skin::kWidgetCenterLine, true));
    drawLines(open_gl, false);
  }

  renderPoints(open_gl, animate);
  renderCorners(open_gl, animate);
}
