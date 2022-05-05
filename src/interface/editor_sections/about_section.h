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
#include "overlay.h"
#include "open_gl_multi_quad.h"
#include "open_gl_image_component.h"

class AppLogo;

class OpenGlDeviceSelector : public OpenGlAutoImageComponent<AudioDeviceSelectorComponent> {
  public:
    OpenGlDeviceSelector(AudioDeviceManager& device_manager,
                         int min_audio_input_channels, int max_audioInput_channels,
                         int min_audio_output_channels, int max_audioOutput_channels,
                         bool show_midi_input_options, bool show_midi_output_selector,
                         bool show_channels_as_stereo_pairs, bool hide_advanced_options_with_button) :
        OpenGlAutoImageComponent<AudioDeviceSelectorComponent>(device_manager,
                                                               min_audio_input_channels, max_audioInput_channels,
                                                               min_audio_output_channels, max_audioOutput_channels,
                                                               show_midi_input_options, show_midi_output_selector,
                                                               show_channels_as_stereo_pairs,
                                                               hide_advanced_options_with_button) {
      setLookAndFeel(DefaultLookAndFeel::instance());
      image_component_.setComponent(this);
    }

    virtual void resized() override {
      OpenGlAutoImageComponent<AudioDeviceSelectorComponent>::resized();
      if (isShowing())
        redoImage();
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlDeviceSelector)
};

class AboutSection : public Overlay {
  public:
    static constexpr int kInfoWidth = 430;
    static constexpr int kBasicInfoHeight = 250;
    static constexpr int kPaddingX = 25;
    static constexpr int kPaddingY = 15;
    static constexpr int kButtonHeight = 30;
    static constexpr int kLeftLogoBuffer = 95;
    static constexpr int kNameRightBuffer = 85;
    static constexpr int kLogoWidth = 96;

    static constexpr float kMultExtraSmall = 0.5f;
    static constexpr float kMultSmall = 0.7f;
    static constexpr float kMultLarge = 1.35f;
    static constexpr float kMultDouble = 2.0f;
    static constexpr float kMultTriple = 3.0f;
    static constexpr float kMultQuadruple = 4.0f;

    AboutSection(const String& name);
    virtual ~AboutSection();
    void setLogoBounds();
    void resized() override;

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override {
      SynthSection::renderOpenGlComponents(open_gl, animate);
    }

    Rectangle<int> getInfoRect();

    void mouseUp(const MouseEvent& e) override;
    void setVisible(bool should_be_visible) override;
    void buttonClicked(Button* clicked_button) override;

  private:
    void setGuiSize(float multiplier);
    void fullScreen();

    std::unique_ptr<OpenGlDeviceSelector> device_selector_;

    std::unique_ptr<OpenGlToggleButton> size_button_extra_small_;
    std::unique_ptr<OpenGlToggleButton> size_button_small_;
    std::unique_ptr<OpenGlToggleButton> size_button_normal_;
    std::unique_ptr<OpenGlToggleButton> size_button_large_;
    std::unique_ptr<OpenGlToggleButton> size_button_double_;
    std::unique_ptr<OpenGlToggleButton> size_button_triple_;
    std::unique_ptr<OpenGlToggleButton> size_button_quadruple_;

    OpenGlQuad body_;
    std::unique_ptr<AppLogo> logo_;
    std::unique_ptr<PlainTextComponent> name_text_;
    std::unique_ptr<PlainTextComponent> version_text_;
    std::unique_ptr<PlainTextComponent> check_updates_text_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutSection)
};

