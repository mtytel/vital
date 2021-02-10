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

#include "synth_parameters.h"

#include "compressor.h"
#include "distortion.h"
#include "digital_svf.h"
#include "synth_constants.h"
#include "random_lfo.h"
#include "synth_lfo.h"
#include "synth_oscillator.h"
#include "synth_strings.h"
#include "voice_handler.h"
#include "wavetable.h"
#include "utils.h"

#include <cfloat>

namespace vital {

  bool compareValueDetails(const ValueDetails* a, const ValueDetails* b) {
    if (a->version_added != b->version_added)
      return a->version_added < b->version_added;
    
    return a->name.compare(b->name) < 0;
  }

  using namespace constants;
  static const std::string kIdDelimiter = "_";
  static const std::string kEnvIdPrefix = "env";
  static const std::string kLfoIdPrefix = "lfo";
  static const std::string kRandomIdPrefix = "random";
  static const std::string kOscIdPrefix = "osc";
  static const std::string kFilterIdPrefix = "filter";
  static const std::string kModulationIdPrefix = "modulation";
  static const std::string kNameDelimiter = " ";
  static const std::string kEnvNamePrefix = "Envelope";
  static const std::string kLfoNamePrefix = "LFO";
  static const std::string kRandomNamePrefix = "Random LFO";
  static const std::string kOscNamePrefix = "Oscillator";
  static const std::string kFilterNamePrefix = "Filter";
  static const std::string kModulationNamePrefix = "Modulation";

  const ValueDetails ValueDetailsLookup::parameter_list[] = {
    { "bypass", 0x000702, 0.0, 1.0, 0.0, 0.0, 60.0,
      ValueDetails::kIndexed, false, "", "Bypass", nullptr },
    { "beats_per_minute", 0x000000, 0.333333333, 5.0, 2.0, 0.0, 60.0,
      ValueDetails::kLinear, false, "", "Beats Per Minute", nullptr },
    { "delay_dry_wet", 0x000000, 0.0, 1.0, 0.3334, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Delay Mix", nullptr },
    { "delay_feedback", 0x000000, -1.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Delay Feedback", nullptr },
    { "delay_frequency", 0x000000, -2.0, 9.0, 2.0, 0.0, 1.0,
      ValueDetails::kExponential, true, " secs", "Delay Frequency", nullptr },
    { "delay_aux_frequency", 0x000507, -2.0, 9.0, 2.0, 0.0, 1.0,
      ValueDetails::kExponential, true, " secs", "Delay Frequency 2", nullptr },
    { "delay_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Delay Switch", strings::kOffOnNames },
    { "delay_style", 0x000000, 0.0, 3.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Delay Style", strings::kDelayStyleNames },
    { "delay_filter_cutoff", 0x000000, 8.0, 136.0, 60.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Delay Filter Cutoff", nullptr },
    { "delay_filter_spread", 0x000000, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Delay Filter Spread", nullptr },
    { "delay_sync", 0x000000, 0.0, 3.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Delay Sync", strings::kFrequencySyncNames },
    { "delay_tempo", 0x000000, 4.0, 12.0, 9.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Delay Tempo", strings::kSyncedFrequencyNames },
    { "delay_aux_sync", 0x000507, 0.0, 3.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Delay Sync 2", strings::kFrequencySyncNames },
    { "delay_aux_tempo", 0x000507, 4.0, 12.0, 9.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Delay Tempo 2", strings::kSyncedFrequencyNames },
    { "distortion_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Distortion Switch", strings::kOffOnNames },
    { "distortion_type", 0x000000, 0.0, 5.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Distortion Type", strings::kDistortionTypeNames },
    { "distortion_drive", 0x000000, Distortion::kMinDrive, Distortion::kMaxDrive, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Distortion Drive", nullptr },
    { "distortion_mix", 0x000000, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Distortion Mix", nullptr },
    { "distortion_filter_order", 0x000000, 0.0, 2.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Distortion Filter Order", strings::kDistortionFilterOrderNames },
    { "distortion_filter_cutoff", 0x000000, 8.0, 136.0, 80.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Distortion Filter Cutoff", nullptr },
    { "distortion_filter_resonance", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Distortion Filter Resonance", nullptr },
    { "distortion_filter_blend", 0x000000, 0.0, 2.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Distortion Filter Blend", nullptr },
    { "legato", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Legato", strings::kOffOnNames },
    { "macro_control_1", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Macro 1", nullptr },
    { "macro_control_2", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Macro 2", nullptr },
    { "macro_control_3", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Macro 3", nullptr },
    { "macro_control_4", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Macro 4", nullptr },
    { "pitch_bend_range", 0x000000, 0.0, 48.0, 2.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, " semitones", "Pitch Bend Range", nullptr },
    { "polyphony", 0x000000, 1.0, kMaxPolyphony - 1, 8.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, " voices", "Polyphony", nullptr },
    { "voice_tune", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, " cents", "Voice Tune", nullptr },
    { "voice_transpose", 0x000604, -48.0, 48.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Voice Transpose", nullptr },
    { "voice_amplitude", 0x000000, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Voice Amplitude", nullptr },
    { "stereo_routing", 0x000000, 0.0, 1.0, 1.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Stereo Routing", nullptr },
    { "stereo_mode", 0x000605, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Stereo Mode", strings::kStereoModeNames },
    { "portamento_time", 0x000000, -10.0, 4.0, -10.0, 0.0, 1.0,
      ValueDetails::kExponential, false, " secs", "Portamento Time", nullptr },
    { "portamento_slope", 0x000000, -8.0, 8.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Portamento Slope", nullptr },
    { "portamento_force", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Portamento Force", strings::kOffOnNames },
    { "portamento_scale", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Portamento Scale", strings::kOffOnNames },
    { "reverb_pre_low_cutoff", 0x000000, 0.0, 128.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Reverb Pre Low Cutoff", nullptr },
    { "reverb_pre_high_cutoff", 0x000000, 0.0, 128.0, 110.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Reverb Pre High Cutoff", nullptr },
    { "reverb_low_shelf_cutoff", 0x000000, 0.0, 128.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Reverb Low Cutoff", nullptr },
    { "reverb_low_shelf_gain", 0x000000, -6.0, 0.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Reverb Low Gain", nullptr },
    { "reverb_high_shelf_cutoff", 0x000000, 0.0, 128.0, 90.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Reverb High Cutoff", nullptr },
    { "reverb_high_shelf_gain", 0x000000, -6.0, 0.0, -1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Reverb High Gain", nullptr },
    { "reverb_dry_wet", 0x000000, 0.0, 1.0, 0.25, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Reverb Mix", nullptr },
    { "reverb_delay", 0x000609, 0.0, 0.3, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " secs", "Reverb Delay", nullptr },
    { "reverb_decay_time", 0x000000, -6.0, 6.0, 0.0, 0.0, 1.0,
      ValueDetails::kExponential, false, " secs", "Reverb Decay Time", nullptr },
    { "reverb_size", 0x000506, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Reverb Size", nullptr },
    { "reverb_chorus_amount", 0x000000, 0.0, 1.0, 0.223607, 0.0, 100.0,
      ValueDetails::kQuadratic, false, "%", "Reverb Chorus Amount", nullptr },
    { "reverb_chorus_frequency", 0x000000, -8.0, 3.0, -2.0, 0.0, 1.0,
      ValueDetails::kExponential, false, " Hz", "Reverb Chorus Frequency", nullptr },
    { "reverb_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Reverb Switch", strings::kOffOnNames },
    { "sub_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sub Switch", strings::kOffOnNames },
    { "sub_direct_out", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sub Direct Out", nullptr },
    { "sub_transpose", 0x000000, -48.0, 48.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sub Transpose", nullptr },
    { "sub_transpose_quantize", 0x000000, 0.0, 8191.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sub Transpose Quantize", nullptr },
    { "sub_tune", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "", "Sub Tune", nullptr },
    { "sub_level", 0x000000, 0.0, 1.0, 0.70710678119, 0.0, 1.0,
      ValueDetails::kQuadratic, false, "", "Sub Level", nullptr },
    { "sub_pan", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Sub Pan", nullptr },
    { "sub_waveform", 0x000000, 0.0, PredefinedWaveFrames::kNumShapes - 1, 2.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sub Osc Waveform", nullptr },
    { "sample_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sample Switch", strings::kOffOnNames },
    { "sample_random_phase", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sample Random Phase", strings::kOffOnNames },
    { "sample_keytrack", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sample Keytrack", strings::kOffOnNames },
    { "sample_loop", 0x000000, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sample Loop", strings::kOffOnNames },
    { "sample_bounce", 0x000603, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sample Bounce", strings::kOffOnNames },
    { "sample_transpose", 0x000000, -48.0, 48.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sample Transpose", nullptr },
    { "sample_transpose_quantize", 0x000000, 0.0, 8191.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sample Transpose Quantize", nullptr },
    { "sample_tune", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "", "Sample Tune", nullptr },
    { "sample_level", 0x000000, 0.0, 1.0, 0.70710678119, 0.0, 1.0,
      ValueDetails::kQuadratic, false, "", "Sample Level", nullptr },
    { "sample_destination", 0x000500, 0.0, constants::kNumSourceDestinations + constants::kNumEffects, 3.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sample Destination", strings::kDestinationNames },
    { "sample_pan", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Sample Pan", nullptr },
    { "velocity_track", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Velocity Track", nullptr },
    { "volume", 0x000000, 0.0, 7399.4404, 5473.0404, -80, 1.0,
      ValueDetails::kSquareRoot, false, "dB", "Volume", nullptr },
    { "phaser_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Phaser Switch", strings::kOffOnNames },
    { "phaser_dry_wet", 0x000000, 0.0, 1.0, 1.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Phaser Mix", nullptr },
    { "phaser_feedback", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Phaser Feedback", nullptr },
    { "phaser_frequency", 0x000000, -5.0, 2.0, -3.0, 0.0, 1.0,
      ValueDetails::kExponential, true, " secs", "Phaser Frequency", nullptr },
    { "phaser_sync", 0x000000, 0.0, 3.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Phaser Sync",strings::kFrequencySyncNames },
    { "phaser_tempo", 0x000000, 0.0, 10.0, 3.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Phaser Tempo", strings::kSyncedFrequencyNames },
    { "phaser_center", 0x000000, 8.0, 136.0, 80.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Phaser Center", nullptr },
    { "phaser_blend", 0x000509, 0.0, 2.0, 1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Phaser Blend", nullptr },
    { "phaser_mod_depth", 0x000000, 0.0, 48.0, 24.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Phaser Mod Depth", nullptr },
    { "phaser_phase_offset", 0x000000, 0, 1.0, 0.33333333, 0.0, kDegreesPerCycle,
      ValueDetails::kLinear, false, "", "Phaser Phase Offset", nullptr },
    { "flanger_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Flanger Switch", strings::kOffOnNames },
    { "flanger_dry_wet", 0x000000, 0.0, 0.5, 0.5, 0.0, 200.0,
      ValueDetails::kLinear, false, "%", "Flanger Mix", nullptr },
    { "flanger_feedback", 0x000000, -1.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Flanger Feedback", nullptr },
    { "flanger_frequency", 0x000000, -5.0, 2.0, 2.0, 0.0, 1.0,
      ValueDetails::kExponential, true, " secs", "Flanger Frequency", nullptr },
    { "flanger_sync", 0x000000, 0.0, 3.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Flanger Sync", strings::kFrequencySyncNames },
    { "flanger_tempo", 0x000000, 0.0, 10.0, 4.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Flanger Tempo", strings::kSyncedFrequencyNames },
    { "flanger_center", 0x000505, 8.0, 136.0, 64.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Flanger Center", nullptr },
    { "flanger_mod_depth", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Flanger Mod Depth", nullptr },
    { "flanger_phase_offset", 0x000000, 0, 1.0, 0.33333333, 0.0, kDegreesPerCycle,
      ValueDetails::kLinear, false, "", "Flanger Phase Offset", nullptr },
    { "chorus_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Chorus Switch", strings::kOffOnNames },
    { "chorus_dry_wet", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Chorus Mix", nullptr },
    { "chorus_feedback", 0x000000, -0.95, 0.95, 0.4, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Chorus Feedback", nullptr },
    { "chorus_cutoff", 0x000000, 8.0, 136.0, 60.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Chorus Filter Cutoff", nullptr },
    { "chorus_spread", 0x000607, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Chorus Filter Spread", nullptr },
    { "chorus_voices", 0x000508, 1.0, 4.0, 4.0, 0.0, 4.0,
      ValueDetails::kIndexed, false, "", "Chorus Voices", nullptr },
    { "chorus_frequency", 0x000000, -6.0, 3.0, -3.0, 0.0, 1.0,
      ValueDetails::kExponential, true, " secs", "Chorus Frequency", nullptr },
    { "chorus_sync", 0x000000, 0.0, 3.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Chorus Sync", strings::kFrequencySyncNames },
    { "chorus_tempo", 0x000000, 0.0, 10.0, 4.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Chorus Tempo", strings::kSyncedFrequencyNames },
    { "chorus_mod_depth", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Chorus Mod Depth", nullptr },
    { "chorus_delay_1", 0x000000, -10.0, -5.64386, -9.0, 0.0, 1000.0,
      ValueDetails::kExponential, false, "ms", "Chorus Delay 1", nullptr },
    { "chorus_delay_2", 0x000000, -10.0, -5.64386, -7.0, 0.0, 1000.0,
      ValueDetails::kExponential, false, " ms", "Chorus Delay 2", nullptr },
    { "compressor_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Compressor Switch", strings::kOffOnNames },
    { "compressor_low_upper_threshold", 0x000000, -80.0, 0.0, -28.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Low Upper Threshold", nullptr },
    { "compressor_band_upper_threshold", 0x000000, -80.0, 0.0, -25.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Band Upper Threshold", nullptr },
    { "compressor_high_upper_threshold", 0x000000, -80.0, 0.0, -30.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "High Upper Threshold", nullptr },
    { "compressor_low_lower_threshold", 0x000000, -80.0, 0.0, -35.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Low Lower Threshold", nullptr },
    { "compressor_band_lower_threshold", 0x000000, -80.0, 0.0, -36.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Band Lower Threshold", nullptr },
    { "compressor_high_lower_threshold", 0x000000, -80.0, 0.0, -35.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "High Lower Threshold", nullptr },
    { "compressor_low_upper_ratio", 0x000000, 0.0, 1.0, 0.9, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Low Upper Ratio", nullptr },
    { "compressor_band_upper_ratio", 0x000000, 0.0, 1.0, 0.857, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Band Upper Ratio", nullptr },
    { "compressor_high_upper_ratio", 0x000000, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "High Upper Ratio", nullptr },
    { "compressor_low_lower_ratio", 0x000000, -1.0, 1.0, 0.8, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Low Lower Ratio", nullptr },
    { "compressor_band_lower_ratio", 0x000000, -1.0, 1.0, 0.8, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Band Lower Ratio", nullptr },
    { "compressor_high_lower_ratio", 0x000000, -1.0, 1.0, 0.8, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "High Lower Ratio", nullptr },
    { "compressor_low_gain", 0x000000, -30.0, 30.0, 16.3, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Compressor Low Gain", nullptr },
    { "compressor_band_gain", 0x000000, -30.0, 30.0, 11.7, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Compressor Band Gain", nullptr },
    { "compressor_high_gain", 0x000000, -30.0, 30.0, 16.3, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "Compressor High Gain", nullptr },
    { "compressor_attack", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Compressor Attack", nullptr },
    { "compressor_release", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Compressor Release", nullptr },
    { "compressor_enabled_bands", 0x000000, 0.0, vital::MultibandCompressor::kNumBandOptions - 1, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Compressor Enabled Bands", strings::kCompressorBandNames },
    { "compressor_mix", 0x000602, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Compressor Mix", nullptr },
    { "compressor_low_band_unused", 0x000000, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Compressor Unused", nullptr },
    { "eq_on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "EQ Switch", strings::kOffOnNames },
    { "eq_low_mode", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "EQ Low Mode", strings::kEqLowModeNames },
    { "eq_low_cutoff", 0x000000, 8.0, 136.0, 40.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "EQ Low Cutoff", nullptr },
    { "eq_low_gain", 0x000000, DigitalSvf::kMinGain, DigitalSvf::kMaxGain, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "EQ Low Gain", nullptr },
    { "eq_low_resonance", 0x000000, 0.0, 1.0, 0.3163, 0.0, 100.0,
      ValueDetails::kQuadratic, false, "%", "EQ Low Resonance", nullptr },
    { "eq_band_mode", 0x000506, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "EQ Band Mode", strings::kEqBandModeNames },
    { "eq_band_cutoff", 0x000000, 8.0, 136.0, 80.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "EQ Band Cutoff", nullptr },
    { "eq_band_gain", 0x000000, DigitalSvf::kMinGain, DigitalSvf::kMaxGain, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "EQ Band Gain", nullptr },
    { "eq_band_resonance", 0x000000, 0.0, 1.0, 0.4473, 0.0, 100.0,
      ValueDetails::kQuadratic, false, "", "EQ Band Resonance", nullptr },
    { "eq_high_mode", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "EQ High Mode", strings::kEqHighModeNames },
    { "eq_high_cutoff", 0x000000, 8.0, 136.0, 100.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "EQ High Cutoff", nullptr },
    { "eq_high_gain", 0x000000, DigitalSvf::kMinGain, DigitalSvf::kMaxGain, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " dB", "EQ High Gain", nullptr },
    { "eq_high_resonance", 0x000000, 0.0, 1.0, 0.3163, 0.0, 100.0,
      ValueDetails::kQuadratic, false, "", "EQ High Resonance", nullptr },
    { "effect_chain_order", 0x000000, 0.0, vital::utils::factorial(vital::kNumEffects) - 1, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Effect Chain Order", nullptr },
    { "voice_priority", 0x000000, 0.0, VoiceHandler::kNumVoicePriorities - 1, VoiceHandler::kRoundRobin, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Voice Priority", strings::kVoicePriorityNames },
    { "voice_override", 0x000700, 0.0, VoiceHandler::kNumVoiceOverrides - 1, VoiceHandler::kKill, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Voice Override", strings::kVoiceOverrideNames },
    { "oversampling", 0x000000, 0.0, 3.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Oversampling", strings::kOversamplingNames },
    { "pitch_wheel", 0x000400, -1.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Pitch Wheel", nullptr },
    { "mod_wheel", 0x000400, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Mod Wheel", nullptr },
    { "mpe_enabled", 0x000501, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "MPE Enabled", strings::kOffOnNames },
    { "view_spectrogram", 0x000803, 0.0, 2.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "View Spectrogram", strings::kOffOnNames },
  };

  const ValueDetails ValueDetailsLookup::env_parameter_list[] = {
    { "delay", 0x000503, 0.0, 1.4142135624, 0.0, 0.0, 1.0,
      ValueDetails::kQuartic, false, " secs", "Delay", nullptr },
    { "attack", 0x000000, 0.0, 2.37842, 0.1495, 0.0, 1.0,
      ValueDetails::kQuartic, false, " secs", "Attack", nullptr },
    { "hold", 0x000504, 0.0, 1.4142135624, 0.0, 0.0, 1.0,
      ValueDetails::kQuartic, false, " secs", "Hold", nullptr },
    { "decay", 0x000000, 0.0, 2.37842, 1.0, 0.0, 1.0,
      ValueDetails::kQuartic, false, " secs", "Decay", nullptr },
    { "release", 0x000000, 0.0, 2.37842, 0.5476, 0.0, 1.0,
      ValueDetails::kQuartic, false, " secs", "Release", nullptr },
    { "attack_power", 0x000000, -20.0, 20.0, 0.0f, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Attack Power", nullptr },
    { "decay_power", 0x000000, -20.0, 20.0, -2.0f, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Decay Power", nullptr },
    { "release_power", 0x000000, -20.0, 20.0, -2.0f, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Release Power", nullptr },
    { "sustain", 0x000000, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Sustain", nullptr },
  };

  const ValueDetails ValueDetailsLookup::lfo_parameter_list[] = {
    { "phase", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Phase", nullptr },
    { "sync_type", 0x000000, 0.0, SynthLfo::kNumSyncTypes - 1, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sync Type", strings::kSyncNames },
    { "frequency", 0x000000, -7.0, 9.0, 1.0, 0.0, 1.0,
      ValueDetails::kExponential, true, " secs", "Frequency", nullptr },
    { "sync", 0x000000, 0.0, SynthLfo::kNumSyncOptions - 1, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sync", strings::kFrequencySyncNames },
    { "tempo", 0x000000, 0.0, 12.0, 7.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Tempo", strings::kSyncedFrequencyNames },
    { "fade_time", 0x000000, 0.0, 8.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " secs", "Fade In", nullptr },
    { "smooth_mode", 0x000801, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Smooth Mode", strings::kOffOnNames },
    { "smooth_time", 0x000801, -10.0, 4.0, -7.5, 0.0, 1.0,
      ValueDetails::kExponential, false, " secs", "Smooth Time", nullptr },
    { "delay_time", 0x000000, 0.0, 4.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " secs", "Delay", nullptr },
    { "stereo", 0x000406, -0.5, 0.5, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Stereo", nullptr },
    { "keytrack_transpose", 0x000704, -60.0, 36.0, -12.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Transpose", nullptr },
    { "keytrack_tune", 0x000704, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "", "Tune", nullptr },
  };

  const ValueDetails ValueDetailsLookup::random_lfo_parameter_list[] = {
    { "style", 0x000401, 0.0, RandomLfo::kNumStyles - 1, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Style", strings::kRandomNames },
    { "frequency", 0x000401, -7.0, 9.0, 1.0, 0.0, 1.0,
      ValueDetails::kExponential, true, " secs", "Frequency", nullptr },
    { "sync", 0x000401, 0.0, SynthLfo::kNumSyncOptions - 1, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sync", strings::kFrequencySyncNames },
    { "tempo", 0x000401, 0.0, 12.0, 8.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Tempo", strings::kSyncedFrequencyNames },
    { "stereo", 0x000401, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Stereo", strings::kOffOnNames },
    { "sync_type", 0x000600, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Sync Type", strings::kOffOnNames },
    { "keytrack_transpose", 0x000704, -60.0, 36.0, -12.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Transpose", nullptr },
    { "keytrack_tune", 0x000704, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "", "Tune", nullptr },
  };

  const ValueDetails ValueDetailsLookup::filter_parameter_list[] = {
    { "mix", 0x000000, 0.0f, 1.0f, 1.0f, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Mix", nullptr },
    { "cutoff", 0x000000, 8.0, 136.0, 60.0, -60.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Cutoff", nullptr },
    { "resonance", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Resonance", nullptr },
    { "drive", 0x000000, 0.0, 20.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "dB", "Drive", nullptr },
    { "blend", 0x000000, 0.0, 2.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Blend", nullptr },
    { "style", 0x000000, 0.0, 9.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Style", strings::kFilterStyleNames },
    { "model", 0x000000, 0.0, kNumFilterModels - 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Model", strings::kFilterModelNames },
    { "on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Switch", strings::kOffOnNames },
    { "blend_transpose", 0x000000, 0.0, 84.0, 42.0, 0.0, 1.0,
      ValueDetails::kLinear, false, " semitones", "Comb Blend Offset", nullptr },
    { "keytrack", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Key Track", nullptr },
    { "formant_x", 0x000000, 0.0, 1.0, 0.5, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Formant X", nullptr },
    { "formant_y", 0x000000, 0.0, 1.0, 0.5, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Formant Y", nullptr },
    { "formant_transpose", 0x000000, -12.0, 12.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Formant Transpose", nullptr },
    { "formant_resonance", 0x000000, 0.3, 1.0, 0.85, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Formant Resonance", nullptr },
    { "formant_spread", 0x000707, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Formant Spread", nullptr },
    { "osc1_input", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "OSC 1 Input", strings::kOffOnNames },
    { "osc2_input", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "OSC 2 Input", strings::kOffOnNames },
    { "osc3_input", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "OSC 3 Input", strings::kOffOnNames },
    { "sample_input", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "SAMPLE Input", strings::kOffOnNames },
    { "filter_input", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "FILTER Input", strings::kOffOnNames },
  };

  const ValueDetails ValueDetailsLookup::osc_parameter_list[] = {
    { "on", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Switch", strings::kOffOnNames },
    { "transpose", 0x000000, -48.0, 48.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Transpose", nullptr },
    { "transpose_quantize", 0x000000, 0.0, 8191.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Transpose Quantize", nullptr },
    { "tune", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "", "Tune", nullptr },
    { "pan", 0x000000, -1.0, 1.0, 0.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Pan", nullptr },
    { "stack_style", 0x000000, 0.0, SynthOscillator::kNumUnisonStackTypes - 1, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Stack Style", strings::kUnisonStackNames },
    { "unison_detune", 0x000000, 0.0, 10.0, 4.472135955, 0.0, 1.0,
      ValueDetails::kQuadratic, false, "%", "Unison Detune", nullptr },
    { "unison_voices", 0x000000, 1.0, 16.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "v", "Unison Voices", nullptr },
    { "unison_blend", 0x000000, 0.0, 1.0, 0.8, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Blend", nullptr },
    { "detune_power", 0x000000, -5.0, 5.0, 1.5, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Detune Power", nullptr },
    { "detune_range", 0x000000, 0.0, 48.0, 2.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Detune Range", nullptr },
    { "level", 0x000000, 0.0, 1.0, 0.70710678119, 0.0, 1.0,
      ValueDetails::kQuadratic, false, "", "Level", nullptr },
    { "midi_track", 0x000000, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Midi Track", strings::kOffOnNames },
    { "smooth_interpolation", 0x000000, 0.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Smooth Interpolation", strings::kOffOnNames },
    { "spectral_unison", 0x000500, 0.0, 1.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Spectral Unison", strings::kOffOnNames },
    { "wave_frame", 0x000000, 0.0, kNumOscillatorWaveFrames - 1, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Wave Frame", nullptr },
    { "frame_spread", 0x000000, -kNumOscillatorWaveFrames / 2, kNumOscillatorWaveFrames / 2, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Unison Frame Spread", nullptr },
    { "stereo_spread", 0x000000, 0.0, 1.0, 1.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Stereo Spread", nullptr },
    { "phase", 0x000000, 0.0, 1.0, 0.5, 0.0, 360.0,
      ValueDetails::kLinear, false, "", "Phase", nullptr },
    { "distortion_phase", 0x000000, 0.0, 1.0, 0.5, 0.0, 360.0,
      ValueDetails::kLinear, false, "", "Distortion Phase", nullptr },
    { "random_phase", 0x000000, 0.0, 1.0, 1.0, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Phase Randomization", nullptr },
    { "distortion_type", 0x000000, 0.0, SynthOscillator::kNumDistortionTypes - 1, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Distortion Type", strings::kPhaseDistortionNames },
    { "distortion_amount", 0x000000, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Distortion Amount", nullptr },
    { "distortion_spread", 0x000000, -0.5, 0.5, 0.0, 0.0, 200.0,
      ValueDetails::kLinear, false, "%", "Distortion Spread", nullptr },
    { "spectral_morph_type", 0x000407, 0.0, SynthOscillator::kNumSpectralMorphTypes - 1, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Frequency Morph Type", strings::kSpectralMorphNames },
    { "spectral_morph_amount", 0x000407, 0.0, 1.0, 0.5, 0.0, 100.0,
      ValueDetails::kLinear, false, "%", "Frequency Morph Amount", nullptr },
    { "spectral_morph_spread", 0x000407, -0.5, 0.5, 0.0, 0.0, 200.0,
      ValueDetails::kLinear, false, "%", "Frequency Morph Spread", nullptr },
    { "destination", 0x000500, 0.0, constants::kNumSourceDestinations + constants::kNumEffects, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Destination", strings::kDestinationNames },
    { "view_2d", 0x000402, 0.0, 2.0, 1.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "View 2D", strings::kOffOnNames },
  };

  const ValueDetails ValueDetailsLookup::mod_parameter_list[] = {
    { "amount", 0x000000, -1.0, 1.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Amount", nullptr },
    { "power", 0x000000, -10.0, 10.0, 0.0, 0.0, 1.0,
      ValueDetails::kLinear, false, "", "Power", nullptr },
    { "bipolar", 0x000000, 0.0, 1.0f, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Bipolar", strings::kOffOnNames },
    { "stereo", 0x000000, 0.0, 1.0f, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Stereo", strings::kOffOnNames },
    { "bypass", 0x000000, 0.0, 1.0f, 0.0, 0.0, 1.0,
      ValueDetails::kIndexed, false, "", "Bypass", strings::kOffOnNames }
  };

  ValueDetailsLookup::ValueDetailsLookup() {
    static constexpr int kNumOscillatorsOld = 2;
    static constexpr int kNewOscillatorVersion = 0x000500;
    static constexpr int kOldMaxModulations = 32;
    static constexpr int kNewModulationVersion = 0x000601;

    int num_parameters = sizeof(parameter_list) / sizeof(ValueDetails);
    for (int i = 0; i < num_parameters; ++i) {
      details_lookup_[parameter_list[i].name] = parameter_list[i];
      details_list_.push_back(&parameter_list[i]);

      VITAL_ASSERT(parameter_list[i].default_value <= parameter_list[i].max);
      VITAL_ASSERT(parameter_list[i].default_value >= parameter_list[i].min);
    }

    int num_env_parameters = sizeof(env_parameter_list) / sizeof(ValueDetails);
    for (int env = 0; env < kNumEnvelopes; ++env)
      addParameterGroup(env_parameter_list, num_env_parameters, env, kEnvIdPrefix, kEnvNamePrefix);

    int num_lfo_parameters = sizeof(lfo_parameter_list) / sizeof(ValueDetails);
    for (int lfo = 0; lfo < kNumLfos; ++lfo)
      addParameterGroup(lfo_parameter_list, num_lfo_parameters, lfo, kLfoIdPrefix, kLfoNamePrefix);

    int num_random_lfo_parameters = sizeof(random_lfo_parameter_list) / sizeof(ValueDetails);
    for (int lfo = 0; lfo < kNumRandomLfos; ++lfo)
      addParameterGroup(random_lfo_parameter_list, num_random_lfo_parameters, lfo, kRandomIdPrefix, kRandomNamePrefix);

    int num_osc_parameters = sizeof(osc_parameter_list) / sizeof(ValueDetails);
    for (int osc = 0; osc < kNumOscillatorsOld; ++osc)
      addParameterGroup(osc_parameter_list, num_osc_parameters, osc, kOscIdPrefix, kOscNamePrefix);
    for (int osc = kNumOscillatorsOld; osc < kNumOscillators; ++osc) {
      addParameterGroup(osc_parameter_list, num_osc_parameters, osc, kOscIdPrefix,
                        kOscNamePrefix, kNewOscillatorVersion);
    }

    int num_filter_parameters = sizeof(filter_parameter_list) / sizeof(ValueDetails);
    for (int filter = 0; filter < kNumFilters; ++filter)
      addParameterGroup(filter_parameter_list, num_filter_parameters, filter, kFilterIdPrefix, kFilterNamePrefix);

    addParameterGroup(filter_parameter_list, num_filter_parameters, "fx", kFilterIdPrefix, kFilterNamePrefix);

    int num_mod_parameters = sizeof(mod_parameter_list) / sizeof(ValueDetails);
    for (int modulation = 0; modulation < kOldMaxModulations; ++modulation) {
      addParameterGroup(mod_parameter_list, num_mod_parameters, modulation,
                        kModulationIdPrefix, kModulationNamePrefix);
    }
    for (int modulation = kOldMaxModulations; modulation < kMaxModulationConnections; ++modulation) {
      addParameterGroup(mod_parameter_list, num_mod_parameters, modulation,
        kModulationIdPrefix, kModulationNamePrefix, kNewModulationVersion);
    }

    details_lookup_["osc_1_on"].default_value = 1.0f;
    details_lookup_["osc_2_destination"].default_value = 1.0f;
    details_lookup_["osc_3_destination"].default_value = 3.0f;
    details_lookup_["filter_1_osc1_input"].default_value = 1.0f;
    details_lookup_["filter_2_osc2_input"].default_value = 1.0f;

    std::sort(details_list_.begin(), details_list_.end(), compareValueDetails);
  }

  void ValueDetailsLookup::addParameterGroup(const ValueDetails* list, int num_parameters, int index,
                                             std::string id_prefix, std::string name_prefix, int version) {
    std::string string_num = std::to_string(index + 1);
    addParameterGroup(list, num_parameters, string_num, id_prefix, name_prefix, version);
  }

  void ValueDetailsLookup::addParameterGroup(const ValueDetails* list, int num_parameters, std::string id,
                                             std::string id_prefix, std::string name_prefix, int version) {
    std::string id_start = id_prefix + kIdDelimiter + id + kIdDelimiter;
    std::string name_start = name_prefix + kNameDelimiter + id + kNameDelimiter;

    for (int i = 0; i < num_parameters; ++i) {
      ValueDetails details = list[i];
      if (version > details.version_added)
        details.version_added = version;

      details.name = id_start + details.name;
      details.local_description = details.display_name;
      details.display_name = name_start + details.display_name;
      details_lookup_[details.name] = details;
      details_list_.push_back(&details_lookup_[details.name]);
    }
  }

  ValueDetailsLookup Parameters::lookup_;

} // namespace vital
