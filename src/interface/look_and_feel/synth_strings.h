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

#include <string>
#include "synth_constants.h"

namespace strings {
  const std::string kOffOnNames[] = {
    "Off",
    "On"
  };

  const std::string kWavetableDimensionNames[] = {
    "3D",
    "2D",
    "SP"
  };

  const std::string kOversamplingNames[] = {
    "1x",
    "2x",
    "4x",
    "8x"
  };

  const std::string kDelayStyleNames[] = {
    "Mono",
    "Stereo",
    "Ping Pong",
    "Mid Ping Pong",
  };

  const std::string kCompressorBandNames[] = {
    "Multiband",
    "Low Band",
    "High Band",
    "Single Band"
  };

  const std::string kCompressorBandShortNames[] = {
    "Multiband",
    "Low Band",
    "High Band",
    "Single"
  };

  const std::string kPresetStyleNames[] = {
    "Bass",
    "Lead",
    "Keys",
    "Pad",
    "Percussion",
    "Sequence",
    "Experiment",
    "SFX",
    "Template",
  };

  const std::string kUnisonStackNames[] = {
    "Unison",
    "Center Drop 12",
    "Center Drop 24",
    "Octave",
    "2x Octave",
    "Power Chord",
    "2x Power Chord",
    "Major Chord",
    "Minor Chord",
    "Harmonics",
    "Odd Harmonics",
  };

  const std::string kFilterStyleNames[] = {
    "12dB",
    "24dB",
    "Notch Blend",
    "Notch Spread",
    "B/P/N"
  };

  const std::string kDiodeStyleNames[] = {
    "Low Shelf",
    "Low Cut"
  };

  const std::string kCombStyleNames[] = {
    "Low High Comb",
    "Low High Flange+",
    "Low High Flange-",
    "Band Spread Comb",
    "Band Spread Flange+",
    "Band Spread Flange-"
  };

  const std::string kFrequencySyncNames[] = {
    "Seconds",
    "Tempo",
    "Tempo Dotted",
    "Tempo Triplets",
    "Keytrack"
  };

  const std::string kDistortionTypeShortNames[] = {
    "Soft Clip",
    "Hard Clip",
    "Lin Fold",
    "Sine Fold",
    "Bit Crush",
    "Down Samp",
  };

  const std::string kDistortionTypeNames[] = {
    "Soft Clip",
    "Hard Clip",
    "Linear Fold",
    "Sine Fold",
    "Bit Crush",
    "Down Sample",
  };

  const std::string kDistortionFilterOrderNames[] = {
    "None",
    "Pre",
    "Post",
  };

  const std::string kFilterModelNames[] = {
    "Analog",
    "Dirty",
    "Ladder",
    "Digital",
    "Diode",
    "Formant",
    "Comb",
    "Phaser",
  };

  const std::string kPredefinedWaveformNames[] = {
    "Sin",
    "Saturated Sin",
    "Triangle",
    "Square",
    "Pulse",
    "Saw",
  };

  const std::string kEffectOrder[] = {
    "chorus",
    "compressor",
    "delay",
    "distortion",
    "eq",
    "filter_fx",
    "flanger",
    "phaser",
    "reverb"
  };

  const std::string kSyncedFrequencyNames[] = {
    "Freeze",
    "32/1",
    "16/1",
    "8/1",
    "4/1",
    "2/1",
    "1/1",
    "1/2",
    "1/4",
    "1/8",
    "1/16",
    "1/32",
    "1/64",
  };

  const std::string kStereoModeNames[vital::StereoEncoder::kNumStereoModes] = {
    "SPREAD",
    "ROTATE"
  };

  const std::string kSmoothModeNames[] = {
    "FADE IN",
    "SMOOTH"
  };

  const std::string kSyncShortNames[] = {
    "Trigger",
    "Sync",
    "Envelope",
    "Sustain Env",
    "Loop Point",
    "Loop Hold",
  };

  const std::string kSyncNames[] = {
    "Trigger",
    "Sync",
    "Envelope",
    "Sustain Envelope",
    "Loop Point",
    "Loop Hold",
  };

  const std::string kRandomShortNames[] = {
    "Perlin",
    "S & H",
    "Sine",
    "Lorenz",
  };

  const std::string kRandomNames[] = {
    "Perlin",
    "Sample & Hold",
    "Sine Interpolate",
    "Lorenz Attractor",
  };

  const std::string kPaintPatternNames[] = {
    "Step",
    "Half",
    "Down",
    "Up",
    "Tri"
  };

  const std::string kVoicePriorityNames[] = {
    "Newest",
    "Oldest",
    "Highest",
    "Lowest",
    "Round Robin"
  };

  const std::string kVoiceOverrideNames[] = {
    "Kill",
    "Steal"
  };

  const std::string kEqHighModeNames[] = {
    "Shelf",
    "Low Pass"
  };

  const std::string kEqBandModeNames[] = {
    "Shelf",
    "Notch"
  };

  const std::string kEqLowModeNames[] = {
    "Shelf",
    "High Pass"
  };

  const std::string kDestinationNames[vital::constants::kNumSourceDestinations + vital::constants::kNumEffects] = {
    "FILTER 1",
    "FILTER 2",
    "FILTER 1+2",
    "EFFECTS",
    "DIRECT OUT",
    "CHORUS",
    "COMPRESSOR",
    "DELAY",
    "DISTORTION",
    "EQ",
    "FX FILTER",
    "FLANGER",
    "PHASER",
    "REVERB",
  };

  const std::string kDestinationMenuNames[vital::constants::kNumSourceDestinations + vital::constants::kNumEffects] = {
    "Filter 1",
    "Filter 2",
    "Filter 1+2",
    "Effects",
    "Direct Out",
    "Chours",
    "Compressor",
    "Delay",
    "Distortion",
    "Equalizer",
    "Effects Filter",
    "Flanger",
    "Phaser",
    "Reverb",
  };

  const std::string kPhaseDistortionNames[] = {
    "None",
    "Sync",
    "Formant",
    "Quantize",
    "Bend",
    "Squeeze",
    "Pulse",
    "FM <- Osc",
    "FM <- Osc",
    "FM <- Sample",
    "RM <- Osc",
    "RM <- Osc",
    "RM <- Sample"
  };

  const std::string kSpectralMorphNames[] = {
    "None",
    "Vocode",
    "Formant Scale",
    "Harmonic Stretch",
    "Inharmonic Stretch",
    "Smear",
    "Random Amplitudes",
    "Low Pass",
    "High Pass",
    "Phase Disperse",
    "Shepard Tone",
    "Spectral Time Skew",
  };
} // namespace strings
