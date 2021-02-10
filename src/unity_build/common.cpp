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

#if !HEADLESS
#include "border_bounds_constrainer.cpp"
#endif

#include "line_generator.cpp"
#include "midi_manager.cpp"
#include "tuning.cpp"
#include "startup.cpp"
#include "synth_gui_interface.cpp"
#include "synth_parameters.cpp"
#include "load_save.cpp"
#include "synth_types.cpp"
#include "synth_base.cpp"
#include "wavetable_component_factory.cpp"
#include "wavetable_keyframe.cpp"
#include "file_source.cpp"
#include "shepard_tone_source.cpp"
#include "frequency_filter_modifier.cpp"
#include "wave_fold_modifier.cpp"
#include "phase_modifier.cpp"
#include "wavetable_creator.cpp"
#include "wave_line_source.cpp"
#include "wave_source.cpp"
#include "wavetable_group.cpp"
#include "wave_window_modifier.cpp"
#include "wavetable_component.cpp"
#include "pitch_detector.cpp"
#include "wave_warp_modifier.cpp"
#include "slew_limit_modifier.cpp"
