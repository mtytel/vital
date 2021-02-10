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

// Debugging.
#if DEBUG
#include <cassert>
#define VITAL_ASSERT(x) assert(x)
#else
#define VITAL_ASSERT(x) ((void)0)
#endif // DEBUG

#define UNUSED(x) ((void)x)

#if !defined(force_inline)
#if defined (_MSC_VER)
  #define force_inline __forceinline
  #define vector_call __vectorcall
#else
  #define force_inline inline __attribute__((always_inline))
  #define vector_call
#endif
#endif

#include "poly_values.h"

namespace vital {

  typedef float mono_float;

  constexpr mono_float kPi = 3.1415926535897932384626433832795f;
  constexpr mono_float kSqrt2 = 1.414213562373095048801688724209698f;
  constexpr mono_float kEpsilon = 1e-16f;
  constexpr int kMaxBufferSize = 128;
  constexpr int kMaxOversample = 8;
  constexpr int kDefaultSampleRate = 44100;
  constexpr mono_float kMinNyquistMult = 0.45351473923f;
  constexpr int kMaxSampleRate = 192000;
  constexpr int kMidiSize = 128;
  constexpr int kMidiTrackCenter = 60;

  constexpr mono_float kMidi0Frequency = 8.1757989156f;
  constexpr mono_float kDbfsIncrease = 6.0f;
  constexpr int kDegreesPerCycle = 360;
  constexpr int kMsPerSec = 1000;
  constexpr int kNotesPerOctave = 12;
  constexpr int kCentsPerNote = 100;
  constexpr int kCentsPerOctave = kNotesPerOctave * kCentsPerNote;

  constexpr int kPpq = 960; // Pulses per quarter note.
  constexpr mono_float kVoiceKillTime = 0.05f;
  constexpr int kNumMidiChannels = 16;
  constexpr int kFirstMidiChannel = 0;
  constexpr int kLastMidiChannel = kNumMidiChannels - 1;

  enum VoiceEvent {
    kInvalid,
    kVoiceIdle,
    kVoiceOn,
    kVoiceHold,
    kVoiceDecay,
    kVoiceOff,
    kVoiceKill,
    kNumVoiceEvents
  };
} // namespace vital

