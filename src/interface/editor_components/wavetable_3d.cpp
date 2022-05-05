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

#include "wavetable_3d.h"

#include "default_look_and_feel.h"
#include "fourier_transform.h"
#include "full_interface.h"
#include "fonts.h"
#include "skin.h"
#include "synth_constants.h"
#include "synth_gui_interface.h"
#include "synth_oscillator.h"
#include "utils.h"

void Wavetable3d::paint3dLine(Graphics& g, vital::Wavetable* wavetable, int index, Colour color,
                              float width, float height, float wave_height_percent,
                              float wave_range_x, float frame_range_x, float wave_range_y, float frame_range_y,
                              float start_x, float start_y, float offset_x, float offset_y) {
  PathStrokeType stroke(2.5f, PathStrokeType::beveled, PathStrokeType::butt);

  float frame_t = index / (vital::kNumOscillatorWaveFrames - 1.0f);
  float wave_start_x = start_x + frame_t * frame_range_x;
  float wave_start_y = start_y + frame_t * frame_range_y;

  float* buffer = wavetable->getBuffer(index);

  Path path;
  float loop_value = 0.5f * (buffer[0] + buffer[vital::Wavetable::kWaveformSize - 1]);
  float loop_y_offset = -wave_height_percent * loop_value;
  path.startNewSubPath(wave_start_x * width, (wave_start_y + loop_y_offset) * height);

  g.setColour(color);
  int inc = vital::Wavetable::kWaveformSize / kBackgroundResolution;

  for (int i = 0; i < vital::Wavetable::kWaveformSize; i += inc) {
    float wave_t = i / (vital::Wavetable::kWaveformSize - 1.0f);
    float value = buffer[i];
    float y_offset = -wave_height_percent * value;
    float x = wave_start_x + wave_t * wave_range_x;
    float y = wave_start_y + wave_t * wave_range_y + y_offset;
    path.lineTo(x * width, y * height);
  }

  path.lineTo((wave_start_x + wave_range_x) * width, (wave_start_y + wave_range_y + loop_y_offset) * height);

  g.strokePath(path, stroke);
}

void Wavetable3d::paint3dBackground(Graphics& g, vital::Wavetable* wavetable, bool active,
                                    Colour background_color, Colour wave_color1, Colour wave_color2,
                                    float width, float height,
                                    float wave_height_percent,
                                    float wave_range_x, float frame_range_x, float wave_range_y, float frame_range_y,
                                    float start_x, float start_y, float offset_x, float offset_y) {
  PathStrokeType stroke(1.0f, PathStrokeType::beveled, PathStrokeType::butt);

  for (int f = vital::kNumOscillatorWaveFrames - 1; f >= -kExtraShadows; --f) {
    float frame_t = f / (vital::kNumOscillatorWaveFrames - 1.0f);
    float wave_start_x = start_x + frame_t * frame_range_x;
    float wave_start_y = start_y + frame_t * frame_range_y;

    if (f >= 0) {
      float* buffer = wavetable->getBuffer(f);

      Path path;
      float loop_value = 0.5f * (buffer[0] + buffer[vital::Wavetable::kWaveformSize - 1]);
      float loop_y_offset = -wave_height_percent * loop_value;
      path.startNewSubPath(wave_start_x * width, (wave_start_y + loop_y_offset) * height);

      Colour wave_color = wave_color1;
      if (f % kColorJump)
        wave_color = wave_color2;
      if (!active)
        wave_color = wave_color.withSaturation(0.0f).interpolatedWith(background_color, 0.5f);

      Colour wave_dip = background_color;
      wave_dip = wave_dip.withAlpha(wave_color.getAlpha());

      g.setGradientFill(ColourGradient(wave_color, wave_start_x * width, start_y * height,
                                       wave_dip, (wave_start_x + offset_x) * width,
                                       (start_y + offset_y) * height, false));
      int inc = vital::Wavetable::kWaveformSize / kBackgroundResolution;

      for (int i = 0; i < vital::Wavetable::kWaveformSize; i += inc) {
        float wave_t = i / (vital::Wavetable::kWaveformSize - 1.0f);
        float value = buffer[i];
        float y_offset = -wave_height_percent * value;
        float x = wave_start_x + wave_t * wave_range_x;
        float y = wave_start_y + wave_t * wave_range_y + y_offset;
        path.lineTo(x * width, y * height);
      }

      path.lineTo((wave_start_x + wave_range_x) * width, (wave_start_y + wave_range_y + loop_y_offset) * height);

      g.strokePath(path, stroke);
    }
  }
}

Wavetable3d::Wavetable3d(int index, const vital::output_map& mono_modulations,
                                    const vital::output_map& poly_modulations) :
    left_line_renderer_(kResolution + 2), right_line_renderer_(kResolution + 2),
    end_caps_(2, Shaders::kRingFragment),
    import_overlay_(Shaders::kColorFragment), drag_load_style_(WavetableCreator::kNone), transform_(kNumBits),
    size_(kResolution), index_(index), wavetable_(nullptr) {
  wavetable_index_ = 0;
  std::string number = std::to_string(index_ + 1);
  std::string wave_frame_name = "osc_" + number + "_wave_frame";
  wave_frame_outputs_ = {
    mono_modulations.at(wave_frame_name),
    poly_modulations.at(wave_frame_name)
  };

  std::string spectral_morph_name = "osc_" + number + "_spectral_morph_amount";
  spectral_morph_outputs_ = {
    mono_modulations.at(spectral_morph_name),
    poly_modulations.at(spectral_morph_name)
  };

  std::string distortion_name = "osc_" + number + "_distortion_amount";
  distortion_outputs_ = {
    mono_modulations.at(distortion_name),
    poly_modulations.at(distortion_name)
  };

  std::string distortion_phase_name = "osc_" + number + "_distortion_phase";
  distortion_phase_outputs_ = {
    mono_modulations.at(distortion_phase_name),
    poly_modulations.at(distortion_phase_name)
  };

  import_overlay_.setTargetComponent(this);
  import_overlay_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
  wavetable_import_text_ = std::make_unique<PlainTextComponent>("wavetable", "WAVETABLE");
  wavetable_import_text_->setJustification(Justification::centred);
  wavetable_import_text_->setFontType(PlainTextComponent::kLight);
  addAndMakeVisible(wavetable_import_text_.get());
  vocode_import_text_ = std::make_unique<PlainTextComponent>("vocode", "VOCODE");
  vocode_import_text_->setJustification(Justification::centred);
  vocode_import_text_->setFontType(PlainTextComponent::kLight);
  addAndMakeVisible(vocode_import_text_.get());
  pitch_splice_import_text_ = std::make_unique<PlainTextComponent>("pitch splice", "PITCH SPLICE");
  pitch_splice_import_text_->setJustification(Justification::centred);
  pitch_splice_import_text_->setFontType(PlainTextComponent::kLight);
  addAndMakeVisible(pitch_splice_import_text_.get());

  wavetable_ = nullptr;
  frame_slider_ = nullptr;
  last_spectral_morph_type_ = vital::SynthOscillator::kNumSpectralMorphTypes;
  last_distortion_type_ = vital::SynthOscillator::kNumDistortionTypes;
  spectral_morph_type_ = vital::SynthOscillator::kNoSpectralMorph;
  distortion_type_ = vital::SynthOscillator::kNone;
  spectral_morph_slider_ = nullptr;
  distortion_slider_ = nullptr;
  distortion_phase_slider_ = nullptr;
  animate_ = false;
  loading_wavetable_ = false;
  last_loading_wavetable_ = false;
  render_type_ = kFrequencyAmplitudes;
  last_render_type_ = kFrequencyAmplitudes;
  active_ = true;
  current_value_ = 0.0;

  vertical_angle_ = kDefaultVerticalAngle;
  horizontal_angle_ = kDefaultHorizontalAngle;
  draw_width_percent_ = kDefaultDrawWidthPercent;
  wave_height_percent_ = kDefaultWaveHeightPercent;
  y_offset_ = 0.0f;
  setDimensionValues();

  addAndMakeVisible(left_line_renderer_);
  addAndMakeVisible(right_line_renderer_);
  addAndMakeVisible(end_caps_);

  left_line_renderer_.setInterceptsMouseClicks(false, false);
  right_line_renderer_.setInterceptsMouseClicks(false, false);
}

Wavetable3d::~Wavetable3d() = default;

void Wavetable3d::paintBackground(Graphics& g) {
  OpenGlComponent::paintBackground(g);
  left_line_renderer_.setParent(parent_);
  right_line_renderer_.setParent(parent_);

  if (wavetable_ == nullptr) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    wavetable_ = parent->getSynth()->getWavetable(index_);
  }
  if (render_type_ != kWave3d)
    return;

  PathStrokeType stroke(1.0f, PathStrokeType::beveled, PathStrokeType::butt);
  Colour background_color = findColour(Skin::kBody, true);
  Colour wave_color1 = findColour(Skin::kWidgetAccent1, true);
  Colour wave_color2 = findColour(Skin::kWidgetAccent2, true);

  paint3dBackground(g, wavetable_, isActive(), background_color, wave_color1, wave_color2,
                    getWidth(), getHeight(),
                    wave_height_percent_, wave_range_x_, frame_range_x_, wave_range_y_, frame_range_y_,
                    start_x_, start_y_, offset_x_, offset_y_);
}

void Wavetable3d::resized() {
  static constexpr float kTextHeightPercent = 0.1f;

  setDimensionValues();
  setColors();

  left_line_renderer_.setBounds(getLocalBounds());
  right_line_renderer_.setBounds(getLocalBounds());
  end_caps_.setBounds(getLocalBounds());
  OpenGlComponent::resized();

  float font_height = getHeight() * kTextHeightPercent;
  int text_height = getHeight() / 2;
  int text_y_adjust = getHeight() / 4;
  int width = getWidth();

  wavetable_import_text_->setTextSize(font_height);
  vocode_import_text_->setTextSize(font_height);
  pitch_splice_import_text_->setTextSize(font_height);

  wavetable_import_text_->setBounds(0, 0, width, text_height);
  vocode_import_text_->setBounds(0, text_y_adjust, width, text_height);
  pitch_splice_import_text_->setBounds(0, 2 * text_y_adjust, width, text_height);
  wavetable_import_text_->redrawImage(false);
  vocode_import_text_->redrawImage(false);
  pitch_splice_import_text_->redrawImage(false);

  import_text_color_ = findColour(Skin::kTextComponentText, true);
  import_overlay_.setColor(findColour(Skin::kOverlayScreen, true));
}

inline vital::poly_float Wavetable3d::getOutputsTotal(
    std::pair<vital::Output*, vital::Output*> outputs, vital::poly_float default_value) {
  if (!outputs.first->owner->enabled() || !animate_)
    return default_value;
  if (num_voices_readout_ == nullptr || num_voices_readout_->value()[0] <= 0.0f)
    return outputs.first->trigger_value;

  return outputs.first->trigger_value + outputs.second->trigger_value;
}

void Wavetable3d::mouseDown(const MouseEvent& e) {
  Component::mouseDown(e);
  if (e.mods.isPopupMenu()) {
    PopupItems options;

    options.addItem(kSave, "Save to Wavetables");
    options.addItem(kCopy, "Copy");
    if (hasMatchingSystemClipboard())
      options.addItem(kPaste, "Paste");

    options.addItem(-1, "");
    options.addItem(kInit, "Initialize");

    FullInterface* full_interface = findParentComponentOfClass<FullInterface>();
    options.addItem(kResynthesizePreset, "Resynthesize Preset to Wavetable");

    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    parent->showPopupSelector(this, e.getPosition(), options, [=](int selection) { respondToMenuCallback(selection); });
  }
  else {
    if (frame_slider_ == nullptr)
      return;

    current_value_ = frame_slider_->getValue();
    last_edit_position_ = e.getPosition();
    frame_slider_->showPopup(true);
  }
}

void Wavetable3d::mouseDrag(const MouseEvent& e) {
  Component::mouseDrag(e);
  if (frame_slider_ == nullptr || e.mods.isRightButtonDown())
    return;

  Point<int> position = e.getPosition();
  int delta = position.y - last_edit_position_.y;
  float range = frame_slider_->getMaximum() - frame_slider_->getMinimum();
  current_value_ -= delta * range / getHeight();
  current_value_ = std::max(std::min(current_value_, frame_slider_->getMaximum()), frame_slider_->getMinimum());
  frame_slider_->setValue(current_value_);
  frame_slider_->showPopup(true);
  last_edit_position_ = position;
}

void Wavetable3d::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  frame_slider_->mouseWheelMove(e, wheel);
}

void Wavetable3d::mouseExit(const MouseEvent& e) {
  frame_slider_->hidePopup(true);
}

bool Wavetable3d::updateRenderValues() {
  bool new_morph = last_spectral_morph_type_ != spectral_morph_type_ ||
                   last_distortion_type_ != distortion_type_ ||
                   last_render_type_ != render_type_ || last_loading_wavetable_ != loading_wavetable_;
  last_spectral_morph_type_ = spectral_morph_type_;
  last_distortion_type_ = distortion_type_;
  last_render_type_ = render_type_;
  last_loading_wavetable_ = loading_wavetable_;

  vital::poly_float wave_frame = getOutputsTotal(wave_frame_outputs_, frame_slider_->getValue());
  vital::poly_float spectral_morph_value = getSpectralMorphValue();
  vital::poly_float distortion_value = getDistortionValue();
  vital::poly_int distortion_phase = getDistortionPhaseValue();

  vital::poly_mask equal = vital::constants::kFullMask;
  equal = equal & vital::poly_float::equal(wave_frame_, wave_frame);
  equal = equal & vital::poly_float::equal(spectral_morph_value_, spectral_morph_value);
  equal = equal & vital::poly_float::equal(distortion_value_, distortion_value);
  equal = equal & vital::poly_int::equal(distortion_phase_, distortion_phase);

  wave_frame_ = wave_frame;
  spectral_morph_value_ = spectral_morph_value;
  distortion_value_ = distortion_value;
  distortion_phase_ = distortion_phase;

  return !loading_wavetable_ && ((~equal).anyMask() || new_morph);
}

void Wavetable3d::loadWaveData(int index) {
  if (wavetable_ == nullptr)
    return;

  float width = getWidth();
  float height = getHeight();

  float wave_height = k2dWaveHeightPercent * height;
  float wave_width = width;
  float wave_range_y = 0.0f;
  float start_x = 0.0f;
  float start_y = height / 2.0f;

  if (last_render_type_ == kWave3d) {
    float wave_frame = getOutputsTotal(wave_frame_outputs_, frame_slider_->getValue())[index];
    float frame_t = vital::utils::clamp(wave_frame / (vital::kNumOscillatorWaveFrames - 1.0f), 0.0f, 1.0f);
    start_x = (0.5f * (1.0f - wave_range_x_ - frame_range_x_) + frame_range_x_ * frame_t) * width;
    start_y = (0.5f * (1.0f - wave_range_y_ - frame_range_y_) + y_offset_ + frame_range_y_ * frame_t) * height;
    wave_width = wave_range_x_ * width;
    wave_range_y = wave_range_y_ * height;
    wave_height = wave_height_percent_ * height;
  }

  loadIntoTimeDomain(index);

  OpenGlLineRenderer* renderer = &left_line_renderer_;
  if (index)
    renderer = &right_line_renderer_;

  vital::poly_float spread(1.0f, 2.0f, 3.0f, 4.0f);
  float* time_domain = process_frame_.time_domain;
  float delta = 1.0f / size_;
  for (int i = 0; i < size_ - vital::poly_float::kSize + 1; i += vital::poly_float::kSize) {
    vital::poly_float t = (spread + i) * delta;

    for (int v = 0; v < vital::poly_float::kSize; ++v) {
      int point_index = i + v + 1;
      renderer->setXAt(point_index, start_x + t[v] * wave_width);
      float y = start_y - time_domain[i + v] * wave_height + t[v] * wave_range_y;
      renderer->setYAt(point_index, y);
    }
  }

  float average = (renderer->yAt(1) + renderer->yAt(size_) - wave_range_y) * 0.5f;
  renderer->setXAt(0, start_x);
  renderer->setYAt(0, average);
  int line_end_index = size_ + 1;
  renderer->setXAt(line_end_index, start_x + wave_width);
  renderer->setYAt(line_end_index, average + wave_range_y);
}

void Wavetable3d::loadSpectrumData(int index) {
  static constexpr float kMinDb = -30.0f;
  static constexpr float kMaxDb = 50.0f;
  static constexpr float kDbRange = kMaxDb - kMinDb;
  static constexpr float kDbBoostPerOctave = 3.0f;

  loadIntoTimeDomain(index);
  process_frame_.toFrequencyDomain();
  std::complex<float>* frequency_domain = process_frame_.frequency_domain;

  OpenGlLineRenderer* renderer = &left_line_renderer_;
  if (index)
    renderer = &right_line_renderer_;

  int width = getWidth();
  int height = getHeight();
  float center = height * 0.5f;
  int num_points = std::min(width, vital::Wavetable::kWaveformSize / 2);
  float scale = 1.0f / num_points;
  int last_frequency = 0;
  for (int i = 0; i <= num_points; ++i) {
    int invert_i = vital::Wavetable::kWaveformSize + 1 - i;
    float t = i * 1.0f / num_points;
    float x = t * width;
    renderer->setXAt(i, x);
    renderer->setXAt(invert_i, x);

    float position = vital::futils::exp2(t * (vital::Wavetable::kFrequencyBins - 1.0f));
    int frequency = std::min<int>(position, vital::Wavetable::kWaveformSize / 2 - 1);
    float frequency_t = position - frequency;

    float amplitude_from = std::abs(frequency_domain[frequency]);
    float amplitude_to = std::abs(frequency_domain[frequency + 1]);
    float amplitude = vital::utils::interpolate(amplitude_from, amplitude_to, frequency_t) * scale;

    for (int f = last_frequency + 1; f < frequency; ++f)
      amplitude = std::max(std::abs(frequency_domain[f]) * scale, amplitude);

    last_frequency = frequency;

    float db = vital::utils::magnitudeToDb(amplitude) + t * vital::Wavetable::kFrequencyBins * kDbBoostPerOctave;
    float y = std::max(db - kMinDb, 0.0f) / kDbRange;
    renderer->setYAt(i, y * center + center);
    renderer->setYAt(invert_i, -y * center + center);
  }

  float end = width * 1.5f;
  for (int i = width; i <= vital::Wavetable::kWaveformSize / 2; ++i) {
    int invert_i = vital::Wavetable::kWaveformSize + 1 - i;
    renderer->setXAt(i, end);
    renderer->setXAt(invert_i, end);
    renderer->setYAt(i, center);
    renderer->setYAt(invert_i, center);
  }
}

void Wavetable3d::init(OpenGlWrapper& open_gl) {
  left_line_renderer_.init(open_gl);
  right_line_renderer_.init(open_gl);
  end_caps_.init(open_gl);
  import_overlay_.init(open_gl);
  wavetable_import_text_->init(open_gl);
  vocode_import_text_->init(open_gl);
  pitch_splice_import_text_->init(open_gl);
}

void Wavetable3d::render(OpenGlWrapper& open_gl, bool animate) {
  animate_ = animate;

  if (render_type_ == kFrequencyAmplitudes)
    renderSpectrum(open_gl);
  else
    renderWave(open_gl);

  if (drag_load_style_ != WavetableCreator::kNone) {
    import_overlay_.render(open_gl, animate);
    Colour background = import_overlay_.getColor();

    wavetable_import_text_->setColor(import_text_color_.interpolatedWith(background, 0.5f));
    vocode_import_text_->setColor(import_text_color_.interpolatedWith(background, 0.5f));
    pitch_splice_import_text_->setColor(import_text_color_.interpolatedWith(background, 0.5f));

    if (drag_load_style_ == WavetableCreator::kWavetableSplice)
      wavetable_import_text_->setColor(import_text_color_);
    else if (drag_load_style_ == WavetableCreator::kVocoded)
      vocode_import_text_->setColor(import_text_color_);
    else if (drag_load_style_ == WavetableCreator::kPitched)
      pitch_splice_import_text_->setColor(import_text_color_);

    wavetable_import_text_->render(open_gl, animate);
    vocode_import_text_->render(open_gl, animate);
    pitch_splice_import_text_->render(open_gl, animate);
  }

  left_line_renderer_.renderCorners(open_gl, animate);
}

void Wavetable3d::renderWave(OpenGlWrapper& open_gl) {
  if (wavetable_ == nullptr)
    return;

  left_line_renderer_.setFill(render_type_ == kWave2d);
  right_line_renderer_.setFill(render_type_ == kWave2d);

  float fill_fade = findValue(Skin::kWidgetFillFade);
  float line_width = findValue(Skin::kWidgetLineWidth);
  float fill_center = findValue(Skin::kWidgetFillCenter);

  left_line_renderer_.setLineWidth(line_width);
  right_line_renderer_.setLineWidth(line_width);

  left_line_renderer_.setFillCenter(fill_center);
  right_line_renderer_.setFillCenter(fill_center);

  bool new_line_data = updateRenderValues();

  if (new_line_data) {
    loadWaveData(0);
    loadWaveData(1);
  }

  Colour left_fill;
  Colour right_fill;
  if (isActive()) {
    left_line_renderer_.setColor(line_left_color_);
    right_line_renderer_.setColor(line_right_color_);
    left_fill = fill_left_color_;
    right_fill = fill_right_color_;
  }
  else {
    left_line_renderer_.setColor(line_disabled_color_);
    right_line_renderer_.setColor(line_disabled_color_);
    left_fill = fill_disabled_color_;
    right_fill = fill_disabled_color_;
  }

  left_line_renderer_.setFillColors(left_fill.withMultipliedAlpha(1.0f - fill_fade), left_fill);
  right_line_renderer_.setFillColors(right_fill.withMultipliedAlpha(1.0f - fill_fade), right_fill);
  left_line_renderer_.render(open_gl, animate_);
  right_line_renderer_.render(open_gl, animate_);

  if (render_type_ == kWave3d) {
    drawPosition(open_gl, 1);
    drawPosition(open_gl, 0);
  }
}

void Wavetable3d::renderSpectrum(OpenGlWrapper& open_gl) {
  float fill_fade = findValue(Skin::kWidgetFillFade);
  left_line_renderer_.setFill(true);
  right_line_renderer_.setFill(true);
  left_line_renderer_.setLineWidth(2.5f);
  right_line_renderer_.setLineWidth(2.5f);

  bool new_data = updateRenderValues();

  if (new_data) {
    loadSpectrumData(0);
    loadSpectrumData(1);
  }

  Colour right_fill = fill_right_color_;
  Colour left_fill = fill_left_color_;
  if (isActive()) {
    right_line_renderer_.setColor(line_right_color_);
    left_line_renderer_.setColor(line_left_color_);
    right_fill = fill_right_color_;
    left_fill = fill_left_color_;
  }
  else {
    right_line_renderer_.setColor(line_disabled_color_);
    left_line_renderer_.setColor(line_disabled_color_);
    left_line_renderer_.setColor(line_disabled_color_);
    right_fill = fill_disabled_color_;
    left_fill = fill_disabled_color_;
  }

  right_line_renderer_.setFillColors(right_fill.withMultipliedAlpha(1.0f - fill_fade), right_fill);
  left_line_renderer_.setFillColors(left_fill.withMultipliedAlpha(1.0f - fill_fade), left_fill);

  right_line_renderer_.render(open_gl, animate_);
  left_line_renderer_.render(open_gl, animate_);
}

void Wavetable3d::destroy(OpenGlWrapper& open_gl) {
  left_line_renderer_.destroy(open_gl);
  right_line_renderer_.destroy(open_gl);
  end_caps_.destroy(open_gl);
  import_overlay_.destroy(open_gl);
  wavetable_import_text_->destroy(open_gl);
  vocode_import_text_->destroy(open_gl);
  pitch_splice_import_text_->destroy(open_gl);
}

void Wavetable3d::drawPosition(OpenGlWrapper& open_gl, int index) {
  Colour color;
  if (index)
    color = line_right_color_;
  else
    color = line_left_color_;

  if (!isActive())
    color = color.withSaturation(0.0f).interpolatedWith(body_color_, 0.5f);
  end_caps_.setColor(color);
  Colour background = findColour(Skin::kWidgetBackground, true);
  end_caps_.setAltColor(color.interpolatedWith(background, 0.5f));

  int draw_width = getWidth();
  int draw_height = getHeight();
  float position_raw_width = kPositionLineWidthRatio * findValue(Skin::kWidgetLineWidth);
  float position_height = 2.0f * position_raw_width / draw_height;
  float position_width = 2.0f * position_raw_width / draw_width;
  end_caps_.setThickness(position_raw_width / 5.0f);

  OpenGlLineRenderer* renderer = &left_line_renderer_;
  if (index)
    renderer = &right_line_renderer_;

  float x = 2.0f * renderer->xAt(0) / draw_width - 1.0f;
  float y = 1.0f - 2.0f * renderer->yAt(0) / draw_height;
  float end_x = 2.0f * renderer->xAt(size_) / draw_width - 1.0f;
  float end_y = 1.0f - 2.0f * renderer->yAt(size_) / draw_height;
  end_caps_.setQuad(0, x - 0.5f * position_width, y - 0.5f * position_height, position_width, position_height);
  end_caps_.setQuad(1, end_x - 0.5f * position_width, end_y - 0.5f * position_height, position_width, position_height);
  end_caps_.render(open_gl, true);
}

void Wavetable3d::setViewSettings(float horizontal_angle, float vertical_angle,
                                  float draw_width, float wave_height, float y_offset) {
  horizontal_angle_ = horizontal_angle;
  vertical_angle_ = vertical_angle;
  draw_width_percent_ = draw_width;
  wave_height_percent_ = wave_height;
  y_offset_ = y_offset;
  setDimensionValues();
}

void Wavetable3d::setRenderType(RenderType render_type) {
  render_type_ = render_type;
  repaintBackground();
}

void Wavetable3d::respondToMenuCallback(int option) {
  if (option == kInit) {
    for (Listener* listener : listeners_)
      listener->loadDefaultWavetable();
    repaintBackground();
    setDirty();
  }
  else if (option == kSave) {
    for (Listener* listener : listeners_)
      listener->saveWavetable();
  }

  else if (option == kResynthesizePreset) {
    for (Listener* listener : listeners_)
      listener->resynthesizeToWavetable();
    repaintBackground();
    setDirty();
  }
  else if (option == kCopy) {
    FullInterface* parent = findParentComponentOfClass<FullInterface>();
    if (parent == nullptr)
      return;

    SystemClipboard::copyTextToClipboard(parent->getWavetableJson(index_).dump());
  }
  else if (option == kPaste) {
    String text = SystemClipboard::getTextFromClipboard();

    try {
      json parsed_json_state = json::parse(text.toStdString(), nullptr, false);
      if (WavetableCreator::isValidJson(parsed_json_state)) {
        loading_wavetable_ = true;
        for (Listener* listener : listeners_)
          listener->loadWavetable(parsed_json_state);
        loading_wavetable_ = false;

        repaintBackground();
        setDirty();
      }
    }
    catch (const json::exception& e) {
    }
  }
}

bool Wavetable3d::hasMatchingSystemClipboard() {
  String text = SystemClipboard::getTextFromClipboard();
  try {
    json parsed_json_state = json::parse(text.toStdString(), nullptr, false);
    return WavetableCreator::isValidJson(parsed_json_state);
  }
  catch (const json::exception& e) {
    return false;
  }
}

void Wavetable3d::audioFileLoaded(const File& file) {
  for (Listener* listener : listeners_) {
    FileInputStream* input_stream = new FileInputStream(file);
    if (input_stream->openedOk())
      listener->loadAudioAsWavetable(file.getFileNameWithoutExtension(), input_stream, drag_load_style_);
  }

  drag_load_style_ = WavetableCreator::kNone;
}

void Wavetable3d::updateDraggingPosition(int x, int y) {
  static constexpr float kDivisionPercent = 3.0f / 8.0f;
  WavetableCreator::AudioFileLoadStyle new_style = WavetableCreator::kVocoded;
  if (y < kDivisionPercent * getHeight())
    new_style = WavetableCreator::kWavetableSplice;
  else if (y > (1.0f - kDivisionPercent) * getHeight())
    new_style = WavetableCreator::kPitched;

  if (new_style != drag_load_style_)
    drag_load_style_ = new_style;
}

void Wavetable3d::setDimensionValues() {
  wave_range_x_ = cosf(horizontal_angle_) * draw_width_percent_;
  frame_range_x_ = -sinf(horizontal_angle_) * draw_width_percent_;
  wave_range_y_ = 2.0f * frame_range_x_ * cosf(vertical_angle_);
  frame_range_y_ = -2.0f * wave_range_x_ * cosf(vertical_angle_);
  start_x_ = 0.5f * (1.0f - wave_range_x_ - frame_range_x_);
  start_y_ = 0.5f * (1.0f - wave_range_y_ - frame_range_y_) + y_offset_;
  float draw_angle = atanf(wave_range_y_ / wave_range_x_);
  offset_x_ = -sinf(draw_angle) * 1.5f * wave_height_percent_;
  offset_y_ = cosf(draw_angle) * 1.5f * wave_height_percent_;
}

void Wavetable3d::setColors() {
  body_color_ = findColour(Skin::kBody, true);
  line_left_color_ = findColour(Skin::kWidgetPrimary1, true);
  line_right_color_ = findColour(Skin::kWidgetPrimary2, true);
  line_disabled_color_ = findColour(Skin::kWidgetPrimaryDisabled, true);
  fill_left_color_ = findColour(Skin::kWidgetSecondary1, true);
  fill_right_color_ = findColour(Skin::kWidgetSecondary2, true);
  fill_disabled_color_ = findColour(Skin::kWidgetSecondaryDisabled, true);
}

vital::poly_float Wavetable3d::getDistortionValue() {
  vital::poly_float distortion = getOutputsTotal(distortion_outputs_, distortion_slider_->getValue());
  vital::poly_float adjusted_distortion = vital::utils::clamp(distortion, 0.0f, 1.0f);
  vital::SynthOscillator::DistortionType distortion_type = (vital::SynthOscillator::DistortionType)distortion_type_;
  vital::SynthOscillator::setDistortionValues(distortion_type, &adjusted_distortion, 1, false);
  return adjusted_distortion;
}

vital::poly_float Wavetable3d::getSpectralMorphValue() {
  vital::poly_float morph = getOutputsTotal(spectral_morph_outputs_, spectral_morph_slider_->getValue());
  vital::poly_float adjusted_morph = vital::utils::clamp(morph, 0.0f, 1.0f);
  vital::SynthOscillator::SpectralMorph morph_type = (vital::SynthOscillator::SpectralMorph)spectral_morph_type_;
  vital::SynthOscillator::setSpectralMorphValues(morph_type, &adjusted_morph, 1, false);
  return adjusted_morph;
}

vital::poly_int Wavetable3d::getDistortionPhaseValue() {
  vital::SynthOscillator::DistortionType distortion_type = (vital::SynthOscillator::DistortionType)distortion_type_;
  if (!vital::SynthOscillator::usesDistortionPhase(distortion_type))
    return 0;
  
  vital::poly_float phase = getOutputsTotal(distortion_phase_outputs_, distortion_phase_slider_->getValue());
  return vital::utils::toInt(phase * UINT_MAX - INT_MAX);
}


void Wavetable3d::loadIntoTimeDomain(int index) {
  loadFrequencyData(index);
  warpSpectrumToWave(index);
  warpPhase(index);
}

void Wavetable3d::loadFrequencyData(int index) {
  wavetable_index_ = std::round(getOutputsTotal(wave_frame_outputs_, frame_slider_->getValue())[index]);
  current_wavetable_data_ = wavetable_->getAllData();
  wavetable_index_ = std::max(0, std::min(wavetable_index_, current_wavetable_data_->num_frames - 1));
}

void Wavetable3d::warpSpectrumToWave(int index) {
  float morph = spectral_morph_value_[index];
  vital::SynthOscillator::SpectralMorph morph_type = (vital::SynthOscillator::SpectralMorph)spectral_morph_type_;

  memset(process_wave_data_, 0, vital::SynthOscillator::kSpectralBufferSize * sizeof(vital::poly_float));
  vital::SynthOscillator::runSpectralMorph(morph_type, morph, current_wavetable_data_, wavetable_index_,
                                           process_wave_data_, &transform_);
}

void Wavetable3d::warpPhase(int index) {
  float distortion = distortion_value_[index];
  vital::SynthOscillator::DistortionType distortion_type = (vital::SynthOscillator::DistortionType)distortion_type_;

  vital::poly_float spread(1.0f, 2.0f, 3.0f, 4.0f);
  float delta = 1.0f / size_;
  float* buffer = (float*)(process_wave_data_ + 1);
  float* time_domain = process_frame_.time_domain;
  for (int i = 0; i < size_ - vital::poly_float::kSize + 1; i += vital::poly_float::kSize) {
    vital::poly_float t = (spread + i) * delta;
    vital::poly_int original_phase = vital::utils::toInt(t * UINT_MAX - INT_MAX) + INT_MAX;
    vital::poly_int adjusted_phase = vital::SynthOscillator::adjustPhase(distortion_type, original_phase,
                                                                         distortion, distortion_phase_);
    
    vital::poly_float window = vital::SynthOscillator::getPhaseWindow(distortion_type, original_phase, adjusted_phase);
    adjusted_phase += distortion_phase_;
    
    vital::poly_float value = window * vital::SynthOscillator::interpolate(buffer, adjusted_phase);
    for (int v = 0; v < vital::poly_float::kSize; ++v)
      time_domain[i + v] = value[v];
  }
}
