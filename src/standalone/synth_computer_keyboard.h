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

#pragma once

#include "JuceHeader.h"
#include "synth_types.h"

#include <set>

namespace vital {
  class SoundEngine;
}

class SynthComputerKeyboard : public vital::StringLayout, public KeyListener {
  public:
    static constexpr int kKeyboardMidiChannel = 1;

    SynthComputerKeyboard() = delete;
    SynthComputerKeyboard(vital::SoundEngine* synth, MidiKeyboardState* keyboard_state);
    ~SynthComputerKeyboard();

    void changeKeyboardOffset(int new_offset);

    // KeyListener
    bool keyPressed(const KeyPress &key, Component *origin) override;
    bool keyStateChanged(bool isKeyDown, Component *origin) override;

  private:
    vital::SoundEngine* synth_;
    MidiKeyboardState* keyboard_state_;
    std::set<char> keys_pressed_;
    int computer_keyboard_offset_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthComputerKeyboard)
};

