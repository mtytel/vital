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
#include "bank_exporter.h"
#include "preset_selector.h"
#include "preset_browser.h"
#include "synth_button.h"
#include "synth_section.h"

class SynthPresetSelector : public SynthSection,
                            public PresetBrowser::Listener, public BankExporter::Listener,
                            public PresetSelector::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void setPresetBrowserVisibility(bool visible) = 0;
        virtual void setBankExporterVisibility(bool visible) = 0;
        virtual void deleteRequested(File file) = 0;
        virtual void bankImported() = 0;
    };

    enum MenuItems {
      kCancelled,
      kInitPreset,
      kSavePreset,
      kImportPreset,
      kExportPreset,
      kImportBank,
      kExportBank,
      kBrowsePresets,
      kLoadTuning,
      kClearTuning,
      kOpenSkinDesigner,
      kLoadSkin,
      kClearSkin,
      kLogOut,
      kLogIn,
      kNumMenuItems
    };

    SynthPresetSelector();
    ~SynthPresetSelector();

    void resized() override;
    void paintBackground(Graphics& g) override { selector_ ->paintBackground(g); }
    void buttonClicked(Button* buttonThatWasClicked) override;
    void newPresetSelected(File preset) override;
    void deleteRequested(File preset) override;
    void hidePresetBrowser() override;
    void hideBankExporter() override;
    void resetText();

    void prevClicked() override;
    void nextClicked() override;
    void textMouseUp(const MouseEvent& e) override;

    void showPopupMenu(Component* anchor);
    void showAlternatePopupMenu(Component* anchor);

    void setModified(bool modified);
    void setSaveSection(SaveSection* save_section) { save_section_ = save_section; }
    void setBrowser(PresetBrowser* browser) {
      if (browser_ != browser) {
        browser_ = browser;
        browser_->addListener(this);
      }
    }
    void setBankExporter(BankExporter* bank_exporter) {
      if (bank_exporter_ != bank_exporter) {
        bank_exporter_ = bank_exporter;
        bank_exporter_->addListener(this);
      }
    }

    void setPresetBrowserVisibile(bool visible);
    void makeBankExporterVisibile();

    void initPreset();
    void savePreset();
    void importPreset();
    void exportPreset();
    void importBank();
    void exportBank();
    void loadTuningFile();
    void clearTuning();
    std::string getTuningName();
    bool hasDefaultTuning();
    std::string loggedInName();
    void signOut();
    void signIn();
    void openSkinDesigner();
    void loadSkin();
    void clearSkin();
    void repaintWithSkin();
    void browsePresets();

    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    void loadFromFile(File& preset);

    std::vector<Listener*> listeners_;

    std::unique_ptr<Skin> full_skin_;
    Component::SafePointer<Component> skin_designer_;

    std::unique_ptr<PresetSelector> selector_;
    std::unique_ptr<OpenGlShapeButton> menu_button_;
    std::unique_ptr<OpenGlShapeButton> save_button_;
    BankExporter* bank_exporter_;
    PresetBrowser* browser_;
    SaveSection* save_section_;
    bool modified_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthPresetSelector)
};

