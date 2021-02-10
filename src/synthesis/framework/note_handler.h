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

#include "common.h"

namespace vital {

  class NoteHandler {
    public:
      virtual ~NoteHandler() { }
      virtual void allSoundsOff() = 0;
      virtual void allNotesOff(int sample) = 0;
      virtual void allNotesOff(int sample, int channel) = 0;
      virtual void noteOn(int note, mono_float velocity, int sample, int channel) = 0;
      virtual void noteOff(int note, mono_float lift, int sample, int channel) = 0;
  };
} // namespace vital

