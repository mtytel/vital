/* Copyright 2013-2019 Matt Tytel
 *
 * pylon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pylon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with pylon.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synth_editor.h"
#include "default_look_and_feel.h"
#include "synth_plugin.h"
#include "load_save.h"

SynthEditor::SynthEditor(SynthPlugin& synth) :
    AudioProcessorEditor(&synth), SynthGuiInterface(&synth), synth_(synth), was_animating_(true) {
  static constexpr int kHeightBuffer = 50;
  
  setLookAndFeel(DefaultLookAndFeel::instance());

  gui_->reset();
  gui_->setOscilloscopeMemory(synth.getOscilloscopeMemory());
  gui_->setAudioMemory(synth.getAudioMemory());
  gui_->animate(LoadSave::shouldAnimateWidgets());

  constrainer_.setMinimumSize(vital::kMinWindowWidth, vital::kMinWindowHeight);
  double ratio = (1.0 * vital::kDefaultWindowWidth) / vital::kDefaultWindowHeight;
  constrainer_.setFixedAspectRatio(ratio);
  constrainer_.setGui(gui_.get());
  setConstrainer(&constrainer_);

  Rectangle<int> total_bounds = Desktop::getInstance().getDisplays().getTotalBounds(true);
  total_bounds.removeFromBottom(kHeightBuffer);

  addAndMakeVisible(gui_.get());
  float window_size = LoadSave::loadWindowSize();
  window_size = std::min(window_size, total_bounds.getWidth() / (1.0f * vital::kDefaultWindowWidth));
  window_size = std::min(window_size, total_bounds.getHeight() / (1.0f * vital::kDefaultWindowHeight));
  int width = std::round(window_size * vital::kDefaultWindowWidth);
  int height = std::round(window_size * vital::kDefaultWindowHeight);
  setResizable(true, true);
  setSize(width, height);
}

void SynthEditor::resized() {
  AudioProcessorEditor::resized();
  gui_->setBounds(getLocalBounds());
}

void SynthEditor::setScaleFactor(float newScale) {
  AudioProcessorEditor::setScaleFactor(newScale);
  gui_->redoBackground();
}

void SynthEditor::updateFullGui() {
  SynthGuiInterface::updateFullGui();
  synth_.updateHostDisplay();
}
