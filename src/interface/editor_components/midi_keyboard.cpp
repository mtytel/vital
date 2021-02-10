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

#include "midi_keyboard.h"

const float MidiKeyboard::kBlackKeyOffsets[kNumBlackKeysPerOctave] = {
  1.0f - 0.6f * kBlackKeyWidthRatio,
  2.0f - 0.4f * kBlackKeyWidthRatio,
  4.0f - 0.7f * kBlackKeyWidthRatio,
  5.0f - 0.5f * kBlackKeyWidthRatio,
  6.0f - 0.3f * kBlackKeyWidthRatio,
};

const bool MidiKeyboard::kWhiteKeys[vital::kNotesPerOctave] = {
  true, false, true, false, true, true, false, true, false, true, false, true
};

namespace {
  int getBlackKeyOctaveOffset(int black_key_index) {
    for (int i = 0; i < vital::kNotesPerOctave; ++i) {
      if (!MidiKeyboard::kWhiteKeys[i]) {
        if (black_key_index == 0)
          return i;

        black_key_index--;
      }
    }
    return -1;
  }

  int getWhiteKeyOctaveOffset(int white_key_index) {
    for (int i = 0; i < vital::kNotesPerOctave; ++i) {
      if (MidiKeyboard::kWhiteKeys[i]) {
        if (white_key_index == 0)
          return i;

        white_key_index--;
      }
    }
    return -1;
  }

  int getBlackKeyIndexFromOffset(int note_offset) {
    int black_key_index = 0;
    for (int i = 0; i < note_offset; ++i) {
      if (!MidiKeyboard::kWhiteKeys[i])
        black_key_index++;
    }
    return black_key_index;
  }

  int getWhiteKeyIndexFromOffset(int note_offset) {
    int white_key_index = 0;
    for (int i = 0; i < note_offset; ++i) {
      if (MidiKeyboard::kWhiteKeys[i])
        white_key_index++;
    }
    return white_key_index;
  }
}

MidiKeyboard::MidiKeyboard(MidiKeyboardState& state) :
    OpenGlComponent("keyboard"), state_(state), midi_channel_(1), hover_note_(-1),
    black_notes_(kNumBlackKeys, Shaders::kRoundedRectangleFragment),
    white_pressed_notes_(kNumWhiteKeys, Shaders::kRoundedRectangleFragment),
    black_pressed_notes_(kNumBlackKeys, Shaders::kRoundedRectangleFragment),
    hover_note_quad_(Shaders::kRoundedRectangleFragment) {
  black_notes_.setTargetComponent(this);
  white_pressed_notes_.setTargetComponent(this);
  black_pressed_notes_.setTargetComponent(this);
  hover_note_quad_.setTargetComponent(this);
  hover_note_quad_.setQuad(0, -2.0f, -2.0f, 0.0f, 0.0f);

  int num_children = getNumChildComponents();

  for (int i = 0; i < num_children; ++i) {
    Component* child = getChildComponent(i);
    child->setWantsKeyboardFocus(false);
  }
}

void MidiKeyboard::paintBackground(Graphics& g) {
  float width = getWidth();
  int height = getHeight();
  g.setColour(black_key_color_);
  for (int i = 1; i < kNumWhiteKeys; ++i) {
    int x = i * width / kNumWhiteKeys;
    g.fillRect(x, 0, 1, height);
  }
}

void MidiKeyboard::parentHierarchyChanged() {
  setColors();
}

void MidiKeyboard::setColors() {
  if (findParentComponentOfClass<SynthGuiInterface>() == nullptr)
    return;
  
  key_press_color_ = findColour(Skin::kWidgetPrimary1, true);
  hover_color_ = findColour(Skin::kWidgetAccent2, true);
  white_key_color_ = findColour(Skin::kWidgetSecondary1, true);
  black_key_color_ = findColour(Skin::kWidgetSecondary2, true);
}

void MidiKeyboard::resized() {
  OpenGlComponent::resized();
  setColors();

  float width = getWidth();
  float height = getHeight();
  float black_key_height = 2.0f * ((int)(height * kBlackKeyHeightRatio)) / height;
  float black_key_y = 1.0f - black_key_height;
  float white_key_width = 2.0f / kNumWhiteKeys;
  float black_key_width = (((int)(kBlackKeyWidthRatio * white_key_width * width / 4.0f)) * 4.0f + 2.0f) / width;
  float octave_width = kNumWhiteKeysPerOctave * white_key_width;
  
  for (int i = 0; i < kNumBlackKeys; ++i) {
    int octave = i / kNumBlackKeysPerOctave;
    int index = i % kNumBlackKeysPerOctave;

    float x = -1.0f + octave_width * octave + kBlackKeyOffsets[index] * white_key_width;
    x = ((int)((x + 1.0f) * width / 2.0f)) * 2.0f / width - 1.0f;
    black_notes_.setQuad(i, x, black_key_y, black_key_width, black_key_height + 0.5f);
  }

  float widget_rounding = findValue(Skin::kWidgetRoundedCorner);
  black_notes_.setRounding(widget_rounding);
  hover_note_quad_.setRounding(widget_rounding);
  black_pressed_notes_.setRounding(widget_rounding);
}

int MidiKeyboard::getNoteAtPosition(Point<float> position) {
  float white_key_position = kNumWhiteKeys * position.x / getWidth();
  int octave = white_key_position / kNumWhiteKeysPerOctave;
  float white_key_in_octave = white_key_position - octave * kNumWhiteKeysPerOctave;

  if (isBlackKeyHeight(position)) {
    for (int i = 0; i < kNumBlackKeysPerOctave; ++i) {
      float note_offset = white_key_in_octave - kBlackKeyOffsets[i];
      if (note_offset <= kBlackKeyWidthRatio && note_offset >= 0.0f) {
        int note = octave * vital::kNotesPerOctave + getBlackKeyOctaveOffset(i);
        return std::min(vital::kMidiSize - 1, std::max(note, 0));
      }
    }
  }

  int white_key_index = std::min<int>(kNumWhiteKeysPerOctave - 1, white_key_in_octave);
  int note = octave * vital::kNotesPerOctave + getWhiteKeyOctaveOffset(white_key_index);
  return std::min(vital::kMidiSize - 1, std::max(note, 0));
}

float MidiKeyboard::getVelocityForNote(int midi, Point<float> position) {
  static constexpr float kMinVelocity = 1.0f / (vital::kMidiSize - 1);

  float velocity = position.y / getHeight();
  if (!isWhiteKey(midi))
    velocity = position.y / (kBlackKeyHeightRatio * getHeight());

  return std::max(kMinVelocity, std::min(1.0f, velocity));
}

void MidiKeyboard::render(OpenGlWrapper& open_gl, bool animate) {
  setPressedKeyPositions();

  hover_note_quad_.setColor(hover_color_);
  int hover_note = hover_note_;

  if (hover_note >= 0) {
    int octave = hover_note / vital::kNotesPerOctave;
    int note_offset = hover_note - octave * vital::kNotesPerOctave;
    if (isWhiteKey(hover_note)) {
      int index = octave * kNumWhiteKeysPerOctave + getWhiteKeyIndexFromOffset(note_offset);
      setWhiteKeyQuad(&hover_note_quad_, 0, index);
      hover_note_quad_.render(open_gl, animate);
    }
    else {
      int index = octave * kNumBlackKeysPerOctave + getBlackKeyIndexFromOffset(note_offset);
      setBlackKeyQuad(&hover_note_quad_, 0, index);
    }
  }

  white_pressed_notes_.setColor(key_press_color_);
  white_pressed_notes_.render(open_gl, animate);

  black_notes_.setColor(black_key_color_);
  black_notes_.render(open_gl, animate);

  if (hover_note >= 0 && !isWhiteKey(hover_note))
    hover_note_quad_.render(open_gl, animate);

  black_pressed_notes_.setColor(key_press_color_);
  black_pressed_notes_.render(open_gl, animate);
}

void MidiKeyboard::setPressedKeyPositions() {
  int num_pressed_white_keys = 0;
  int num_pressed_black_keys = 0;
  int white_key_index = 0;
  int black_key_index = 0;
  for (int i = 0; i < vital::kMidiSize; ++i) {
    bool white_key = isWhiteKey(i);
    if (state_.isNoteOnForChannels(0xffff, i)) {
      if (white_key) {
        setWhiteKeyQuad(&white_pressed_notes_, num_pressed_white_keys, white_key_index);
        num_pressed_white_keys++;
      }
      else {
        setBlackKeyQuad(&black_pressed_notes_, num_pressed_black_keys, black_key_index);
        num_pressed_black_keys++;
      }
    }

    if (white_key)
      white_key_index++;
    else
      black_key_index++;
  }

  white_pressed_notes_.setNumQuads(num_pressed_white_keys);
  black_pressed_notes_.setNumQuads(num_pressed_black_keys);
}

void MidiKeyboard::setWhiteKeyQuad(OpenGlMultiQuad* quads, int quad_index, int white_key_index) {
  float full_width = getWidth();
  int start_x = white_key_index * full_width / kNumWhiteKeys + 1;
  int end_x = (white_key_index + 1) * full_width / kNumWhiteKeys;
  float x = 2.0f * start_x / full_width - 1.0f;
  float width = 2.0f * (end_x - start_x) / full_width;

  quads->setQuad(quad_index, x, -2.0f, width, 4.0f);
}

void MidiKeyboard::setBlackKeyQuad(OpenGlMultiQuad* quads, int quad_index, int black_key_index) {
  float x = black_notes_.getQuadX(black_key_index);
  float y = black_notes_.getQuadY(black_key_index);
  float width = black_notes_.getQuadWidth(black_key_index);
  float height = black_notes_.getQuadHeight(black_key_index);
  float border = 2.0f / getWidth();
  float y_adjust = 2.0f / getHeight();

  quads->setQuad(quad_index, x + border, y + y_adjust, width - 2.0f * border, height);
}
