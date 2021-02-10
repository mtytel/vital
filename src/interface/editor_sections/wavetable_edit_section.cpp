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

#include "wavetable_edit_section.h"

#include "skin.h"
#include "paths.h"
#include "filter_section.h"
#include "full_interface.h"
#include "bar_renderer.h"
#include "load_save.h"
#include "oscillator_section.h"
#include "synth_slider.h"
#include "synth_gui_interface.h"
#include "text_look_and_feel.h"
#include "utils.h"
#include "wavetable_playhead_info.h"
#include "wavetable_component_factory.h"
#include "wavetable_overlay_factory.h"
#include "wavetable_component_overlay.h"

namespace {
  void barViewerCallback(int result, WavetableEditSection* edit_section) {
    if (result == WavetableEditSection::kCancel)
      return;

    if (result == WavetableEditSection::kPowerScale)
      edit_section->setPowerScale(true);
    else if (result == WavetableEditSection::kAmplitudeScale)
      edit_section->setPowerScale(false);
    else
      edit_section->setZoom(WavetableEditSection::getZoomScale(result));
  }

  force_inline int chunkNameToData(const char* chunk_name) {
    return static_cast<int>(ByteOrder::littleEndianInt(chunk_name));
  }

  FileSource::FadeStyle getFadeStyleFromWavetableString(String data) {
    if (data.isEmpty())
      return FileSource::kFreqInterpolate;

    if (data.substring(0, 3) == "<!>") {
      StringArray tokens;
      tokens.addTokens(data.substring(3), " ", "");
      if (tokens.size() < 2 || tokens[1].isEmpty())
        return FileSource::kFreqInterpolate;

      char fade_character = tokens[1][0];
      if (fade_character == '0')
        return FileSource::kNoInterpolate;
      if (fade_character == '1')
        return FileSource::kTimeInterpolate;

      return FileSource::kFreqInterpolate;
    }

    return FileSource::kFreqInterpolate;
  }

  String getAuthorFromWavetableString(String data) {
    if (data.substring(0, 3) == "<!>") {
      int start = data.indexOf("[");
      int end = data.indexOf("]");
      if (start < end && start >= 0)
        return data.substring(start + 1, end);
    }

    return "";
  }

  void menuCallback(int result, WavetableEditSection* wavetable_edit_section) {
    if (result == WavetableEditSection::kSaveAsWavetable)
      wavetable_edit_section->saveAsWavetable(); 
    else if (result == WavetableEditSection::kImportWavetable)
      wavetable_edit_section->importWavetable();
    else if (result == WavetableEditSection::kExportWavetable)
      wavetable_edit_section->exportWavetable();
    else if (result == WavetableEditSection::kExportWav)
      wavetable_edit_section->exportToWav();
    else if (result == WavetableEditSection::kResynthesizeWavetable)
      wavetable_edit_section->resynthesizeToWavetable();
  }
}

String WavetableEditSection::getWavetableDataString(InputStream* input_stream) {
  int first_chunk = input_stream->readInt();
  if (first_chunk != chunkNameToData("RIFF"))
    return "";

  int length = input_stream->readInt();
  int data_end = static_cast<int>(input_stream->getPosition()) + length;

  if (input_stream->readInt() != chunkNameToData("WAVE"))
    return "";

  while (!input_stream->isExhausted() && input_stream->getPosition() < data_end) {
    int chunk_label = input_stream->readInt();
    int chunk_length = input_stream->readInt();

    if (chunk_label == chunkNameToData("clm ")) {
      MemoryBlock memory_block;
      input_stream->readIntoMemoryBlock(memory_block, chunk_length);
      return memory_block.toString();
    }

    input_stream->setPosition(input_stream->getPosition() + chunk_length);
  }

  return "";
}

WavetableEditSection::WavetableEditSection(int index, WavetableCreator* wavetable_creator) :
    SynthSection("oscillator " + String(index + 1)), index_(index), zoom_(8),
    power_scale_(true), obscure_time_domain_(false), obscure_freq_amplitude_(false), obscure_freq_phase_(false),
    wavetable_creator_(wavetable_creator) {
  format_manager_.registerBasicFormats();
  current_overlay_ = nullptr;
  int num_bars = vital::WaveFrame::kNumRealComplex;
  int num_frames = vital::kNumOscillatorWaveFrames;
  int waveform_size = vital::Wavetable::kWaveformSize;
  wave_frame_slider_ = nullptr;

  oscillator_waveform_ = std::make_unique<WaveSourceEditor>(waveform_size);
  addOpenGlComponent(oscillator_waveform_.get());
  oscillator_waveform_->setFill(true);
  oscillator_waveform_->addRoundedCorners();

  frequency_amplitudes_ = std::make_unique<BarRenderer>(num_bars);
  addOpenGlComponent(frequency_amplitudes_.get());
  frequency_amplitudes_->setSquareScale(true);
  frequency_amplitudes_->addRoundedCorners();

  frequency_phases_ = std::make_unique<BarRenderer>(num_bars);
  addOpenGlComponent(frequency_phases_.get());
  frequency_phases_->addRoundedCorners();

  wavetable_organizer_ = std::make_unique<WavetableOrganizer>(wavetable_creator_, num_frames);
  addSubSection(wavetable_organizer_.get());
  wavetable_organizer_->addListener(this);

  wavetable_component_list_ = std::make_unique<WavetableComponentList>(wavetable_creator_);
  addSubSection(wavetable_component_list_.get());
  wavetable_component_list_->addListener(this);
  wavetable_component_list_->addListener(wavetable_organizer_.get());

  wavetable_playhead_ = std::make_unique<WavetablePlayhead>(num_frames);
  addSubSection(wavetable_playhead_.get());
  wavetable_playhead_->addListener(wavetable_organizer_.get());
  wavetable_playhead_->addListener(this);

  wavetable_playhead_info_ = std::make_unique<WavetablePlayheadInfo>();
  addAndMakeVisible(wavetable_playhead_info_.get());
  wavetable_playhead_->addListener(wavetable_playhead_info_.get());

  exit_button_ = std::make_unique<OpenGlShapeButton>("Exit");
  addAndMakeVisible(exit_button_.get());
  addOpenGlComponent(exit_button_->getGlComponent());
  exit_button_->addListener(this);
  exit_button_->setShape(Paths::exitX());

  frequency_amplitude_settings_ = std::make_unique<OpenGlShapeButton>("Settings");
  addAndMakeVisible(frequency_amplitude_settings_.get());
  addOpenGlComponent(frequency_amplitude_settings_->getGlComponent());
  frequency_amplitude_settings_->addListener(this);
  frequency_amplitude_settings_->setAlwaysOnTop(true);
  frequency_amplitude_settings_->setShape(Paths::gear());

  preset_selector_ = std::make_unique<PresetSelector>();
  addSubSection(preset_selector_.get());
  preset_selector_->addListener(this);

  menu_button_ = std::make_unique<OpenGlShapeButton>("Menu");
  addAndMakeVisible(menu_button_.get());
  addOpenGlComponent(menu_button_->getGlComponent());
  menu_button_->addListener(this);
  menu_button_->setTriggeredOnMouseDown(true);
  menu_button_->setShape(Paths::menu());

  for (int i = 0; i < WavetableComponentFactory::kNumComponentTypes; ++i) {
    WavetableComponentFactory::ComponentType type = static_cast<WavetableComponentFactory::ComponentType>(i);
    overlays_[i].reset(WavetableOverlayFactory::createOverlay(type));
    overlays_[i]->setParent(this);
    overlays_[i]->addFrameListener(this);
    addSubSection(overlays_[i].get());
    overlays_[i]->setVisible(false);
    wavetable_organizer_->addListener(overlays_[i].get());
  }

  init();

  hideCurrentOverlay();
  obscure_time_domain_ = false;
  setZoom(zoom_);
  setPowerScale(power_scale_);
  wavetable_organizer_->selectDefaultFrame();

  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
  setPresetSelectorText();

  setSkinOverride(Skin::kWavetableEditor);
}

WavetableEditSection::~WavetableEditSection() {
  current_overlay_ = nullptr;
}

Rectangle<int> WavetableEditSection::getFrameEditBounds() {
  static constexpr float kHeightRatio = 0.66f;

  int title_width = getTopHeight();
  int large_padding = findValue(Skin::kLargePadding);
  int height = (getHeight() - title_width) * kHeightRatio;
  return Rectangle<int>(large_padding, title_width, getWidth() - 2 * large_padding, height);
}

Rectangle<int> WavetableEditSection::getTimelineBounds() {
  Rectangle<int> edit_bounds = getFrameEditBounds();
  int large_padding = findValue(Skin::kLargePadding);
  int height = getHeight() - edit_bounds.getBottom() - large_padding - getPadding();
  return Rectangle<int>(large_padding, edit_bounds.getBottom() + large_padding, edit_bounds.getWidth(), height);
}

void WavetableEditSection::paintBackground(Graphics& g) {
  paintBody(g, getFrameEditBounds());
  paintBody(g, getTimelineBounds());

  paintChildrenBackgrounds(g);

  g.saveState();
  Rectangle<int> bounds = getLocalArea(preset_selector_.get(), preset_selector_->getLocalBounds());
  g.reduceClipRegion(bounds);
  g.setOrigin(bounds.getTopLeft());
  preset_selector_->paintBackground(g);
  g.restoreState();
}

void WavetableEditSection::paintBackgroundShadow(Graphics& g) {
  paintTabShadow(g, getFrameEditBounds());
  paintTabShadow(g, getTimelineBounds());
}

void WavetableEditSection::resized() {
  setColors();

  int padding = getPadding();

  if (current_overlay_)
    current_overlay_->setPadding(padding);

  int title_width = getTopHeight();
  int button_height = 20 * getSizeRatio();

  title_bounds_ = Rectangle<int>(0, 0, getWidth(), title_width);

  exit_button_->setBounds(title_bounds_.getRight() - title_width, title_bounds_.getY() + padding,
                          title_width, title_width);

  Rectangle<int> edit_bounds = getFrameEditBounds();
  int widget_margin = getWidgetMargin();
  int edit_x = edit_bounds.getX() + widget_margin;
  int edit_width = edit_bounds.getWidth() - 2 * widget_margin;
  int osc_height = edit_bounds.getHeight() * 0.58f;
  int amp_height = edit_bounds.getHeight() * 0.26f;
  int phase_height = edit_bounds.getHeight() - osc_height - amp_height - 4 * widget_margin;
  oscillator_waveform_->setBounds(edit_x, edit_bounds.getY() + widget_margin, edit_width, osc_height);
  frequency_amplitudes_->setBounds(edit_x, oscillator_waveform_->getBottom() + widget_margin, edit_width, amp_height);
  frequency_amplitude_settings_->setBounds(edit_x, frequency_amplitudes_->getY(), button_height, button_height);
  frequency_phases_->setBounds(edit_x, frequency_amplitudes_->getBottom() + widget_margin, edit_width, phase_height);

  Rectangle<int> timeline_bounds = getTimelineBounds();
  int wavetable_y = timeline_bounds.getY();
  int playhead_height = timeline_bounds.getHeight() * WavetableOrganizer::kHandleHeightPercent;
  int organizer_x = timeline_bounds.getX() + timeline_bounds.getWidth() / 4;
  int organizer_width = timeline_bounds.getRight() - organizer_x;
  int info_width = organizer_x - timeline_bounds.getX();
  wavetable_playhead_info_->setBounds(timeline_bounds.getX(), wavetable_y, info_width, playhead_height);
  wavetable_playhead_->setBounds(organizer_x, wavetable_y, organizer_width, playhead_height);
  wavetable_component_list_->setBounds(timeline_bounds.getX(), wavetable_y + playhead_height,
                                       info_width, timeline_bounds.getHeight() - playhead_height);

  wavetable_organizer_->setBounds(organizer_x, wavetable_y + playhead_height,
                                  organizer_width, timeline_bounds.getHeight() - playhead_height);
  wavetable_playhead_->setPadding(wavetable_organizer_->handleWidth() / 2.0f);
  wavetable_component_list_->setRowHeight(wavetable_organizer_->handleWidth());

  int preset_selector_width = getWidth() / 3;
  int preset_selector_height = title_width * 0.6f;
  int preset_selector_buffer = (title_width - preset_selector_height) * 0.5f;
  int preset_selector_x = (getWidth() - preset_selector_width + 2 * preset_selector_height) / 2;
  preset_selector_->setBounds(preset_selector_x, preset_selector_buffer,
                              preset_selector_width - preset_selector_height, preset_selector_height);
  preset_selector_->setRoundAmount(preset_selector_height / 2.0f);
  
  menu_button_->setBounds(preset_selector_->getRight(), preset_selector_buffer,
                          preset_selector_height, preset_selector_height);
  menu_button_->setShape(Paths::menu(preset_selector_height));

  setOverlayPosition();

  SynthSection::resized();
}

void WavetableEditSection::reset() {
  clear();
  wavetable_organizer_->clear();
  wavetable_component_list_->clear();

  wavetable_organizer_->init();
  wavetable_component_list_->init();
  init();
  if (isVisible())
    wavetable_organizer_->selectDefaultFrame();
  SynthSection::reset();
}

void WavetableEditSection::visibilityChanged() {
  setPresetSelectorText();
  if (isVisible()) {
    if (!wavetable_organizer_->hasSelectedFrames())
      wavetable_organizer_->selectDefaultFrame();

    updateGlDisplay();
  }
}

void WavetableEditSection::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  static constexpr float kMouseWheelSensitivity = 0.75f;
  static constexpr float kMinZoom = 1.0f;
  static constexpr float kMaxZoom = 32.0f;

  Point<int> position = e.getPosition();
  if (frequency_phases_->getBounds().contains(position) || frequency_amplitudes_->getBounds().contains(position)) {
    zoom_ *= std::pow(2.0f, kMouseWheelSensitivity * wheel.deltaY);
    zoom_ = std::max(std::min(zoom_, kMaxZoom), kMinZoom);

    frequency_amplitudes_->setScale(zoom_);
    frequency_phases_->setScale(zoom_);
    if (current_overlay_)
      current_overlay_->setFrequencyZoom(zoom_);
  }
}

void WavetableEditSection::playheadMoved(int position) {
  updateGlDisplay();
  if (wave_frame_slider_)
    wave_frame_slider_->setValue(position);
}

void WavetableEditSection::frameDoneEditing() {
  render();
}

void WavetableEditSection::frameChanged() {
  int max_frame = std::max(0, wavetable_creator_->getWavetable()->numFrames() - 1);
  int position = std::min(max_frame, wavetable_playhead_->position());
  render(position);
}

void WavetableEditSection::prevClicked() {
  File wavetable_file = LoadSave::getShiftedFile(LoadSave::kWavetableFolderName, vital::kWavetableExtensionsList,
                                                 LoadSave::kAdditionalWavetableFoldersName, getCurrentFile(), -1);
  if (wavetable_file.exists())
    loadFile(wavetable_file);
}

void WavetableEditSection::nextClicked() {
  File wavetable_file = LoadSave::getShiftedFile(LoadSave::kWavetableFolderName, vital::kWavetableExtensionsList,
                                                 LoadSave::kAdditionalWavetableFoldersName, getCurrentFile(), 1);
  if (wavetable_file.exists())
    loadFile(wavetable_file);
}

void WavetableEditSection::textMouseDown(const MouseEvent& e) {
  static constexpr int kBrowserWidth = 600;
  static constexpr int kBrowserHeight = 400;
  Rectangle<int> bounds(preset_selector_->getX(), preset_selector_->getBottom(),
                        kBrowserWidth * size_ratio_, kBrowserHeight * size_ratio_);
  bounds = getLocalArea(this, bounds);
  showPopupBrowser(this, bounds, LoadSave::getWavetableDirectories(), vital::kWavetableExtensionsList,
                   LoadSave::kWavetableFolderName, LoadSave::kAdditionalWavetableFoldersName);
}

void WavetableEditSection::setPresetSelectorText() {
  std::string name = wavetable_creator_->getName();
  std::string author = wavetable_creator_->getAuthor();
  if (author.size() == 0)
    preset_selector_->setText(name);
  else
    preset_selector_->setText(name, "-", author);
}

void WavetableEditSection::showPopupMenu() {
  PopupItems options;

  options.addItem(kSaveAsWavetable, "Save As Wavetable");
  options.addItem(kImportWavetable, "Import Wavetable");
  options.addItem(kExportWavetable, "Export Wavetable");
  options.addItem(kExportWav, "Export to .wav File");
  options.addItem(kResynthesizeWavetable, "Synthesize Preset to Table");

  showPopupSelector(this, Point<int>(menu_button_->getX(), menu_button_->getBottom()), options,
                                     [=](int selection) { menuCallback(selection, this); });
}

void WavetableEditSection::hideCurrentOverlay() {
  if (current_overlay_)
    current_overlay_->setVisible(false);

  current_overlay_ = nullptr;
  obscure_time_domain_ = false;
  obscure_freq_amplitude_ = false;
  obscure_freq_phase_ = false;
}

void WavetableEditSection::clearOverlays() {
  hideCurrentOverlay();

  for (int i = 0; i < WavetableComponentFactory::kNumComponentTypes; ++i) {
    overlays_[i]->setVisible(false);
    overlays_[i]->reset();
  }

  type_lookup_.clear();
}

void WavetableEditSection::setColors() {
  Colour primary_color = findColour(Skin::kWidgetPrimaryDisabled, true);
  Colour background = primary_color.withAlpha(0.0f);
  Colour secondary_color = findColour(Skin::kWidgetSecondaryDisabled, true);

  float fade_alpha = 1.0f - findValue(Skin::kWidgetFillFade);
  if (obscure_time_domain_) {
    oscillator_waveform_->setColor(primary_color.interpolatedWith(background, kObscureAmount));

    Colour fill_color = secondary_color.interpolatedWith(background, kObscureAmount);
    oscillator_waveform_->setFillColors(fill_color.withMultipliedAlpha(fade_alpha), fill_color);
  }
  else {
    oscillator_waveform_->setColor(primary_color);
    oscillator_waveform_->setFillColors(secondary_color.withMultipliedAlpha(fade_alpha), secondary_color);
  }

  if (obscure_freq_amplitude_)
    frequency_amplitudes_->setColor(secondary_color.interpolatedWith(background, kObscureAmount));
  else
    frequency_amplitudes_->setColor(secondary_color);

  if (obscure_freq_phase_)
    frequency_phases_->setColor(secondary_color.interpolatedWith(background, kObscureAmount));
  else
    frequency_phases_->setColor(secondary_color);
}

void WavetableEditSection::render() {
  wavetable_creator_->render();
  updateGlDisplay();
}

void WavetableEditSection::render(int position) {
  wavetable_creator_->render(position);
  updateGlDisplay();
}

void WavetableEditSection::updateGlDisplay() {
  int position = wavetable_playhead_->position();
  VITAL_ASSERT(position >= 0 && position <= vital::kNumOscillatorWaveFrames);
  updateTimeDomain(wavetable_creator_->getWavetable()->getBuffer(position));
  updateFrequencyDomain(wavetable_creator_->getWavetable()->getBuffer(position));
}

void WavetableEditSection::setOverlayPosition() {
  int edit_height = frequency_amplitudes_->getHeight() * 0.33f;
  edit_bounds_ = Rectangle<int>(0, oscillator_waveform_->getBottom() + getPadding(), getWidth(), edit_height);

  if (current_overlay_) {
    current_overlay_->setBounds(0, 0, getWidth(), wavetable_playhead_->getY());
    obscure_time_domain_ = current_overlay_->setTimeDomainBounds(oscillator_waveform_->getBounds());
    obscure_freq_amplitude_ = current_overlay_->setFrequencyAmplitudeBounds(frequency_amplitudes_->getBounds());
    obscure_freq_phase_ = current_overlay_->setPhaseBounds(frequency_phases_->getBounds());
    current_overlay_->setPadding(getPadding());
    current_overlay_->setEditBounds(edit_bounds_);
  }
}

void WavetableEditSection::updateTimeDomain(float* time_domain) {
  oscillator_waveform_->loadWaveform(time_domain);
}

void WavetableEditSection::updateFrequencyDomain(float* time_domain) {
  static constexpr float kAmplitudeEpsilon = 0.0000001f;
  static constexpr float kPhaseEpsilon = 0.0001f;
  compute_frame_.loadTimeDomain(time_domain);

  for (int i = 0; i < vital::WaveFrame::kWaveformSize / 2; ++i) {
    std::complex<float> val = compute_frame_.frequency_domain[i];
    float amplitude = std::abs(val) / vital::WaveFrame::kWaveformSize;
    float phase = amplitude > kAmplitudeEpsilon ? std::arg(val) : -vital::kPi / 2.0f;
    frequency_amplitudes_->setScaledY(i, amplitude);
    if (phase >= vital::kPi - kPhaseEpsilon)
      phase = -vital::kPi;
    frequency_phases_->setY(i, phase / vital::kPi);
  }
}

int WavetableEditSection::loadAudioFile(AudioSampleBuffer& destination, InputStream* audio_stream) {
  audio_stream->setPosition(0);
  std::unique_ptr<AudioFormatReader> format_reader(
      format_manager_.createReaderFor(std::unique_ptr<InputStream>(audio_stream)));
  if (format_reader == nullptr)
    return 0;

  int num_samples = static_cast<int>(format_reader->lengthInSamples);
  destination.setSize(format_reader->numChannels, num_samples);
  format_reader->read(&destination, 0, num_samples, 0, true, true);
  return format_reader->sampleRate;
}

void WavetableEditSection::componentAdded(WavetableComponent* component) {
  hideCurrentOverlay();

  WavetableComponentFactory::ComponentType type = component->getType();
  type_lookup_[component] = type;
  current_overlay_ = overlays_[type].get();
  current_overlay_->setComponent(component);
  current_overlay_->setVisible(true);

  current_overlay_->setPadding(getPadding());
  current_overlay_->setPowerScale(power_scale_);
  current_overlay_->setFrequencyZoom(zoom_);
  setOverlayPosition();
}

void WavetableEditSection::componentRemoved(WavetableComponent* component) {
  type_lookup_.erase(component);

  for (int i = 0; i < WavetableComponentFactory::kNumComponentTypes; ++i) {
    if (overlays_[i]->getComponent() == component) {
      overlays_[i]->setVisible(false);
      overlays_[i]->resetOverlay();
      overlays_[i]->reset();
      if (overlays_[i].get() == current_overlay_)
        hideCurrentOverlay();
    }
  }
}

void WavetableEditSection::componentsChanged() {
  render();
}

void WavetableEditSection::loadDefaultWavetable() {
  wavetable_creator_->init();
  reset();
}

void WavetableEditSection::saveAsWavetable() {
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  if (parent)
    parent->saveWavetable(index_);
}

void WavetableEditSection::importWavetable() {
  FileChooser open_box("Import Wavetable", File(), vital::kWavetableExtensionsList);
  if (open_box.browseForFileToOpen()) {
    if (!open_box.getResult().exists())
      return;
    loadFile(open_box.getResult());
  }
}

void WavetableEditSection::exportWavetable() {
  FileChooser save_box("Export Wavetable", File(), String("*.") + vital::kWavetableExtension);
  if (save_box.browseForFileToSave(true)) {
    json wavetable_data = wavetable_creator_->stateToJson();
    File file = save_box.getResult().withFileExtension(vital::kWavetableExtension);
    file.replaceWithText(wavetable_data.dump());
  }
}

void WavetableEditSection::exportToWav() {
  static constexpr int kWavetableSampleRate = 88200;
  static constexpr int kNumWaveframes = 256;
  
  FileChooser save_box("Export to .wav File", File(), String("*.wav"));
  if (!save_box.browseForFileToSave(true))
    return;

  File file = save_box.getResult().withFileExtension("wav");
  if (!file.hasWriteAccess())
    return;

  file.deleteFile();
  std::unique_ptr<FileOutputStream> file_stream = file.createOutputStream();
  WavAudioFormat wav_format;
  StringPairArray meta_data;
  meta_data.set("clm ", "<!>2048 20000000 wavetable (vital.audio)");
  std::unique_ptr<AudioFormatWriter> writer(wav_format.createWriterFor(file_stream.get(), kWavetableSampleRate,
                                                                       1, 16, meta_data, 0));

  int total_samples = vital::WaveFrame::kWaveformSize * kNumWaveframes;
  std::unique_ptr<float[]> buffer = std::make_unique<float[]>(total_samples);
  wavetable_creator_->renderToBuffer(buffer.get(), kNumWaveframes, vital::WaveFrame::kWaveformSize);

  float* channel = buffer.get();
  writer->writeFromFloatArrays(&channel, 1, total_samples);
  writer->flush();
  file_stream->flush();

  writer = nullptr;
  file_stream.release();
}

void WavetableEditSection::loadFile(const File& wavetable_file) {
  clear();
  if (wavetable_file.getFileExtension() == ".wav") {
    FileInputStream* input_stream = new FileInputStream(wavetable_file);
    loadAudioAsWavetable(wavetable_file.getFileNameWithoutExtension(), input_stream,
                         WavetableCreator::kWavetableSplice);
  }
  else {
    String data_string = wavetable_file.loadFileAsString();

    try {
      json wavetable_data = json::parse(data_string.toStdString(), nullptr, false);
      wavetable_creator_->jsonToState(wavetable_data);
    }
    catch (const json::exception& e) {
      return;
    }

    wavetable_creator_->setName(wavetable_file.getFileNameWithoutExtension().toStdString());
  }

  setPresetSelectorText();
  std::string path = wavetable_file.getFullPathName().toStdString();
  wavetable_creator_->setFileLoaded(path);
  reset();
  render();
}

void WavetableEditSection::loadWavetable(json& wavetable_data) {
  clear();
  try {
    wavetable_creator_->jsonToState(wavetable_data);
  }
  catch (const json::exception& e) { }
  reset();
}

bool WavetableEditSection::loadAudioAsWavetable(String name, InputStream* audio_stream,
                                                WavetableCreator::AudioFileLoadStyle style) {
  AudioSampleBuffer sample_buffer;
  String wavetable_string = getWavetableDataString(audio_stream);
  int sample_rate = loadAudioFile(sample_buffer, audio_stream);
  if (sample_rate == 0)
    return false;

  FileSource::FadeStyle fade_style = getFadeStyleFromWavetableString(wavetable_string);
  clear();
  wavetable_creator_->initFromAudioFile(sample_buffer.getReadPointer(0), sample_buffer.getNumSamples(),
                                        sample_rate, style, fade_style);
  wavetable_creator_->setName(name.toStdString());
  wavetable_creator_->setAuthor(getAuthorFromWavetableString(wavetable_string).toStdString());
  reset();
  return true;
}

void WavetableEditSection::resynthesizeToWavetable() {
  static constexpr float kResynthesizeTime = 4.0f;
  static constexpr int kResynthesizeNote = 16;

  SynthGuiInterface* synth_interface = findParentComponentOfClass<SynthGuiInterface>();
  if (synth_interface == nullptr)
    return;

  int sample_rate = synth_interface->getSynth()->getSampleRate();
  int total_samples = sample_rate * kResynthesizeTime;
  std::unique_ptr<float[]> data = std::make_unique<float[]>(total_samples);

  synth_interface->getSynth()->renderAudioForResynthesis(data.get(), total_samples, kResynthesizeNote);
  clear();
  wavetable_creator_->initFromAudioFile(data.get(), total_samples, sample_rate,
                                        WavetableCreator::kPitched, FileSource::kWaveBlend);
  wavetable_creator_->setName("Resynthesize");
  FileSource* file_source = dynamic_cast<FileSource*>(wavetable_creator_->getGroup(0)->getComponent(0));
  if (file_source)
    file_source->setWindowSize(sample_rate / vital::utils::midiNoteToFrequency(kResynthesizeNote));
  wavetable_creator_->render();
  reset();
}

void WavetableEditSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == menu_button_.get())
    showPopupMenu();
  else if (clicked_button == exit_button_.get()) {
    FullInterface* parent = findParentComponentOfClass<FullInterface>();
    if (parent)
      parent->hideWavetableEditSection();
  }
  else if (clicked_button == frequency_amplitude_settings_.get()) {
    PopupItems options;
    options.addItem(kPowerScale, "Power Scale");
    options.addItem(kAmplitudeScale, "Amplitude Scale");
    options.addItem(-1, "");
    options.addItem(kZoom1, "Zoom 1x");
    options.addItem(kZoom2, "Zoom 2x");
    options.addItem(kZoom4, "Zoom 4x");
    options.addItem(kZoom8, "Zoom 8x");
    options.addItem(kZoom16, "Zoom 16x");

    showPopupSelector(this, Point<int>(clicked_button->getX(), clicked_button->getBottom()), options,
                      [=](int selection) { barViewerCallback(selection, this); });
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void WavetableEditSection::positionsUpdated() {
  render();
}

void WavetableEditSection::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe) {
    WavetableComponent* component = keyframe->owner();
    if (current_overlay_ && current_overlay_->getComponent() == component)
      return;

    WavetableComponentFactory::ComponentType type = component->getType();

    current_overlay_ = overlays_[type].get();
    current_overlay_->setComponent(component);
    current_overlay_->setVisible(true);
    current_overlay_->setPadding(getPadding());
    current_overlay_->setPowerScale(power_scale_);
    current_overlay_->setFrequencyZoom(zoom_);
    setOverlayPosition();
  }
  else
    hideCurrentOverlay();
}

void WavetableEditSection::frameDragged(WavetableKeyframe* keyframe, int position) {
  wavetable_playhead_->setPosition(position);
}

void WavetableEditSection::wheelMoved(const MouseEvent& e, const MouseWheelDetails& wheel) {
  wavetable_component_list_->scroll(e, wheel);
}

void WavetableEditSection::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  setColors();
  SynthSection::renderOpenGlComponents(open_gl, animate);

  WavetableComponentOverlay* current_overlay = current_overlay_;
  if (current_overlay) {
    if (!current_overlay->initialized())
      current_overlay->initOpenGlComponents(open_gl);

    current_overlay->renderOpenGlComponents(open_gl, animate);
  }

  oscillator_waveform_->renderCorners(open_gl, animate);
  frequency_amplitudes_->renderCorners(open_gl, animate);
  frequency_phases_->renderCorners(open_gl, animate);
}

void WavetableEditSection::setPowerScale(bool power_scale) {
  power_scale_ = power_scale;
  frequency_amplitudes_->setPowerScale(power_scale_);
  if (current_overlay_)
    current_overlay_->setPowerScale(power_scale_);
}

void WavetableEditSection::setZoom(int zoom) {
  zoom_ = zoom;
  frequency_amplitudes_->setScale(zoom_);
  frequency_phases_->setScale(zoom_);
  if (current_overlay_)
    current_overlay_->setFrequencyZoom(zoom_);
}

void WavetableEditSection::clear() {
  clearOverlays();
}

void WavetableEditSection::init() {
  for (int g = 0; g < wavetable_creator_->numGroups(); ++g) {
    WavetableGroup* group = wavetable_creator_->getGroup(g);
    for (int i = 0; i < group->numComponents(); ++i)
      componentAdded(group->getComponent(i));
  }
  hideCurrentOverlay();
}
