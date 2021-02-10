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

#include "synth_base.h"

#include "sample_source.h"
#include "sound_engine.h"
#include "load_save.h"
#include "memory.h"
#include "modulation_connection_processor.h"
#include "startup.h"
#include "synth_gui_interface.h"
#include "synth_parameters.h"
#include "utils.h"

SynthBase::SynthBase() : expired_(false) {
  expired_ = LoadSave::isExpired();
  self_reference_ = std::make_shared<SynthBase*>();
  *self_reference_ = this;

  engine_ = std::make_unique<vital::SoundEngine>();
  engine_->setTuning(&tuning_);

  mod_connections_.reserve(vital::kMaxModulationConnections);

  for (int i = 0; i < vital::kNumOscillators; ++i) {
    vital::Wavetable* wavetable = engine_->getWavetable(i);
    if (wavetable) {
      wavetable_creators_[i] = std::make_unique<WavetableCreator>(wavetable);
      wavetable_creators_[i]->init();
    }
  }

  keyboard_state_ = std::make_unique<MidiKeyboardState>();
  midi_manager_ = std::make_unique<MidiManager>(this, keyboard_state_.get(), &save_info_, this);

  last_played_note_ = 0.0f;
  last_num_pressed_ = 0;
  audio_memory_ = std::make_unique<vital::StereoMemory>(vital::kAudioMemorySamples);
  memset(oscilloscope_memory_, 0, 2 * vital::kOscilloscopeMemoryResolution * sizeof(vital::poly_float));
  memset(oscilloscope_memory_write_, 0, 2 * vital::kOscilloscopeMemoryResolution * sizeof(vital::poly_float));
  memory_reset_period_ = vital::kOscilloscopeMemoryResolution;
  memory_input_offset_ = 0;
  memory_index_ = 0;

  controls_ = engine_->getControls();

  Startup::doStartupChecks(midi_manager_.get());
}

SynthBase::~SynthBase() { }

void SynthBase::valueChanged(const std::string& name, vital::mono_float value) {
  controls_[name]->set(value);
}

void SynthBase::valueChangedInternal(const std::string& name, vital::mono_float value) {
  valueChanged(name, value);
  setValueNotifyHost(name, value);
}

void SynthBase::valueChangedThroughMidi(const std::string& name, vital::mono_float value) {
  controls_[name]->set(value);
  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, name, value);
  setValueNotifyHost(name, value);
  callback->post();
}

void SynthBase::pitchWheelMidiChanged(vital::mono_float value) {
  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, "pitch_wheel", value);
  callback->post();
}

void SynthBase::modWheelMidiChanged(vital::mono_float value) {
  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, "mod_wheel", value);
  callback->post();
}

void SynthBase::pitchWheelGuiChanged(vital::mono_float value) {
  engine_->setZonedPitchWheel(value, 0, vital::kNumMidiChannels - 1);
}

void SynthBase::modWheelGuiChanged(vital::mono_float value) {
  engine_->setModWheelAllChannels(value);
}

void SynthBase::presetChangedThroughMidi(File preset) {
  SynthGuiInterface* gui_interface = getGuiInterface();
  if (gui_interface) {
    gui_interface->updateFullGui();
    gui_interface->notifyFresh();
  }
}

void SynthBase::valueChangedExternal(const std::string& name, vital::mono_float value) {
  valueChanged(name, value);
  if (name == "mod_wheel")
    engine_->setModWheelAllChannels(value);
  else if (name == "pitch_wheel")
    engine_->setZonedPitchWheel(value, 0, vital::kNumMidiChannels - 1);

  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, name, value);
  callback->post();
}

vital::ModulationConnection* SynthBase::getConnection(const std::string& source, const std::string& destination) {
  for (vital::ModulationConnection* connection : mod_connections_) {
    if (connection->source_name == source && connection->destination_name == destination)
      return connection;
  }
  return nullptr;
}

int SynthBase::getConnectionIndex(const std::string& source, const std::string& destination) {
  vital::ModulationConnectionBank& modulation_bank = getModulationBank();
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    vital::ModulationConnection* connection = modulation_bank.atIndex(i);
    if (connection->source_name == source && connection->destination_name == destination)
      return i;
  }
  return -1;
}

vital::modulation_change SynthBase::createModulationChange(vital::ModulationConnection* connection) {
  vital::modulation_change change;
  change.source = engine_->getModulationSource(connection->source_name);
  change.mono_destination = engine_->getMonoModulationDestination(connection->destination_name);
  change.mono_modulation_switch = engine_->getMonoModulationSwitch(connection->destination_name);
  VITAL_ASSERT(change.source != nullptr);
  VITAL_ASSERT(change.mono_destination != nullptr);
  VITAL_ASSERT(change.mono_modulation_switch != nullptr);

  change.destination_scale = vital::Parameters::getParameterRange(connection->destination_name);
  change.poly_modulation_switch = engine_->getPolyModulationSwitch(connection->destination_name);
  change.poly_destination = engine_->getPolyModulationDestination(connection->destination_name);
  change.modulation_processor = connection->modulation_processor.get();

  int num_audio_rate = 0;
  vital::ModulationConnectionBank& modulation_bank = getModulationBank();
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    if (modulation_bank.atIndex(i)->source_name == connection->source_name &&
        modulation_bank.atIndex(i)->destination_name != connection->destination_name &&
        !modulation_bank.atIndex(i)->modulation_processor->isControlRate()) {
      num_audio_rate++;
    }
  }
  change.num_audio_rate = num_audio_rate;
  return change;
}

bool SynthBase::isInvalidConnection(const vital::modulation_change& change) {
  return change.poly_destination && change.poly_destination->router() == change.modulation_processor;
}

void SynthBase::connectModulation(vital::ModulationConnection* connection) {
  vital::modulation_change change = createModulationChange(connection);
  if (isInvalidConnection(change)) {
    connection->destination_name = "";
    connection->source_name = "";
  }
  else if (mod_connections_.count(connection) == 0) {
    change.disconnecting = false;
    mod_connections_.push_back(connection);
    modulation_change_queue_.enqueue(change);
  }
}

bool SynthBase::connectModulation(const std::string& source, const std::string& destination) {
  vital::ModulationConnection* connection = getConnection(source, destination);
  bool create = connection == nullptr;
  if (create)
    connection = getModulationBank().createConnection(source, destination);

  if (connection)
    connectModulation(connection);

  return create;
}

void SynthBase::disconnectModulation(vital::ModulationConnection* connection) {
  if (mod_connections_.count(connection) == 0)
    return;

  vital::modulation_change change = createModulationChange(connection);
  connection->source_name = "";
  connection->destination_name = "";

  mod_connections_.remove(connection);
  change.disconnecting = true;
  modulation_change_queue_.enqueue(change);
}

void SynthBase::disconnectModulation(const std::string& source, const std::string& destination) {
  vital::ModulationConnection* connection = getConnection(source, destination);
  if (connection)
    disconnectModulation(connection);
}

void SynthBase::clearModulations() {
  clearModulationQueue();
  
  while (mod_connections_.size()) {
    vital::ModulationConnection* connection = *mod_connections_.begin();
    mod_connections_.remove(connection);
    vital::modulation_change change = createModulationChange(connection);
    change.disconnecting = true;
    engine_->disconnectModulation(change);
    connection->source_name = "";
    connection->destination_name = "";
  }

  int num_connections = static_cast<int>(getModulationBank().numConnections());
  for (int i = 0; i < num_connections; ++i)
    getModulationBank().atIndex(i)->modulation_processor->lineMapGenerator()->initLinear();

  engine_->disableUnnecessaryModSources();
}

void SynthBase::forceShowModulation(const std::string& source, bool force) {
  if (force)
    engine_->enableModSource(source);
  else if (!isSourceConnected(source))
    engine_->disableModSource(source);
}

bool SynthBase::isModSourceEnabled(const std::string& source) {
  return engine_->isModSourceEnabled(source);
}

int SynthBase::getNumModulations(const std::string& destination) {
  int connections = 0;
  for (vital::ModulationConnection* connection : mod_connections_) {
    if (connection->destination_name == destination)
      connections++;
  }
  return connections;
}

std::vector<vital::ModulationConnection*> SynthBase::getSourceConnections(const std::string& source) {
  std::vector<vital::ModulationConnection*> connections;
  for (vital::ModulationConnection* connection : mod_connections_) {
    if (connection->source_name == source)
      connections.push_back(connection);
  }
  return connections;
}

bool SynthBase::isSourceConnected(const std::string& source) {
  for (vital::ModulationConnection* connection : mod_connections_) {
    if (connection->source_name == source)
      return true;
  }
  return false;
}

std::vector<vital::ModulationConnection*> SynthBase::getDestinationConnections(const std::string& destination) {
  std::vector<vital::ModulationConnection*> connections;
  for (vital::ModulationConnection* connection : mod_connections_) {
    if (connection->destination_name == destination)
      connections.push_back(connection);
  }
  return connections;
}

const vital::StatusOutput* SynthBase::getStatusOutput(const std::string& name) {
  return engine_->getStatusOutput(name);
}

vital::Wavetable* SynthBase::getWavetable(int index) {
  return engine_->getWavetable(index);
}

WavetableCreator* SynthBase::getWavetableCreator(int index) {
  return wavetable_creators_[index].get();
}

vital::Sample* SynthBase::getSample() {
  return engine_->getSample();
}

LineGenerator* SynthBase::getLfoSource(int index) {
  return engine_->getLfoSource(index);
}

json SynthBase::saveToJson() {
  return LoadSave::stateToJson(this, getCriticalSection());
}

int SynthBase::getSampleRate() {
  return engine_->getSampleRate();
}

void SynthBase::initEngine() {
  clearModulations();
  if (getWavetableCreator(0)) {
    for (int i = 0; i < vital::kNumOscillators; ++i)
      getWavetableCreator(i)->init();

    engine_->getSample()->init();
  }

  for (int i = 0; i < vital::kNumLfos; ++i)
    getLfoSource(i)->initTriangle();

  vital::control_map controls = engine_->getControls();
  for (auto& control : controls) {
    vital::ValueDetails details = vital::Parameters::getDetails(control.first);
    control.second->set(details.default_value);
  }
  checkOversampling();

  clearActiveFile();
}

void SynthBase::loadTuningFile(const File& file) {
  tuning_.loadFile(file);
}

void SynthBase::loadInitPreset() {
  pauseProcessing(true);
  engine_->allSoundsOff();
  initEngine();
  LoadSave::initSaveInfo(save_info_);
  pauseProcessing(false);
}

bool SynthBase::loadFromJson(const json& data) {
  pauseProcessing(true);
  engine_->allSoundsOff();
  try {
    bool result = LoadSave::jsonToState(this, save_info_, data);
    pauseProcessing(false);
    return result;
  }
  catch (const json::exception& e) {
    pauseProcessing(false);
    throw e;
  }
}

bool SynthBase::loadFromFile(File preset, std::string& error) {
  if (!preset.exists())
    return false;
  
  try {
    json parsed_json_state = json::parse(preset.loadFileAsString().toStdString(), nullptr);
    if (!loadFromJson(parsed_json_state)) {
      error = "Preset was created with a newer version.";
      return false;
    }

    active_file_ = preset;
  }
  catch (const json::exception& e) {
    error = "Preset file is corrupted.";
    return false;
  }
  
  setPresetName(preset.getFileNameWithoutExtension());

  SynthGuiInterface* gui_interface = getGuiInterface();
  if (gui_interface) {
    gui_interface->updateFullGui();
    gui_interface->notifyFresh();
  }

  return true;
}

void SynthBase::renderAudioToFile(File file, float seconds, float bpm, std::vector<int> notes, bool render_images) {
  static constexpr int kSampleRate = 44100;
  static constexpr int kPreProcessSamples = 44100;
  static constexpr int kFadeSamples = 200;
  static constexpr int kBufferSize = 64;
  static constexpr int kVideoRate = 30;
  static constexpr int kImageNumberPlaces = 3;
  static constexpr int kImageWidth = 500;
  static constexpr int kImageHeight = 250;
  static constexpr int kOscilloscopeResolution = 512;
  static constexpr float kFadeRatio = 0.3f;

  ScopedLock lock(getCriticalSection());

  processModulationChanges();
  engine_->setSampleRate(kSampleRate);
  engine_->setBpm(bpm);
  engine_->updateAllModulationSwitches();

  double sample_time = 1.0 / getSampleRate();
  double current_time = -kPreProcessSamples * sample_time;

  for (int samples = 0; samples < kPreProcessSamples; samples += kBufferSize) {
    engine_->correctToTime(current_time);
    current_time += kBufferSize * sample_time;
    engine_->process(kBufferSize);
  }

  for (int note : notes)
    engine_->noteOn(note, 0.7f, 0, 0);

  file.deleteFile();
  std::unique_ptr<FileOutputStream> file_stream = file.createOutputStream();
  WavAudioFormat wav_format;
  std::unique_ptr<AudioFormatWriter> writer(wav_format.createWriterFor(file_stream.get(), kSampleRate, 2, 16, {}, 0));

  int on_samples = seconds * kSampleRate;
  int total_samples = on_samples + seconds * kSampleRate * kFadeRatio;
  std::unique_ptr<float[]> left_buffer = std::make_unique<float[]>(kBufferSize);
  std::unique_ptr<float[]> right_buffer = std::make_unique<float[]>(kBufferSize);
  float* buffers[2] = { left_buffer.get(), right_buffer.get() };
  const vital::mono_float* engine_output = (const vital::mono_float*)engine_->output(0)->buffer;

#if JUCE_MODULE_AVAILABLE_juce_graphics
  int current_image_index = -1;
  PNGImageFormat png;
  File images_folder = File::getCurrentWorkingDirectory().getChildFile("images");
  if (!images_folder.exists() && render_images)
    images_folder.createDirectory();
  const vital::poly_float* memory = getOscilloscopeMemory();
#endif

  for (int samples = 0; samples < total_samples; samples += kBufferSize) {
    engine_->correctToTime(current_time);
    current_time += kBufferSize * sample_time;
    engine_->process(kBufferSize);
    updateMemoryOutput(kBufferSize, engine_->output(0)->buffer);

    if (on_samples > samples && on_samples <= samples + kBufferSize) {
      for (int note : notes)
        engine_->noteOff(note, 0.5f, 0, 0);
    }

    for (int i = 0; i < kBufferSize; ++i) {
      vital::mono_float t = (total_samples - samples) / (1.0f * kFadeSamples);
      t = vital::utils::min(t, 1.0f);
      left_buffer[i] = t * engine_output[vital::poly_float::kSize * i];
      right_buffer[i] = t * engine_output[vital::poly_float::kSize * i + 1];
    }

    writer->writeFromFloatArrays(buffers, 2, kBufferSize);

  #if JUCE_MODULE_AVAILABLE_juce_graphics
    int image_index = (samples * kVideoRate) / kSampleRate;
    if (image_index > current_image_index && render_images) {
      current_image_index = image_index;
      String number(image_index);
      while (number.length() < kImageNumberPlaces)
        number = "0" + number;

      File image_file = images_folder.getChildFile("rendered_image" + number + ".png");
      FileOutputStream image_file_stream(image_file);
      Image image(Image::RGB, kImageWidth, kImageHeight, true);
      Graphics g(image);
      g.fillAll(Colour(0xff1d2125));

      Path left_path;
      Path right_path;
      left_path.startNewSubPath(-2.0f, kImageHeight / 2);
      right_path.startNewSubPath(-2.0f, kImageHeight / 2);

      for (int i = 0; i < kOscilloscopeResolution; ++i) {
        float t = i / (kOscilloscopeResolution - 1.0f);
        float memory_spot = (1.0f * i * vital::kOscilloscopeMemoryResolution) / kOscilloscopeResolution;
        int memory_index = memory_spot;
        float remainder = memory_spot - memory_index;
        vital::poly_float from = memory[memory_index];
        vital::poly_float to = memory[memory_index + 1];
        vital::poly_float y = -vital::utils::interpolate(from, to, remainder) * kImageHeight / 2.0f + kImageHeight / 2;
        left_path.lineTo(t * kImageWidth, y[0]);
        right_path.lineTo(t * kImageWidth, y[1]);
      }
      left_path.lineTo(kImageWidth + 2.0f, kImageHeight / 2.0f);
      right_path.lineTo(kImageWidth + 2.0f, kImageHeight / 2.0f);

      g.setColour(Colour(0x64aa88ff));
      g.fillPath(left_path);
      g.fillPath(right_path);

      g.setColour(Colour(0xffaa88ff));
      g.strokePath(left_path, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
      g.strokePath(right_path, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));

      png.writeImageToStream(image, image_file_stream);
    }
  #endif
  }

  writer->flush();
  file_stream->flush();

  writer = nullptr;
  file_stream.release();
}

void SynthBase::renderAudioForResynthesis(float* data, int samples, int note) {
  static constexpr int kPreProcessSamples = 44100;
  static constexpr int kBufferSize = 64;

  ScopedLock lock(getCriticalSection());

  double sample_time = 1.0 / getSampleRate();
  double current_time = -kPreProcessSamples * sample_time;

  engine_->allSoundsOff();
  for (int s = 0; s < kPreProcessSamples; s += kBufferSize) {
    engine_->correctToTime(current_time);
    current_time += kBufferSize * sample_time;
    engine_->process(kBufferSize);
  }

  engine_->noteOn(note, 0.7f, 0, 0);
  const vital::poly_float* engine_output = engine_->output(0)->buffer;
  float max_value = 0.01f;
  for (int s = 0; s < samples; s += kBufferSize) {
    int num_samples = std::min(samples - s, kBufferSize);
    engine_->correctToTime(current_time);
    current_time += num_samples * sample_time;
    engine_->process(num_samples);

    for (int i = 0; i < num_samples; ++i) {
      float sample = engine_output[i][0];
      data[s + i] = sample;
      max_value = std::max(max_value, fabsf(sample));
    }
  }

  float scale = 1.0f / max_value;
  for (int s = 0; s < samples; ++s)
    data[s] *= scale;

  engine_->allSoundsOff();
}

bool SynthBase::saveToFile(File preset) {
  preset = preset.withFileExtension(String(vital::kPresetExtension));

  File parent = preset.getParentDirectory();
  if (!parent.exists()) {
    if (!parent.createDirectory().wasOk() || !parent.hasWriteAccess())
      return false;
  }

  setPresetName(preset.getFileNameWithoutExtension());

  SynthGuiInterface* gui_interface = getGuiInterface();
  if (gui_interface)
    gui_interface->notifyFresh();

  if (preset.replaceWithText(saveToJson().dump())) {
    active_file_ = preset;
    return true;
  }
  return false;
}

bool SynthBase::saveToActiveFile() {
  if (!active_file_.exists() || !active_file_.hasWriteAccess())
    return false;

  return saveToFile(active_file_);
}

void SynthBase::setMpeEnabled(bool enabled) {
  midi_manager_->setMpeEnabled(enabled);
}

void SynthBase::processAudio(AudioSampleBuffer* buffer, int channels, int samples, int offset) {
  if (expired_)
    return;

  engine_->process(samples);
  writeAudio(buffer, channels, samples, offset);
}

void SynthBase::processAudioWithInput(AudioSampleBuffer* buffer, const vital::poly_float* input_buffer,
                                      int channels, int samples, int offset) {
  if (expired_)
    return;

  engine_->processWithInput(input_buffer, samples);
  writeAudio(buffer, channels, samples, offset);
}

void SynthBase::writeAudio(AudioSampleBuffer* buffer, int channels, int samples, int offset) {
  const vital::mono_float* engine_output = (const vital::mono_float*)engine_->output(0)->buffer;
  for (int channel = 0; channel < channels; ++channel) {
    float* channel_data = buffer->getWritePointer(channel, offset);

    for (int i = 0; i < samples; ++i) {
      channel_data[i] = engine_output[vital::poly_float::kSize * i + channel];
      VITAL_ASSERT(std::isfinite(channel_data[i]));
    }
  }

  updateMemoryOutput(samples, engine_->output(0)->buffer);
}

void SynthBase::processMidi(MidiBuffer& midi_messages, int start_sample, int end_sample) {
  bool process_all = end_sample == 0;
  for (const MidiMessageMetadata message : midi_messages) {
    int midi_sample = message.samplePosition;
    if (process_all || (midi_sample >= start_sample && midi_sample < end_sample))
      midi_manager_->processMidiMessage(message.getMessage(), midi_sample - start_sample);
  }
}

void SynthBase::processKeyboardEvents(MidiBuffer& buffer, int num_samples) {
  midi_manager_->replaceKeyboardMessages(buffer, num_samples);
}

void SynthBase::processModulationChanges() {
  vital::modulation_change change;
  while (getNextModulationChange(change)) {
    if (change.disconnecting)
      engine_->disconnectModulation(change);
    else
      engine_->connectModulation(change);
  }
}

void SynthBase::updateMemoryOutput(int samples, const vital::poly_float* audio) {
  for (int i = 0; i < samples; ++i)
    audio_memory_->push(audio[i]);

  vital::mono_float last_played = engine_->getLastActiveNote();
  last_played = vital::utils::clamp(last_played, kOutputWindowMinNote, kOutputWindowMaxNote);

  int num_pressed = engine_->getNumPressedNotes();
  int output_inc = std::max<int>(1, engine_->getSampleRate() / vital::kOscilloscopeMemorySampleRate);
  int oscilloscope_samples = 2 * vital::kOscilloscopeMemoryResolution;

  if (last_played && (last_played_note_ != last_played || num_pressed > last_num_pressed_)) {
    last_played_note_ = last_played;
    
    vital::mono_float frequency = vital::utils::midiNoteToFrequency(last_played_note_);
    vital::mono_float period = engine_->getSampleRate() / frequency;
    int window_length = output_inc * vital::kOscilloscopeMemoryResolution;

    memory_reset_period_ = period;
    while (memory_reset_period_ < window_length)
      memory_reset_period_ += memory_reset_period_;

    memory_reset_period_ = std::min(memory_reset_period_, 2.0f * window_length);
    memory_index_ = 0;
    vital::utils::copyBuffer(oscilloscope_memory_, oscilloscope_memory_write_, oscilloscope_samples);
  }
  last_num_pressed_ = num_pressed;

  for (; memory_input_offset_ < samples; memory_input_offset_ += output_inc) {
    int input_index = vital::utils::iclamp(memory_input_offset_, 0, samples);
    memory_index_ = vital::utils::iclamp(memory_index_, 0, oscilloscope_samples - 1);
    VITAL_ASSERT(input_index >= 0);
    VITAL_ASSERT(input_index < samples);
    VITAL_ASSERT(memory_index_ >= 0);
    VITAL_ASSERT(memory_index_ < oscilloscope_samples);
    oscilloscope_memory_write_[memory_index_++] = audio[input_index];

    if (memory_index_ * output_inc >= memory_reset_period_) {
      memory_input_offset_ += memory_reset_period_ - memory_index_ * output_inc;
      memory_index_ = 0;
      vital::utils::copyBuffer(oscilloscope_memory_, oscilloscope_memory_write_, oscilloscope_samples);
    }
  }

  memory_input_offset_ -= samples;
}

void SynthBase::armMidiLearn(const std::string& name) {
  midi_manager_->armMidiLearn(name);
}

void SynthBase::cancelMidiLearn() {
  midi_manager_->cancelMidiLearn();
}

void SynthBase::clearMidiLearn(const std::string& name) {
  midi_manager_->clearMidiLearn(name);
}

bool SynthBase::isMidiMapped(const std::string& name) {
  return midi_manager_->isMidiMapped(name);
}

void SynthBase::setAuthor(const String& author) {
  save_info_["author"] = author;
}

void SynthBase::setComments(const String& comments) {
  save_info_["comments"] = comments;
}

void SynthBase::setStyle(const String& style) {
  save_info_["style"] = style;
}

void SynthBase::setPresetName(const String& preset_name) {
  save_info_["preset_name"] = preset_name;
}

void SynthBase::setMacroName(int index, const String& macro_name) {
  save_info_["macro" + std::to_string(index + 1)] = macro_name;
}

String SynthBase::getAuthor() {
  return save_info_["author"];
}

String SynthBase::getComments() {
  return save_info_["comments"];
}

String SynthBase::getStyle() {
  return save_info_["style"];
}

String SynthBase::getPresetName() {
  return save_info_["preset_name"];
}

String SynthBase::getMacroName(int index) {
  String name = save_info_["macro" + std::to_string(index + 1)];
  if (name.trim().isEmpty())
    return "MACRO " + String(index + 1);
  return name;
}

const vital::StereoMemory* SynthBase::getEqualizerMemory() {
  if (engine_)
    return engine_->getEqualizerMemory();
  return nullptr;
}

vital::ModulationConnectionBank& SynthBase::getModulationBank() {
  return engine_->getModulationBank();
}

void SynthBase::notifyOversamplingChanged() {
  pauseProcessing(true);
  engine_->allSoundsOff();
  checkOversampling();
  pauseProcessing(false);
}

void SynthBase::checkOversampling() {
  return engine_->checkOversampling();
}

void SynthBase::ValueChangedCallback::messageCallback() {
  if (auto synth_base = listener.lock()) {
    SynthGuiInterface* gui_interface = (*synth_base)->getGuiInterface();
    if (gui_interface) {
      gui_interface->updateGuiControl(control_name, value);
      if (control_name != "pitch_wheel")
        gui_interface->notifyChange();
    }
  }
}
