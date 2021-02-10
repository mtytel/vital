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
#include "preset_selector.h"
#include "wave_frame.h"
#include "wave_source_editor.h"
#include "wavetable.h"
#include "wavetable_component_list.h"
#include "wavetable_component_overlay.h"
#include "wavetable_creator.h"
#include "wavetable_organizer.h"
#include "wavetable_playhead.h"

class BarRenderer;
class WavetablePlayheadInfo;

class WavetableEditSection : public SynthSection,
                             public PresetSelector::Listener,
                             WavetableOrganizer::Listener,
                             WavetableComponentList::Listener,
                             WavetablePlayhead::Listener,
                             WavetableComponentOverlay::Listener {
  public:
    static constexpr float kObscureAmount = 0.4f;
    static constexpr float kAlphaFade = 0.3f;

    static inline float getZoomScale(int zoom) {
      return 1 << (zoom - kZoom1);
    }
                               
    static String getWavetableDataString(InputStream* input_stream);

    enum MenuItems {
      kCancelled,
      kSaveAsWavetable,
      kImportWavetable,
      kExportWavetable,
      kExportWav,
      kResynthesizeWavetable,
      kNumMenuItems
    };

    enum BarEditorMenu {
      kCancel = 0,
      kPowerScale,
      kAmplitudeScale,
      kZoom1,
      kZoom2,
      kZoom4,
      kZoom8,
      kZoom16,
    };

    WavetableEditSection(int index, WavetableCreator* wavetable_creator);
    virtual ~WavetableEditSection();

    Rectangle<int> getFrameEditBounds();
    Rectangle<int> getTimelineBounds();
    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override;
    void resized() override;
    void reset() override;
    void visibilityChanged() override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;

    void componentAdded(WavetableComponent* component) override;
    void componentRemoved(WavetableComponent* component) override;
    void componentsReordered() override { }
    void componentsChanged() override;

    int getTopHeight() {
      static constexpr int kTopHeight = 48;
      return size_ratio_ * kTopHeight;
    }

    void playheadMoved(int position) override;

    void setWaveFrameSlider(Slider* slider) { wave_frame_slider_ = slider; }

    void frameDoneEditing() override;
    void frameChanged() override;

    void prevClicked() override;
    void nextClicked() override;
    void textMouseDown(const MouseEvent& e) override;

    void loadDefaultWavetable();
    void saveAsWavetable();
    void importWavetable();
    void exportWavetable();
    void exportToWav();
    void loadFile(const File& wavetable_file) override;
    File getCurrentFile() override { return File(wavetable_creator_->getLastFileLoaded()); }

    void loadWavetable(json& wavetable_data);
    json getWavetableJson() { return wavetable_creator_->stateToJson(); }
    bool loadAudioAsWavetable(String name, InputStream* audio_stream, WavetableCreator::AudioFileLoadStyle style);
    void resynthesizeToWavetable();

    virtual void buttonClicked(Button* clicked_button) override;
    virtual void positionsUpdated() override;
    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override;
    virtual void wheelMoved(const MouseEvent& e, const MouseWheelDetails& wheel) override;

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;

    void setPowerScale(bool power_scale);
    void setZoom(int zoom);

    void clear();
    void init();
    std::string getLastBrowsedWavetable() { return wavetable_creator_->getLastFileLoaded(); }
    std::string getName() { return wavetable_creator_->getName(); }

  private:
    void setPresetSelectorText();
    void showPopupMenu();
    void hideCurrentOverlay();
    void clearOverlays();
    void setColors();
    void render();
    void render(int position);
    void updateGlDisplay();
    void setOverlayPosition();
    void updateTimeDomain(float* time_domain);
    void updateFrequencyDomain(float* time_domain);
    int loadAudioFile(AudioSampleBuffer& destination, InputStream* audio_stream);

    int index_;
    float zoom_;
    bool power_scale_;
    bool obscure_time_domain_;
    bool obscure_freq_amplitude_;
    bool obscure_freq_phase_;

    AudioFormatManager format_manager_;

    std::unique_ptr<BarRenderer> frequency_amplitudes_;
    std::unique_ptr<BarRenderer> frequency_phases_;
    std::unique_ptr<WaveSourceEditor> oscillator_waveform_;
    std::unique_ptr<WavetableOrganizer> wavetable_organizer_;
    std::unique_ptr<WavetableComponentList> wavetable_component_list_;
    std::unique_ptr<WavetablePlayhead> wavetable_playhead_;
    std::unique_ptr<WavetablePlayheadInfo> wavetable_playhead_info_;
    std::unique_ptr<OpenGlShapeButton> exit_button_;
    std::unique_ptr<OpenGlShapeButton> frequency_amplitude_settings_;
    std::unique_ptr<PresetSelector> preset_selector_;
    std::unique_ptr<OpenGlShapeButton> menu_button_;

    Slider* wave_frame_slider_;

    vital::WaveFrame compute_frame_;
    WavetableCreator* wavetable_creator_;
    std::map<WavetableComponent*, WavetableComponentFactory::ComponentType> type_lookup_;
    std::unique_ptr<WavetableComponentOverlay> overlays_[WavetableComponentFactory::kNumComponentTypes];
    WavetableComponentOverlay* current_overlay_;
    Rectangle<int> edit_bounds_;
    Rectangle<int> title_bounds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableEditSection)
};

