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

#include "bar_editor.h"
#include "wavetable_component_overlay.h"
#include "wave_source.h"
#include "wave_source_editor.h"

class TextSelector;

class WaveSourceOverlay : public WavetableComponentOverlay,
                          public WaveSourceEditor::Listener,
                          public BarEditor::Listener {
  public:
    static constexpr int kDefaultXGrid = 6;
    static constexpr int kDefaultYGrid = 4;
    static constexpr float kDefaultPhase = -0.5f;
    static constexpr float kBarAlpha = 0.75f;

    WaveSourceOverlay();

    void resized() override;

    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds) override;
    virtual bool setTimeDomainBounds(Rectangle<int> bounds) override;
    virtual bool setFrequencyAmplitudeBounds(Rectangle<int> bounds) override;
    virtual bool setPhaseBounds(Rectangle<int> bounds) override;

    void updateFrequencyDomain(std::complex<float>* frequency_domain);
    void loadFrequencyDomain();

    void valuesChanged(int start, int end, bool mouse_up) override;
    void barsChanged(int start, int end, bool mouse_up) override;

    void sliderValueChanged(Slider* moved_slider) override;

    void setPowerScale(bool scale) override { frequency_amplitudes_->setPowerScale(scale); }
    void setFrequencyZoom(float zoom) override {
      frequency_amplitudes_->setScale(zoom);
      frequency_phases_->setScale(zoom);
    }

    void setInterpolationType(WaveSource::InterpolationStyle style, WaveSource::InterpolationMode mode);

    void setWaveSource(WaveSource* wave_source) {
      wave_source_ = wave_source;
      current_frame_ = nullptr;
    }

  protected:
    WaveSource* wave_source_;
    vital::WaveFrame* current_frame_;
    std::unique_ptr<WaveSourceEditor> oscillator_;
    std::unique_ptr<BarEditor> frequency_amplitudes_;
    std::unique_ptr<BarEditor> frequency_phases_;
    std::unique_ptr<TextSelector> interpolation_type_;
    std::unique_ptr<SynthSlider> horizontal_grid_;
    std::unique_ptr<SynthSlider> vertical_grid_;
    std::unique_ptr<Component> horizontal_incrementers_;
    std::unique_ptr<Component> vertical_incrementers_;

    std::unique_ptr<Slider> interpolation_selector_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveSourceOverlay)
};

