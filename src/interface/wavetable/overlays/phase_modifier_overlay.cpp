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

#include "phase_modifier_overlay.h"

#include "skin.h"
#include "wave_frame.h"
#include "text_look_and_feel.h"

namespace {
  const std::string phase_style_lookup[PhaseModifier::kNumPhaseStyles] = {
    "Normal",
    "+Even/-Odd",
    "Harmonic",
    "Harm +Even/-Odd",
    "Clear"
  };
} // namespace

PhaseModifierOverlay::PhaseModifierOverlay() : WavetableComponentOverlay("PHASE SHIFTER"), phase_modifier_(nullptr) {
  current_frame_ = nullptr;
  editor_ = std::make_unique<PhaseEditor>();
  addOpenGlComponent(editor_.get());
  editor_->addListener(this);
  editor_->setAlwaysOnTop(true);

  slider_ = std::make_unique<PhaseEditor>();
  addOpenGlComponent(slider_.get());
  slider_->addListener(this);
  slider_->setMaxTickHeight(1.0f);
  slider_->setAlwaysOnTop(true);

  phase_text_ = std::make_unique<TextEditor>();
  addChildComponent(phase_text_.get());
  phase_text_->addListener(this);
  phase_text_->setSelectAllWhenFocused(true);
  phase_text_->setMouseClickGrabsKeyboardFocus(true);
  phase_text_->setText("0");

  phase_style_ = std::make_unique<TextSelector>("Harmonic Phase");
  addSlider(phase_style_.get());
  phase_style_->setAlwaysOnTop(true);
  phase_style_->getImageComponent()->setAlwaysOnTop(true);
  phase_style_->setLookAndFeel(TextLookAndFeel::instance());
  phase_style_->addListener(this);
  phase_style_->setRange(0, PhaseModifier::kNumPhaseStyles - 1);
  phase_style_->setLongStringLookup(phase_style_lookup);
  phase_style_->setStringLookup(phase_style_lookup);
  phase_style_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  mix_ = std::make_unique<SynthSlider>("Phase Mix");
  addSlider(mix_.get());
  mix_->setAlwaysOnTop(true);
  mix_->getQuadComponent()->setAlwaysOnTop(true);
  mix_->setRange(0.0, 1.0);
  mix_->setDoubleClickReturnValue(true, 1.0);
  mix_->addListener(this);
  mix_->setSliderStyle(Slider::LinearBar);

  controls_background_.clearTitles();
  controls_background_.addTitle("STYLE");
  controls_background_.addTitle("");
  controls_background_.addTitle("MIX");
}

void PhaseModifierOverlay::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe == nullptr) {
    editor_->setVisible(false);
    current_frame_ = nullptr;
  }
  else if (keyframe->owner() == phase_modifier_) {
    editor_->setVisible(true);
    current_frame_ = phase_modifier_->getKeyframe(keyframe->index());
    editor_->setPhase(current_frame_->getPhase());
    slider_->setPhase(current_frame_->getPhase());
    mix_->setValue(current_frame_->getMix(), dontSendNotification);
    mix_->redoImage();
    phase_style_->setValue(phase_modifier_->getPhaseStyle());
  }
}

void PhaseModifierOverlay::setEditBounds(Rectangle<int> bounds) {
  static constexpr float kPhaseStyleWidthHeightRatio = 4.0f;
  static constexpr float kPhaseWidthHeightRatio = 8.0f;
  static constexpr float kMixWidthHeightRatio = 5.0f;
  static constexpr float kMixPaddingRatio = 0.5f;

  int padding = getPadding();
  int phase_style_width = bounds.getHeight() * kPhaseStyleWidthHeightRatio;
  int phase_width = bounds.getHeight() * kPhaseWidthHeightRatio;
  int mix_width = bounds.getHeight() * kMixWidthHeightRatio;
  int mix_padding = bounds.getHeight() * kMixPaddingRatio;
  int total_width = phase_style_width + phase_width + mix_width + 2 * padding;
  setControlsWidth(total_width);
  WavetableComponentOverlay::setEditBounds(bounds);

  int x = bounds.getX() + (bounds.getWidth() - total_width) / 2;
  int title_height = WavetableComponentOverlay::kTitleHeightRatio * bounds.getHeight();
  int y = bounds.getY();
  int y_title = y + title_height;
  int height = bounds.getHeight();
  int height_title = height - title_height;

  phase_style_->setTextHeightPercentage(0.4f);
  phase_style_->setBounds(x, y_title, phase_style_width, height_title);
  slider_->setBounds(phase_style_->getRight() + padding, y, phase_width, height);
  mix_->setBounds(slider_->getRight() + padding + mix_padding, y_title, mix_width - 2 * mix_padding, height_title);

  phase_style_->redoImage();
  mix_->redoImage();

  controls_background_.clearLines();
  controls_background_.addLine(phase_style_width);
  controls_background_.addLine(phase_style_width + phase_width + padding);
}

bool PhaseModifierOverlay::setTimeDomainBounds(Rectangle<int> bounds) {
  editor_->setBounds(bounds);
  editor_->setColor(findColour(Skin::kLightenScreen, true));
  slider_->setColor(findColour(Skin::kLightenScreen, true));
  return false;
}

void PhaseModifierOverlay::textEditorReturnKeyPressed(TextEditor& text_editor) {
  setPhase(text_editor.getText());
  notifyChanged(true);
}

void PhaseModifierOverlay::textEditorFocusLost(TextEditor& text_editor) {
  setPhase(text_editor.getText());
  notifyChanged(true);
}

void PhaseModifierOverlay::phaseChanged(float phase, bool mouse_up) {
  if (current_frame_ == nullptr)
    return;

  phase_text_->setText(String(phase * vital::kDegreesPerCycle / (2.0f * vital::kPi)));
  slider_->setPhase(phase);
  editor_->setPhase(phase);
  current_frame_->setPhase(phase);

  notifyChanged(mouse_up);
}

void PhaseModifierOverlay::sliderValueChanged(Slider* moved_slider) {
  if (phase_modifier_ == nullptr || current_frame_ == nullptr)
    return;
  
  if (moved_slider == phase_style_.get()) {
    int value = phase_style_->getValue();
    phase_modifier_->setPhaseStyle(static_cast<PhaseModifier::PhaseStyle>(value));
    notifyChanged(true);
  }
  else if (moved_slider == mix_.get()) {
    if (current_frame_)
      current_frame_->setMix(mix_->getValue());
    notifyChanged(false);
  }
  else
    notifyChanged(false);
}

void PhaseModifierOverlay::sliderDragEnded(Slider* moved_slider) {
  notifyChanged(true);
}

void PhaseModifierOverlay::setPhase(String phase_string) {
  float phase = 2.0f * vital::kPi / vital::kDegreesPerCycle * phase_string.getFloatValue();
  if (current_frame_)
    current_frame_->setPhase(phase);
  editor_->setPhase(phase);
}
