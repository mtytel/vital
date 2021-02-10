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

class EnvelopeEditor;
class SynthSlider;

class DragMagnifyingGlass : public OpenGlShapeButton {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void magnifyDragged(Point<float> delta) = 0;
        virtual void magnifyDoubleClicked() = 0;
    };

    DragMagnifyingGlass();

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& e) override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    Point<float> last_position_;
    Point<int> mouse_down_position_;
    std::vector<Listener*> listeners_;
};

class EnvelopeSection : public SynthSection, public DragMagnifyingGlass::Listener {
  public:
    EnvelopeSection(String name, std::string value_prepend,
                    const vital::output_map& mono_modulations,
                    const vital::output_map& poly_modulations);
    virtual ~EnvelopeSection();

    void paintBackground(Graphics& g) override;
    void resized() override;
    void reset() override;
    void magnifyDragged(Point<float> delta) override;
    void magnifyDoubleClicked() override;

  private:
    std::unique_ptr<EnvelopeEditor> envelope_;
    std::unique_ptr<SynthSlider> delay_;
    std::unique_ptr<SynthSlider> attack_;
    std::unique_ptr<SynthSlider> attack_power_;
    std::unique_ptr<SynthSlider> hold_;
    std::unique_ptr<SynthSlider> decay_;
    std::unique_ptr<SynthSlider> decay_power_;
    std::unique_ptr<SynthSlider> sustain_;
    std::unique_ptr<SynthSlider> release_;
    std::unique_ptr<SynthSlider> release_power_;
    std::unique_ptr<DragMagnifyingGlass> drag_magnifying_glass_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeSection)
};

