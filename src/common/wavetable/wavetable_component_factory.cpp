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

#include "wavetable_component_factory.h"
#include "file_source.h"
#include "frequency_filter_modifier.h"
#include "phase_modifier.h"
#include "shepard_tone_source.h"
#include "slew_limit_modifier.h"
#include "wave_frame.h"
#include "wave_fold_modifier.h"
#include "wave_line_source.h"
#include "wave_source.h"
#include "wave_warp_modifier.h"
#include "wave_window_modifier.h"

WavetableComponent* WavetableComponentFactory::createComponent(ComponentType type) {
  switch (type) {
    case kWaveSource:
      return new WaveSource();
    case kLineSource:
      return new WaveLineSource();
    case kFileSource:
      return new FileSource();
    case kShepardToneSource:
      return new ShepardToneSource();
    case kPhaseModifier:
      return new PhaseModifier();
    case kWaveWindow:
      return new WaveWindowModifier();
    case kFrequencyFilter:
      return new FrequencyFilterModifier();
    case kSlewLimiter:
      return new SlewLimitModifier();
    case kWaveFolder:
      return new WaveFoldModifier();
    case kWaveWarp:
      return new WaveWarpModifier();
    default:
      VITAL_ASSERT(false);
      return nullptr;
  }
}

WavetableComponent* WavetableComponentFactory::createComponent(const std::string& type) {
  if (type == "Wave Source")
    return new WaveSource();
  if (type == "Line Source")
    return new WaveLineSource();
  if (type == "Audio File Source")
    return new FileSource();
  if (type == "Shepard Tone Source")
    return new ShepardToneSource();
  if (type == "Phase Shift")
    return new PhaseModifier();
  if (type == "Wave Window")
    return new WaveWindowModifier();
  if (type == "Frequency Filter")
    return new FrequencyFilterModifier();
  if (type == "Slew Limiter")
    return new SlewLimitModifier();
  if (type == "Wave Folder")
    return new WaveFoldModifier();
  if (type == "Wave Warp")
    return new WaveWarpModifier();

  VITAL_ASSERT(false);
  return nullptr;
}

std::string WavetableComponentFactory::getComponentName(ComponentType type) {
  switch (type) {
    case kWaveSource:
      return "Wave Source";
    case kLineSource:
      return "Line Source";
    case kFileSource:
      return "Audio File Source";
    case kShepardToneSource:
      return "Shepard Tone Source";
    case kPhaseModifier:
      return "Phase Shift";
    case kWaveWindow:
      return "Wave Window";
    case kFrequencyFilter:
      return "Frequency Filter";
    case kSlewLimiter:
      return "Slew Limiter";
    case kWaveFolder:
      return "Wave Folder";
    case kWaveWarp:
      return "Wave Warp";
    default:
      return "";
  }
}
