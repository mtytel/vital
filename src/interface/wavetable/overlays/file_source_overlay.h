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

#include "audio_file_drop_source.h"
#include "wavetable_component_overlay.h"
#include "file_source.h"
#include "pitch_detector.h"
#include "sample_viewer.h"
#include "text_selector.h"
#include "synth_section.h"

class AudioFileViewer;
class AudioFilePosition;

class AudioFileViewer : public SynthSection, public AudioFileDropSource {
  public:
    class DragListener {
      public:
        virtual ~DragListener() { }
        virtual void positionMovedRelative(float ratio, bool mouse_up) = 0;
    };

    static constexpr float kResolution = 256;

    AudioFileViewer();
    virtual ~AudioFileViewer() { }

    void resized() override;
    void clearAudioPositions();
    void setAudioPositions();
    void setWindowValues();

    void setWindowPosition(float window_position) { window_position_ = window_position; setWindowValues(); }
    void setWindowSize(float window_size) { window_size_ = window_size; setWindowValues(); }
    void setWindowFade(float window_fade) { window_fade_ = window_fade; setWindowValues(); }

    void audioFileLoaded(const File& file) override;
    void fileDragEnter(const StringArray& files, int x, int y) override;
    void fileDragExit(const StringArray& files) override;

    float updateMousePosition(Point<float> position);
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    AudioSampleBuffer* getSampleBuffer() { return &sample_buffer_; }
    int getSampleRate() const { return sample_rate_; }

    void setFileSource(FileSource* file_source) { file_source_ = file_source; setAudioPositions(); }
    void addDragListener(DragListener* listener) { drag_listeners_.push_back(listener); }

  private:
    std::vector<DragListener*> drag_listeners_;

    OpenGlLineRenderer top_;
    OpenGlLineRenderer bottom_;
    OpenGlQuad dragging_quad_;

    float window_position_;
    float window_size_;
    float window_fade_;

    AudioSampleBuffer sample_buffer_;
    int sample_rate_;
    FileSource* file_source_;
    Point<float> last_mouse_position_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFileViewer)
};

class FileSourceOverlay : public WavetableComponentOverlay, TextEditor::Listener,
                          AudioFileDropSource::Listener, AudioFileViewer::DragListener {
  public:
    FileSourceOverlay();
    virtual ~FileSourceOverlay();

    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds) override;

    void sliderValueChanged(Slider* moved_slider) override;
    void sliderDragEnded(Slider* moved_slider) override;

    void buttonClicked(Button* clicked_button) override;

    void audioFileLoaded(const File& file) override;

    void textEditorReturnKeyPressed(TextEditor& text_editor) override;
    void textEditorFocusLost(TextEditor& text_editor) override;

    void positionMovedRelative(float ratio, bool mouse_up) override;

    void setFileSource(FileSource* file_source);
    void loadFile(const File& file) override;

  protected:
    void setTextEditorVisuals(TextEditor* text_editor, float height);
    void loadWindowSizeText();
    void loadStartingPositionText();

    void clampStartingPosition();
    void loadFilePressed();

    FileSource* file_source_;
    FileSource::FileSourceKeyframe* current_frame_;

    std::unique_ptr<OpenGlTextEditor> start_position_;
    std::unique_ptr<OpenGlTextEditor> window_size_;
    std::unique_ptr<SynthSlider> window_fade_;
    std::unique_ptr<TextButton> load_button_;
    std::unique_ptr<TextSelector> fade_style_;
    std::unique_ptr<TextSelector> phase_style_;
    std::unique_ptr<OpenGlToggleButton> normalize_gain_;
    std::unique_ptr<AudioFileViewer> audio_thumbnail_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileSourceOverlay)
};
