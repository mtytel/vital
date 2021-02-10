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
#include "memory.h"
#include "oscillator_advanced_section.h"
#include "synth_constants.h"
#include "synth_section.h"

class TextSelector;
class DisplaySettings;
class OversampleSettings;
class VoiceSettings;
class OutputDisplays;
class OscillatorSection;

class TuningSelector : public TextSelector {
  public:
    enum TuningStyle {
      kDefault,
      k7Limit,
      k5Limit,
      kPythagorean,
      kNumTunings
    };

    TuningSelector(String name);
    virtual ~TuningSelector();

    void mouseDown(const MouseEvent& e) override;
    void valueChanged() override;
    void parentHierarchyChanged() override;
    void mouseWheelMove(const MouseEvent&, const MouseWheelDetails& wheel) override { }

    void setTuning(int tuning);

  private:
    void loadTuning(TuningStyle tuning);
    void loadTuningFile();
    void loadTuningFile(const File& file);
    String getTuningName();

    void setCustomString(std::string custom_string) { 
      strings_[kNumTunings] = custom_string;
      repaint();
    }
    std::string strings_[kNumTunings + 1];
};

class MasterControlsInterface  : public SynthSection {
  public:
    MasterControlsInterface(const vital::output_map& mono_modulations,
                            const vital::output_map& poly_modulations, bool synth);
    virtual ~MasterControlsInterface();

    void paintBackground(Graphics& g) override;
    void resized() override;

    void setOscillatorBounds(int index, Rectangle<int> bounds) { oscillator_advanceds_[index]->setBounds(bounds); }
    void passOscillatorSection(int index, const OscillatorSection* oscillator);
    void setOscilloscopeMemory(const vital::poly_float* memory);
    void setAudioMemory(const vital::StereoMemory* memory);

  private:
    std::unique_ptr<OscillatorAdvancedSection> oscillator_advanceds_[vital::kNumOscillators];
    std::unique_ptr<DisplaySettings> display_settings_;
    std::unique_ptr<OversampleSettings> oversample_settings_;
    std::unique_ptr<VoiceSettings> voice_settings_;
    std::unique_ptr<OutputDisplays> output_displays_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterControlsInterface)
};

