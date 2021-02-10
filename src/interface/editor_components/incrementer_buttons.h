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
#include "skin.h"

class IncrementerButtons : public Component, public Button::Listener {
  public:
    IncrementerButtons(Slider* slider) : slider_(slider), active_(true) {
      increment_ = std::make_unique<ShapeButton>("Increment", Colours::black, Colours::black, Colours::black);
      addAndMakeVisible(increment_.get());
      increment_->addListener(this);
      Path increment_shape;
      increment_shape.startNewSubPath(Point<float>(0.5f, 0.1f));
      increment_shape.lineTo(Point<float>(0.2f, 0.45f));
      increment_shape.lineTo(Point<float>(0.8f, 0.45f));
      increment_shape.closeSubPath();

      increment_shape.startNewSubPath(Point<float>(0.0f, 0.0f));
      increment_shape.closeSubPath();
      increment_shape.startNewSubPath(Point<float>(1.0f, 0.5f));
      increment_shape.closeSubPath();

      increment_shape.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      increment_shape.addLineSegment(Line<float>(0.5f, 0.5f, 0.5f, 0.5f), 0.2f);
      increment_->setShape(increment_shape, true, true, false);

      decrement_ = std::make_unique<ShapeButton>("Increment", Colours::black, Colours::black, Colours::black);
      addAndMakeVisible(decrement_.get());
      decrement_->addListener(this);
      Path decrement_shape;
      decrement_shape.startNewSubPath(Point<float>(0.5f, 0.4f));
      decrement_shape.lineTo(Point<float>(0.2f, 0.05f));
      decrement_shape.lineTo(Point<float>(0.8f, 0.05f));
      decrement_shape.closeSubPath();

      decrement_shape.startNewSubPath(Point<float>(0.0f, 0.0f));
      decrement_shape.closeSubPath();
      decrement_shape.startNewSubPath(Point<float>(1.0f, 0.5f));
      decrement_shape.closeSubPath();

      decrement_shape.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      decrement_shape.addLineSegment(Line<float>(0.5f, 0.5f, 0.5f, 0.5f), 0.2f);
      decrement_->setShape(decrement_shape, true, true, false);
    }

    void setActive(bool active) {
      active_ = active;
      repaint();
    }

    void resized() override {
      Rectangle<int> increment_bounds = getLocalBounds();
      Rectangle<int> decrement_bounds = increment_bounds.removeFromBottom(getHeight() / 2);
      increment_->setBounds(increment_bounds);
      decrement_->setBounds(decrement_bounds);
    }

    void paint(Graphics& g) override {
      setColors();
    }

    void buttonClicked(Button* clicked_button) override {
      double value = slider_->getValue();
      if (clicked_button == increment_.get())
        slider_->setValue(value + 1.0);
      else if (clicked_button == decrement_.get())
        slider_->setValue(value - 1.0);
    }

  private:
    void setColors() {
      Colour normal = findColour(Skin::kIconButtonOff, true);
      Colour hover = findColour(Skin::kIconButtonOffHover, true);
      Colour down = findColour(Skin::kIconButtonOffPressed, true);
      increment_->setColours(normal, hover, down);
      decrement_->setColours(normal, hover, down);
    }

    Slider* slider_;
    bool active_;

    std::unique_ptr<ShapeButton> increment_;
    std::unique_ptr<ShapeButton> decrement_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IncrementerButtons)
};

