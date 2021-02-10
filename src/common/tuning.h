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

#include "common.h"
#include "json/json.h"

using json = nlohmann::json;

class Tuning {
  public:
    static constexpr int kTuningSize = 2 * vital::kMidiSize;
    static constexpr int kTuningCenter = vital::kMidiSize;
    static Tuning getTuningForFile(File file);

    static String allFileExtensions();
    static int noteToMidiKey(const String& note);

    Tuning();
    Tuning(File file);

    void loadScale(std::vector<float> scale);
    void loadFile(File file);
    void setConstantTuning(float note);
    void setDefaultTuning();
    vital::mono_float convertMidiNote(int note) const;
    void setStartMidiNote(int start_midi_note) { scale_start_midi_note_ = start_midi_note; }
    void setReferenceNote(int reference_note) { reference_midi_note_ = reference_note; }
    void setReferenceFrequency(float frequency);
    void setReferenceNoteFrequency(int midi_note, float frequency);
    void setReferenceRatio(float ratio);
    std::string getName() const { 
      if (mapping_name_.size() == 0)
        return tuning_name_;
      if (tuning_name_.size() == 0)
        return mapping_name_;
      return tuning_name_ + " / " + mapping_name_;
    }

    void setName(const std::string& name) {
      mapping_name_ = "";
      tuning_name_ = name;
    }
    bool isDefault() const { return default_; }

    json stateToJson() const;
    void jsonToState(const json& data);
    void loadScalaFile(const StringArray& scala_lines);

  private:
    void loadScalaFile(File scala_file);
    void loadKeyboardMapFile(File kbm_file);
    void loadTunFile(File tun_file);

    int scale_start_midi_note_;
    float reference_midi_note_;
    std::vector<float> scale_;
    std::vector<int> keyboard_mapping_;
    vital::mono_float tuning_[kTuningSize];
    std::string tuning_name_;
    std::string mapping_name_;
    bool default_;

    JUCE_LEAK_DETECTOR(Tuning)
};

