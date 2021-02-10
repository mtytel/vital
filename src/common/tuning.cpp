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

#include "tuning.h"

#include "utils.h"

namespace {
  constexpr char kScalaFileExtension[] = ".scl";
  constexpr char kKeyboardMapExtension[] = ".kbm";
  constexpr char kTunFileExtension[] = ".tun";
  constexpr int kDefaultMidiReference = 60;
  constexpr char kScalaKbmComment = '!';
  constexpr char kTunComment = ';';

  enum ScalaReadingState {
    kDescription,
    kScaleLength,
    kScaleRatios
  };

  enum KbmPositions {
    kMapSizePosition,
    kStartMidiMapPosition,
    kEndMidiMapPosition,
    kMidiMapMiddlePosition,
    kReferenceNotePosition,
    kReferenceFrequencyPosition,
    kScaleDegreePosition,
  };

  enum TunReadingState {
    kScanningForSection,
    kTuning,
    kExactTuning
  };

  String extractFirstToken(const String& source) {
    StringArray tokens;
    tokens.addTokens(source, false);
    return tokens[0];
  }

  float readCentsToTranspose(const String& cents) {
    return cents.getFloatValue() / vital::kCentsPerNote;
  }

  float readRatioToTranspose(const String& ratio) {
    StringArray tokens;
    tokens.addTokens(ratio, "/", "");
    float value = tokens[0].getIntValue();

    if (tokens.size() == 2)
      value /= tokens[1].getIntValue();

    return vital::utils::ratioToMidiTranspose(value);
  }

  String readTunSection(const String& line) {
    return line.substring(1, line.length() - 1).toLowerCase();
  }

  bool isBaseFrequencyAssignment(const String& line) {
    return line.upToFirstOccurrenceOf("=", false, true).toLowerCase().trim() == "basefreq";
  }

  int getNoteAssignmentIndex(const String& line) {
    String variable = line.upToFirstOccurrenceOf("=", false, true);
    StringArray tokens;
    tokens.addTokens(variable, false);
    if (tokens.size() <= 1 || tokens[0].toLowerCase() != "note")
      return -1;
    int index = tokens[1].getIntValue();
    if (index < 0 || index >= vital::kMidiSize)
      return -1;
    return index;
  }

  float getAssignmentValue(const String& line) {
    String value = line.fromLastOccurrenceOf("=", false, true).trim();
    return value.getFloatValue();
  }
}

String Tuning::allFileExtensions() {
  return String("*") + kScalaFileExtension + String(";") +
         String("*") + kKeyboardMapExtension + String(";") +
         String("*") + kTunFileExtension;
}

int Tuning::noteToMidiKey(const String& note_text) {
  constexpr int kNotesInScale = 7;
  constexpr int kOctaveStart = -1;
  constexpr int kScale[kNotesInScale] = { -3, -1, 0, 2, 4, 5, 7 };

  String text = note_text.toLowerCase().removeCharacters(" ");
  if (note_text.length() < 2)
    return -1;

  char note_in_scale = text[0] - 'a';
  if (note_in_scale < 0 || note_in_scale >= kNotesInScale)
    return -1;

  int offset = kScale[note_in_scale];
  text = text.substring(1);
  if (text[0] == '#') {
    text = text.substring(1);
    offset++;
  }
  else if (text[0] == 'b') {
    text = text.substring(1);
    offset--;
  }

  if (text.length() == 0)
    return -1;

  bool negative = false;
  if (text[0] == '-') {
    text = text.substring(1);
    negative = true;
    if (text.length() == 0)
      return -1;
  }
  int octave = text[0] - '0';
  if (negative)
    octave = -octave;
  octave = octave - kOctaveStart;
  return vital::kNotesPerOctave * octave + offset;
}

Tuning Tuning::getTuningForFile(File file) {
  return Tuning(file);
}

void Tuning::loadFile(File file) {
  String extension = file.getFileExtension().toLowerCase();
  if (extension == String(kScalaFileExtension))
    loadScalaFile(file);
  else if (extension == String(kTunFileExtension))
    loadTunFile(file);
  else if (extension == String(kKeyboardMapExtension))
    loadKeyboardMapFile(file);

  default_ = false;
}

void Tuning::loadScalaFile(const StringArray& scala_lines) {
  ScalaReadingState state = kDescription;

  int scale_length = 1;
  std::vector<float> scale;
  scale.push_back(0.0f);

  for (const String& line : scala_lines) {
    String trimmed_line = line.trim();
    if (trimmed_line.length() > 0 && trimmed_line[0] == kScalaKbmComment)
      continue;

    if (scale.size() >= scale_length + 1)
      break;

    switch (state) {
      case kDescription:
        state = kScaleLength;
        break;
      case kScaleLength:
        scale_length = extractFirstToken(trimmed_line).getIntValue();
        state = kScaleRatios;
        break;
      case kScaleRatios: {
        String tuning = extractFirstToken(trimmed_line);
        if (tuning.contains("."))
          scale.push_back(readCentsToTranspose(tuning));
        else
          scale.push_back(readRatioToTranspose(tuning));
        break;
      }
    }
  }

  keyboard_mapping_.clear();
  for (int i = 0; i < scale.size() - 1; ++i)
    keyboard_mapping_.push_back(i);
  scale_start_midi_note_ = kDefaultMidiReference;
  reference_midi_note_ = 0;

  loadScale(scale);
  default_ = false;
}

void Tuning::loadScalaFile(File scala_file) {
  StringArray lines;
  scala_file.readLines(lines);
  loadScalaFile(lines);
  tuning_name_ = scala_file.getFileNameWithoutExtension().toStdString();
}

void Tuning::loadKeyboardMapFile(File kbm_file) {
  static constexpr int kHeaderSize = 7;

  StringArray lines;
  kbm_file.readLines(lines);

  float header_data[kHeaderSize];
  memset(header_data, 0, kHeaderSize * sizeof(float));
  int header_position = 0;
  int map_size = 0;
  int last_scale_value = 0;
  keyboard_mapping_.clear();

  for (const String& line : lines) {
    String trimmed_line = line.trim();
    if (trimmed_line.length() > 0 && trimmed_line[0] == kScalaKbmComment)
      continue;

    if (header_position >= kHeaderSize) {
      String token = extractFirstToken(trimmed_line);
      if (token.toLowerCase()[0] != 'x')
        last_scale_value = token.getIntValue();
     
      keyboard_mapping_.push_back(last_scale_value);

      if (keyboard_mapping_.size() >= map_size)
        break;
    }
    else {
      header_data[header_position] = extractFirstToken(trimmed_line).getFloatValue();
      if (header_position == kMapSizePosition)
        map_size = header_data[header_position];
      header_position++;
    }
  }

  setStartMidiNote(header_data[kMidiMapMiddlePosition]);
  setReferenceNoteFrequency(header_data[kReferenceNotePosition], header_data[kReferenceFrequencyPosition]);
  loadScale(scale_);

  mapping_name_ = kbm_file.getFileNameWithoutExtension().toStdString();
}

void Tuning::loadTunFile(File tun_file) {
  keyboard_mapping_.clear();

  TunReadingState state = kScanningForSection;
  StringArray lines;
  tun_file.readLines(lines);

  int last_read_note = 0;
  float base_frequency = vital::kMidi0Frequency;
  std::vector<float> scale;
  for (int i = 0; i < vital::kMidiSize; ++i)
    scale.push_back(i);

  for (const String& line : lines) {
    String trimmed_line = line.trim();
    if (trimmed_line.length() == 0 || trimmed_line[0] == kTunComment)
      continue;

    if (trimmed_line[0] == '[') {
      String section = readTunSection(trimmed_line);
      if (section == "tuning")
        state = kTuning;
      else if (section == "exact tuning")
        state = kExactTuning;
      else
        state = kScanningForSection;
    }
    else if (state == kTuning || state == kExactTuning) {
      if (isBaseFrequencyAssignment(trimmed_line))
        base_frequency = getAssignmentValue(trimmed_line);
      else {
        int index = getNoteAssignmentIndex(trimmed_line);
        last_read_note = std::max(last_read_note, index);
        if (index >= 0)
          scale[index] = getAssignmentValue(trimmed_line) / vital::kCentsPerNote;
      }
    }
  }

  scale.resize(last_read_note + 1);
  
  loadScale(scale);
  setStartMidiNote(0);
  setReferenceFrequency(base_frequency);
  tuning_name_ = tun_file.getFileNameWithoutExtension().toStdString();
}

Tuning::Tuning() : default_(true) {
  scale_start_midi_note_ = kDefaultMidiReference;
  reference_midi_note_ = 0;

  setDefaultTuning();
}

Tuning::Tuning(File file) : Tuning() {
  loadFile(file);
}

void Tuning::loadScale(std::vector<float> scale) {
  scale_ = scale;
  if (scale.size() <= 1) {
    setConstantTuning(kDefaultMidiReference);
    return;
  }

  int scale_size = static_cast<int>(scale.size() - 1);
  int mapping_size = scale_size;
  if (keyboard_mapping_.size())
    mapping_size = static_cast<int>(keyboard_mapping_.size());

  float octave_offset = scale[scale_size];
  int start_octave = -kTuningCenter / mapping_size - 1;
  int mapping_position = -kTuningCenter - start_octave * mapping_size;

  float current_offset = start_octave * octave_offset;
  for (int i = 0; i < kTuningSize; ++i) {
    if (mapping_position >= mapping_size) {
      current_offset += octave_offset;
      mapping_position = 0;
    }

    int note_in_scale = mapping_position;
    if (keyboard_mapping_.size())
      note_in_scale = keyboard_mapping_[mapping_position];

    tuning_[i] = current_offset + scale[note_in_scale];
    mapping_position++;
  }
}

void Tuning::setConstantTuning(float note) {
  for (int i = 0; i < kTuningSize; ++i)
    tuning_[i] = note;
}

void Tuning::setDefaultTuning() {
  for (int i = 0; i < kTuningSize; ++i)
    tuning_[i] = i - kTuningCenter;

  scale_.clear();
  for (int i = 0; i <= vital::kNotesPerOctave; ++i)
    scale_.push_back(i);

  keyboard_mapping_.clear();

  default_ = true;
  tuning_name_ = "";
  mapping_name_ = "";
}

vital::mono_float Tuning::convertMidiNote(int note) const {
  int scale_offset = note - scale_start_midi_note_;
  return tuning_[kTuningCenter + scale_offset] + scale_start_midi_note_ + reference_midi_note_;
}

void Tuning::setReferenceFrequency(float frequency) {
  setReferenceNoteFrequency(0, frequency);
}

void Tuning::setReferenceNoteFrequency(int midi_note, float frequency) {
  reference_midi_note_ = vital::utils::frequencyToMidiNote(frequency) - midi_note;
}

void Tuning::setReferenceRatio(float ratio) {
  reference_midi_note_ = vital::utils::ratioToMidiTranspose(ratio);
}

json Tuning::stateToJson() const {
  json data;
  data["scale_start_midi_note"] = scale_start_midi_note_;
  data["reference_midi_note"] = reference_midi_note_;
  data["tuning_name"] = tuning_name_;
  data["mapping_name"] = mapping_name_;
  data["default"] = default_;

  json scale_data;
  for (float scale_value : scale_)
    scale_data.push_back(scale_value);
  data["scale"] = scale_data;

  if (keyboard_mapping_.size()) {
    json mapping_data;
    for (int mapping_value : keyboard_mapping_)
      mapping_data.push_back(mapping_value);
    data["mapping"] = mapping_data;
  }

  return data;
}

void Tuning::jsonToState(const json& data) {
  scale_start_midi_note_ = data["scale_start_midi_note"];
  reference_midi_note_ = data["reference_midi_note"];
  std::string tuning_name = data["tuning_name"];
  tuning_name_ = tuning_name;
  std::string mapping_name = data["mapping_name"];
  mapping_name_ = mapping_name;
  
  if (data.count("default"))
    default_ = data["default"];

  json scale_data = data["scale"];
  scale_.clear();
  for (json& value : scale_data) {
    float scale_value = value;
    scale_.push_back(scale_value);
  }

  keyboard_mapping_.clear();
  if (data.count("mapping")) {
    json mapping_data = data["mapping"];
    for (json& value : mapping_data) {
      int keyboard_value = value;
      keyboard_mapping_.push_back(keyboard_value);
    }
  }

  loadScale(scale_);
}
