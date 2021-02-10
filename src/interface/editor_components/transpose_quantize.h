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
#include "common.h"
#include "open_gl_image_component.h"
#include "synth_section.h"

class TransposeQuantizeCallOut : public SynthSection {
  public:
    static constexpr float kTitleHeightRatio = 0.2f;
    static constexpr float kGlobalHeightRatio = 0.2f;
    static constexpr float kTitleTextRatio = 0.7f;

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void quantizeUpdated() = 0;
    };

    TransposeQuantizeCallOut(bool* selected, bool* global_snap);

    void paint(Graphics& g) override;
    void resized() override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    void buttonClicked(Button* clicked_button) override;

    int getHoverIndex(const MouseEvent& e);

    void notify() {
      for (Listener* listener : listeners_)
        listener->quantizeUpdated();
    }

    void addQuantizeListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    std::vector<Listener*> listeners_;
    Rectangle<float> key_bounds_[vital::kNotesPerOctave];
    std::unique_ptr<ToggleButton> global_snap_button_;
    bool* selected_;
    bool* global_snap_;
    int hover_index_;
    bool enabling_;
    bool disabling_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransposeQuantizeCallOut)
};

class TransposeQuantizeButton : public OpenGlImageComponent, public TransposeQuantizeCallOut::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void quantizeUpdated() = 0;
    };

    TransposeQuantizeButton();

    void paint(Graphics& g) override;
    void paintBackground(Graphics& g) override { }
    void resized() override;

    void mouseDown(const MouseEvent& e) override;
    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    void quantizeUpdated() override;
    int getValue();
    void setValue(int value);
    void addQuantizeListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    std::vector<Listener*> listeners_;
    bool selected_[vital::kNotesPerOctave];
    bool global_snap_;
    Rectangle<float> key_bounds_[vital::kNotesPerOctave];
    bool hover_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransposeQuantizeButton)
};
