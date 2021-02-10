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

#include "volume_section.h"
#include "peak_meter_viewer.h"
#include "fonts.h"
#include "skin.h"
#include "synth_slider.h"
#include "synth_parameters.h"

class VolumeSlider : public SynthSlider {
  public:
    VolumeSlider(String name) : SynthSlider(name), point_y_(0), end_y_(1) {
      paintToImage(true);
      details_ = vital::Parameters::getDetails("volume");
    }

    void paint(Graphics& g) override {
      float x = getPositionOfValue(getValue());

      Path arrow;
      arrow.startNewSubPath(x, point_y_);
      int arrow_height = end_y_ - point_y_;
      arrow.lineTo(x + arrow_height / 2.0f, end_y_);
      arrow.lineTo(x - arrow_height / 2.0f, end_y_);
      arrow.closeSubPath();
      g.setColour(findColour(Skin::kLinearSliderThumb, true));
      g.fillPath(arrow);
    }

    void setPointY(int y) { point_y_ = y; repaint(); }
    void setEndY(int y) { end_y_ = y; repaint(); }
    int getEndY() { return end_y_; }

  private:
    vital::ValueDetails details_;
    int point_y_;
    int end_y_;
};

VolumeSection::VolumeSection(String name) : SynthSection(name) {
  peak_meter_left_ = std::make_unique<PeakMeterViewer>(true);
  addOpenGlComponent(peak_meter_left_.get());
  peak_meter_right_ = std::make_unique<PeakMeterViewer>(false);
  addOpenGlComponent(peak_meter_right_.get());

  volume_ = std::make_unique<VolumeSlider>("volume");
  addSlider(volume_.get());
  volume_->setSliderStyle(Slider::LinearBar);
  volume_->setPopupPlacement(BubbleComponent::below);
}

VolumeSection::~VolumeSection() { }

int VolumeSection::getMeterHeight() {
  return getHeight() / 8;
}

int VolumeSection::getBuffer() {
  return getHeight() / 2 - getMeterHeight();
}

void VolumeSection::resized() {
  int meter_height = getMeterHeight();
  int volume_height = meter_height * 6.0f;
  int end_volume = meter_height * 3.5f;
  int padding = 1;
  int buffer = getBuffer();

  peak_meter_left_->setBounds(0, buffer, getWidth(), meter_height);
  peak_meter_right_->setBounds(0, peak_meter_left_->getBottom() + padding, getWidth(), meter_height);
  volume_->setPointY(peak_meter_right_->getBottom() + padding - buffer);
  volume_->setEndY(end_volume);
  volume_->setBounds(0, buffer, getWidth(), volume_height);

  SynthSection::resized();
}

void VolumeSection::paintBackground(Graphics& g) {
  SynthSection::paintKnobShadows(g);
  SynthSection::paintChildrenBackgrounds(g);

  int ticks_y = peak_meter_right_->getBottom() + getPadding();
  int tick_height = peak_meter_right_->getHeight() / 2;
  vital::ValueDetails details = vital::Parameters::getDetails("volume");
  
  g.setColour(findColour(Skin::kLightenScreen, true));
  for (int decibel = -66; decibel <= 6; decibel += 6) {
    float offset = decibel - details.post_offset;
    float percent = offset * offset / (details.max - details.min);
    int x = percent * getWidth();
    g.drawRect(x, ticks_y, 1, tick_height);
  }
}
