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

#include "synth_section.h"

class PresetSelector : public SynthSection {
  public:
    static constexpr float kDefaultFontHeightRatio = 0.63f;

    class Listener {
      public:
        virtual ~Listener() { }

        virtual void prevClicked() = 0;
        virtual void nextClicked() = 0;
        virtual void textMouseUp(const MouseEvent& e) { }
        virtual void textMouseDown(const MouseEvent& e) { }
    };

    PresetSelector();
    ~PresetSelector();

    void paintBackground(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void buttonClicked(Button* clicked_button) override;
    void mouseEnter(const MouseEvent& e) override { hover_ = true; }
    void mouseExit(const MouseEvent& e) override { hover_ = false; }

    void setText(String text);
    void setText(String left, String center, String right);
    String getText() { return text_->getText(); }
    void setFontRatio(float ratio) { font_height_ratio_ = ratio; }
    void setRoundAmount(float round_amount) { round_amount_ = round_amount; }

    void addListener(Listener* listener) { listeners_.push_back(listener); }

    void clickPrev();
    void clickNext();
    void setTextComponent(bool text_component) { text_component_ = text_component; }

  private:
    void textMouseDown(const MouseEvent& e);
    void textMouseUp(const MouseEvent& e);

    std::vector<Listener*> listeners_;

    float font_height_ratio_;
    float round_amount_;
    bool hover_;
    bool text_component_;

    std::unique_ptr<PlainTextComponent> text_;
    std::unique_ptr<OpenGlShapeButton> prev_preset_;
    std::unique_ptr<OpenGlShapeButton> next_preset_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetSelector)
};

