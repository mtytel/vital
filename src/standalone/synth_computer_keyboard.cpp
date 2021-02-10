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

#include "synth_computer_keyboard.h"

#include "synth_constants.h"
#include "sound_engine.h"

SynthComputerKeyboard::SynthComputerKeyboard(vital::SoundEngine* synth,
                                             MidiKeyboardState* keyboard_state) {
  synth_ = synth;
  keyboard_state_ = keyboard_state;
  computer_keyboard_offset_ = vital::kDefaultKeyboardOffset;
  layout_ = vital::kDefaultKeyboard;
  up_key_ = vital::kDefaultKeyboardOctaveUp;
  down_key_ = vital::kDefaultKeyboardOctaveDown;
}

SynthComputerKeyboard::~SynthComputerKeyboard() {
}

void SynthComputerKeyboard::changeKeyboardOffset(int new_offset) {
  for (int i = 0; i < layout_.length(); ++i) {
    int note = computer_keyboard_offset_ + i;
    keyboard_state_->noteOff(kKeyboardMidiChannel, note, 0.5f);
    keys_pressed_.erase(layout_[i]);
  }

  int max = (vital::kMidiSize / vital::kNotesPerOctave - 1) * vital::kNotesPerOctave;
  computer_keyboard_offset_ = vital::utils::iclamp(new_offset, 0, max);
}

bool SynthComputerKeyboard::keyPressed(const KeyPress &key, Component *origin) {
  return false;
}

bool SynthComputerKeyboard::keyStateChanged(bool isKeyDown, Component *origin) {
  bool consumed = false;
  for (int i = 0; i < layout_.length(); ++i) {
    int note = computer_keyboard_offset_ + i;

    ModifierKeys modifiers = ModifierKeys::getCurrentModifiersRealtime();
    if (KeyPress::isKeyCurrentlyDown(layout_[i]) &&
        !keys_pressed_.count(layout_[i]) && isKeyDown && !modifiers.isCommandDown()) {
      keys_pressed_.insert(layout_[i]);
      keyboard_state_->noteOn(kKeyboardMidiChannel, note, 1.0f);
    }
    else if (!KeyPress::isKeyCurrentlyDown(layout_[i]) && keys_pressed_.count(layout_[i])) {
      keys_pressed_.erase(layout_[i]);
      keyboard_state_->noteOff(kKeyboardMidiChannel, note, 0.5f);
    }
    consumed = true;
  }

  if (KeyPress::isKeyCurrentlyDown(down_key_)) {
    if (!keys_pressed_.count(down_key_)) {
      keys_pressed_.insert(down_key_);
      changeKeyboardOffset(computer_keyboard_offset_ - vital::kNotesPerOctave);
      consumed = true;
    }
  }
  else
    keys_pressed_.erase(down_key_);

  if (KeyPress::isKeyCurrentlyDown(up_key_)) {
    if (!keys_pressed_.count(up_key_)) {
      keys_pressed_.insert(up_key_);
      changeKeyboardOffset(computer_keyboard_offset_ + vital::kNotesPerOctave);
      consumed = true;
    }
  }
  else
    keys_pressed_.erase(up_key_);

  if (KeyPress::isKeyCurrentlyDown(KeyPress::spaceKey)) {
    if (!keys_pressed_.count(' ')) {
      keys_pressed_.insert(' ');
      synth_->correctToTime(0.0);
      consumed = true;
    }
  }
  else
    keys_pressed_.erase(' ');

  return consumed;
}
