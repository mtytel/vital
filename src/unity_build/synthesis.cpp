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

#include "fir_halfband_decimator.cpp"
#include "digital_svf.cpp"
#include "dc_filter.cpp"
#include "linkwitz_riley_filter.cpp"
#include "synth_filter.cpp"
#include "formant_filter.cpp"
#include "decimator.cpp"
#include "upsampler.cpp"
#include "diode_filter.cpp"
#include "comb_filter.cpp"
#include "vocal_tract.cpp"
#include "formant_manager.cpp"
#include "dirty_filter.cpp"
#include "iir_halfband_decimator.cpp"
#include "sallen_key_filter.cpp"
#include "phaser_filter.cpp"
#include "ladder_filter.cpp"
#include "synth_oscillator.cpp"
#include "sample_source.cpp"
#include "wave_frame.cpp"
#include "wavetable.cpp"
#include "utils.cpp"
#include "feedback.cpp"
#include "voice_handler.cpp"
#include "processor.cpp"
#include "synth_module.cpp"
#include "operators.cpp"
#include "processor_router.cpp"
#include "value.cpp"
#include "trigger_random.cpp"
#include "synth_lfo.cpp"
#include "random_lfo.cpp"
#include "line_map.cpp"
#include "envelope.cpp"
#include "smooth_value.cpp"
#include "peak_meter.cpp"
#include "value_switch.cpp"
#include "legato_filter.cpp"
#include "portamento_slope.cpp"
#include "filter_module.cpp"
#include "filters_module.cpp"
#include "envelope_module.cpp"
#include "lfo_module.cpp"
#include "random_lfo_module.cpp"
#include "reorderable_effect_chain.cpp"
#include "chorus_module.cpp"
#include "modulation_connection_processor.cpp"
#include "equalizer_module.cpp"
#include "distortion_module.cpp"
#include "flanger_module.cpp"
#include "reverb_module.cpp"
#include "delay_module.cpp"
#include "comb_module.cpp"
#include "compressor_module.cpp"
#include "formant_module.cpp"
#include "sample_module.cpp"
#include "oscillator_module.cpp"
#include "phaser_module.cpp"
#include "producers_module.cpp"
#include "phaser.cpp"
#include "distortion.cpp"
#include "compressor.cpp"
#include "delay.cpp"
#include "reverb.cpp"
#include "sound_engine.cpp"
#include "synth_voice_handler.cpp"
