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

#include "wavetable_overlay_factory.h"
#include "file_source_overlay.h"
#include "frequency_filter_overlay.h"
#include "phase_modifier_overlay.h"
#include "shepard_tone_source.h"
#include "slew_limiter_overlay.h"
#include "wave_fold_overlay.h"
#include "wave_line_source_overlay.h"
#include "wave_source_overlay.h"
#include "wave_warp_overlay.h"
#include "wave_window_overlay.h"

WavetableComponentOverlay* WavetableOverlayFactory::createOverlay(
    WavetableComponentFactory::ComponentType component_type) {
  switch (component_type) {
    case WavetableComponentFactory::kWaveSource:
      return new WaveSourceOverlay();
    case WavetableComponentFactory::kLineSource:
      return new WaveLineSourceOverlay();
    case WavetableComponentFactory::kFileSource:
      return new FileSourceOverlay();
    case WavetableComponentFactory::kShepardToneSource:
      return new WaveSourceOverlay();
    case WavetableComponentFactory::kPhaseModifier:
      return new PhaseModifierOverlay();
    case WavetableComponentFactory::kWaveWindow:
      return new WaveWindowOverlay();
    case WavetableComponentFactory::kFrequencyFilter:
      return new FrequencyFilterOverlay();
    case WavetableComponentFactory::kSlewLimiter:
      return new SlewLimiterOverlay();
    case WavetableComponentFactory::kWaveFolder:
      return new WaveFoldOverlay();
    case WavetableComponentFactory::kWaveWarp:
      return new WaveWarpOverlay();
    default:
      VITAL_ASSERT(false);
      return nullptr;
  }
}

void WavetableOverlayFactory::setOverlayOwner(WavetableComponentOverlay* overlay, WavetableComponent* owner) {
  WavetableComponentFactory::ComponentType component_type;
  if (owner)
    component_type = owner->getType();
  else if (overlay->getComponent())
    component_type = overlay->getComponent()->getType();
  else
    return;

  switch (component_type) {
    case WavetableComponentFactory::kWaveSource: {
      WaveSource* wave_source = dynamic_cast<WaveSource*>(owner);
      dynamic_cast<WaveSourceOverlay*>(overlay)->setWaveSource(wave_source);
      break;
    }
    case WavetableComponentFactory::kLineSource: {
      WaveLineSource* line_source = dynamic_cast<WaveLineSource*>(owner);
      dynamic_cast<WaveLineSourceOverlay*>(overlay)->setLineSource(line_source);
      break;
    }
    case WavetableComponentFactory::kFileSource: {
      FileSource* file_source = dynamic_cast<FileSource*>(owner);
      dynamic_cast<FileSourceOverlay*>(overlay)->setFileSource(file_source);
      break;
    }
    case WavetableComponentFactory::kShepardToneSource: {
      ShepardToneSource* shepard_tone_source = dynamic_cast<ShepardToneSource*>(owner);
      dynamic_cast<WaveSourceOverlay*>(overlay)->setWaveSource(shepard_tone_source);
      break;
    }
    case WavetableComponentFactory::kPhaseModifier: {
      PhaseModifier* phase_modifier = dynamic_cast<PhaseModifier*>(owner);
      dynamic_cast<PhaseModifierOverlay*>(overlay)->setPhaseModifier(phase_modifier);
      break;
    }
    case WavetableComponentFactory::kWaveWindow: {
      WaveWindowModifier* wave_window_modifier = dynamic_cast<WaveWindowModifier*>(owner);
      dynamic_cast<WaveWindowOverlay*>(overlay)->setWaveWindowModifier(wave_window_modifier);
      break;
    }
    case WavetableComponentFactory::kFrequencyFilter: {
      FrequencyFilterModifier* filter_modifier = dynamic_cast<FrequencyFilterModifier*>(owner);
      dynamic_cast<FrequencyFilterOverlay*>(overlay)->setFilterModifier(filter_modifier);
      break;
    }
    case WavetableComponentFactory::kSlewLimiter: {
      SlewLimitModifier* slew_modifier = dynamic_cast<SlewLimitModifier*>(owner);
      dynamic_cast<SlewLimiterOverlay*>(overlay)->setSlewLimitModifier(slew_modifier);
      break;
    }
    case WavetableComponentFactory::kWaveFolder: {
      WaveFoldModifier* wave_fold_modifier = dynamic_cast<WaveFoldModifier*>(owner);
      dynamic_cast<WaveFoldOverlay*>(overlay)->setWaveFoldModifier(wave_fold_modifier);
      break;
    }
    case WavetableComponentFactory::kWaveWarp: {
      WaveWarpModifier* wave_warp_modifier = dynamic_cast<WaveWarpModifier*>(owner);
      dynamic_cast<WaveWarpOverlay*>(overlay)->setWaveWarpModifier(wave_warp_modifier);
      break;
    }
    default:
      VITAL_ASSERT(false);
  }
}
