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

#include "authentication.h"
#include "authentication_section.h"
#include "download_section.h"
#include "header_section.h"
#include "effects_interface.h"
#include "memory.h"
#include "modulation_matrix.h"
#include "open_gl_background.h"
#include "shaders.h"
#include "synth_section.h"
#include "update_check_section.h"
#include "wavetable_creator.h"

class AboutSection;
class BankExporter;
class BendSection;
class DeleteSection;
class ExpiredSection;
class ExtraModSection;
class HeaderSection;
class KeyboardInterface;
class MasterControlsInterface;
class ModulationInterface;
class ModulationManager;
class PortamentoSection;
class PresetBrowser;
class SaveSection;
class SynthesisInterface;
struct SynthGuiData;
class SynthSlider;
class WavetableEditSection;
class VoiceSection;

class FullInterface : public SynthSection, public AuthenticationSection::Listener, public HeaderSection::Listener,
                      public DownloadSection::Listener, public UpdateCheckSection::Listener,
                      public EffectsInterface::Listener, public ModulationMatrix::Listener,
                      public OpenGLRenderer, DragAndDropContainer {
  public:
    static constexpr double kMinOpenGlVersion = 1.4;

    FullInterface(SynthGuiData* synth_gui_data);

    FullInterface();
    virtual ~FullInterface();

    void setOscilloscopeMemory(const vital::poly_float* memory);
    void setAudioMemory(const vital::StereoMemory* memory);

    void createModulationSliders(const vital::output_map& mono_modulations,
                                 const vital::output_map& poly_modulations);

    virtual void paintBackground(Graphics& g) override;
    void copySkinValues(const Skin& skin);
    void reloadSkin(const Skin& skin);

    void repaintChildBackground(SynthSection* child);
    void repaintSynthesisSection();
    void repaintOpenGlBackground(OpenGlComponent* component);
    void redoBackground();
    void checkShouldReposition(bool resize = true);
    void parentHierarchyChanged() override {
      SynthSection::parentHierarchyChanged();
      checkShouldReposition();
    }
    virtual void resized() override;
    void animate(bool animate) override;
    void reset() override;
    void setAllValues(vital::control_map& controls) override;

    void dataDirectoryChanged() override;
    void noDownloadNeeded() override;

    void needsUpdate() override;

    void loggedIn() override;

    void setWavetableNames();
    void startDownload();

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void showAboutSection() override;
    void deleteRequested(File preset) override;
    void tabSelected(int index) override;
    void clearTemporaryTab(int current_tab) override;
    void setPresetBrowserVisibility(bool visible, int current_tab) override;
    void setBankExporterVisibility(bool visible, int current_tab) override;
    void bankImported() override;

    void effectsMoved() override;
    void modulationsScrolled() override;

    void setFocus();
    void notifyChange();
    void notifyFresh();
    void externalPresetLoaded(const File& preset);
    void showFullScreenSection(SynthSection* full_screen);
    void showWavetableEditSection(int index);
    std::string getLastBrowsedWavetable(int index);
    std::string getWavetableName(int index);
    std::string getSignedInName();
    void signOut();
    void signIn();
    void hideWavetableEditSection();
    void loadWavetableFile(int index, const File& wavetable);
    void loadWavetable(int index, json& wavetable_data);
    void loadDefaultWavetable(int index);
    void resynthesizeToWavetable(int index);
    void saveWavetable(int index);
    void saveLfo(const json& data);
    json getWavetableJson(int index);
    bool loadAudioAsWavetable(int index, const String& name, InputStream* audio_stream,
                              WavetableCreator::AudioFileLoadStyle style);
    void popupBrowser(SynthSection* owner, Rectangle<int> bounds, std::vector<File> directories,
                      String extensions, std::string passthrough_name, std::string additional_folders_name);
    void popupBrowserUpdate(SynthSection* owner);
    void popupSelector(Component* source, Point<int> position, const PopupItems& options,
                       std::function<void(int)> callback, std::function<void()> cancel);
    void dualPopupSelector(Component* source, Point<int> position, int width,
                           const PopupItems& options, std::function<void(int)> callback);
    void popupDisplay(Component* source, const std::string& text,
                      BubbleComponent::BubblePlacement placement, bool primary);
    void hideDisplay(bool primary);
    void modulationChanged();
    void modulationValueChanged(int index);
    void openSaveDialog() { save_section_->setIsPreset(true); save_section_->setVisible(true); }
    void enableRedoBackground(bool enable) {
      enable_redo_background_ = enable;
      if (enable)
        resized();
    }

    float getResizingScale() const { return width_ * 1.0f / resized_width_; }
    float getPixelScaling() const override { return display_scale_; }
    int getPixelMultiple() const override { return pixel_multiple_; }
    void toggleOscillatorZoom(int index);
    void toggleFilter1Zoom();
    void toggleFilter2Zoom();

  private:
    bool wavetableEditorsInitialized() {
      for (int i = 0; i < vital::kNumOscillators; ++i) {
        if (wavetable_edits_[i] == nullptr)
          return false;
      }
      return true;
    }

    Authentication auth_;
    std::map<std::string, SynthSlider*> slider_lookup_;
    std::map<std::string, Button*> button_lookup_;
    std::unique_ptr<ModulationManager> modulation_manager_;
    std::unique_ptr<ModulationMatrix> modulation_matrix_;

    std::unique_ptr<AboutSection> about_section_;
    std::unique_ptr<AuthenticationSection> authentication_;
    std::unique_ptr<UpdateCheckSection> update_check_section_;
    std::unique_ptr<Component> standalone_settings_section_;

    std::unique_ptr<HeaderSection> header_;
    std::unique_ptr<SynthesisInterface> synthesis_interface_;
    std::unique_ptr<MasterControlsInterface> master_controls_interface_;
    std::unique_ptr<ModulationInterface> modulation_interface_;
    std::unique_ptr<ExtraModSection> extra_mod_section_;
    std::unique_ptr<EffectsInterface> effects_interface_;
    std::unique_ptr<WavetableEditSection> wavetable_edits_[vital::kNumOscillators];
    std::unique_ptr<KeyboardInterface> keyboard_interface_;
    std::unique_ptr<BendSection> bend_section_;
    std::unique_ptr<PortamentoSection> portamento_section_;
    std::unique_ptr<VoiceSection> voice_section_;
    std::unique_ptr<PresetBrowser> preset_browser_;
    std::unique_ptr<PopupBrowser> popup_browser_;
    std::unique_ptr<SinglePopupSelector> popup_selector_;
    std::unique_ptr<DualPopupSelector> dual_popup_selector_;
    std::unique_ptr<PopupDisplay> popup_display_1_;
    std::unique_ptr<PopupDisplay> popup_display_2_;
    std::unique_ptr<BankExporter> bank_exporter_;
    std::unique_ptr<SaveSection> save_section_;
    std::unique_ptr<DeleteSection> delete_section_;
    std::unique_ptr<DownloadSection> download_section_;
    std::unique_ptr<ExpiredSection> expired_section_;
    SynthSection* full_screen_section_;

    int width_;
    int resized_width_;
    float last_render_scale_;
    float display_scale_;
    int pixel_multiple_;
    bool setting_all_values_;
    bool unsupported_;
    bool animate_;
    bool enable_redo_background_;
    bool needs_download_;
    CriticalSection open_gl_critical_section_;
    OpenGLContext open_gl_context_;
    std::unique_ptr<Shaders> shaders_;
    OpenGlWrapper open_gl_;
    Image background_image_;
    OpenGlBackground background_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FullInterface)
};
