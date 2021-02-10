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

#include "file_source_overlay.h"

#include "audio_file_drop_source.h"
#include "fonts.h"
#include "skin.h"
#include "text_look_and_feel.h"
#include "tuning.h"
#include "wave_frame.h"
#include "sample_viewer.h"

namespace {
  const std::string kFadeLookup[] = {
    "File Blend",
    "None",
    "Time",
    "Spectral",
  };
  const std::string kPhaseLookup[] = {
    "None",
    "Clear",
    "Vocode",
  };

  float windowTextToSize(String text, int sample_rate) {
    static constexpr float kMaxWindowSize = 9999.9f;
    String trimmed = text.trim();
    int note_midi = Tuning::noteToMidiKey(text);
    if (note_midi < 0 || note_midi >= vital::kMidiSize)
      return vital::utils::clamp(trimmed.getFloatValue(), 1.0f, kMaxWindowSize);
    return sample_rate / vital::utils::midiNoteToFrequency(note_midi);
  }
    
  float positionTextToSize(String text) {
    return text.trim().getFloatValue();
  }
}

AudioFileViewer::AudioFileViewer() : SynthSection("Audio File"), top_(kResolution), bottom_(kResolution),
                                     dragging_quad_(Shaders::kRoundedRectangleFragment), sample_rate_(0),
                                     file_source_(nullptr) {
  window_position_ = 0.0f;
  window_size_ = 1.0f;
  window_fade_ = 0.0f;

  addOpenGlComponent(&top_);
  addOpenGlComponent(&bottom_);
  addOpenGlComponent(&dragging_quad_);
  top_.setInterceptsMouseClicks(false, false);
  bottom_.setInterceptsMouseClicks(false, false);

  top_.setFill(true);
  bottom_.setFill(true);

  dragging_quad_.setTargetComponent(this);
}

void AudioFileViewer::resized() {
  static constexpr float kBuffer = 0.1f;
  static constexpr float kCenterAlpha = 0.1f;

  int buffer = getHeight() * kBuffer;
  Rectangle<int> bounds(0, buffer, getWidth(), getHeight() - 2 * buffer);
  top_.setBounds(bounds);
  bottom_.setBounds(bounds);

  top_.setLineWidth(3.0f);
  bottom_.setLineWidth(3.0f);
  Colour line = findColour(Skin::kWidgetPrimary1, true);
  Colour fill = findColour(Skin::kWidgetSecondary1, true).withAlpha(kCenterAlpha);
  top_.setColor(line);
  bottom_.setColor(line);
  top_.setFillColor(fill);
  bottom_.setFillColor(fill);
  dragging_quad_.setColor(findColour(Skin::kOverlayScreen, true));

  float line_boost = findValue(Skin::kWidgetLineBoost);
  top_.setBoostAmount(line_boost);
  bottom_.setBoostAmount(line_boost);

  top_.setFillBoostAmount(1.0f / kCenterAlpha);
  bottom_.setFillBoostAmount(1.0f / kCenterAlpha);

  float delta = getWidth() / (kResolution - 1.0f);
  for (int i = 0; i < kResolution; ++i) {
    top_.setXAt(i, delta * i);
    bottom_.setXAt(i, delta * i);
  }

  setAudioPositions();
}

void AudioFileViewer::clearAudioPositions() {
  float center = top_.getHeight() * 0.5f;
  for (int i = 0; i < kResolution; ++i) {
    top_.setYAt(i, center);
    bottom_.setYAt(i, center);
  }
}

void AudioFileViewer::setAudioPositions() {
  if (file_source_ == nullptr) {
    clearAudioPositions();
    return;
  }

  const FileSource::SampleBuffer* sample_buffer = file_source_->buffer();
  if (sample_buffer->size == 0 || sample_buffer->data == nullptr) {
    clearAudioPositions();
    return;
  }

  const float* buffer = sample_buffer->data.get();
  int sample_length = sample_buffer->size;

  float center = top_.getHeight() * 0.5f;
  for (int i = 0; i < kResolution; ++i) {
    int start_index = std::min<int>(sample_length * i / kResolution, sample_length);
    int end_index = std::min<int>((sample_length * (i + 1) + kResolution - 1) / kResolution, sample_length);
    float max = buffer[start_index];
    for (int i = start_index + 1; i < end_index; ++i)
      max = std::max(max, buffer[i]);
    top_.setYAt(i, center - max * center);
    bottom_.setYAt(i, center + max * center);
  }

  setWindowValues();
}

void AudioFileViewer::setWindowValues() {
  if (file_source_ == nullptr)
    return;

  const FileSource::SampleBuffer* sample_buffer = file_source_->buffer();
  if (sample_buffer->size == 0 || sample_buffer->data == nullptr)
    return;

  window_size_ = file_source_->getWindowSize() / sample_buffer->size;

  float fade_length = std::max(window_fade_ * window_size_, 1.0f / kResolution);
  float start = window_position_ - fade_length * 0.5f;
  float end = window_position_ + window_size_ + fade_length * 0.5f;

  for (int i = 0; i < kResolution; ++i) {
    float position = i / (kResolution - 1.0f);
    float window_phase = std::min(position - start, end - position) / fade_length;
    window_phase = std::max(std::min(window_phase, 1.0f), 0.0f) * vital::kPi;
    float window_value = 0.5f - cosf(window_phase) * 0.5f;

    top_.setBoostLeft(i, window_value);
    bottom_.setBoostLeft(i, window_value);
  }
}

void AudioFileViewer::audioFileLoaded(const File& file) {
  static constexpr int kMaxFileSamples = 176400;
  std::unique_ptr<AudioFormatReader> format_reader(format_manager_.createReaderFor(file));

  if (format_reader) {
    int num_samples = (int)std::min<long long>(format_reader->lengthInSamples, kMaxFileSamples);
    sample_buffer_.setSize(format_reader->numChannels, num_samples);
    format_reader->read(&sample_buffer_, 0, num_samples, 0, true, true);
  }

  dragging_quad_.setActive(false);
}

void AudioFileViewer::fileDragEnter(const StringArray& files, int x, int y) {
  dragging_quad_.setActive(true);
}

void AudioFileViewer::fileDragExit(const StringArray& files) {
  dragging_quad_.setActive(false);
}

float AudioFileViewer::updateMousePosition(Point<float> position) {
  float ratio = (position.x - last_mouse_position_.x) / getWidth();
  last_mouse_position_ = position;
  return ratio;
}

void AudioFileViewer::mouseDown(const MouseEvent& e) {
  updateMousePosition(e.position);
}

void AudioFileViewer::mouseDrag(const MouseEvent& e) {
  float ratio = updateMousePosition(e.position);

  for (DragListener* listener : drag_listeners_)
    listener->positionMovedRelative(ratio, false);
}

void AudioFileViewer::mouseUp(const MouseEvent& e) {
  float ratio = updateMousePosition(e.position);

  for (DragListener* listener : drag_listeners_)
    listener->positionMovedRelative(ratio, true);
}

FileSourceOverlay::FileSourceOverlay() : WavetableComponentOverlay("FILE SOURCE"), file_source_(nullptr) {
  current_frame_ = nullptr;
  load_button_ = std::make_unique<TextButton>("LOAD");
  addAndMakeVisible(load_button_.get());
  load_button_->addListener(this);
  load_button_->setLookAndFeel(TextLookAndFeel::instance());
  load_button_->setButtonText("LOAD");

  fade_style_ = std::make_unique<TextSelector>("Fade Style");
  addSlider(fade_style_.get());
  fade_style_->setAlwaysOnTop(true);
  fade_style_->getImageComponent()->setAlwaysOnTop(true);
  fade_style_->addListener(this);
  fade_style_->setRange(0, FileSource::kNumFadeStyles - 1, 1);
  fade_style_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  fade_style_->setLookAndFeel(TextLookAndFeel::instance());
  fade_style_->setStringLookup(kFadeLookup);
  fade_style_->setLongStringLookup(kFadeLookup);

  phase_style_ = std::make_unique<TextSelector>("Phase Style");
  addSlider(phase_style_.get());
  phase_style_->setAlwaysOnTop(true);
  phase_style_->getImageComponent()->setAlwaysOnTop(true);
  phase_style_->addListener(this);
  phase_style_->setRange(0, FileSource::kNumPhaseStyles - 1, 1);
  phase_style_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  phase_style_->setStringLookup(kPhaseLookup);
  phase_style_->setLookAndFeel(TextLookAndFeel::instance());
  phase_style_->setLongStringLookup(kPhaseLookup);

  normalize_gain_ = std::make_unique<OpenGlToggleButton>("NORMALIZE");
  addAndMakeVisible(normalize_gain_.get());
  addOpenGlComponent(normalize_gain_->getGlComponent());
  normalize_gain_->setAlwaysOnTop(true);
  normalize_gain_->getGlComponent()->setAlwaysOnTop(true);
  normalize_gain_->setNoBackground();
  normalize_gain_->setLookAndFeel(TextLookAndFeel::instance());
  normalize_gain_->addListener(this);

  audio_thumbnail_ = std::make_unique<AudioFileViewer>();
  addSubSection(audio_thumbnail_.get());
  audio_thumbnail_->setAlwaysOnTop(true);
  audio_thumbnail_->addListener(this);
  audio_thumbnail_->addDragListener(this);

#if !defined(NO_TEXT_ENTRY)
  start_position_ = std::make_unique<OpenGlTextEditor>("Start Position");
  addAndMakeVisible(start_position_.get());
  addOpenGlComponent(start_position_->getImageComponent());
  start_position_->setAlwaysOnTop(true);
  start_position_->getImageComponent()->setAlwaysOnTop(true);
  start_position_->addListener(this);
  start_position_->setLookAndFeel(TextLookAndFeel::instance());
  start_position_->setJustification(Justification::centred);

  window_size_ = std::make_unique<OpenGlTextEditor>("Window Size");
  addAndMakeVisible(window_size_.get());
  addOpenGlComponent(window_size_->getImageComponent());
  window_size_->setAlwaysOnTop(true);
  window_size_->getImageComponent()->setAlwaysOnTop(true);
  window_size_->addListener(this);
  window_size_->setLookAndFeel(TextLookAndFeel::instance());
  window_size_->setJustification(Justification::centred);
#endif

  window_fade_ = std::make_unique<SynthSlider>("File Source Window Fade");
  addSlider(window_fade_.get());
  window_fade_->setAlwaysOnTop(true);
  window_fade_->getImageComponent()->setAlwaysOnTop(true);
  window_fade_->addListener(this);
  window_fade_->setRange(0.0f, 1.0f);
  window_fade_->setDoubleClickReturnValue(true, 1.0f);
  window_fade_->setLookAndFeel(TextLookAndFeel::instance());
  window_fade_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  controls_background_.clearTitles();
  controls_background_.addTitle("");
  controls_background_.addTitle("POSITION");
  controls_background_.addTitle("WINDOW SIZE");
  controls_background_.addTitle("WINDOW FADE");
  controls_background_.addTitle("BLEND STYLE");
  controls_background_.addTitle("PHASE STYLE");
  controls_background_.addTitle("");
}

FileSourceOverlay::~FileSourceOverlay() { }

void FileSourceOverlay::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe == nullptr)
    current_frame_ = nullptr;
  else if (keyframe->owner() == file_source_) {
    current_frame_ = file_source_->getKeyframe(keyframe->index());
    
    double start_position = current_frame_->getStartPosition();
    double window_fade = current_frame_->getWindowFade();
    double window_size = file_source_->getWindowSize();

    if (start_position_)
      start_position_->setText(String(start_position, 1));
    window_fade_->setValue(window_fade, dontSendNotification);

    if (window_size_)
      window_size_->setText(String(window_size, 1));

    double num_samples = file_source_->buffer()->size;
    if (num_samples) {
      audio_thumbnail_->setWindowPosition(start_position / num_samples);
      audio_thumbnail_->setWindowSize(window_size / num_samples);
    }
    else {
      audio_thumbnail_->setWindowPosition(0.0f);
      audio_thumbnail_->setWindowSize(1.0f);
    }
    audio_thumbnail_->setWindowFade(window_fade);

    normalize_gain_->setToggleState(file_source_->getNormalizeGain(), dontSendNotification);
    fade_style_->setValue(file_source_->getFadeStyle(), dontSendNotification);
    phase_style_->setValue(file_source_->getPhaseStyle(), dontSendNotification);
    fade_style_->redoImage();
    phase_style_->redoImage();
  }
}

void FileSourceOverlay::setEditBounds(Rectangle<int> bounds) {
  static constexpr float kTextBoxWidthHeightRatio = 2.5f;
  static constexpr float kWindowFadeWidthHeightRatio = 3.0f;
  static constexpr float kSelectorWidthHeightRatio = 3.0f;
  static constexpr float kNormalizeWidthHeightRatio = 2.5f;
  static constexpr float kBorderWidthHeightRatio = 1.5f;

  if (bounds.getWidth() <= 0)
    return;

  int padding = getPadding();
  int text_box_width = bounds.getHeight() * kTextBoxWidthHeightRatio;
  int window_fade_width = bounds.getHeight() * kWindowFadeWidthHeightRatio;
  int selector_width = bounds.getHeight() * kSelectorWidthHeightRatio;
  int normalize_width = bounds.getHeight() * kNormalizeWidthHeightRatio;
  int border_width = bounds.getHeight() * kBorderWidthHeightRatio;
  int total_width = bounds.getWidth() - 2 * border_width;
  int audio_width = total_width - normalize_width - 2 * selector_width - window_fade_width -
                    2 * text_box_width - 6 * padding;

  setControlsWidth(total_width);
  WavetableComponentOverlay::setEditBounds(bounds);

  int title_height = WavetableComponentOverlay::kTitleHeightRatio * bounds.getHeight();
  int x = border_width;
  int y = bounds.getY();
  int y_title = y + title_height;
  int height = bounds.getHeight();
  int height_title = height - title_height;
  phase_style_->setTextHeightPercentage(0.4f);
  fade_style_->setTextHeightPercentage(0.4f);
  audio_thumbnail_->setBounds(x + 1, y + 1, audio_width - 1, height - 2);

  int edit_x = audio_thumbnail_->getRight() + padding;
  if (start_position_) {
    start_position_->setBounds(edit_x, y_title, text_box_width, height_title - 1);
    window_size_->setBounds(start_position_->getRight() + padding, y_title, text_box_width, height_title - 1);
    edit_x = window_size_->getRight() + padding;
  }
  window_fade_->setBounds(edit_x, y_title, window_fade_width, height_title);
  fade_style_->setBounds(window_fade_->getRight() + padding, y_title, selector_width, height_title);
  phase_style_->setBounds(fade_style_->getRight() + padding, y_title, selector_width, height_title);
  int normalize_padding = height / 6;
  normalize_gain_->setBounds(phase_style_->getRight() + padding, y + normalize_padding,
                             normalize_width, height - 2 * normalize_padding);

  controls_background_.clearLines();
  controls_background_.addLine(audio_width);
  controls_background_.addLine(audio_width + text_box_width + padding);
  controls_background_.addLine(audio_width + 2 * text_box_width + 2 * padding);
  controls_background_.addLine(audio_width + 2 * text_box_width + window_fade_width + 3 * padding);
  controls_background_.addLine(audio_width + 2 * text_box_width + window_fade_width + selector_width + 4 * padding);
  controls_background_.addLine(audio_width + 2 * text_box_width + window_fade_width + 2 * selector_width + 5 * padding);

  if (start_position_ && window_size_) {
    setTextEditorVisuals(window_size_.get(), height_title);
    setTextEditorVisuals(start_position_.get(), height_title);

    start_position_->redoImage();
    window_size_->redoImage();
  }
  window_fade_->redoImage();
  fade_style_->redoImage();
  phase_style_->redoImage();
}

void FileSourceOverlay::sliderValueChanged(Slider* moved_slider) {
  if (current_frame_ == nullptr || file_source_ == nullptr)
    return;

  if (moved_slider == window_fade_.get()) {
    current_frame_->setWindowFade(window_fade_->getValue());
    audio_thumbnail_->setWindowFade(window_fade_->getValue());
  }

  if (moved_slider == fade_style_.get())
    file_source_->setFadeStyle((FileSource::FadeStyle)static_cast<int>(fade_style_->getValue()));

  if (moved_slider == phase_style_.get())
    file_source_->setPhaseStyle((FileSource::PhaseStyle)static_cast<int>(phase_style_->getValue()));

  notifyChanged(moved_slider == fade_style_.get() || moved_slider == phase_style_.get());
}

void FileSourceOverlay::sliderDragEnded(Slider* moved_slider) {
  notifyChanged(true);
}

void FileSourceOverlay::loadFile(const File& file) {
  if (!file.exists() || file_source_ == nullptr)
    return;

  audio_thumbnail_->audioFileLoaded(file);
  AudioSampleBuffer* sample_buffer = audio_thumbnail_->getSampleBuffer();
  int sample_rate = audio_thumbnail_->getSampleRate();
  file_source_->loadBuffer(sample_buffer->getReadPointer(0, 0), sample_buffer->getNumSamples(), sample_rate);
  file_source_->detectPitch();
  audio_thumbnail_->setAudioPositions();

  clampStartingPosition();
  if (start_position_)
    textEditorReturnKeyPressed(*start_position_);

  notifyChanged(true);
}

void FileSourceOverlay::loadFilePressed() {
  FileChooser load_file("Load Audio File", File::getSpecialLocation(File::userHomeDirectory), String("*.wav"));
  if (load_file.browseForFileToOpen())
    loadFile(load_file.getResult());
}

void FileSourceOverlay::buttonClicked(Button* clicked_button) {
  if (file_source_ == nullptr)
    return;
  
  if (clicked_button == load_button_.get())
    loadFilePressed();
  else if (clicked_button == normalize_gain_.get()) {
    file_source_->setNormalizeGain(normalize_gain_->getToggleState());
    notifyChanged(true);
  }
}

void FileSourceOverlay::audioFileLoaded(const File& file) {
  loadFile(file);
}

void FileSourceOverlay::textEditorReturnKeyPressed(TextEditor& text_editor) {
  if (&text_editor == window_size_.get())
    loadWindowSizeText();
  else if (&text_editor == start_position_.get())
    loadStartingPositionText();
}

void FileSourceOverlay::textEditorFocusLost(TextEditor& text_editor) {
  if (&text_editor == window_size_.get())
    loadWindowSizeText();
  else if (&text_editor == start_position_.get())
    loadStartingPositionText();
}

void FileSourceOverlay::positionMovedRelative(float ratio, bool mouse_up) {
  if (file_source_ == nullptr)
    return;
  
  int num_samples = file_source_->buffer()->size;
  float max_position = num_samples - file_source_->getWindowSize();
  float position = 0.0f;
  if (start_position_)
    position = positionTextToSize(start_position_->getText());
  position += ratio * max_position;
  position = std::max(0.0f, std::min(max_position, position));
  if (start_position_)
    start_position_->setText(String(position, 1), false);

  if (num_samples && current_frame_) {
    current_frame_->setStartPosition(position);
    audio_thumbnail_->setWindowPosition(position / num_samples);
  }

  notifyChanged(mouse_up);
}

void FileSourceOverlay::setTextEditorVisuals(TextEditor* text_editor, float height) {
  text_editor->setColour(CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
  text_editor->setColour(TextEditor::textColourId, findColour(Skin::kBodyText, true));
  text_editor->setColour(TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
  text_editor->setColour(TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));

  Font font = Fonts::instance()->monospace().withPointHeight(height * 0.6f);
  text_editor->setFont(font);
  text_editor->applyFontToAllText(font);
  text_editor->resized();
}

void FileSourceOverlay::loadWindowSizeText() {
  if (file_source_ == nullptr || window_size_ == nullptr)
    return;
  
  float window_size = windowTextToSize(window_size_->getText(), file_source_->buffer()->sample_rate);
  if (window_size > 0.0f) {
    file_source_->setWindowSize(window_size);
    window_size_->setText(String(window_size, 1));

    int num_samples = file_source_->buffer()->size;
    if (num_samples)
      audio_thumbnail_->setWindowSize(window_size / num_samples);

    getParentComponent()->grabKeyboardFocus();
    notifyChanged(true);
  }
}

void FileSourceOverlay::loadStartingPositionText() {
  if (file_source_ == nullptr || current_frame_ == nullptr)
    return;
  
  clampStartingPosition();
  float position = 0.0f;
  if (start_position_)
    position = positionTextToSize(start_position_->getText());
  if (position >= 0.0f) {
    int num_samples = file_source_->buffer()->size;
    if (num_samples) {
      current_frame_->setStartPosition(position);
      audio_thumbnail_->setWindowPosition(position / num_samples);
    }

    getParentComponent()->grabKeyboardFocus();
    notifyChanged(true);
  }
}

void FileSourceOverlay::setFileSource(FileSource* file_source) {
  current_frame_ = nullptr;
  file_source_ = file_source;
  audio_thumbnail_->setFileSource(file_source);
  clampStartingPosition();
}

void FileSourceOverlay::clampStartingPosition() {
  if (start_position_ == nullptr)
    return;

  float max_position = file_source_->buffer()->size - file_source_->getWindowSize();
  float position = positionTextToSize(start_position_->getText());
  position = std::max(0.0f, std::min(position, max_position));
  start_position_->setText(String(position, 1));
}
