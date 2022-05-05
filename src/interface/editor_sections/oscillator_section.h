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
#include "open_gl_image_component.h"
#include "preset_selector.h"
#include "transpose_quantize.h"
#include "wavetable_3d.h"

class IncrementerButtons;
class OpenGlShapeButton;
class PresetSelector;
class SaveSection;
class TextSelector;
class UnisonViewer;
class WavetableCreator;

class OscillatorSection : public SynthSection, public PresetSelector::Listener,
                          TextEditor::Listener, Wavetable3d::Listener, TransposeQuantizeButton::Listener, Timer {
  public:
    static constexpr float kSectionWidthRatio = 0.19f;

    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void distortionTypeChanged(OscillatorSection* section, int type) = 0;
        virtual void oscillatorDestinationChanged(OscillatorSection* section, int destination) = 0;
    };

    OscillatorSection(
                      int index, const vital::output_map& mono_modulations, const vital::output_map& poly_modulations);
    virtual ~OscillatorSection();

    void setSkinValues(const Skin& skin, bool top_level) override;
    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void reset() override {
      SynthSection::reset();
      wavetable_->setDirty();
    }
    void buttonClicked(Button* clicked_button) override;
    void setAllValues(vital::control_map& controls) override;

    void textEditorReturnKeyPressed(TextEditor& text_editor) override;
    void textEditorFocusLost(TextEditor& text_editor) override;

    void timerCallback() override;

    void setActive(bool active) override;
    void setName(String name);

    void resetOscillatorModulationDistortionType();
    void addListener(Listener* listener) { listeners_.push_back(listener); }

    std::string loadWavetableFromText(const String& text);
    Slider* getWaveFrameSlider();
    void setDistortionSelected(int selection);
    int getDistortion() const { return current_distortion_type_; }
    void setSpectralMorphSelected(int selection);
    void setDestinationSelected(int selection);
    void toggleFilterInput(int filter_index, bool on);

    void loadBrowserState();
    void setIndexSelected();
    void setLanguage(int index);
    void languageSelectCancelled();
    void prevClicked() override;
    void nextClicked() override;
    void textMouseDown(const MouseEvent& e) override;
    void quantizeUpdated() override;

    bool loadAudioAsWavetable(String name, InputStream* audio_stream,
                              WavetableCreator::AudioFileLoadStyle style) override;
    void loadWavetable(json& wavetable_data) override;
    void loadDefaultWavetable() override;
    void resynthesizeToWavetable() override;
    void saveWavetable() override;
    void loadFile(const File& wavetable_file) override;
    File getCurrentFile() override { return current_file_; }
    std::string getFileName() override { return wavetable_->getWavetable()->getName(); }
    std::string getFileAuthor() override { return wavetable_->getWavetable()->getAuthor(); }

    int index() const { return index_; }

    SynthSlider* getVoicesSlider() const { return unison_voices_.get(); }
    const SynthSlider* getWaveFrameSlider() const { return wave_frame_.get(); }
    const SynthSlider* getSpectralMorphSlider() const { return spectral_morph_amount_.get(); }
    const SynthSlider* getDistortionSlider() const { return distortion_amount_.get(); }
    Rectangle<float> getWavetableRelativeBounds();

  private:
    void showTtwtSettings();
    void setupSpectralMorph();
    void setupDistortion();
    void setupDestination();
    void setDistortionPhaseVisible(bool visible);
    void notifySpectralMorphTypeChange();
    void notifyDistortionTypeChange();
    void notifyDestinationChange();

    std::vector<Listener*> listeners_;
    int index_;
    File current_file_;

    std::string distortion_control_name_;
    std::string spectral_morph_control_name_;
    std::string destination_control_name_;
    std::string quantize_control_name_;
    int current_distortion_type_;
    int current_spectral_morph_type_;
    int current_destination_;
    bool show_ttwt_error_;
    bool showing_language_menu_;
    int ttwt_language_;

    std::unique_ptr<SynthButton> oscillator_on_;
    std::unique_ptr<SynthButton> dimension_button_;
    std::unique_ptr<SynthSlider> dimension_value_;
    std::unique_ptr<PresetSelector> preset_selector_;
    std::unique_ptr<Wavetable3d> wavetable_;
    std::unique_ptr<UnisonViewer> unison_viewer_;

    std::unique_ptr<TransposeQuantizeButton> transpose_quantize_button_;
    std::unique_ptr<SynthSlider> transpose_;
    std::unique_ptr<SynthSlider> tune_;

    std::unique_ptr<PlainTextComponent> distortion_type_text_;
    std::unique_ptr<ShapeButton> distortion_type_selector_;
    std::unique_ptr<SynthSlider> distortion_amount_;
    std::unique_ptr<SynthSlider> distortion_phase_;
    std::unique_ptr<SynthSlider> phase_;
    std::unique_ptr<SynthSlider> random_phase_;

    std::unique_ptr<PlainTextComponent> spectral_morph_type_text_;
    std::unique_ptr<ShapeButton> spectral_morph_type_selector_;
    std::unique_ptr<SynthSlider> spectral_morph_amount_;

    std::unique_ptr<PlainTextComponent> destination_text_;
    std::unique_ptr<ShapeButton> destination_selector_;

    std::unique_ptr<SynthSlider> level_;
    std::unique_ptr<SynthSlider> pan_;
    std::unique_ptr<SynthSlider> wave_frame_;

    std::unique_ptr<SynthSlider> unison_voices_;
    std::unique_ptr<SynthSlider> unison_detune_;
    std::unique_ptr<SynthSlider> unison_detune_power_;
    std::unique_ptr<OpenGlShapeButton> edit_button_;

    OpenGlQuad ttwt_overlay_;
    std::unique_ptr<OpenGlTextEditor> ttwt_;
    std::unique_ptr<SynthButton> ttwt_settings_;
    std::unique_ptr<PlainTextComponent> ttwt_error_text_;

    std::unique_ptr<OpenGlShapeButton> prev_destination_;
    std::unique_ptr<OpenGlShapeButton> next_destination_;
    std::unique_ptr<OpenGlShapeButton> prev_spectral_;
    std::unique_ptr<OpenGlShapeButton> next_spectral_;
    std::unique_ptr<OpenGlShapeButton> prev_distortion_;
    std::unique_ptr<OpenGlShapeButton> next_distortion_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorSection)
};

