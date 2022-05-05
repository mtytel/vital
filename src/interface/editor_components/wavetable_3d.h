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

#include "open_gl_line_renderer.h"
#include "audio_file_drop_source.h"
#include "common.h"
#include "open_gl_image.h"
#include "synth_oscillator.h"
#include "synth_types.h"
#include "wavetable.h"
#include "wavetable_creator.h"

namespace vital {
  struct Output;
  class Wavetable;
} // namespace vital

class Wavetable3d : public OpenGlComponent, public AudioFileDropSource {
  public:
    static constexpr float kDefaultVerticalAngle = 1.132f;
    static constexpr float kDefaultHorizontalAngle = -0.28f;
    static constexpr float kDefaultDrawWidthPercent = 0.728f;
    static constexpr float kDefaultWaveHeightPercent = 0.083f;
    static constexpr float kPositionWidth = 8.0f;
    static constexpr float kPositionLineWidthRatio = 1.8f;
    static constexpr int kColorJump = 16;
    static constexpr int kDownsampleResolutionAmount = 0;
    static constexpr int kResolution = vital::Wavetable::kWaveformSize >> kDownsampleResolutionAmount;
    static constexpr int kNumBits = vital::WaveFrame::kWaveformBits;
    static constexpr int kBackgroundResolution = 128;
    static constexpr int kExtraShadows = 20;
    static constexpr float k2dWaveHeightPercent = 0.25f;
  
    static void paint3dLine(Graphics& g, vital::Wavetable* wavetable, int index, Colour color,
                            float width, float height, float wave_height_percent,
                            float wave_range_x, float frame_range_x, float wave_range_y, float frame_range_y,
                            float start_x, float start_y, float offset_x, float offset_y);
  
    static void paint3dBackground(Graphics& g, vital::Wavetable* wavetable, bool active,
                                  Colour background_color, Colour wave_color1, Colour wave_color2,
                                  float width, float height,
                                  float wave_height_percent,
                                  float wave_range_x, float frame_range_x, float wave_range_y, float frame_range_y,
                                  float start_x, float start_y, float offset_x, float offset_y);

    enum MenuOptions {
      kCancel,
      kCopy,
      kPaste,
      kInit,
      kSave,
      kResynthesizePreset,
      kLogIn,
      kNumMenuOptions
    };

    enum RenderType {
      kWave3d,
      kWave2d,
      kFrequencyAmplitudes,
      kNumRenderTypes
    };

    class Listener {
      public:
        virtual ~Listener() { }
        virtual bool loadAudioAsWavetable(String name, InputStream* audio_stream,
                                          WavetableCreator::AudioFileLoadStyle style) = 0;
        virtual void loadWavetable(json& wavetable_data) = 0;
        virtual void loadDefaultWavetable() = 0;
        virtual void resynthesizeToWavetable() = 0;
        virtual void saveWavetable() = 0;
    };

    Wavetable3d(int index, const vital::output_map& mono_modulations, const vital::output_map& poly_modulations);
    virtual ~Wavetable3d();

    void init(OpenGlWrapper& open_gl) override;
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void renderWave(OpenGlWrapper& open_gl);
    void renderSpectrum(OpenGlWrapper& open_gl);
    void destroy(OpenGlWrapper& open_gl) override;
    void paintBackground(Graphics& g) override;
    void resized() override;

    inline vital::poly_float getOutputsTotal(std::pair<vital::Output*, vital::Output*> outputs,
                                             vital::poly_float default_value);

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void setFrameSlider(SynthSlider* slider) { frame_slider_ = slider; }
    void setSpectralMorphSlider(Slider* slider) { spectral_morph_slider_ = slider; }
    void setDistortionSlider(Slider* slider) { distortion_slider_ = slider; }
    void setDistortionPhaseSlider(Slider* slider) { distortion_phase_slider_ = slider; }
    void setViewSettings(float horizontal_angle, float vertical_angle,
                         float draw_width, float wave_height, float y_offset);
    void setRenderType(RenderType render_type);
    RenderType getRenderType() const { return render_type_; }
    void setSpectralMorphType(int spectral_morph_type) { spectral_morph_type_ = spectral_morph_type; }
    void setDistortionType(int distortion_type) { distortion_type_ = distortion_type; }

    void respondToMenuCallback(int option);
    bool hasMatchingSystemClipboard();

    void setActive(bool active) { active_ = active; }
    bool isActive() { return active_; }

    void audioFileLoaded(const File& file) override;
    void updateDraggingPosition(int x, int y);

    void fileDragEnter(const StringArray& files, int x, int y) override {
      updateDraggingPosition(x, y);
    }

    void fileDragMove(const StringArray& files, int x, int y) override {
      updateDraggingPosition(x, y);
    }

    void fileDragExit(const StringArray& files) override {
      drag_load_style_ = WavetableCreator::kNone;
    }

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void setLoadingWavetable(bool loading) { loading_wavetable_ = loading; }
    void setDirty() { last_spectral_morph_type_ = -1; }
    vital::Wavetable* getWavetable() { return wavetable_; }

  private:
    bool updateRenderValues();
    void loadIntoTimeDomain(int index);
    void loadWaveData(int index);
    void loadSpectrumData(int index);
    void drawPosition(OpenGlWrapper& open_gl, int index);
    void setDimensionValues();
    void setColors();
    vital::poly_float getDistortionValue();
    vital::poly_float getSpectralMorphValue();
    vital::poly_int getDistortionPhaseValue();
    void loadFrequencyData(int index);
    void warpSpectrumToWave(int index);
    void warpPhase(int index);

    OpenGlLineRenderer left_line_renderer_;
    OpenGlLineRenderer right_line_renderer_;
    OpenGlMultiQuad end_caps_;

    Colour import_text_color_;
    OpenGlQuad import_overlay_;
    std::unique_ptr<PlainTextComponent> wavetable_import_text_;
    std::unique_ptr<PlainTextComponent> vocode_import_text_;
    std::unique_ptr<PlainTextComponent> pitch_splice_import_text_;

    Colour body_color_;
    Colour line_left_color_;
    Colour line_right_color_;
    Colour line_disabled_color_;
    Colour fill_left_color_;
    Colour fill_right_color_;
    Colour fill_disabled_color_;

    std::vector<Listener*> listeners_;
    std::pair<vital::Output*, vital::Output*> wave_frame_outputs_;
    std::pair<vital::Output*, vital::Output*> spectral_morph_outputs_;
    std::pair<vital::Output*, vital::Output*> distortion_outputs_;
    std::pair<vital::Output*, vital::Output*> distortion_phase_outputs_;

    int last_spectral_morph_type_;
    int last_distortion_type_;
    int spectral_morph_type_;
    int distortion_type_;
    vital::poly_float wave_frame_;
    vital::poly_float spectral_morph_value_;
    vital::poly_float distortion_value_;
    vital::poly_int distortion_phase_;

    SynthSlider* frame_slider_;
    Slider* spectral_morph_slider_;
    Slider* distortion_slider_;
    Slider* distortion_phase_slider_;
    Point<int> last_edit_position_;
    WavetableCreator::AudioFileLoadStyle drag_load_style_;
    vital::WaveFrame process_frame_;
    vital::FourierTransform transform_;
    vital::poly_float process_wave_data_[vital::SynthOscillator::kSpectralBufferSize];
    const vital::Wavetable::WavetableData* current_wavetable_data_;
    int wavetable_index_;

    bool animate_;
    bool loading_wavetable_;
    bool last_loading_wavetable_;
    RenderType render_type_;
    RenderType last_render_type_;
    bool active_;
    int size_;
    int index_;
    vital::Wavetable* wavetable_;

    double current_value_;
    float vertical_angle_;
    float horizontal_angle_;
    float draw_width_percent_;
    float wave_height_percent_;
    float y_offset_;

    float wave_range_x_;
    float frame_range_x_;
    float wave_range_y_;
    float frame_range_y_;
    float start_x_;
    float start_y_;
    float offset_x_;
    float offset_y_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Wavetable3d)
};

