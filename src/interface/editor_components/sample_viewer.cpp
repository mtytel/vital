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

#include "sample_viewer.h"

#include "skin.h"
#include "synth_gui_interface.h"

SampleViewer::SampleViewer() : OpenGlLineRenderer(kResolution), bottom_(kResolution),
                               dragging_overlay_(Shaders::kColorFragment) {
  addAndMakeVisible(bottom_);
  animate_ = false;
  active_ = true;
  sample_phase_output_ = nullptr;
  last_phase_ = 0.0f;
  sample_ = nullptr;
  dragging_audio_file_ = false;
  addBottomRoundedCorners();

  dragging_overlay_.setTargetComponent(this);
                                 
  setFill(true);
  bottom_.setFill(true);
  setLineWidth(2.0f);
  bottom_.setLineWidth(2.0f);
}

SampleViewer::~SampleViewer() { }

void SampleViewer::audioFileLoaded(const File& file) {
  for (Listener* listener : listeners_)
    listener->sampleLoaded(file);
  
  setLinePositions();
}

void SampleViewer::repaintAudio() {
  dragging_audio_file_ = false;
  setLinePositions();
}

void SampleViewer::setLinePositions() {
  if (sample_ == nullptr)
    return;

  double sample_length = sample_->originalLength();
  const vital::mono_float* buffer = sample_->buffer();
  float center = getHeight() / 2.0f;
  for (int i = 0; i < kResolution; ++i) {
    int start_index = std::min<int>(sample_length * i / kResolution, sample_length);
    int end_index = std::min<int>((sample_length * (i + 1) + kResolution - 1) / kResolution, sample_length);
    float max = buffer[start_index];
    for (int i = start_index + 1; i < end_index; ++i)
      max = std::max(max, buffer[i]);
    setYAt(i, center - max * center);
    bottom_.setYAt(i, center + max * center);
  }
}

std::string SampleViewer::getName() {
  if (sample_)
    return sample_->getName();
  return "";
}

void SampleViewer::resized() {
  bottom_.setBounds(getLocalBounds());
  dragging_overlay_.setColor(findColour(Skin::kOverlayScreen, true));

  for (int i = 0; i < kResolution; ++i) {
    float t = i / (kResolution - 1.0f);
    setXAt(i, getWidth() * t);
    bottom_.setXAt(i, getWidth() * t);
  }

  if (sample_phase_output_ == nullptr) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent)
      sample_phase_output_ = parent->getSynth()->getStatusOutput("sample_phase");
  }

  OpenGlLineRenderer::resized();
  setLinePositions();
}

void SampleViewer::render(OpenGlWrapper& open_gl, bool animate) {
  animate_ = animate;

  float boost_amount = findValue(Skin::kWidgetLineBoost);
  float fill_boost_amount = findValue(Skin::kWidgetFillBoost);
  setBoostAmount(boost_amount);
  bottom_.setBoostAmount(boost_amount);
  setFillBoostAmount(fill_boost_amount);
  bottom_.setFillBoostAmount(fill_boost_amount);
  
  if (sample_ == nullptr)
    return;

  int sample_length = sample_->originalLength();
  if (sample_phase_output_ == nullptr || sample_length == 0)
    return;

  vital::poly_float encoded_phase = sample_phase_output_->value();
  std::pair<vital::poly_float, vital::poly_float> decoded = vital::utils::decodePhaseAndVoice(encoded_phase);
  vital::poly_float phase = decoded.first;
  vital::poly_float voice = decoded.second;

  vital::poly_mask switch_mask = vital::poly_float::notEqual(voice, last_voice_);
  vital::poly_float phase_reset = vital::utils::max(0.0f, phase);
  last_phase_ = vital::utils::maskLoad(last_phase_, phase_reset, switch_mask);

  if (!sample_phase_output_->isClearValue(phase) && vital::poly_float::notEqual(phase, 0.0f).anyMask() != 0) {
    vital::poly_float phase_delta = vital::poly_float::abs(phase - last_phase_);
    vital::poly_float decay = vital::poly_float(1.0f) - phase_delta * kSpeedDecayMult;
    decay = vital::utils::clamp(decay, kBoostDecay, 1.0f);
    decayBoosts(decay);
    bottom_.decayBoosts(decay);

    if (animate_) {
      boostRange(last_phase_, phase, 0, decay);
      bottom_.boostRange(last_phase_, phase, 0, decay);
    }
  }
  else {
    decayBoosts(kBoostDecay);
    bottom_.decayBoosts(kBoostDecay);
  }

  last_phase_ = phase;
  last_voice_ = voice;

  float fill_fade = 0.0f;
  if (parent_)
    fill_fade = parent_->findValue(Skin::kWidgetFillFade);

  Colour line, fill;
  if (isActive()) {
    line = findColour(Skin::kWidgetPrimary2, true);
    fill = findColour(Skin::kWidgetSecondary2, true);
  }
  else {
    line = findColour(Skin::kWidgetPrimaryDisabled, true);
    fill = findColour(Skin::kWidgetSecondaryDisabled, true);
  }

  setColor(line);
  bottom_.setColor(line);
  setFillColors(fill.withMultipliedAlpha(1.0f - fill_fade), fill);
  bottom_.setFillColors(fill.withMultipliedAlpha(1.0f - fill_fade), fill);
  
  drawLines(open_gl, false);
  bottom_.drawLines(open_gl, false);

  if (isActive()) {
    line = findColour(Skin::kWidgetPrimary1, true);
    fill = findColour(Skin::kWidgetSecondary1, true);
  }
  else {
    line = findColour(Skin::kWidgetPrimaryDisabled, true);
    fill = findColour(Skin::kWidgetSecondaryDisabled, true);
  }
  
  setColor(line);
  bottom_.setColor(line);
  setFillColors(fill.withMultipliedAlpha(1.0f - fill_fade), fill);
  bottom_.setFillColors(fill.withMultipliedAlpha(1.0f - fill_fade), fill);
  
  drawLines(open_gl, anyBoostValue());
  bottom_.drawLines(open_gl, anyBoostValue());

  if (dragging_audio_file_)
    dragging_overlay_.render(open_gl, animate);
  renderCorners(open_gl, animate);
}
