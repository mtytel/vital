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

#include "envelope_editor.h"

#include "fonts.h"
#include "skin.h"
#include "shaders.h"
#include "synth_gui_interface.h"
#include "utils.h"

namespace {
  String formatTime(float time) {
    if (time < 1.0f) {
      int ms_value = time * vital::kMsPerSec;
      return String(ms_value) + "ms";
    }

    float sec_value = int(time * 10.0f) / 10.0f;
    return String(sec_value) + "s";
  }

  constexpr float kMinWindowSize = 0.125f;
  constexpr float kMaxWindowSize = 64.0f;
}

EnvelopeEditor::EnvelopeEditor(
    const String& prefix,
    const vital::output_map& mono_modulations,
    const vital::output_map& poly_modulations) : OpenGlLineRenderer(kTotalPoints),
                                                 drag_circle_(Shaders::kCircleFragment),
                                                 hover_circle_(Shaders::kRingFragment),
                                                 grid_lines_(kMaxGridLines), sub_grid_lines_(kMaxGridLines),
                                                 position_circle_(Shaders::kRingFragment),
                                                 point_circles_(kNumSections, Shaders::kRingFragment),
                                                 power_circles_(kNumSections, Shaders::kCircleFragment) {
  addAndMakeVisible(drag_circle_);
  addAndMakeVisible(hover_circle_);
  addAndMakeVisible(grid_lines_);
  addAndMakeVisible(sub_grid_lines_);
  addAndMakeVisible(position_circle_);
  addAndMakeVisible(point_circles_);
  addAndMakeVisible(power_circles_);
  hover_circle_.setThickness(1.0f);
                                                   
  for (int i = 0; i < kMaxTimesShown; ++i) {
    times_[i] = std::make_unique<PlainTextComponent>("Time", "");
    times_[i]->setJustification(Justification::centredLeft);
    times_[i]->setScissor(true);
    addAndMakeVisible(times_[i].get());
  }

  enableBackwardBoost(false);
  parent_ = nullptr;
  delay_hover_ = false;
  attack_hover_ = false;
  hold_hover_ = false;
  sustain_hover_ = false;
  release_hover_ = false;
  attack_power_hover_ = false;
  decay_power_hover_ = false;
  release_power_hover_ = false;
  mouse_down_ = false;

  animate_ = false;
  reset_positions_ = true;
  size_ratio_ = 1.0f;
  window_time_ = 4.0f;

  current_position_alpha_ = 0.0f;
  last_phase_ = 0.0f;

  envelope_phase_ = nullptr;

  attack_slider_ = nullptr;
  attack_power_slider_ = nullptr;
  decay_slider_ = nullptr;
  decay_power_slider_ = nullptr;
  sustain_slider_ = nullptr;
  release_slider_ = nullptr;
  release_power_slider_ = nullptr;

  delay_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "_delay");
  attack_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "_attack");
  hold_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "_hold");
  decay_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "_decay");
  sustain_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "_sustain");
  release_outputs_ = getOutputs(mono_modulations, poly_modulations, prefix + "_release");

  setFill(true);
  setFillCenter(-1.0f);
}

EnvelopeEditor::~EnvelopeEditor() { }

void EnvelopeEditor::paintBackground(Graphics& g) {
  setBackgroundColor(findColour(Skin::kWidgetBackground, true));
  OpenGlComponent::paintBackground(g);

  setColors();
}

void EnvelopeEditor::parentHierarchyChanged() {
  parent_ = findParentComponentOfClass<SynthGuiInterface>();

  if (envelope_phase_ == nullptr && parent_)
    envelope_phase_ = parent_->getSynth()->getStatusOutput(getName().toStdString() + "_phase");
  
  if (parent_) {
    setColors();
    setTimePositions();
  }

  OpenGlLineRenderer::parentHierarchyChanged();
}

void EnvelopeEditor::pickHoverPosition(Point<float> position) {
  position.x = unpadX(position.x);
  position.y = unpadY(position.y);
  float delay_x = getSliderDelayX();
  float attack_x = getSliderAttackX();
  float hold_x = getSliderHoldX();
  float decay_x = getSliderDecayX();
  float sustain_y = getSliderSustainY();
  float release_x = getSliderReleaseX();

  Point<float> delay_point(delay_x, getHeight());
  Point<float> attack_power_point((delay_x + attack_x) / 2.0f, getSliderAttackValue(0.5f));
  Point<float> top_point(attack_x, 0);
  Point<float> hold_point(hold_x, 0);
  Point<float> decay_power_point((hold_x + decay_x) / 2.0f, getSliderDecayValue(0.5f));
  Point<float> sustain_point(decay_x, sustain_y);
  Point<float> release_power_point((decay_x + release_x) / 2.0f, getSliderReleaseValue(0.5f));
  Point<float> release_point(release_x, getHeight());

  std::vector<Point<float>> points = { top_point, sustain_point, release_point };
  if (delay_x > 0.0f)
    points.push_back(delay_point);
  if (hold_x > attack_x)
    points.push_back(hold_point);
  if (release_x - decay_x > kMinPointDistanceForPower && sustain_y < getHeight())
    points.push_back(release_power_point);
  if (decay_x - attack_x > kMinPointDistanceForPower && sustain_y > 0)
    points.push_back(decay_power_point);
  if (attack_x - delay_x > kMinPointDistanceForPower)
    points.push_back(attack_power_point);

  float closest_distance_squared = getHeight() * getHeight();
  for (const Point<float>& point : points) {
    float distance = position.getDistanceSquaredFrom(point);
    closest_distance_squared = std::min(closest_distance_squared, distance);
  }

  bool release_hover = position.getDistanceSquaredFrom(release_point) <= closest_distance_squared;
  bool sustain_hover = position.getDistanceSquaredFrom(sustain_point) <= closest_distance_squared;
  bool attack_hover = position.getDistanceSquaredFrom(top_point) <= closest_distance_squared;
  bool delay_hover = position.getDistanceSquaredFrom(delay_point) == closest_distance_squared;
  bool hold_hover = hold_x > attack_x && position.getDistanceSquaredFrom(hold_point) == closest_distance_squared;
  bool release_power_hover = position.getDistanceSquaredFrom(release_power_point) == closest_distance_squared;
  bool decay_power_hover = position.getDistanceSquaredFrom(decay_power_point) == closest_distance_squared;
  bool attack_power_hover = position.getDistanceSquaredFrom(attack_power_point) == closest_distance_squared;

  if (delay_hover != delay_hover_ || attack_hover != attack_hover_ || hold_hover != hold_hover_ ||
      sustain_hover != sustain_hover_ || release_hover != release_hover_ ||
      attack_power_hover != attack_power_hover_ || decay_power_hover != decay_power_hover_ ||
      release_power_hover != release_power_hover_) {
    delay_hover_ = delay_hover;
    attack_hover_ = attack_hover;
    hold_hover_ = hold_hover;
    sustain_hover_ = sustain_hover;
    release_hover_ = release_hover;
    attack_power_hover_ = attack_power_hover;
    decay_power_hover_ = decay_power_hover;
    release_power_hover_ = release_power_hover;
    resetPositions();
  }
}

void EnvelopeEditor::mouseMove(const MouseEvent& e) {
  Point<float> position = e.getPosition().toFloat();
  pickHoverPosition(position);
}

void EnvelopeEditor::mouseExit(const MouseEvent& e) {
  delay_hover_ = false;
  attack_hover_ = false;
  hold_hover_ = false;
  sustain_hover_ = false;
  release_hover_ = false;
  attack_power_hover_ = false;
  decay_power_hover_ = false;
  release_power_hover_ = false;
  resetPositions();
}

void EnvelopeEditor::mouseDown(const MouseEvent& e) {
  mouse_down_ = true;
  last_edit_position_ = e.position;
  resetPositions();
}

void EnvelopeEditor::mouseDrag(const MouseEvent& e) {
  float delta_power = (last_edit_position_.y - e.position.y) * kPowerMouseMultiplier;
  last_edit_position_ = e.position;

  if (delay_hover_)
    setDelayX(last_edit_position_.x);
  else if (release_hover_)
    setReleaseX(last_edit_position_.x);
  else if (sustain_hover_) {
    setDecayX(last_edit_position_.x);
    setSustainY(last_edit_position_.y);
  }
  else if (attack_hover_)
    setAttackX(last_edit_position_.x);
  else if (hold_hover_)
    setHoldX(last_edit_position_.x);
  else if (attack_power_hover_)
    setAttackPower(attack_power_slider_->getValue() + delta_power);
  else if (decay_power_hover_)
    setDecayPower(decay_power_slider_->getValue() + delta_power);
  else if (release_power_hover_)
    setReleasePower(release_power_slider_->getValue() + delta_power);

  resetPositions();
}

void EnvelopeEditor::mouseDoubleClick(const MouseEvent& e) {
  if (attack_power_hover_)
    setAttackPower(0.0f);
  else if (decay_power_hover_)
    setDecayPower(0.0f);
  else if (release_power_hover_)
    setReleasePower(0.0f);
}

void EnvelopeEditor::mouseUp(const MouseEvent& e) {
  mouse_down_ = false;
  resetPositions();
}

void EnvelopeEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  static constexpr float kMouseWheelSensitivity = 1.0f;

  zoom(std::pow(2.0f, -kMouseWheelSensitivity * wheel.deltaY));
}

void EnvelopeEditor::magnifyZoom(Point<float> delta) {
  static constexpr float kMouseWheelSensitivity = 0.02f;

  zoom(std::pow(2.0f, kMouseWheelSensitivity * delta.y));
}

void EnvelopeEditor::magnifyReset() {
  static constexpr float kResetBuffer = 0.25f;

  window_time_ = (1.0f + kResetBuffer) * getSliderReleaseX() * window_time_ / getWidth();
  window_time_ = std::max(std::min(window_time_, kMaxWindowSize), kMinWindowSize);
  setTimePositions();
  resetPositions();
}

void EnvelopeEditor::zoom(float amount) {
  window_time_ *= amount;
  window_time_ = std::max(std::min(window_time_, kMaxWindowSize), kMinWindowSize);
  setTimePositions();
  resetPositions();
}

void EnvelopeEditor::guiChanged(SynthSlider* slider) {
  resetPositions();
}

inline float EnvelopeEditor::getSliderDelayX() {
  if (delay_slider_ == nullptr)
    return 0.0f;

  float time = delay_slider_->getAdjustedValue(delay_slider_->getValue());
  return getWidth() * time / window_time_;
}

inline float EnvelopeEditor::getSliderAttackX() {
  if (attack_slider_ == nullptr)
    return 0.0f;

  float time = attack_slider_->getAdjustedValue(attack_slider_->getValue());
  return getSliderDelayX() + getWidth() * time / window_time_;
}

inline float EnvelopeEditor::getSliderHoldX() {
  if (hold_slider_ == nullptr)
    return 0.0f;

  float time = hold_slider_->getAdjustedValue(hold_slider_->getValue());
  return getSliderAttackX()  + getWidth() * time / window_time_;
}

float EnvelopeEditor::getSliderDecayX() {
  if (decay_slider_ == nullptr)
    return 0.0f;

  float time = decay_slider_->getAdjustedValue(decay_slider_->getValue());
  return getSliderHoldX() + getWidth() * time / window_time_;
}

float EnvelopeEditor::getSliderSustainY() {
  if (sustain_slider_ == nullptr)
    return 0.0f;

  float percent = sustain_slider_->valueToProportionOfLength(sustain_slider_->getValue());
  return getHeight() * (1.0f - percent);
}

float EnvelopeEditor::getSliderReleaseX() {
  if (release_slider_ == nullptr)
    return 0.0f;

  float time = release_slider_->getAdjustedValue(release_slider_->getValue());
  return getSliderDecayX() + getWidth() * time / window_time_;
}

inline float EnvelopeEditor::getDelayTime(int index) {
  vital::poly_float delays = getOutputsTotal(delay_outputs_, delay_slider_->getValue());
  return delay_slider_->getAdjustedValue(std::max<float>(0.0f, delays[index]));
}

inline float EnvelopeEditor::getAttackTime(int index) {
  vital::poly_float attacks = getOutputsTotal(attack_outputs_, attack_slider_->getValue());
  return attack_slider_->getAdjustedValue(std::max<float>(0.0f, attacks[index]));
}

inline float EnvelopeEditor::getHoldTime(int index) {
  vital::poly_float holds = getOutputsTotal(hold_outputs_, hold_slider_->getValue());
  return hold_slider_->getAdjustedValue(std::max<float>(0.0f, holds[index]));
}

inline float EnvelopeEditor::getDecayTime(int index) {
  vital::poly_float decays = getOutputsTotal(decay_outputs_, decay_slider_->getValue());
  return decay_slider_->getAdjustedValue(std::max<float>(0.0f, decays[index]));
}

inline float EnvelopeEditor::getReleaseTime(int index) {
  vital::poly_float releases = getOutputsTotal(release_outputs_, release_slider_->getValue());
  return release_slider_->getAdjustedValue(std::max<float>(0.0f, releases[index]));
}

inline float EnvelopeEditor::getDelayX(int index) {
  if (index < 0)
    return getSliderDelayX();

  return getWidth() * getDelayTime(index) / window_time_;
}

inline float EnvelopeEditor::getAttackX(int index) {
  if (index < 0)
    return getSliderAttackX();

  return getDelayX(index) + getWidth() * getAttackTime(index) / window_time_;
}

inline float EnvelopeEditor::getHoldX(int index) {
  if (index < 0)
    return getSliderHoldX();

  return getAttackX(index) + getWidth() * getHoldTime(index) / window_time_;
}

float EnvelopeEditor::getDecayX(int index) {
  if (index < 0)
    return getSliderDecayX();

  return getHoldX(index) + getWidth() * getDecayTime(index) / window_time_;
}

float EnvelopeEditor::getSustainY(int index) {
  if (index < 0)
    return getSliderSustainY();

  vital::poly_float sustains = getOutputsTotal(sustain_outputs_, sustain_slider_->getValue());
  float percent = sustains[index] / sustain_slider_->getRange().getLength();
  percent = vital::utils::clamp(percent, 0.0f, 1.0f);
  return getHeight() * (1.0f - percent);
}

float EnvelopeEditor::getReleaseX(int index) {
  if (index < 0)
    return getSliderReleaseX();

  return getDecayX(index) + getWidth() * getReleaseTime(index) / window_time_;
}

float EnvelopeEditor::getBackupPhase(float phase, int index) {
  static constexpr float kBackupTime = 1.0f / 50.0f;
  static constexpr float kTotalPhase = vital::kVoiceKill - vital::kVoiceOn;
  static constexpr float kDecayPoint = (vital::kVoiceDecay - 1.0f * vital::kVoiceOn) / kTotalPhase;
  static constexpr float kReleasePoint = (vital::kVoiceOff - 1.0f * vital::kVoiceOn) / kTotalPhase;

  float time = kBackupTime;
  float current_phase = phase;

  if (current_phase == kReleasePoint)
    return phase;

  if (current_phase > kReleasePoint) {
    float release_time = getReleaseTime(index);
    if (release_time <= 0.0f)
      current_phase = kReleasePoint;
    else {
      float phase_delta = time / release_time;
      float time_released = release_time * (current_phase - kReleasePoint);

      current_phase = current_phase - phase_delta;
      if (current_phase >= kReleasePoint)
        return current_phase;

      time -= time_released;
      current_phase = std::max(current_phase, kReleasePoint);
    }
  }
  if (current_phase > kDecayPoint) {
    float decay_time = getDecayTime(index);
    if (decay_time <= 0.0f)
      current_phase = kDecayPoint;
    else {
      float phase_delta = time / decay_time;
      float time_decayed = decay_time * (current_phase - kDecayPoint);

      current_phase = current_phase - phase_delta;
      if (current_phase >= kDecayPoint)
        return current_phase;

      time -= time_decayed;
      current_phase = std::max(current_phase, kDecayPoint);
    }
  }
  float attack_time = getAttackTime(index) + getDelayTime(index);
  if (attack_time <= 0.0f)
    return 0.0f;

  float phase_delta = time / attack_time;
  return std::max(0.0f, current_phase - phase_delta);
}

vital::poly_float EnvelopeEditor::getBackupPhase(vital::poly_float phase) {
  vital::poly_float backup = 0.0f;
  backup.set(0, getBackupPhase(phase[0], 0));
  backup.set(1, getBackupPhase(phase[1], 1));
  return backup;
}

float EnvelopeEditor::getEnvelopeValue(float t, float power, float start, float end) {
  return start + (end - start) * vital::futils::powerScale(t, power);
}

inline float EnvelopeEditor::getSliderAttackValue(float t) {
  float power = attack_power_slider_->getValue();
  return getHeight() - getEnvelopeValue(1.0f - t, power, getHeight(), 0.0f);
}

inline float EnvelopeEditor::getSliderDecayValue(float t) {
  float power = decay_power_slider_->getValue();
  return getEnvelopeValue(t, power, 0.0f, getSliderSustainY());
}

inline float EnvelopeEditor::getSliderReleaseValue(float t) {
  float power = release_power_slider_->getValue();
  return getEnvelopeValue(t, power, getSliderSustainY(), getHeight());
}

inline float EnvelopeEditor::getAttackValue(float t, int index) {
  if (index < 0)
    return getSliderAttackValue(t);

  float power = attack_power_slider_->getValue();
  return getHeight() - getEnvelopeValue(1.0f - t, power, getHeight(), 0.0f);
}

inline float EnvelopeEditor::getDecayValue(float t, int index) {
  if (index < 0)
    return getSliderDecayValue(t);

  float power = decay_power_slider_->getValue();
  return getEnvelopeValue(t, power, 0.0f, getSustainY(index));
}

inline float EnvelopeEditor::getReleaseValue(float t, int index) {
  if (index < 0)
    return getSliderReleaseValue(t);

  float power = release_power_slider_->getValue();
  return getEnvelopeValue(t, power, getSustainY(index), getHeight());
}

void EnvelopeEditor::setDelayX(float x) {
  if (delay_slider_ == nullptr)
    return;

  float time = x * window_time_ / getWidth();
  delay_slider_->setValueFromAdjusted(time);
}

void EnvelopeEditor::setAttackX(float x) {
  if (attack_slider_ == nullptr)
    return;

  float time = (x - getSliderDelayX()) * window_time_ / getWidth();
  attack_slider_->setValueFromAdjusted(time);
}

void EnvelopeEditor::setHoldX(float x) {
  if (delay_slider_ == nullptr)
    return;

  float time = (x - getSliderAttackX()) * window_time_ / getWidth();
  hold_slider_->setValueFromAdjusted(time);
}

void EnvelopeEditor::setPower(SynthSlider* slider, float power) {
  power = vital::utils::clamp(power, (float)slider->getMinimum(), (float)slider->getMaximum());
  slider->setValue(power);
}

void EnvelopeEditor::setAttackPower(float power) {
  setPower(attack_power_slider_, power);
}

void EnvelopeEditor::setDecayPower(float power) {
  setPower(decay_power_slider_, power);
}

void EnvelopeEditor::setReleasePower(float power) {
  setPower(release_power_slider_, power);
}

void EnvelopeEditor::setDecayX(float x) {
  if (decay_slider_ == nullptr)
    return;

  float time = (x - getSliderHoldX()) * window_time_ / getWidth();
  decay_slider_->setValueFromAdjusted(time);
  window_time_ = std::max(window_time_, x * window_time_ / getWidth());
  window_time_ = std::max(std::min(window_time_, kMaxWindowSize), kMinWindowSize);
}

void EnvelopeEditor::setSustainY(float y) {
  if (sustain_slider_ == nullptr)
    return;

  float percent = vital::utils::clamp(1.0 - y / getHeight(), 0.0f, 1.0f);
  sustain_slider_->setValue(sustain_slider_->proportionOfLengthToValue(percent));
}

void EnvelopeEditor::setReleaseX(float x) {
  if (release_slider_ == nullptr)
    return;

  float time = (x - getSliderDecayX()) * window_time_ / getWidth();
  release_slider_->setValueFromAdjusted(time);
  window_time_ = std::max(window_time_, x * window_time_ / getWidth());
  window_time_ = std::max(std::min(window_time_, kMaxWindowSize), kMinWindowSize);
}

void EnvelopeEditor::setDelaySlider(SynthSlider* delay_slider) {
  delay_slider_ = delay_slider;
  delay_slider_->addSliderListener(this);
}

void EnvelopeEditor::setAttackSlider(SynthSlider* attack_slider) {
  attack_slider_ = attack_slider;
  attack_slider_->addSliderListener(this);
}

void EnvelopeEditor::setHoldSlider(SynthSlider* hold_slider) {
  hold_slider_ = hold_slider;
  hold_slider_->addSliderListener(this);
}

void EnvelopeEditor::setAttackPowerSlider(SynthSlider* attack_power_slider) {
  attack_power_slider_ = attack_power_slider;
  attack_power_slider_->addSliderListener(this);
}

void EnvelopeEditor::setDecaySlider(SynthSlider* decay_slider) {
  decay_slider_ = decay_slider;
  decay_slider_->addSliderListener(this);
}

void EnvelopeEditor::setDecayPowerSlider(SynthSlider* decay_power_slider) {
  decay_power_slider_ = decay_power_slider;
  decay_power_slider_->addSliderListener(this);
}

void EnvelopeEditor::setSustainSlider(SynthSlider* sustain_slider) {
  sustain_slider_ = sustain_slider;
  sustain_slider_->addSliderListener(this);
}

void EnvelopeEditor::setReleaseSlider(SynthSlider* release_slider) {
  release_slider_ = release_slider;
  release_slider_->addSliderListener(this);
}

void EnvelopeEditor::setReleasePowerSlider(SynthSlider* release_power_slider) {
  release_power_slider_ = release_power_slider;
  release_power_slider_->addSliderListener(this);
}

inline vital::poly_float EnvelopeEditor::getOutputsTotal(
    std::pair<vital::Output*, vital::Output*> outputs, vital::poly_float default_value) {
  if (!animate_ || !outputs.first->owner->enabled())
    return default_value;
  if (num_voices_readout_ == nullptr || num_voices_readout_->value()[0] <= 0.0f)
    return outputs.first->trigger_value;

  return outputs.first->trigger_value + outputs.second->trigger_value;
}

void EnvelopeEditor::resetEnvelopeLine(int index) {
  float delay_x = getDelayX(index);
  float attack_x = getAttackX(index);
  float hold_x = getHoldX(index);
  float decay_x = getDecayX(index);
  float release_x = getReleaseX(index);

  for (int i = 0; i < kNumPointsPerSection; ++i) {
    float t = (1.0f * i) / kNumPointsPerSection;
    float x = vital::utils::interpolate(delay_x, attack_x, t);
    float y = getAttackValue(t, index);
    setXAt(i, padX(x));
    setYAt(i, padY(y));
  }

  for (int i = 0; i < kNumPointsPerSection; ++i) {
    float t = (1.0f * i) / kNumPointsPerSection;
    float x = vital::utils::interpolate(attack_x, hold_x, t);
    setXAt(i + kNumPointsPerSection, padX(x));
    setYAt(i + kNumPointsPerSection, padY(0.0f));
  }

  for (int i = 0; i < kNumPointsPerSection; ++i) {
    float t = (1.0f * i) / kNumPointsPerSection;
    float x = vital::utils::interpolate(hold_x, decay_x, t);
    float y = getDecayValue(t, index);
    setXAt(i + 2 * kNumPointsPerSection, padX(x));
    setYAt(i + 2 * kNumPointsPerSection, padY(y));
  }

  for (int i = 0; i <= kNumPointsPerSection; ++i) {
    float t = (1.0f * i) / kNumPointsPerSection;
    float x = vital::utils::interpolate(decay_x, release_x, t);
    float y = getReleaseValue(t, index);
    setXAt(i + 3 * kNumPointsPerSection, padX(x));
    setYAt(i + 3 * kNumPointsPerSection, padY(y));
  }
}

std::pair<float, float> EnvelopeEditor::getPosition(int index) {
  float phase = envelope_phase_->value()[index];

  if (envelope_phase_->isClearValue(phase) || phase < vital::kVoiceOn || phase >= vital::kVoiceKill)
    return { -1.0f, -1.0f};

  float delay_time = getDelayTime(index);
  float attack_time = getAttackTime(index);
  float hold_time = getHoldTime(index);
  float decay_time = getDecayTime(index);
  float release_time = getReleaseTime(index);
  int stage = phase;
  float stage_phase = phase - stage;

  float time = 0.0f;
  float value = 0.0f;
  if (stage == vital::kVoiceOn) {
    time = delay_time + stage_phase * attack_time;
    value = getAttackValue(stage_phase, index);
  }
  else if (stage == vital::kVoiceHold) {
    time = delay_time + attack_time + stage_phase * hold_time;
    value = 1.0f;
  }
  else if (stage == vital::kVoiceDecay) {
    time = delay_time + attack_time + hold_time + stage_phase * decay_time;
    value = getDecayValue(stage_phase, index);
  }
  else if (stage == vital::kVoiceOff) {
    time = delay_time + attack_time + hold_time + decay_time + stage_phase * release_time;
    value = getReleaseValue(stage_phase, index);
  }

  float x = 2.0f * time / window_time_ - 1.0f;
  float y = 1.0 - 2.0f * value / getHeight();
  return { padOpenGlX(x), padOpenGlY(y) };
}

inline float EnvelopeEditor::padX(float x) {
  return x * (1.0f - kPaddingX) + kPaddingX * getWidth() / 2.0f;
}

inline float EnvelopeEditor::padY(float y) {
  return y * (1.0f - kPaddingY / 2.0f) + kPaddingY * getHeight() / 2.0f;
}

inline float EnvelopeEditor::unpadX(float x) {
  return (x - kPaddingX * getWidth() / 2.0f) / (1.0f - kPaddingX);
}

inline float EnvelopeEditor::unpadY(float y) {
  return (y - kPaddingY * getHeight() / 2.0f) / (1.0f - kPaddingY / 2.0f);
}

inline float EnvelopeEditor::padOpenGlX(float x) {
  return x * (1.0f - kPaddingX);
}

inline float EnvelopeEditor::padOpenGlY(float y) {
  return y * (1.0f - kPaddingY / 2.0f) - kPaddingY / 2.0f;
}

void EnvelopeEditor::init(OpenGlWrapper& open_gl) {
  OpenGlLineRenderer::init(open_gl);
  drag_circle_.init(open_gl);
  hover_circle_.init(open_gl);
  grid_lines_.init(open_gl);
  sub_grid_lines_.init(open_gl);
  point_circles_.init(open_gl);
  power_circles_.init(open_gl);
  position_circle_.init(open_gl);
  
  for (int i = 0; i < kMaxTimesShown; ++i)
    times_[i]->init(open_gl);
}

void EnvelopeEditor::render(OpenGlWrapper& open_gl, bool animate) {
  for (int i = 0; i < kMaxTimesShown; ++i)
    times_[i]->render(open_gl, animate);
  
  setGlPositions();
  grid_lines_.render(open_gl, animate);
  sub_grid_lines_.render(open_gl, animate);

  setLineWidth(findValue(Skin::kWidgetLineWidth));
  setFillCenter(findValue(Skin::kWidgetFillCenter));
  vital::poly_float input_phase = envelope_phase_->value();
  vital::poly_mask off_mask = vital::poly_float::equal(input_phase, vital::kVoiceKill);
  float phase_length = vital::kVoiceKill - vital::kVoiceOn;
  vital::poly_float phase = (input_phase - vital::kVoiceOn) * (1.0f / phase_length);
  phase = vital::utils::maskLoad(phase, 1.0f, off_mask);
  phase = vital::utils::min(phase, 1.0f);

  vital::poly_mask reset_mask = vital::poly_float::greaterThan(last_phase_, phase);
  vital::poly_float backup_phase = getBackupPhase(phase);
  last_phase_ = vital::utils::maskLoad(last_phase_, backup_phase, reset_mask);

  if (!animate_)
    last_phase_ = phase;

  animate_ = animate;
  bool animating = animate;
  if (parent_)
    animating = animating && parent_->getSynth()->isModSourceEnabled(getName().toStdString());

  Colour envelope_graph_fill = fill_left_color_;
  float fill_fade = findValue(Skin::kWidgetFillFade);
  Colour envelope_graph_fill_fade = envelope_graph_fill.withMultipliedAlpha(1.0f - fill_fade);
  Colour envelope_graph_fill_stereo = fill_right_color_;
  Colour envelope_graph_fill_stereo_fade = envelope_graph_fill_stereo.withMultipliedAlpha(1.0f - fill_fade);

  if (animating) {
    decayBoosts(kTailDecay);

    float release_point = (vital::kVoiceOff - vital::kVoiceOn) / phase_length;
    vital::poly_mask released_mask = vital::poly_float::greaterThan(phase, release_point);
    released_mask = released_mask & vital::poly_float::lessThan(last_phase_, release_point) & ~reset_mask;
    last_phase_ = vital::utils::maskLoad(last_phase_, release_point, released_mask);

    last_phase_ = vital::utils::max(last_phase_, 0.0f);
    if (!envelope_phase_->isClearValue(input_phase))
      boostRange(last_phase_, phase, 0, kTailDecay);
    last_phase_ = phase;

    setFill(true);
    setBoostAmount(findValue(Skin::kWidgetLineBoost));
    setFillBoostAmount(findValue(Skin::kWidgetFillBoost));
    resetEnvelopeLine(1);
    setIndex(1);
    setColor(line_right_color_);
    setFillColors(envelope_graph_fill_stereo_fade, envelope_graph_fill_stereo);
    drawLines(open_gl, false);

    resetEnvelopeLine(0);
    setIndex(0);
    setColor(line_left_color_);
    setFillColors(envelope_graph_fill_fade, envelope_graph_fill);
    drawLines(open_gl, anyBoostValue());

    setFill(false);
    setBoostAmount(0.0f);
    setFillBoostAmount(0.0f);
    resetEnvelopeLine(-1);
    setColor(line_center_color_);
    drawLines(open_gl, anyBoostValue());

    setViewPort(open_gl);
    drawPosition(open_gl, 1);
    drawPosition(open_gl, 0);
  }
  else {
    setBoostAmount(0.0f);
    setFillBoostAmount(0.0f);
    decayBoosts(0.0f);
    resetEnvelopeLine(-1);

    setFill(true);
    setColor(line_right_color_);
    setFillColors(envelope_graph_fill_stereo_fade, envelope_graph_fill_stereo);
    drawLines(open_gl, false);

    setColor(line_left_color_);
    setFillColors(envelope_graph_fill_fade, envelope_graph_fill);
    drawLines(open_gl, anyBoostValue());

    setFill(false);
    setColor(line_center_color_);
    drawLines(open_gl, anyBoostValue());
  }

  point_circles_.setColor(line_center_color_);
  point_circles_.setAltColor(background_color_);
  point_circles_.render(open_gl, animate);

  power_circles_.setColor(line_center_color_);
  power_circles_.render(open_gl, animate);

  drag_circle_.render(open_gl, animate);
  hover_circle_.render(open_gl, animate);

  renderCorners(open_gl, animate);
}

void EnvelopeEditor::destroy(OpenGlWrapper& open_gl) {
  drag_circle_.destroy(open_gl);
  hover_circle_.destroy(open_gl);
  grid_lines_.destroy(open_gl);
  sub_grid_lines_.destroy(open_gl);
  point_circles_.destroy(open_gl);
  power_circles_.destroy(open_gl);
  position_circle_.destroy(open_gl);
  
  for (int i = 0; i < kMaxTimesShown; ++i)
    times_[i]->destroy(open_gl);
  
  OpenGlLineRenderer::destroy(open_gl);
}

void EnvelopeEditor::setEditingCircleBounds() {
  float width = getWidth();
  float height = getHeight();
  float delay_x = padOpenGlX(getSliderDelayX() * 2.0f / width - 1.0f);
  float attack_x = padOpenGlX(getSliderAttackX() * 2.0f / width - 1.0f);
  float hold_x = padOpenGlX(getSliderHoldX() * 2.0f / width - 1.0f);
  float decay_x = padOpenGlX(getSliderDecayX() * 2.0f / width - 1.0f);
  float sustain_y = padOpenGlY(1.0f - getSliderSustainY() * 2.0f / height);
  float release_x = padOpenGlX(getSliderReleaseX() * 2.0f / width - 1.0f);
  float bottom = padOpenGlY(-1.0f);
  float top = padOpenGlY(1.0f);

  float grab_width = kMarkerGrabRadius * size_ratio_ * 4.0f / width;
  float grab_height = kMarkerGrabRadius * size_ratio_ * 4.0f / height;
  float hover_width = kMarkerHoverRadius * size_ratio_ * 4.0f / width;
  float hover_height = kMarkerHoverRadius * size_ratio_ * 4.0f / height;
  Point<float> grab_point(-10, -10);
  if (delay_hover_)
    grab_point = Point<float>(delay_x, bottom);
  else if (release_hover_)
    grab_point = Point<float>(release_x, bottom);
  else if (sustain_hover_)
    grab_point = Point<float>(decay_x, sustain_y);
  else if (attack_hover_)
    grab_point = Point<float>(attack_x, top);
  else if (hold_hover_)
    grab_point = Point<float>(hold_x, top);
  else if (attack_power_hover_)
    grab_point = Point<float>((delay_x + attack_x) / 2.0f, 1.0f - 2.0f * padY(getSliderAttackValue(0.5f)) / height);
  else if (decay_power_hover_)
    grab_point = Point<float>((hold_x + decay_x) / 2.0f, 1.0f - 2.0f * padY(getSliderDecayValue(0.5f)) / height);
  else if (release_power_hover_)
    grab_point = Point<float>((decay_x + release_x) / 2.0f, 1.0f - 2.0f * padY(getSliderReleaseValue(0.5f)) / height);

  drag_circle_.setColor(findColour(Skin::kWidgetAccent2, true));
  if (mouse_down_) {
    drag_circle_.setQuad(0, grab_point.x - grab_width * 0.5f, grab_point.y - grab_height * 0.5f,
                          grab_width, grab_height);
  }
  else
    drag_circle_.setQuad(0, -2.0f, -2.0f, 0.0f, 0.0f);

  hover_circle_.setColor(findColour(Skin::kWidgetAccent1, true));
  hover_circle_.setQuad(0, grab_point.x - hover_width * 0.5f, grab_point.y - hover_height * 0.5f,
                        hover_width, hover_height);
}

void EnvelopeEditor::setTimePositions() {
  static constexpr float kTimeDisplayBuffer = 0.025f;
  static constexpr float kDrawWidth = 0.1f;

  float powers = logf(window_time_) / logf(kRulerDivisionSize);
  float current_division = floorf(powers);
  float transition = powers - current_division;
  float big_time_chunk = powf(kRulerDivisionSize, current_division) / 2.0f;
  float little_time_chunk = big_time_chunk / kRulerDivisionSize;
  
  float font_height = kTimeDisplaySize * getHeight();
  float font_buffer = kTimeDisplayBuffer * getHeight();
  float font_draw_height = font_height + font_buffer;
  float font_y = getHeight() - font_draw_height;
  float font_draw_width = getWidth() * kDrawWidth;

  float width = getWidth();

  float t = 1.0f - transition;
  Colour lighten = findColour(Skin::kLightenScreen, true);
  Colour big_color = background_color_.overlaidWith(lighten);
  Colour little_color = background_color_.overlaidWith(lighten.withMultipliedAlpha(t * t));
  int index = 1;
  for (; index * little_time_chunk < window_time_ && index < kMaxTimesShown; ++index) {
    if (index % kRulerDivisionSize)
      times_[index]->setColor(little_color);
    else
      times_[index]->setColor(big_color);

    float time = index * little_time_chunk;
    int x = padX(width * time / window_time_);
    String display = formatTime(time);
    times_[index]->setText(display);
    times_[index]->setVisible(true);
    times_[index]->setBounds(x + font_buffer, font_y, font_draw_width, font_draw_height);
    times_[index]->redrawImage(false);
  }
  for (; index < kMaxTimesShown; ++index)
    times_[index]->setVisible(false);
}

void EnvelopeEditor::setGridPositions() {
  float powers = logf(window_time_) / logf(kRulerDivisionSize);
  float current_division = floorf(powers);
  float transition = powers - current_division;
  float big_time_chunk = powf(kRulerDivisionSize, current_division) / 2.0f;
  float little_time_chunk = big_time_chunk / kRulerDivisionSize;

  float width = getWidth();

  float t = 1.0f - transition;
  float line_width = 2.0f / width;
  sub_grid_lines_.setColor(time_color_.withMultipliedAlpha(t * t));
  int sub_index = 0;
  for (int i = 1; i * little_time_chunk < window_time_; ++i) {
    if (i % kRulerDivisionSize) {
      float time = i * little_time_chunk;
      float x = padOpenGlX(2.0f * time / window_time_ - 1.0f);
      sub_grid_lines_.setQuad(sub_index, x, -1.0f, line_width, 2.0f);
      sub_index++;
    }
  }
  sub_grid_lines_.setNumQuads(sub_index);

  int index = 0;
  grid_lines_.setColor(time_color_);
  for (int i = 1; i * big_time_chunk < window_time_; ++i) {
    float time = i * big_time_chunk;
    float x = padOpenGlX(2.0f * time / window_time_ - 1.0f);
    grid_lines_.setQuad(index, x, -1.0f, line_width, 2.0f);
    index++;
  }
  grid_lines_.setNumQuads(index);
}

void EnvelopeEditor::setPointPositions() {
  float width = getWidth();
  float height = getHeight();

  float delay_x = padOpenGlX(getSliderDelayX() * 2.0f / width - 1.0f);
  float attack_x = padOpenGlX(getSliderAttackX() * 2.0f / width - 1.0f);
  float hold_x = padOpenGlX(getSliderHoldX() * 2.0f / width - 1.0f);
  float decay_x = padOpenGlX(getSliderDecayX() * 2.0f / width - 1.0f);
  float sustain_y = padOpenGlY(1.0f - getSliderSustainY() * 2.0f / height);
  float release_x = padOpenGlX(getSliderReleaseX() * 2.0f / width - 1.0f);
  float bottom = padOpenGlY(-1.0f);
  float top = padOpenGlY(1.0f);

  float marker_width = size_ratio_ * 2.0f * kMarkerWidth / width;
  float marker_height = size_ratio_ * 2.0f * kMarkerWidth / height;
  point_circles_.setThickness(size_ratio_ * kMarkerWidth * 0.5f * kRingThickness);
  point_circles_.setQuad(0, attack_x - marker_width * 0.5f, top - marker_height * 0.5f, marker_width, marker_height);
  if (hold_x == attack_x)
    point_circles_.setQuad(1, -2.0f, -2.0f, 0.0f, 0.0f);
  else
    point_circles_.setQuad(1, hold_x - marker_width * 0.5f, top - marker_height * 0.5f, marker_width, marker_height);

  point_circles_.setQuad(2, decay_x - marker_width * 0.5f, sustain_y - marker_height * 0.5f,
                         marker_width, marker_height);
  point_circles_.setQuad(3, release_x - marker_width * 0.5f, bottom - marker_height * 0.5f,
                         marker_width, marker_height);

  float power_width = size_ratio_ * 2.0f * kPowerMarkerWidth / width;
  float power_height = size_ratio_ * 2.0f * kPowerMarkerWidth / height;
  float min_power_distance = kMinPointDistanceForPower * 2.0f / width;
  if (attack_x - delay_x > min_power_distance) {
    float power_attack_x = (delay_x + attack_x) * 0.5;
    float power_attack_y = padOpenGlY(1.0f - getSliderAttackValue(0.5f) * 2.0f / height);
    power_circles_.setQuad(0, power_attack_x - power_width * 0.5f, power_attack_y - power_height * 0.5f,
                           power_width, power_height);
  }
  else 
    power_circles_.setQuad(0, -2.0f, -2.0f, power_width, power_height);

  if (decay_x - hold_x > min_power_distance && sustain_y < top) {
    float power_decay_x = (hold_x + decay_x) * 0.5;
    float power_decay_y = padOpenGlY(1.0f - getSliderDecayValue(0.5f) * 2.0f / height);
    power_circles_.setQuad(1, power_decay_x - power_width * 0.5f, power_decay_y - power_height * 0.5f,
                           power_width, power_height);
  }
  else
    power_circles_.setQuad(1, -2.0f, -2.0f, 0.0f, 0.0f);

  if (release_x - decay_x > min_power_distance && sustain_y > bottom) {
    float power_release_x = (decay_x + release_x) * 0.5;
    float power_release_y = padOpenGlY(1.0f - getSliderReleaseValue(0.5f) * 2.0f / height);
    power_circles_.setQuad(2, power_release_x - power_width * 0.5f, power_release_y - power_height * 0.5f,
                           power_width, power_height);
  }
  else
    power_circles_.setQuad(2, -2.0f, -2.0f, 0.0f, 0.0f);
}

void EnvelopeEditor::setGlPositions() {
  if (!reset_positions_)
    return;

  reset_positions_ = false;

  setEditingCircleBounds();
  setGridPositions();
  setPointPositions();
}

void EnvelopeEditor::setColors() {
  line_left_color_ = findColour(Skin::kWidgetPrimary1, true);
  line_right_color_ = findColour(Skin::kWidgetPrimary2, true);
  line_center_color_ = findColour(Skin::kWidgetCenterLine, true);
  fill_left_color_ = findColour(Skin::kWidgetSecondary1, true);
  fill_right_color_ = findColour(Skin::kWidgetSecondary2, true);
  background_color_ = findColour(Skin::kWidgetBackground, true);
  time_color_ = findColour(Skin::kLightenScreen, true);

  drag_circle_.setColor(findColour(Skin::kWidgetAccent2, true));
  hover_circle_.setColor(findColour(Skin::kWidgetAccent1, true));
}

void EnvelopeEditor::drawPosition(OpenGlWrapper& open_gl, int index) {
  static constexpr float kMinPositionAlphaDecay = 0.9f;
  static constexpr float kCenterFade = 0.2f;
  if (envelope_phase_ == nullptr)
    return;

  std::pair<float, float> position = getPosition(index);
  float current_alpha = current_position_alpha_[index];
  float x = position.first;
  float y = position.second;
  if (y > -1.0f)
    current_position_alpha_.set(index, 1.0f);
  else {
    float release = getOutputsTotal(release_outputs_, release_slider_->getValue())[index];
    release = vital::utils::max(release, 0.0f);
    current_position_alpha_.set(index, current_position_alpha_[index] * std::min(kMinPositionAlphaDecay, release));
  }

  if (current_alpha == 0.0f)
    return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  float current_phase = envelope_phase_->value()[index];
  if (current_phase <= vital::kVoiceKill && current_phase >= vital::kVoiceOn) {
    float width = getWidth();
    float height = getHeight();
    float marker_width = size_ratio_ * 2.0f * kMarkerWidth / width;
    float marker_height = size_ratio_ * 2.0f * kMarkerWidth / height;

    position_circle_.setQuad(0, x - marker_width * 0.5f, y - marker_height * 0.5f, marker_width, marker_height);
  }

  float current_position_alpha = current_position_alpha_[index];
  float mult = std::max(current_position_alpha, 0.0f);
  mult *= mult;
  Colour color;
  if (index)
    color = line_right_color_;
  else
    color = line_left_color_;

  Colour alt_color = color.interpolatedWith(background_color_, kCenterFade);
  position_circle_.setThickness(size_ratio_ * 0.5f * kMarkerWidth * kRingThickness);
  position_circle_.setColor(color.withMultipliedAlpha(mult));
  position_circle_.setAltColor(alt_color.withMultipliedAlpha(mult));
  position_circle_.render(open_gl, true);
}
