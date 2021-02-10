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
#include "synth_constants.h"

class Paths {
  public:
    static constexpr int kLogoWidth = 1701;

    Paths() = delete;

    static Path fromSvgData(const void* data, size_t data_size) {
      std::unique_ptr<Drawable> drawable(Drawable::createFromImageData(data, data_size));
      return drawable->getOutlineAsPath();
    }

    static Path vitalRing() {
      Path path = fromSvgData((const void*)BinaryData::vital_ring_svg, BinaryData::vital_ring_svgSize);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(kLogoWidth, kLogoWidth, kLogoWidth, kLogoWidth), 0.2f);
      return path;
    }

    static Path vitalV() {
      Path path = fromSvgData((const void*)BinaryData::vital_v_svg, BinaryData::vital_ring_svgSize);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(kLogoWidth, kLogoWidth, kLogoWidth, kLogoWidth), 0.2f);
      return path;
    }

    static Path vitalWord() {
      return fromSvgData((const void*)BinaryData::vital_word_svg, BinaryData::vital_word_svgSize);
    }

    static Path vitalWordRing() {
      return fromSvgData((const void*)BinaryData::vital_word_ring_svg, BinaryData::vital_word_ring_svgSize);
    }

    static Path chorus() {
      return fromSvgData((const void*)BinaryData::chorus_svg, BinaryData::chorus_svgSize);
    }

    static Path compressor() {
      return fromSvgData((const void*)BinaryData::compressor_svg, BinaryData::compressor_svgSize);
    }

    static Path delay() {
      return fromSvgData((const void*)BinaryData::delay_svg, BinaryData::delay_svgSize);
    }

    static Path distortion() {
      return fromSvgData((const void*)BinaryData::distortion_svg, BinaryData::distortion_svgSize);
    }

    static Path equalizer() {
      return fromSvgData((const void*)BinaryData::equalizer_svg, BinaryData::equalizer_svgSize);
    }

    static Path effectsFilter() {
      return fromSvgData((const void*)BinaryData::effects_filter_svg, BinaryData::effects_filter_svgSize);
    }

    static Path flanger() {
      return fromSvgData((const void*)BinaryData::flanger_svg, BinaryData::flanger_svgSize);
    }

    static Path folder() {
      return fromSvgData((const void*)BinaryData::folder_svg, BinaryData::folder_svgSize);
    }

    static Path phaser() {
      return fromSvgData((const void*)BinaryData::phaser_svg, BinaryData::phaser_svgSize);
    }

    static Path reverb() {
      return fromSvgData((const void*)BinaryData::reverb_svg, BinaryData::reverb_svgSize);
    }

    static Path prev() {
      static const PathStrokeType arrow_stroke(0.1f, PathStrokeType::JointStyle::curved,
                                               PathStrokeType::EndCapStyle::rounded);

      Path prev_line, prev_shape;
      prev_line.startNewSubPath(0.65f, 0.3f);
      prev_line.lineTo(0.35f, 0.5f);
      prev_line.lineTo(0.65f, 0.7f);

      arrow_stroke.createStrokedPath(prev_shape, prev_line);
      prev_shape.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      prev_shape.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return prev_shape;
    }

    static Path next() {
      static const PathStrokeType arrow_stroke(0.1f, PathStrokeType::JointStyle::curved,
                                               PathStrokeType::EndCapStyle::rounded);

      Path next_line, next_shape;
      next_line.startNewSubPath(0.35f, 0.3f);
      next_line.lineTo(0.65f, 0.5f);
      next_line.lineTo(0.35f, 0.7f);

      arrow_stroke.createStrokedPath(next_shape, next_line);
      next_shape.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      next_shape.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return next_shape;
    }

    static Path clock() {
      static const float kClockAngle = 1.0f;
      static const float kClockWidth = 0.4f;
      static const float kBuffer = (1.0f - kClockWidth) / 2.0f;

      Path path;
      path.addPieSegment(kBuffer, kBuffer, kClockWidth, kClockWidth, 0.0f, kClockAngle - 2.0f * vital::kPi, 0.0f);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path dragDropArrows() {
      return fromSvgData((const void*)BinaryData::drag_drop_arrows_svg, BinaryData::drag_drop_arrows_svgSize);
    }

    static Path note() {
      static constexpr float kLeftAdjustment = 1.0f / 32.0f;
      static constexpr float kNoteWidth = 1.0f / 4.0f;
      static constexpr float kNoteHeight = 1.0f / 5.0f;
      static constexpr float kBarHeight = 2.0f / 5.0f;
      static constexpr float kBarWidth = 1.0f / 20.0f;
      static constexpr float kY = 1.0f - (1.0f - kBarHeight + kNoteHeight / 2.0f) / 2.0f;
      static constexpr float kX = (1.0f - kNoteWidth) / 2.0f - kLeftAdjustment;

      Path path;
      path.addEllipse(kX, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path tripletNotes() {
      static constexpr float kNoteWidth = 1.0f / 5.0f;
      static constexpr float kNoteHeight = 1.0f / 6.0f;
      static constexpr float kX = (1.0f - 3.0f * kNoteWidth) / 2.0f;
      static constexpr float kBarWidth = 1.0f / 20.0f;
      static constexpr float kBarHeight = 1.0f / 4.0f;
      static constexpr float kY = 1.0f - (1.0f - kBarHeight) / 2.0f;

      Path path;
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight - kBarWidth,
                        2.0f * kNoteWidth + kBarWidth, kBarWidth);

      path.addEllipse(kX, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.addEllipse(kX + kNoteWidth, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth + kNoteWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.addEllipse(kX + 2.0f * kNoteWidth, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth + 2.0f * kNoteWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path menu() {
      Path path;
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      path.addLineSegment(Line<float>(0.2f, 0.3f, 0.8f, 0.3f), 0.05f);
      path.addLineSegment(Line<float>(0.2f, 0.5f, 0.8f, 0.5f), 0.05f);
      path.addLineSegment(Line<float>(0.2f, 0.7f, 0.8f, 0.7f), 0.05f);
      return path;
    }

    static Path menu(int width) {
      static constexpr float kBuffer = 0.2f;
      static constexpr float kLineWidth = 0.04f;
      static constexpr float kSpacing = 0.2f;

      int buffer = std::round(width * kBuffer);
      int line_width = std::max<int>(1, width * kLineWidth);
      int spacing = width * kSpacing;

      float center = (line_width % 2) * 0.5f + (width / 2);
      Path path;
      path.addLineSegment(Line<float>(buffer, center - spacing, width - buffer, center - spacing), line_width);
      path.addLineSegment(Line<float>(buffer, center, width - buffer, center), line_width);
      path.addLineSegment(Line<float>(buffer, center + spacing, width - buffer, center + spacing), line_width);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static Path plus(int width) {
      static constexpr float kBuffer = 0.35f;
      static constexpr float kLineWidth = 0.04f;

      int buffer = std::round(width * kBuffer);
      int line_width = std::max<int>(1, width * kLineWidth);

      float center = (line_width % 2) * 0.5f + (width / 2);
      Path path;
      path.addLineSegment(Line<float>(buffer, center, width - buffer, center), line_width);
      path.addLineSegment(Line<float>(center, buffer, center, width - buffer), line_width);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static Path plusOutline() {
      static constexpr float kBuffer = 0.2f;
      static constexpr float kLineWidth = 0.15f;

      float start = kBuffer;
      float end = 1.0f - kBuffer;
      float close = 0.5f - kLineWidth * 0.5f;
      float far = 0.5f + kLineWidth * 0.5f;

      Path path;
      path.startNewSubPath(start, close);
      path.lineTo(start, far);
      path.lineTo(close, far);
      path.lineTo(close, end);
      path.lineTo(far, end);
      path.lineTo(far, far);
      path.lineTo(end, far);
      path.lineTo(end, close);
      path.lineTo(far, close);
      path.lineTo(far, start);
      path.lineTo(close, start);
      path.lineTo(close, close);
      path.lineTo(start, close);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path save(int width) {
      static constexpr float kLineWidth = 0.04f;
      static constexpr float kSpacingX = 0.3f;
      static constexpr float kSpacingY = 0.25f;
      static constexpr float kArrowMovement = 0.14f;
      static constexpr float kBoxWrap = 0.07f;

      int line_width = std::max<int>(1, width * kLineWidth);
      int spacing_x = width * kSpacingX;
      int spacing_y = width * kSpacingY;
      float movement = width * kArrowMovement / 2.0f;
      float wrap = width * kBoxWrap;

      float center = (line_width % 2) * 0.5f + (width / 2);

      Path outline;
      outline.startNewSubPath(center, center - spacing_y);
      outline.lineTo(center, center + movement);

      outline.startNewSubPath(center - 2 * movement, center - movement);
      outline.lineTo(center, center + movement);
      outline.lineTo(center + 2 * movement, center - movement);

      outline.startNewSubPath(center - spacing_x + wrap, center);
      outline.lineTo(center - spacing_x, center);
      outline.lineTo(center - spacing_x, center + spacing_y);
      outline.lineTo(center + spacing_x, center + spacing_y);
      outline.lineTo(center + spacing_x, center);
      outline.lineTo(center + spacing_x - wrap, center);

      Path path;
      PathStrokeType stroke(line_width, PathStrokeType::curved, PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static Path downTriangle() {
      Path path;

      path.startNewSubPath(0.33f, 0.4f);
      path.lineTo(0.66f, 0.4f);
      path.lineTo(0.5f, 0.6f);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path upTriangle() {
      Path path;

      path.startNewSubPath(0.33f, 0.6f);
      path.lineTo(0.66f, 0.6f);
      path.lineTo(0.5f, 0.4f);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path exitX() {
      Path outline;
      outline.startNewSubPath(0.25f, 0.25f);
      outline.lineTo(0.75f, 0.75f);
      outline.startNewSubPath(0.25f, 0.75f);
      outline.lineTo(0.75f, 0.25f);

      Path path;
      PathStrokeType stroke(0.03f, PathStrokeType::curved, PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path thickX() {
      Path outline;
      outline.startNewSubPath(0.25f, 0.25f);
      outline.lineTo(0.75f, 0.75f);
      outline.startNewSubPath(0.25f, 0.75f);
      outline.lineTo(0.75f, 0.25f);

      Path path;
      PathStrokeType stroke(0.1f, PathStrokeType::curved, PathStrokeType::butt);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path star() {
      Path path;
      path.addStar(Point<float>(0.5f, 0.5f), 5, 0.2f, 0.4f);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path keyboard() {
      Path path;
      path.addLineSegment(Line<float>(0.2f, 0.2f, 0.2f, 0.2f), 0.01f);
      path.addLineSegment(Line<float>(0.8f, 0.8f, 0.8f, 0.8f), 0.01f);

      path.startNewSubPath(0.24f, 0.35f);
      path.lineTo(0.24f, 0.65f);
      path.lineTo(0.41f, 0.65f);
      path.lineTo(0.41f, 0.5f);
      path.lineTo(0.35f, 0.5f);
      path.lineTo(0.35f, 0.35f);
      path.closeSubPath();

      path.startNewSubPath(0.48f, 0.65f);
      path.lineTo(0.48f, 0.35f);
      path.lineTo(0.52f, 0.35f);
      path.lineTo(0.52f, 0.5f);
      path.lineTo(0.58f, 0.5f);
      path.lineTo(0.58f, 0.65f);
      path.lineTo(0.42f, 0.65f);
      path.lineTo(0.42f, 0.5f);
      path.lineTo(0.48f, 0.5f);
      path.closeSubPath();

      path.startNewSubPath(0.65f, 0.35f);
      path.lineTo(0.76f, 0.35f);
      path.lineTo(0.76f, 0.65f);
      path.lineTo(0.59f, 0.65f);
      path.lineTo(0.59f, 0.5f);
      path.lineTo(0.65f, 0.5f);
      path.lineTo(0.65f, 0.35f);
      path.closeSubPath();

      return path;
    }

    static Path keyboardBordered() {
      Path board = keyboard();

      board.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.01f);
      board.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.01f);
      return board;
    }

    static Path fullKeyboard() {
      Path path;
      path.startNewSubPath(1, 0);
      path.lineTo(1, 2);
      path.lineTo(23, 2);
      path.lineTo(23, 1);
      path.lineTo(15, 1);
      path.lineTo(15, 0);
      path.closeSubPath();

      path.startNewSubPath(29, 0);
      path.lineTo(29, 1);
      path.lineTo(25, 1);
      path.lineTo(25, 2);
      path.lineTo(47, 2);
      path.lineTo(47, 1);
      path.lineTo(43, 1);
      path.lineTo(43, 0);
      path.closeSubPath();

      path.startNewSubPath(57, 0);
      path.lineTo(57, 1);
      path.lineTo(49, 1);
      path.lineTo(49, 2);
      path.lineTo(71, 2);
      path.lineTo(71, 0);
      path.closeSubPath();

      path.startNewSubPath(73, 0);
      path.lineTo(73, 2);
      path.lineTo(95, 2);
      path.lineTo(95, 1);
      path.lineTo(83, 1);
      path.lineTo(83, 0);
      path.closeSubPath();

      path.startNewSubPath(99, 0);
      path.lineTo(99, 1);
      path.lineTo(97, 1);
      path.lineTo(97, 2);
      path.lineTo(119, 2);
      path.lineTo(119, 1);
      path.lineTo(112, 1);
      path.lineTo(112, 0);
      path.closeSubPath();

      path.startNewSubPath(128, 0);
      path.lineTo(128, 1);
      path.lineTo(121, 1);
      path.lineTo(121, 2);
      path.lineTo(143, 2);
      path.lineTo(143, 1);
      path.lineTo(141, 1);
      path.lineTo(141, 0);
      path.closeSubPath();

      path.startNewSubPath(157, 0);
      path.lineTo(157, 1);
      path.lineTo(145, 1);
      path.lineTo(145, 2);
      path.lineTo(167, 2);
      path.lineTo(167, 0);
      path.closeSubPath();

      return path;
    }

    static Path gear() {
      static constexpr float kRadius = 0.3f;
      static constexpr float kGearOuterRatio = 1.2f;
      static constexpr int kNumGearTeeth = 8;

      Path path;
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      float offset = 0.5f - kRadius;
      float diameter = 2.0f * kRadius;
      path.addPieSegment(offset, offset, diameter, diameter, 0.0f, 2.0f * vital::kPi, 0.5f);
      for (int i = 0; i < kNumGearTeeth; ++i) {
        float phase = 2.0f * i * vital::kPi / kNumGearTeeth;
        float x_offset = kRadius * cosf(phase);
        float y_offset = kRadius * sinf(phase);
        Line<float> line(0.5f + x_offset, 0.5f + y_offset,
                         0.5f + kGearOuterRatio * x_offset, 0.5f + kGearOuterRatio * y_offset);
        path.addLineSegment(line, 0.13f);
      }

      return path;
    }

    static Path magnifyingGlass() {
      static constexpr float kRadius = 0.22f;
      static constexpr float kWidthRatio = 0.3f;
      static constexpr float kGlassOffset = 0.2f;
      static constexpr float kSqrt2 = 1.41421356237f;

      Path path;
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      float diameter = 2.0f * kRadius;
      path.addPieSegment(kGlassOffset, kGlassOffset, diameter, diameter, 0.0f, 2.0f * vital::kPi, 1.0f - kWidthRatio);

      float line_width = kWidthRatio * kRadius;
      float line_start = kGlassOffset + kSqrt2 * kRadius + line_width / 2.0f;
      path.addLineSegment(Line<float>(line_start, line_start, 1.0f - kGlassOffset, 1.0f - kGlassOffset), line_width);

      return path;
    }

    static Path save() {
      Path outline;
      outline.startNewSubPath(0.5f, 0.25f);
      outline.lineTo(0.5f, 0.6f);
      outline.startNewSubPath(0.35f, 0.45f);
      outline.lineTo(0.5f, 0.6f);
      outline.lineTo(0.65f, 0.45f);

      outline.startNewSubPath(0.25f, 0.5f);
      outline.lineTo(0.2f, 0.5f);
      outline.lineTo(0.2f, 0.75f);
      outline.lineTo(0.8f, 0.75f);
      outline.lineTo(0.8f, 0.5f);
      outline.lineTo(0.75f, 0.5f);

      Path path;
      PathStrokeType stroke(0.05f, PathStrokeType::curved, PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path loop() {
      Path outline;
      outline.startNewSubPath(0.68f, 0.3f);
      outline.lineTo(0.85f, 0.3f);
      outline.lineTo(0.85f, 0.7f);
      outline.lineTo(0.15f, 0.7f);
      outline.lineTo(0.15f, 0.3f);
      outline.lineTo(0.61f, 0.3f);
      PathStrokeType outer_stroke(0.12f, PathStrokeType::curved, PathStrokeType::rounded);

      Path path;
      outer_stroke.createStrokeWithArrowheads(path, outline, 0.0f, 0.0f, 0.4f, 0.26f);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path bounce() {
      Path left_outline, right_outline;
      left_outline.startNewSubPath(0.5f, 0.5f);
      left_outline.lineTo(0.0f, 0.5f);
      left_outline.startNewSubPath(0.5f, 0.5f);
      left_outline.lineTo(1.0f, 0.5f);

      PathStrokeType stroke(0.12f, PathStrokeType::curved, PathStrokeType::butt);
      Path left_path, right_path;
      stroke.createStrokeWithArrowheads(left_path, left_outline, 0.0f, 0.0f, 0.4f, 0.26f);
      stroke.createStrokeWithArrowheads(right_path, right_outline, 0.0f, 0.0f, 0.4f, 0.26f);

      Path path;
      path.addPath(left_path);
      path.addPath(right_path);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path bipolar() {
      Path left_outline, right_outline;
      left_outline.startNewSubPath(0.3f, 0.5f);
      left_outline.lineTo(0.0f, 0.5f);
      left_outline.startNewSubPath(0.7f, 0.5f);
      left_outline.lineTo(1.0f, 0.5f);

      PathStrokeType stroke(0.12f, PathStrokeType::curved, PathStrokeType::rounded);
      Path left_path, right_path;
      stroke.createStrokeWithArrowheads(left_path, left_outline, 0.0f, 0.0f, 0.4f, 0.26f);
      stroke.createStrokeWithArrowheads(right_path, right_outline, 0.0f, 0.0f, 0.4f, 0.26f);

      Path path;
      path.addEllipse(0.4f, 0.4f, 0.2f, 0.2f);
      path.addPath(left_path);
      path.addPath(right_path);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path shuffle() {
      Path over;
      over.startNewSubPath(0.1f, 0.7f);
      over.lineTo(0.25f, 0.7f);
      over.lineTo(0.55f, 0.3f);
      over.lineTo(0.95f, 0.3f);

      Path under_first;
      under_first.startNewSubPath(0.1f, 0.3f);
      under_first.lineTo(0.25f, 0.3f);
      under_first.lineTo(0.325f, 0.4f);

      Path under_second;
      under_second.startNewSubPath(0.475f, 0.6f);
      under_second.lineTo(0.55f, 0.7f);
      under_second.lineTo(0.95f, 0.7f);

      PathStrokeType stroke(0.12f, PathStrokeType::curved, PathStrokeType::butt);
      Path over_path;
      stroke.createStrokeWithArrowheads(over_path, over, 0.0f, 0.0f, 0.35f, 0.26f);
      Path under_path_first;
      stroke.createStrokedPath(under_path_first, under_first);
      Path under_path_second;
      stroke.createStrokeWithArrowheads(under_path_second, under_second, 0.0f, 0.0f, 0.35f, 0.26f);

      Path path;
      path.addPath(over_path);
      path.addPath(under_path_first);
      path.addPath(under_path_second);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path pencil() {
      static constexpr float kPencilDimension = 0.14f;
      static constexpr float kEraserLength = 0.6f * kPencilDimension;
      static constexpr float kSeparatorWidth = 0.15f * kPencilDimension;
      static constexpr float kPencilRemoval = kEraserLength + kSeparatorWidth;
      static constexpr float kBorder = 0.22f;

      Path path;
      path.startNewSubPath(kBorder, 1.0f - kBorder);
      path.lineTo(kBorder + kPencilDimension, 1.0f - kBorder);
      path.lineTo(1.0f - kBorder - kPencilRemoval, kBorder + kPencilDimension + kPencilRemoval);
      path.lineTo(1.0f - kBorder - kPencilRemoval - kPencilDimension, kBorder + kPencilRemoval);
      path.lineTo(kBorder, 1.0f - kBorder - kPencilDimension);
      path.closeSubPath();

      path.startNewSubPath(1.0f - kBorder - kPencilDimension, kBorder);
      path.lineTo(1.0f - kBorder, kBorder + kPencilDimension);
      path.lineTo(1.0f - kBorder - kEraserLength, kBorder + kPencilDimension + kEraserLength);
      path.lineTo(1.0f - kBorder - kEraserLength - kPencilDimension, kBorder + kEraserLength);
      path.closeSubPath();
      
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path halfSinCurve() {
      static constexpr float kBorder = 0.15f;
      static constexpr float kLineWidth = 0.1f;
      static constexpr float kEndpointStrokeWidth = 0.08f;
      static constexpr float kEndpointRadius = 0.09f;
      static constexpr int kNumCurvePoints = 16;
      static constexpr float kFullRadians = vital::kPi * 2.0f;
      static constexpr float kBumpIn = kEndpointRadius;
      static constexpr float kAdjustXIn = kBumpIn + kEndpointRadius / 2.0f;

      Path curve;
      float start_x = kBorder + kAdjustXIn;
      float start_y = 1.0f - kBorder - kBumpIn;
      float end_x = 1.0f - kBorder - kAdjustXIn;
      float end_y = kBorder + kBumpIn;

      curve.startNewSubPath(start_x, end_x);
      for (int i = 0; i < kNumCurvePoints; ++i) {
        float t = (1.0f + i) / kNumCurvePoints;
        float x = t * end_x + (1.0f - t) * start_x;
        float y_t = sinf((t - 0.5f) * vital::kPi) * 0.5f + 0.5f;
        float y = y_t * end_y + (1.0f - y_t) * start_y;
        curve.lineTo(x, y);
      }

      Path path;
      PathStrokeType line_stroke(kLineWidth, PathStrokeType::curved, PathStrokeType::butt);
      PathStrokeType endpoint_stroke(kEndpointStrokeWidth, PathStrokeType::curved, PathStrokeType::butt);
      line_stroke.createStrokedPath(path, curve);

      Path arc;
      arc.addCentredArc(end_x + kBumpIn, end_y, kEndpointRadius, kEndpointRadius, 0.0f, 0.0f, kFullRadians, true);
      arc.addCentredArc(start_x - kBumpIn, start_y, kEndpointRadius, kEndpointRadius, 0.0f, 0.0f, kFullRadians, true);
      Path end_points;
      endpoint_stroke.createStrokedPath(end_points, arc);
      path.addPath(end_points);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path ellipses() {
      Path path;
      path.addEllipse(0.1f, 0.4f, 0.2f, 0.2f);
      path.addEllipse(0.4f, 0.4f, 0.2f, 0.2f);
      path.addEllipse(0.7f, 0.4f, 0.2f, 0.2f);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path paintBrush() {
      static constexpr float kBrushWideDimension = 0.3f;
      static constexpr float kBrushHandleDimension = 0.08f;
      static constexpr float kBrushLength = 0.6f * kBrushWideDimension;
      static constexpr float kSeparatorWidth = 0.15f * kBrushWideDimension;
      static constexpr float kConnectionDistance = kBrushLength + kSeparatorWidth;
      static constexpr float kHandleDistance = kConnectionDistance + kBrushWideDimension * 0.8f;
      static constexpr float kBorder = 0.15f;

      Path path;
      path.startNewSubPath(kBorder, 1.0f - kBorder);
      path.lineTo(kBorder + kBrushHandleDimension, 1.0f - kBorder);
      path.lineTo(1.0f - kBorder - kHandleDistance, kBorder + kBrushHandleDimension + kHandleDistance);
      path.lineTo(1.0f - kBorder - kConnectionDistance, kBorder + kBrushWideDimension + kConnectionDistance);
      path.lineTo(1.0f - kBorder - kConnectionDistance - kBrushWideDimension, kBorder + kConnectionDistance);
      path.lineTo(1.0f - kBorder - kHandleDistance - kBrushHandleDimension, kBorder + kHandleDistance);
      path.lineTo(kBorder, 1.0f - kBorder - kBrushHandleDimension);
      path.closeSubPath();

      path.startNewSubPath(1.0f - kBorder - kBrushWideDimension, kBorder);
      path.lineTo(1.0f - kBorder, kBorder + kBrushWideDimension);
      path.lineTo(1.0f - kBorder - kBrushLength, kBorder + kBrushWideDimension + kBrushLength);
      path.lineTo(1.0f - kBorder - kBrushLength - kBrushWideDimension, kBorder + kBrushLength);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path createFilterStroke(const Path& outline, float line_thickness = 0.1f) {
      PathStrokeType stroke(line_thickness, PathStrokeType::curved, PathStrokeType::rounded);

      Path path;
      stroke.createStrokedPath(path, outline);
      return path;
    }

    static Path lowPass(float line_thickness = 0.1f) {
      Path outline;
      outline.startNewSubPath(1.0f, 0.8f);
      outline.lineTo(0.7f, 0.3f);
      outline.lineTo(0.5f, 0.5f);
      outline.lineTo(0.0f, 0.5f);
      Path path = createFilterStroke(outline, line_thickness);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static Path highPass() {
      Path outline;
      outline.startNewSubPath(0.0f, 0.8f);
      outline.lineTo(0.3f, 0.3f);
      outline.lineTo(0.5f, 0.5f);
      outline.lineTo(1.0f, 0.5f);
      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static Path bandPass() {
      static constexpr float kMiddle = 3.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kMiddle);
      outline.lineTo(0.5f, 0.25f);
      outline.lineTo(2.0f / 3.0f, kMiddle);
      outline.lineTo(1.0f, kBottom);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static Path leftArrow() {
      static constexpr float kArrowAmount = 1.0f / 3.0f;
      static constexpr float kBuffer = 0.0f;

      Path outline;
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(kBuffer, 0.5f);
      outline.startNewSubPath(kBuffer, 0.5f);
      outline.lineTo(kBuffer + kArrowAmount, 0.5f - kArrowAmount);
      outline.startNewSubPath(kBuffer, 0.5f);
      outline.lineTo(kBuffer + kArrowAmount, 0.5f + kArrowAmount);
      return createFilterStroke(outline);
    }

    static Path rightArrow() {
      static constexpr float kArrowAmount = 1.0f / 3.0f;
      static constexpr float kBuffer = 0.0f;

      Path outline;
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(kBuffer, 0.5f);
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(1.0f - kArrowAmount - kBuffer, 0.5f - kArrowAmount);
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(1.0f - kArrowAmount - kBuffer, 0.5f + kArrowAmount);
      return createFilterStroke(outline);
    }

    static Path phaser1() {
      Path outline;
      outline.startNewSubPath(0.0f, 0.5f);
      outline.lineTo(1.0f / 3.0f, 3.0f / 4.0f);
      outline.lineTo(1.0f / 2.0f, 1.0f / 4.0f);
      outline.lineTo(2.0f / 3.0f, 3.0f / 4.0f);
      outline.lineTo(1.0f, 0.5f);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.15f, 1.0f, 0.15f), 0.1f);
      path.addLineSegment(Line<float>(1.0f, 0.85f, 1.0f, 0.85f), 0.1f);
      return path;
    }

    static Path phaser3() {
      static constexpr int kNumHumps = 5;

      static const float kUp = 1.0f / 4.0f;
      static const float kDown = 3.0f / 4.0f;

      float delta = 1.0f / (2 * kNumHumps + 2);
      Path outline;
      outline.startNewSubPath(0.0f, 0.5f);

      float position = 0.0f;
      for (int i = 0; i < kNumHumps; ++i) {
        position += delta;
        outline.lineTo(position, kDown);
        position += delta;
        outline.lineTo(position, kUp);
      }
      position += delta;
      outline.lineTo(position, kDown);
      outline.lineTo(1.0f, 0.5f);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.15f, 1.0f, 0.15f), 0.1f);
      path.addLineSegment(Line<float>(1.0f, 0.85f, 1.0f, 0.85f), 0.1f);
      return path;
    }

    static Path notch() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      Path outline;
      outline.startNewSubPath(0.0f, kTop);
      outline.lineTo(1.0f / 3.0f, kTop);
      outline.lineTo(1.0f / 2.0f, kBottom);
      outline.lineTo(2.0f / 3.0f, kTop);
      outline.lineTo(1.0f, kTop);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }

    static Path narrowBand() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kBottom);
      outline.lineTo(0.5f, kTop);
      outline.lineTo(2.0f / 3.0f, kBottom);
      outline.lineTo(1.0f, kBottom);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }

    static Path wideBand() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kTop);
      outline.lineTo(2.0f / 3.0f, kTop);
      outline.lineTo(1.0f, kBottom);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }
};

