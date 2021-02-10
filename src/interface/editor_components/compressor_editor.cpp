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

#include "compressor_editor.h"

#include "skin.h"
#include "shaders.h"
#include "synth_gui_interface.h"
#include "utils.h"

namespace {
  float getOpenGlYForDb(float db) {
    float t = (db - CompressorEditor::kMinDb) / (CompressorEditor::kMaxDb - CompressorEditor::kMinDb);
    return 2.0f * t - 1.0f;
  }

  vital::poly_float getOpenGlYForDb(vital::poly_float db) {
    vital::poly_float t = (db - CompressorEditor::kMinDb) / (CompressorEditor::kMaxDb - CompressorEditor::kMinDb);
    return vital::utils::clamp(t * 2.0f - 1.0f, -1.0f, 1.0f);
  }

  vital::poly_float getOpenGlYForMagnitude(vital::poly_float magnitude) {
    vital::poly_float db = vital::utils::magnitudeToDb(vital::utils::max(0.0001f, magnitude));
    return getOpenGlYForDb(db);
  }

  void setQuadIfRatioMatch(OpenGlMultiQuad& quads, float ratio, float ratio_match,
                           int index, float x, float y, float w, float h) {
    if (ratio == ratio_match || (ratio > 0.0f && ratio_match > 0.0f) || (ratio < 0.0f && ratio_match < 0.0f))
      quads.setQuad(index, x, y, w, h);
    else
      quads.setQuad(index, -2.0f, -2.0f, 0.0f, 0.0f);
  }

  std::string formatString(float value, std::string suffix) {
    static constexpr int kMaxDecimalPlaces = 4;
    String format = String(value, kMaxDecimalPlaces);

    int display_characters = kMaxDecimalPlaces;
    if (format[0] == '-')
      display_characters += 1;

    format = format.substring(0, display_characters);
    if (format.getLastCharacter() == '.')
      format = format.removeCharacters(".");

    return format.toStdString() + suffix;
  }
} // namespace

CompressorEditor::CompressorEditor() : hover_quad_(Shaders::kColorFragment),
                                       input_dbs_(kNumChannels, Shaders::kColorFragment),
                                       output_dbs_(kNumChannels, Shaders::kRoundedRectangleFragment),
                                       thresholds_(kNumChannels, Shaders::kColorFragment),
                                       ratio_lines_(kTotalRatioLines, Shaders::kFadeSquareFragment) {
  addRoundedCorners();
  addAndMakeVisible(hover_quad_);
  addAndMakeVisible(input_dbs_);
  addAndMakeVisible(output_dbs_);
  addAndMakeVisible(thresholds_);
  addAndMakeVisible(ratio_lines_);

  parent_ = nullptr;
  section_parent_ = nullptr;
  hover_ = kNone;
  animate_ = false;
  high_band_active_ = true;
  low_band_active_ = true;
  size_ratio_ = 1.0f;
  active_ = true;

  low_upper_threshold_ = kMaxDb;
  band_upper_threshold_ = kMaxDb;
  high_upper_threshold_ = kMaxDb;
  low_lower_threshold_ = kMinDb;
  band_lower_threshold_ = kMinDb;
  high_lower_threshold_ = kMinDb;
  low_upper_ratio_ = 0.0f;
  band_upper_ratio_ = 0.0f;
  high_upper_ratio_ = 0.0f;
  low_lower_ratio_ = 0.0f;
  band_lower_ratio_ = 0.0f;
  high_lower_ratio_ = 0.0f;

  low_input_ms_ = nullptr;
  band_input_ms_ = nullptr;
  high_input_ms_ = nullptr;

  low_output_ms_ = nullptr;
  band_output_ms_ = nullptr;
  high_output_ms_ = nullptr;
}

CompressorEditor::~CompressorEditor() { }

CompressorEditor::DragPoint CompressorEditor::getHoverPoint(const MouseEvent& e) {
  float low_band_high_position = 3.0f * e.position.x / ((1.0f - kCompressorAreaBuffer) * getWidth());
  int index = low_band_high_position;
  float local_position = low_band_high_position - index;
  if (index >= 3 || index < 0 || local_position < 3.0f * kCompressorAreaBuffer)
    return kNone;

  if (index == 0 && !low_band_active_)
    index = 1;
  if (index == 2 && !high_band_active_)
    index = 1;

  float upper_threshold_values[] = { low_upper_threshold_, band_upper_threshold_, high_upper_threshold_ };
  float lower_threshold_values[] = { low_lower_threshold_, band_lower_threshold_, high_lower_threshold_ };
  DragPoint upper_threshold_points[] = { kLowUpperThreshold, kBandUpperThreshold, kHighUpperThreshold };
  DragPoint lower_threshold_points[] = { kLowLowerThreshold, kBandLowerThreshold, kHighLowerThreshold };
  DragPoint upper_ratio_points[] = { kLowUpperRatio, kBandUpperRatio, kHighUpperRatio };
  DragPoint lower_ratio_points[] = { kLowLowerRatio, kBandLowerRatio, kHighLowerRatio };

  float grab_radius = kGrabRadius * size_ratio_;
  int upper_handle_y = std::max(grab_radius, getYForDb(upper_threshold_values[index]));
  int lower_handle_y = std::min(getHeight() - grab_radius, getYForDb(lower_threshold_values[index]));

  float delta_upper = e.position.y - upper_handle_y;
  float delta_lower = e.position.y - lower_handle_y;
  if (fabsf(delta_upper) <= grab_radius && fabsf(delta_upper) < fabsf(delta_lower))
    return upper_threshold_points[index];
  if (fabsf(delta_lower) <= grab_radius)
    return lower_threshold_points[index];
  if (delta_upper < 0.0f)
    return upper_ratio_points[index];
  if (delta_lower > 0.0f)
    return lower_ratio_points[index];
  return kNone;
}

void CompressorEditor::mouseDown(const MouseEvent& e) {
  last_mouse_position_ = e.getPosition();
  mouseDrag(e);
}

void CompressorEditor::mouseDoubleClick(const MouseEvent& e) {
  if (isRatio(hover_)) {
    switch (hover_) {
      case kLowUpperRatio:
        setLowUpperRatio(0.0f);
        break;
      case kBandUpperRatio:
        setBandUpperRatio(0.0f);
        break;
      case kHighUpperRatio:
        setHighUpperRatio(0.0f);
        break;
      case kLowLowerRatio:
        setLowLowerRatio(0.0f);
        break;
      case kBandLowerRatio:
        setBandLowerRatio(0.0f);
        break;
      case kHighLowerRatio:
        setHighLowerRatio(0.0f);
        break;
      default:
        break;
    }
  }
}

void CompressorEditor::mouseMove(const MouseEvent& e) {
  hover_ = getHoverPoint(e);

  if (hover_ != kNone)
    setMouseCursor(MouseCursor::BottomEdgeResizeCursor);
  else
    setMouseCursor(MouseCursor::NormalCursor);
}

void CompressorEditor::mouseDrag(const MouseEvent& e) {
  if (hover_ == kNone || parent_ == nullptr)
    return;

  float delta = (e.getPosition().y - last_mouse_position_.y) * kMouseMultiplier / getHeight();
  float delta_db_value = (kMinDb - kMaxDb) * delta;

  last_mouse_position_ = e.getPosition();
  float delta_ratio = delta * kRatioEditMultiplier;

  if (e.mods.isShiftDown()) {
    setLowUpperThreshold(low_upper_threshold_ + delta_db_value, false);
    setBandUpperThreshold(band_upper_threshold_ + delta_db_value, false);
    setHighUpperThreshold(high_upper_threshold_ + delta_db_value, false);
    setLowLowerThreshold(low_lower_threshold_ + delta_db_value, false);
    setBandLowerThreshold(band_lower_threshold_ + delta_db_value, false);
    setHighLowerThreshold(high_lower_threshold_ + delta_db_value, false);
  }
  else {
    switch (hover_) {
      case kLowUpperThreshold:
        setLowUpperThreshold(low_upper_threshold_ + delta_db_value, true);
        break;
      case kLowUpperRatio:
        setLowUpperRatio(low_upper_ratio_ + delta_ratio);
        break;
      case kBandUpperThreshold:
        setBandUpperThreshold(band_upper_threshold_ + delta_db_value, true);
        break;
      case kBandUpperRatio:
        setBandUpperRatio(band_upper_ratio_ + delta_ratio);
        break;
      case kHighUpperThreshold:
        setHighUpperThreshold(high_upper_threshold_ + delta_db_value, true);
        break;
      case kHighUpperRatio:
        setHighUpperRatio(high_upper_ratio_ + delta_ratio);
        break;
      case kLowLowerThreshold:
        setLowLowerThreshold(low_lower_threshold_ + delta_db_value, true);
        break;
      case kLowLowerRatio:
        setLowLowerRatio(low_lower_ratio_ - delta_ratio);
        break;
      case kBandLowerThreshold:
        setBandLowerThreshold(band_lower_threshold_ + delta_db_value, true);
        break;
      case kBandLowerRatio:
        setBandLowerRatio(band_lower_ratio_ - delta_ratio);
        break;
      case kHighLowerThreshold:
        setHighLowerThreshold(high_lower_threshold_ + delta_db_value, true);
        break;
      case kHighLowerRatio:
        setHighLowerRatio(high_lower_ratio_ - delta_ratio);
        break;
      default:
        break;
    }
  }
}

void CompressorEditor::mouseUp(const MouseEvent& e) {
  if (isRatio(hover_))
    setMouseCursor(MouseCursor::BottomEdgeResizeCursor);

  section_parent_->hidePopupDisplay(true);
}

void CompressorEditor::mouseExit(const MouseEvent& e) {
  setMouseCursor(MouseCursor::NormalCursor);
  hover_ = kNone;
}

void CompressorEditor::paintBackground(Graphics& g) {
  OpenGlComponent::paintBackground(g);

  g.setColour(findColour(Skin::kLightenScreen, true));
  for (int i = 1; i < kDbLineSections; ++i) {
    float t = (i * 1.0f) / kDbLineSections;
    int y = getHeight() * t;
    g.fillRect(0, y, getWidth(), 1);
  }
}

void CompressorEditor::resized() {
  OpenGlComponent::resized();
  hover_quad_.setBounds(getLocalBounds());
  input_dbs_.setBounds(getLocalBounds());
  output_dbs_.setBounds(getLocalBounds());
  thresholds_.setBounds(getLocalBounds());
  ratio_lines_.setBounds(getLocalBounds());
}

void CompressorEditor::init(OpenGlWrapper& open_gl) {
  OpenGlComponent::init(open_gl);
  hover_quad_.init(open_gl);
  input_dbs_.init(open_gl);
  output_dbs_.init(open_gl);
  thresholds_.init(open_gl);
  ratio_lines_.init(open_gl);
}

void CompressorEditor::render(OpenGlWrapper& open_gl, bool animate) {
  renderCompressor(open_gl, animate);
  renderCorners(open_gl, animate);
}

void CompressorEditor::setThresholdPositions(int low_start, int low_end, int band_start, int band_end,
                                             int high_start, int high_end, float ratio_match) {
  thresholds_.setColor(getColorForRatio(ratio_match));
  float width = getWidth();

  float low_start_x = low_start * 2.0f / width - 1.0f;
  float low_width = (low_end - low_start) * 2.0f / width;
  float band_start_x = band_start * 2.0f / width - 1.0f;
  float band_width = (band_end - band_start) * 2.0f / width;
  float high_start_x = high_start * 2.0f / width - 1.0f;
  float high_width = (high_end - high_start) * 2.0f / width;

  setQuadIfRatioMatch(thresholds_, -low_lower_ratio_, ratio_match, 0,
                      low_start_x, -1.0f, low_width, getOpenGlYForDb(low_lower_threshold_) + 1.0f);
  setQuadIfRatioMatch(thresholds_, low_upper_ratio_, ratio_match, 1,
                      low_start_x, 1.0f, low_width, getOpenGlYForDb(low_upper_threshold_) - 1.0f);
  setQuadIfRatioMatch(thresholds_, -band_lower_ratio_, ratio_match, 2,
                      band_start_x, -1.0f, band_width, getOpenGlYForDb(band_lower_threshold_) + 1.0f);
  setQuadIfRatioMatch(thresholds_, band_upper_ratio_, ratio_match, 3,
                      band_start_x, 1.0f, band_width, getOpenGlYForDb(band_upper_threshold_) - 1.0f);
  setQuadIfRatioMatch(thresholds_, -high_lower_ratio_, ratio_match, 4,
                      high_start_x, -1.0f, high_width, getOpenGlYForDb(high_lower_threshold_) + 1.0f);
  setQuadIfRatioMatch(thresholds_, high_upper_ratio_, ratio_match, 5,
                      high_start_x, 1.0f, high_width, getOpenGlYForDb(high_upper_threshold_) - 1.0f);
}

void CompressorEditor::setRatioLines(int start_index, int start_x, int end_x,
                                     float threshold, float ratio, bool upper, bool hover) {

  float db_position = kDbLineSections * (threshold - kMinDb) / (kMaxDb - kMinDb);
  int db_index = db_position;
  float db_change = -(kMaxDb - kMinDb) / kDbLineSections;
  if (upper) {
    db_change = (kMaxDb - kMinDb) / kDbLineSections;
    db_index = ceil(db_position);
  }

  float width = getWidth();
  float x = start_x * 2.0f / width - 1.0f;
  float ratio_width = (end_x - start_x) * 2.0f / width;
  float ratio_height = 4.0f / getHeight();

  float mult = hover ? 5.0f : 2.5f;

  float db = db_index * (kMaxDb - kMinDb) / kDbLineSections + kMinDb;
  for (int i = 0; i < kRatioDbLines; ++i) {
    float adjusted_db = getCompressedDb(db, threshold, ratio, threshold, ratio);
    ratio_lines_.setQuad(start_index + i, x, getOpenGlYForDb(adjusted_db) - ratio_height * 0.5f,
                         ratio_width, ratio_height);
    ratio_lines_.setShaderValue(start_index + i, (kRatioDbLines - i) * mult / kRatioDbLines);
    db += db_change;
  }
}

void CompressorEditor::setRatioLinePositions(int low_start, int low_end, int band_start, int band_end,
                                             int high_start, int high_end) {
  setRatioLines(0, low_start, low_end, low_upper_threshold_, low_upper_ratio_,
                true, hover_ == kLowUpperRatio);
  setRatioLines(kRatioDbLines, low_start, low_end, low_lower_threshold_, low_lower_ratio_,
                false, hover_ == kLowLowerRatio);
  setRatioLines(2 * kRatioDbLines, band_start, band_end, band_upper_threshold_, band_upper_ratio_,
                true, hover_ == kBandUpperRatio);
  setRatioLines(3 * kRatioDbLines, band_start, band_end, band_lower_threshold_, band_lower_ratio_,
                false, hover_ == kBandLowerRatio);
  setRatioLines(4 * kRatioDbLines, high_start, high_end, high_upper_threshold_, high_upper_ratio_,
                true, hover_ == kHighUpperRatio);
  setRatioLines(5 * kRatioDbLines, high_start, high_end, high_lower_threshold_, high_lower_ratio_,
                false, hover_ == kHighLowerRatio);
}

void CompressorEditor::renderHover(OpenGlWrapper& open_gl, int low_start, int low_end,
                                   int band_start, int band_end, int high_start, int high_end) {
  if (hover_ == kNone)
    return;

  float width = getWidth();
  float height = getHeight();
  float threshold_height = 2.0f / height;

  float low_start_x = low_start * 2.0f / width - 1.0f;
  float low_width = (low_end - low_start) * 2.0f / width;
  float band_start_x = band_start * 2.0f / width - 1.0f;
  float band_width = (band_end - band_start) * 2.0f / width;
  float high_start_x = high_start * 2.0f / width - 1.0f;
  float high_width = (high_end - high_start) * 2.0f / width;

  switch (hover_) {
    case kLowUpperThreshold:
      hover_quad_.setQuad(0, low_start_x, getOpenGlYForDb(low_upper_threshold_) - 0.5f * threshold_height,
                          low_width, threshold_height);
      break;
    case kBandUpperThreshold:
      hover_quad_.setQuad(0, band_start_x, getOpenGlYForDb(band_upper_threshold_) - 0.5f * threshold_height,
                          band_width, threshold_height);
      break;
    case kHighUpperThreshold:
      hover_quad_.setQuad(0, high_start_x, getOpenGlYForDb(high_upper_threshold_) - 0.5f * threshold_height,
                          high_width, threshold_height);
      break;
    case kLowLowerThreshold:
      hover_quad_.setQuad(0, low_start_x, getOpenGlYForDb(low_lower_threshold_) - 0.5f * threshold_height,
                          low_width, threshold_height);
      break;
    case kBandLowerThreshold:
      hover_quad_.setQuad(0, band_start_x, getOpenGlYForDb(band_lower_threshold_) - 0.5f * threshold_height,
                          band_width, threshold_height);
      break;
    case kHighLowerThreshold:
      hover_quad_.setQuad(0, high_start_x, getOpenGlYForDb(high_lower_threshold_) - 0.5f * threshold_height,
                          high_width, threshold_height);
      break;
    default:
      return;
  }

  if (isRatio(hover_))
    hover_quad_.setColor(findColour(Skin::kLightenScreen, true));
  else
    hover_quad_.setColor(findColour(Skin::kWidgetCenterLine, true));

  hover_quad_.render(open_gl, true);
}

void CompressorEditor::renderCompressor(OpenGlWrapper& open_gl, bool animate) {
  static constexpr float kOutputBarHeight = 2.2f;
  if (low_input_ms_ == nullptr || band_input_ms_ == nullptr || high_input_ms_ == nullptr ||
      low_output_ms_ == nullptr || band_output_ms_ == nullptr || high_output_ms_ == nullptr) {
    return;
  }

  vital::poly_float low_rms = vital::utils::sqrt(low_input_ms_->value());
  vital::poly_float low_input_y = getOpenGlYForMagnitude(low_rms);

  vital::poly_float scaled_low_rms = vital::utils::sqrt(low_output_ms_->value());
  vital::poly_float low_output_y = getOpenGlYForMagnitude(scaled_low_rms);

  vital::poly_float band_rms = vital::utils::sqrt(band_input_ms_->value());
  vital::poly_float band_input_y = getOpenGlYForMagnitude(band_rms);

  vital::poly_float scaled_band_rms = vital::utils::sqrt(band_output_ms_->value());
  vital::poly_float band_output_y = getOpenGlYForMagnitude(scaled_band_rms);

  vital::poly_float high_rms = vital::utils::sqrt(high_input_ms_->value());
  vital::poly_float high_input_y = getOpenGlYForMagnitude(high_rms);

  vital::poly_float scaled_high_rms = vital::utils::sqrt(high_output_ms_->value());
  vital::poly_float high_output_y = getOpenGlYForMagnitude(scaled_high_rms);

  int width = getWidth();
  float active_area = 1.0f - 4.0f * kCompressorAreaBuffer;
  float active_section_width = active_area / kMaxBands;

  int low_start = std::round(kCompressorAreaBuffer * width);
  int low_end = std::round((kCompressorAreaBuffer + active_section_width) * width);
  int band_start = std::round((2.0f * kCompressorAreaBuffer + active_section_width) * width);
  int band_end = width - band_start;
  int high_start = width - low_end;
  int high_end = width - low_start;

  if (!low_band_active_) {
    band_start = low_start;
    low_start = low_end = -width;
  }
  if (!high_band_active_) {
    band_end = high_end;
    high_start = high_end = -width;
  }

  setThresholdPositions(low_start, low_end, band_start, band_end, high_start, high_end, 1.0f);
  thresholds_.render(open_gl, true);

  setThresholdPositions(low_start, low_end, band_start, band_end, high_start, high_end, 0.0f);
  thresholds_.render(open_gl, true);

  setThresholdPositions(low_start, low_end, band_start, band_end, high_start, high_end, -1.0f);
  thresholds_.render(open_gl, true);

  setRatioLinePositions(low_start, low_end, band_start, band_end, high_start, high_end);
  ratio_lines_.setColor(findColour(Skin::kLightenScreen, true));
  ratio_lines_.render(open_gl, true);

  renderHover(open_gl, low_start, low_end, band_start, band_end, high_start, high_end);

  int bar_width = kBarWidth * active_section_width * width;
  int low_middle = (low_start + low_end) * 0.5f;
  int band_middle = (band_start + band_end) * 0.5f;
  int high_middle = (high_start + high_end) * 0.5f;

  float gl_bar_width = bar_width * 2.0f / width;
  float low_left = (low_middle - bar_width) * 2.0f / width - 1.0f;
  float low_right = (low_middle + 1) * 2.0f / width - 1.0f;
  float band_left = (band_middle - bar_width) * 2.0f / width - 1.0f;
  float band_right = (band_middle + 1) * 2.0f / width - 1.0f;
  float high_left = (high_middle - bar_width) * 2.0f / width - 1.0f;
  float high_right = (high_middle + 1) * 2.0f / width - 1.0f;
  output_dbs_.setQuad(0, low_left, low_output_y[0] - kOutputBarHeight, gl_bar_width, kOutputBarHeight);
  output_dbs_.setQuad(1, low_right, low_output_y[1] - kOutputBarHeight, gl_bar_width, kOutputBarHeight);
  output_dbs_.setQuad(2, band_left, band_output_y[0] - kOutputBarHeight, gl_bar_width, kOutputBarHeight);
  output_dbs_.setQuad(3, band_right, band_output_y[1] - kOutputBarHeight, gl_bar_width, kOutputBarHeight);
  output_dbs_.setQuad(4, high_left, high_output_y[0] - kOutputBarHeight, gl_bar_width, kOutputBarHeight);
  output_dbs_.setQuad(5, high_right, high_output_y[1] - kOutputBarHeight, gl_bar_width, kOutputBarHeight);

  float input_height = 2.0f / getHeight();
  input_dbs_.setQuad(0, low_left, low_input_y[0] - 0.5f * input_height, gl_bar_width, input_height);
  input_dbs_.setQuad(1, low_right, low_input_y[1] - 0.5f * input_height, gl_bar_width, input_height);
  input_dbs_.setQuad(2, band_left, band_input_y[0] - 0.5f * input_height, gl_bar_width, input_height);
  input_dbs_.setQuad(3, band_right, band_input_y[1] - 0.5f * input_height, gl_bar_width, input_height);
  input_dbs_.setQuad(4, high_left, high_input_y[0] - 0.5f * input_height, gl_bar_width, input_height);
  input_dbs_.setQuad(5, high_right, high_input_y[1] - 0.5f * input_height, gl_bar_width, input_height);

  output_dbs_.setColor(findColour(Skin::kWidgetPrimary1, true));
  output_dbs_.render(open_gl, animate);

  input_dbs_.setColor(findColour(Skin::kWidgetPrimary2, true));
  input_dbs_.render(open_gl, animate);
}

void CompressorEditor::destroy(OpenGlWrapper& open_gl) {
  OpenGlComponent::destroy(open_gl);
  hover_quad_.destroy(open_gl);
  input_dbs_.destroy(open_gl);
  output_dbs_.destroy(open_gl);
  thresholds_.destroy(open_gl);
  ratio_lines_.destroy(open_gl);  
}

void CompressorEditor::setAllValues(vital::control_map& controls) {
  low_upper_threshold_ = controls["compressor_low_upper_threshold"]->value();
  band_upper_threshold_ = controls["compressor_band_upper_threshold"]->value();
  high_upper_threshold_ = controls["compressor_high_upper_threshold"]->value();
  low_lower_threshold_ = controls["compressor_low_lower_threshold"]->value();
  band_lower_threshold_ = controls["compressor_band_lower_threshold"]->value();
  high_lower_threshold_ = controls["compressor_high_lower_threshold"]->value();
  low_upper_ratio_ = controls["compressor_low_upper_ratio"]->value();
  band_upper_ratio_ = controls["compressor_band_upper_ratio"]->value();
  high_upper_ratio_ = controls["compressor_high_upper_ratio"]->value();
  low_lower_ratio_ = controls["compressor_low_lower_ratio"]->value();
  band_lower_ratio_ = controls["compressor_band_lower_ratio"]->value();
  high_lower_ratio_ = controls["compressor_high_lower_ratio"]->value();
}

void CompressorEditor::setLowUpperThreshold(float db, bool clamp) {
  low_upper_threshold_ = db;
  db = vital::utils::clamp(db, kMinEditDb, kMaxEditDb);
  SynthBase* synth = parent_->getSynth();

  if (clamp)
    low_upper_threshold_ = db;
  synth->valueChangedInternal("compressor_low_upper_threshold", db);
  if (low_upper_threshold_ < low_lower_threshold_ && clamp)
    setLowLowerThreshold(db, clamp);

  section_parent_->showPopupDisplay(this, formatString(low_upper_threshold_, " dB"), BubbleComponent::below, true);
}

void CompressorEditor::setBandUpperThreshold(float db, bool clamp) {
  band_upper_threshold_ = db;
  db = vital::utils::clamp(db, kMinEditDb, kMaxEditDb);
  SynthBase* synth = parent_->getSynth();
  
  if (clamp)
    band_upper_threshold_ = db;
  synth->valueChangedInternal("compressor_band_upper_threshold", db);
  if (band_upper_threshold_ < band_lower_threshold_ && clamp)
    setBandLowerThreshold(db, clamp);

  section_parent_->showPopupDisplay(this, formatString(band_upper_threshold_, " dB"), BubbleComponent::below, true);
}

void CompressorEditor::setHighUpperThreshold(float db, bool clamp) {
  high_upper_threshold_ = db;
  db = vital::utils::clamp(db, kMinEditDb, kMaxEditDb);
  SynthBase* synth = parent_->getSynth();
  
  if (clamp)
    high_upper_threshold_ = db;
  synth->valueChangedInternal("compressor_high_upper_threshold", db);
  if (high_upper_threshold_ < high_lower_threshold_ && clamp)
    setHighLowerThreshold(db, clamp);

  section_parent_->showPopupDisplay(this, formatString(high_upper_threshold_, " dB"), BubbleComponent::below, true);
}

void CompressorEditor::setLowLowerThreshold(float db, bool clamp) {
  low_lower_threshold_ = db;
  db = vital::utils::clamp(db, kMinEditDb, kMaxEditDb);
  SynthBase* synth = parent_->getSynth();

  if (clamp)
    low_lower_threshold_ = db;
  synth->valueChangedInternal("compressor_low_lower_threshold", db);
  if (low_lower_threshold_ > low_upper_threshold_ && clamp)
    setLowUpperThreshold(db, clamp);

  section_parent_->showPopupDisplay(this, formatString(low_lower_threshold_, " dB"), BubbleComponent::below, true);
}

void CompressorEditor::setBandLowerThreshold(float db, bool clamp) {
  band_lower_threshold_ = db;
  db = vital::utils::clamp(db, kMinEditDb, kMaxEditDb);
  SynthBase* synth = parent_->getSynth();

  if (clamp)
    band_lower_threshold_ = db;
  synth->valueChangedInternal("compressor_band_lower_threshold", db);
  if (band_lower_threshold_ > band_upper_threshold_ && clamp)
    setBandUpperThreshold(db, clamp);

  section_parent_->showPopupDisplay(this, formatString(band_lower_threshold_, " dB"), BubbleComponent::below, true);
}

void CompressorEditor::setHighLowerThreshold(float db, bool clamp ) {
  high_lower_threshold_ = db;
  db = vital::utils::clamp(db, kMinEditDb, kMaxEditDb);
  SynthBase* synth = parent_->getSynth();

  if (clamp)
    high_lower_threshold_ = db;
  synth->valueChangedInternal("compressor_high_lower_threshold", db);
  if (high_lower_threshold_ > high_upper_threshold_ && clamp)
    setHighUpperThreshold(db, clamp);

  section_parent_->showPopupDisplay(this, formatString(high_lower_threshold_, " dB"), BubbleComponent::below, true);
}

void CompressorEditor::setLowUpperRatio(float ratio) {
  SynthBase* synth = parent_->getSynth();

  low_upper_ratio_ = vital::utils::clamp(ratio, kMinUpperRatio, kMaxUpperRatio);
  synth->valueChangedInternal("compressor_low_upper_ratio", ratio);
}

void CompressorEditor::setBandUpperRatio(float ratio) {
  SynthBase* synth = parent_->getSynth();

  band_upper_ratio_ = vital::utils::clamp(ratio, kMinUpperRatio, kMaxUpperRatio);
  synth->valueChangedInternal("compressor_band_upper_ratio", ratio);
}

void CompressorEditor::setHighUpperRatio(float ratio) {
  SynthBase* synth = parent_->getSynth();

  high_upper_ratio_ = vital::utils::clamp(ratio, kMinUpperRatio, kMaxUpperRatio);
  synth->valueChangedInternal("compressor_high_upper_ratio", ratio);
}

void CompressorEditor::setLowLowerRatio(float ratio) {
  SynthBase* synth = parent_->getSynth();

  low_lower_ratio_ = vital::utils::clamp(ratio, kMinLowerRatio, kMaxLowerRatio);
  synth->valueChangedInternal("compressor_low_lower_ratio", ratio);
}

void CompressorEditor::setBandLowerRatio(float ratio) {
  SynthBase* synth = parent_->getSynth();

  band_lower_ratio_ = vital::utils::clamp(ratio, kMinLowerRatio, kMaxLowerRatio);
  synth->valueChangedInternal("compressor_band_lower_ratio", ratio);
}

void CompressorEditor::setHighLowerRatio(float ratio) {
  SynthBase* synth = parent_->getSynth();

  high_lower_ratio_ = vital::utils::clamp(ratio, kMinLowerRatio, kMaxLowerRatio);
  synth->valueChangedInternal("compressor_high_lower_ratio", ratio);
}

String CompressorEditor::formatValue(float value) {
  static constexpr int number_length = 5;
  static constexpr int max_decimals = 3;

  String format = String(value, max_decimals);
  format = format.substring(0, number_length);
  int spaces = number_length - format.length();

  for (int i = 0; i < spaces; ++i)
    format = " " + format;

  return format;
}

void CompressorEditor::parentHierarchyChanged() {
  if (parent_ == nullptr)
    parent_ = findParentComponentOfClass<SynthGuiInterface>();

  if (section_parent_ == nullptr)
    section_parent_ = findParentComponentOfClass<SynthSection>();

  if (parent_ == nullptr)
    return;

  if (low_input_ms_ == nullptr)
    low_input_ms_ = parent_->getSynth()->getStatusOutput("compressor_low_input");
  if (band_input_ms_ == nullptr)
    band_input_ms_ = parent_->getSynth()->getStatusOutput("compressor_band_input");
  if (high_input_ms_ == nullptr)
    high_input_ms_ = parent_->getSynth()->getStatusOutput("compressor_high_input");
  if (low_output_ms_ == nullptr)
    low_output_ms_ = parent_->getSynth()->getStatusOutput("compressor_low_output");
  if (band_output_ms_ == nullptr)
    band_output_ms_ = parent_->getSynth()->getStatusOutput("compressor_band_output");
  if (high_output_ms_ == nullptr)
    high_output_ms_ = parent_->getSynth()->getStatusOutput("compressor_high_output");

  OpenGlComponent::parentHierarchyChanged();
}

float CompressorEditor::getYForDb(float db) {
  return getHeight() * (1.0f - getOpenGlYForDb(vital::utils::clamp(db, kMinDb, kMaxDb))) * 0.5f;
}

float CompressorEditor::getCompressedDb(float input_db, float upper_threshold, float upper_ratio,
                                        float lower_threshold, float lower_ratio) {
  if (input_db < lower_threshold)
    return vital::utils::interpolate(input_db, lower_threshold, lower_ratio);
  if (input_db > upper_threshold)
    return vital::utils::interpolate(input_db, upper_threshold, upper_ratio);
  return input_db;
}

Colour CompressorEditor::getColorForRatio(float ratio) {
  if (!active_)
    return findColour(Skin::kWidgetSecondaryDisabled, true);

  if (ratio > 0.0f)
    return findColour(Skin::kWidgetSecondary1, true);
  if (ratio < 0.0f)
    return findColour(Skin::kWidgetSecondary2, true);

  return findColour(Skin::kWidgetSecondaryDisabled, true);
}
