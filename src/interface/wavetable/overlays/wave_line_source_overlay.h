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

#include "wave_line_source.h"
#include "line_editor.h"
#include "wavetable_component_overlay.h"

class WaveLineSourceOverlay : public WavetableComponentOverlay, public LineEditor::Listener {
  public:
    static constexpr int kDefaultXGrid = 6;
    static constexpr int kDefaultYGrid = 4;
    static constexpr float kFillAlpha = 0.6f;

    WaveLineSourceOverlay();
    virtual ~WaveLineSourceOverlay();

    void resized() override;

    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds) override;
    virtual bool setTimeDomainBounds(Rectangle<int> bounds) override;

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override {
      editor_->setSizeRatio(getSizeRatio());
      SynthSection::renderOpenGlComponents(open_gl, animate);
    }

    void setPhase(float phase) override { }
    void lineEditorScrolled(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void togglePaintMode(bool enabled, bool temporary_switch) override { }
    void fileLoaded() override;
    void importLfo() override { }
    void exportLfo() override { }
    void pointChanged(int index, Point<float> position, bool mouse_up) override;
    void powersChanged(bool mouse_up) override;
    void pointAdded(int index, Point<float> position) override;
    void pointRemoved(int index) override;
    void pointsAdded(int index, int num_points_added) override;
    void pointsRemoved(int index, int num_points_removed) override;

    void sliderValueChanged(Slider* moved_slider) override;
    void sliderDragEnded(Slider* moved_slider) override;

    void setLineSource(WaveLineSource* line_source) {
      line_source_ = line_source;
      editor_->setModel(default_line_generator_.get());
      current_frame_ = nullptr;
    }

  protected:
    WaveLineSource* line_source_;
    WaveLineSource::WaveLineSourceKeyframe* current_frame_;
    std::unique_ptr<LineGenerator> default_line_generator_;
    std::unique_ptr<LineEditor> editor_;
    std::unique_ptr<SynthSlider> pull_power_;
    std::unique_ptr<SynthSlider> horizontal_grid_;
    std::unique_ptr<SynthSlider> vertical_grid_;
    std::unique_ptr<Component> horizontal_incrementers_;
    std::unique_ptr<Component> vertical_incrementers_;
    std::unique_ptr<Slider> interpolation_selector_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveLineSourceOverlay)
};

