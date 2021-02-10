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
#include "sample_viewer.h"
#include "transpose_quantize.h"

class SynthSlider;
class OpenGlShapeButton;

class SampleSection : public SynthSection, public SampleViewer::Listener, public PresetSelector::Listener,
                      TransposeQuantizeButton::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void sampleDestinationChanged(SampleSection* sample, int destination) = 0;
    };

    SampleSection(String name);
    ~SampleSection();

    void parentHierarchyChanged() override;

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void setActive(bool active) override;
    void resized() override;
    void reset() override;
    void setAllValues(vital::control_map& controls) override;
    void buttonClicked(Button* clicked_button) override;
    void setDestinationSelected(int selection);
    void setupDestination();
    void toggleFilterInput(int filter_index, bool on);

    void loadFile(const File& file) override;
    void sampleLoaded(const File& file) override { loadFile(file); }
    File getCurrentFile() override { return File(sample_->getLastBrowsedFile()); }

    void prevClicked() override;
    void nextClicked() override;
    void textMouseDown(const MouseEvent& e) override;
    void quantizeUpdated() override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    std::vector<Listener*> listeners_;
    std::unique_ptr<TransposeQuantizeButton> transpose_quantize_button_;
    std::unique_ptr<SynthSlider> transpose_;
    std::unique_ptr<SynthSlider> tune_;
    std::unique_ptr<SynthSlider> pan_;
    std::unique_ptr<SynthSlider> level_;
    std::unique_ptr<SampleViewer> sample_viewer_;
    std::unique_ptr<PresetSelector> preset_selector_;

    int current_destination_;
    std::string destination_control_name_;
    std::unique_ptr<PlainTextComponent> destination_text_;
    std::unique_ptr<ShapeButton> destination_selector_;
    std::unique_ptr<OpenGlShapeButton> prev_destination_;
    std::unique_ptr<OpenGlShapeButton> next_destination_;

    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<OpenGlShapeButton> loop_;
    std::unique_ptr<OpenGlShapeButton> bounce_;
    std::unique_ptr<OpenGlShapeButton> keytrack_;
    std::unique_ptr<OpenGlShapeButton> random_phase_;

    AudioSampleBuffer sample_buffer_;
    vital::Sample* sample_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleSection)
};

