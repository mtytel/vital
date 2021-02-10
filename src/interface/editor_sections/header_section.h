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
#include "bank_exporter.h"
#include "synth_preset_selector.h"

class BankExporter;
class LogoButton;
class TabSelector;
class Oscilloscope;
class Spectrogram;
class PresetBrowser;
class VolumeSection;

class LogoSection : public SynthSection {
  public:
    static constexpr float kLogoPaddingY = 2.0f;

    class Listener {
      public:
        virtual ~Listener() { }

        virtual void showAboutSection() = 0;
    };

    LogoSection();

    void resized() override;
    void paintBackground(Graphics& g) override;
    void buttonClicked(Button* clicked_button) override;
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    std::vector<Listener*> listeners_;
    std::unique_ptr<LogoButton> logo_button_;
};

class HeaderSection : public SynthSection, public SaveSection::Listener,
                      public SynthPresetSelector::Listener, public LogoSection::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void showAboutSection() = 0;
        virtual void deleteRequested(File preset) = 0;
        virtual void tabSelected(int index) = 0;
        virtual void clearTemporaryTab(int current_tab) = 0;
        virtual void setPresetBrowserVisibility(bool visible, int current_tab) = 0;
        virtual void setBankExporterVisibility(bool visible, int current_tab) = 0;
        virtual void bankImported() = 0;
    };

    HeaderSection();

    void paintBackground(Graphics& g) override;
    void resized() override;
    void reset() override;
    void setAllValues(vital::control_map& controls) override;
    void buttonClicked(Button* clicked_button) override;
    void sliderValueChanged(Slider* slider) override;

    void setPresetBrowserVisibility(bool visible) override;
    void setBankExporterVisibility(bool visible) override;
    void deleteRequested(File preset) override;
    void bankImported() override;
    void save(File preset) override;

    void setTemporaryTab(String name);

    void showAboutSection() override {
      for (Listener* listener : listeners_)
        listener->showAboutSection();
    }

    void setOscilloscopeMemory(const vital::poly_float* memory);
    void setAudioMemory(const vital::StereoMemory* memory);

    void notifyChange();
    void notifyFresh();
  
    void setSaveSection(SaveSection* save_section) { 
      synth_preset_selector_->setSaveSection(save_section);
      save_section->addSaveListener(this);
    }

    void setBrowser(PresetBrowser* browser) { synth_preset_selector_->setBrowser(browser); }
    void setBankExporter(BankExporter* exporter) { synth_preset_selector_->setBankExporter(exporter); }
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void setTabOffset(int offset) { tab_offset_ = offset; repaint(); }

  private:
    std::vector<Listener*> listeners_;

    std::unique_ptr<LogoSection> logo_section_;
    std::unique_ptr<TabSelector> tab_selector_;
    int tab_offset_;
    std::unique_ptr<PlainTextComponent> temporary_tab_;
    std::unique_ptr<OpenGlShapeButton> exit_temporary_button_;

    std::unique_ptr<SynthButton> view_spectrogram_;
    std::unique_ptr<Oscilloscope> oscilloscope_;
    std::unique_ptr<Spectrogram> spectrogram_;
    std::unique_ptr<SynthPresetSelector> synth_preset_selector_;
    std::unique_ptr<VolumeSection> volume_section_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderSection)
};

