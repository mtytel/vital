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

#include "bar_renderer.h"
#include "modulation_button.h"
#include "open_gl_component.h"
#include "open_gl_multi_quad.h"
#include "synth_module.h"
#include "synth_section.h"
#include "synth_slider.h"

#include <set>

class ExpandModulationButton;
class ModulationMatrix;
class ModulationMeter;
class ModulationDestination;

class ModulationAmountKnob : public SynthSlider {
  public:
    enum MenuOptions {
      kDisconnect = 0xff,
      kToggleBypass,
      kToggleBipolar,
      kToggleStereo,
    };

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void disconnectModulation(ModulationAmountKnob* modulation_knob) = 0;
        virtual void setModulationBypass(ModulationAmountKnob* modulation_knob, bool bypass) = 0;
        virtual void setModulationBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) = 0;
        virtual void setModulationStereo(ModulationAmountKnob* modulation_knob, bool stereo) = 0;
    };

    ModulationAmountKnob(String name, int index);

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void handleModulationMenuCallback(int result);

    void makeVisible(bool visible);
    void hideImmediately();

    void setCurrentModulator(bool current);
    void setDestinationComponent(Component* component, const std::string& name);
    Colour getInternalColor();
    void setSource(const std::string& name);

    Colour withBypassSaturation(Colour color) const {
      if (bypass_)
        return color.withSaturation(0.0f);
      return color;
    }

    virtual Colour getUnselectedColor() const override {
      return withBypassSaturation(SynthSlider::getUnselectedColor());
    }

    virtual Colour getSelectedColor() const override {
      return withBypassSaturation(SynthSlider::getSelectedColor());
    }

    virtual Colour getThumbColor() const override {
      return withBypassSaturation(SynthSlider::getThumbColor());
    }

    void setBypass(bool bypass) { bypass_ = bypass; setColors(); }
    void setStereo(bool stereo) { stereo_ = stereo; }
    void setBipolar(bool bipolar) { bipolar_ = bipolar; }
    bool isBypass() { return bypass_; }
    bool isStereo() { return stereo_; }
    bool isBipolar() { return bipolar_; }
    bool enteringValue() { return text_entry_ && text_entry_->isVisible(); }
    bool isCurrentModulator() { return current_modulator_; }
    int index() { return index_; }

    void setAux(String name) {
      aux_name_ = name;
      setName(aux_name_);
      setModulationAmount(1.0f);
    }
    bool hasAux() { return !aux_name_.isEmpty(); }
    void removeAux() {
      aux_name_ = "";
      setName(name_);
      setModulationAmount(0.0f);
    }
    String getOriginalName() { return name_; }

    force_inline bool hovering() const {
      return hovering_;
    }

    void addModulationAmountListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    void toggleBypass();

    std::vector<Listener*> listeners_;

    Point<int> mouse_down_position_;
    Component* color_component_;
    String aux_name_;
    String name_;
    bool editing_;
    int index_;
    bool showing_;
    bool hovering_;
    bool current_modulator_;
    bool bypass_;
    bool stereo_;
    bool bipolar_;
    bool draw_background_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationAmountKnob)
};

class ModulationExpansionBox : public OpenGlQuad {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
      
        virtual void expansionFocusLost() = 0;
    };

    ModulationExpansionBox() : OpenGlQuad(Shaders::kRoundedRectangleFragment) { }

    void focusLost(FocusChangeType cause) override {
      OpenGlQuad::focusLost(cause);

      for (Listener* listener : listeners_)
        listener->expansionFocusLost();
    }

    void setAmountControls(std::vector<ModulationAmountKnob*> amount_controls) { amount_controls_ = amount_controls; }
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    std::vector<ModulationAmountKnob*> amount_controls_;
    std::vector<Listener*> listeners_;
};

class ModulationManager : public SynthSection,
                          public ModulationButton::Listener,
                          public ModulationAmountKnob::Listener,
                          public SynthSlider::SliderListener,
                          public ModulationExpansionBox::Listener {
  public:
    static constexpr int kIndicesPerMeter = 6;
    static constexpr float kDragImageWidthPercent = 0.018f;

    ModulationManager(std::map<std::string, ModulationButton*> modulation_buttons,
                      std::map<std::string, SynthSlider*> sliders,
                      vital::output_map mono_modulations,
                      vital::output_map poly_modulations);
    ~ModulationManager();

    void createModulationMeter(const vital::Output* mono_total, const vital::Output* poly_total,
                               SynthSlider* slider, OpenGlMultiQuad* quads, int index);
    void createModulationSlider(std::string name, SynthSlider* slider, bool poly);

    void connectModulation(std::string source, std::string destination);
    void removeModulation(std::string source, std::string destination);
    void setModulationSliderValue(int index, float value);
    void setModulationSliderBipolar(int index, bool bipolar);
    void setModulationSliderValues(int index, float value);
    void setModulationSliderScale(int index);
    void setModulationValues(std::string source, std::string destination,
                             vital::mono_float amount, bool bipolar, bool stereo, bool bypass);
    void reset() override;
    void initAuxConnections();

    void resized() override;
    void parentHierarchyChanged() override;
    void updateModulationMeterLocations();
    void modulationAmountChanged(SynthSlider* slider) override;
    void modulationRemoved(SynthSlider* slider) override;

    void modulationDisconnected(vital::ModulationConnection* connection, bool last) override;
    void modulationSelected(ModulationButton* source) override;
    void modulationClicked(ModulationButton* source) override;
    void modulationCleared() override;
    bool hasFreeConnection();
    void startModulationMap(ModulationButton* source, const MouseEvent& e) override;
    void modulationDragged(const MouseEvent& e) override;
    void modulationWheelMoved(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void clearTemporaryModulation();
    void clearTemporaryHoverModulation();
    void modulationDraggedToHoverSlider(ModulationAmountKnob* hover_slider);
    void modulationDraggedToComponent(Component* component, bool bipolar);
    void setTemporaryModulationBipolar(Component* component, bool bipolar);
    void endModulationMap() override;
    void modulationLostFocus(ModulationButton* source) override;

    void expansionFocusLost() override {
      hideModulationAmountCallout();
    }

    void clearModulationSource();

    void disconnectModulation(ModulationAmountKnob* modulation_knob) override;
    void setModulationSettings(ModulationAmountKnob* modulation_knob);
    void setModulationBypass(ModulationAmountKnob* modulation_knob, bool bypass) override;
    void setModulationBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) override;
    void setModulationStereo(ModulationAmountKnob* modulation_knob, bool stereo) override;

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void drawModulationDestinations(OpenGlWrapper& open_gl);
    void drawCurrentModulator(OpenGlWrapper& open_gl);
    void drawDraggingModulation(OpenGlWrapper& open_gl);
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void renderMeters(OpenGlWrapper& open_gl, bool animate);
    void renderSourceMeters(OpenGlWrapper& open_gl, int index);
    void updateSmoothModValues();
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void paintBackground(Graphics& g) override { positionModulationAmountSliders(); }

    void setModulationAmounts();
    void setVisibleMeterBounds();

    void hoverStarted(SynthSlider* slider) override;
    void hoverEnded(SynthSlider* slider) override;
    void menuFinished(SynthSlider* slider) override;
    void modulationsChanged(const std::string& name) override;
    int getIndexForModulationSlider(Slider* slider);
    int getModulationIndex(std::string source, std::string destination);
    vital::ModulationConnection* getConnectionForModulationSlider(Slider* slider);
    vital::ModulationConnection* getConnection(int index);
    vital::ModulationConnection* getConnection(const std::string& source, const std::string& dest);
    void mouseDown(SynthSlider* slider) override;
    void mouseUp(SynthSlider* slider) override;
    void doubleClick(SynthSlider* slider) override;
    void beginModulationEdit(SynthSlider* slider) override;
    void endModulationEdit(SynthSlider* slider) override;
    void sliderValueChanged(Slider* slider) override;
    void buttonClicked(Button* button) override;
    void hideUnusedHoverModulations();
    float getAuxMultiplier(int index);
    void addAuxConnection(int from_index, int to_index);
    void removeAuxDestinationConnection(int to_index);
    void removeAuxSourceConnection(int from_index);

  private:
    void setDestinationQuadBounds(ModulationDestination* destination);
    void makeCurrentModulatorAmountsVisible();
    void makeModulationsVisible(SynthSlider* destination, bool visible);
    void positionModulationAmountSlidersInside(const std::string& source,
                                               std::vector<vital::ModulationConnection*> connections);
    void positionModulationAmountSlidersCallout(const std::string& source,
                                                std::vector<vital::ModulationConnection*> connections);
    void showModulationAmountCallout(const std::string& source);
    void hideModulationAmountCallout();
    void positionModulationAmountSliders(const std::string& source);
    void positionModulationAmountSliders();
    bool enteringHoverValue();
    void showModulationAmountOverlay(ModulationAmountKnob* slider);
    void hideModulationAmountOverlay();

    std::unique_ptr<Component> modulation_destinations_;

    ModulationButton* current_source_;
    ExpandModulationButton* current_expanded_modulation_;
    ModulationDestination* temporarily_set_destination_;
    SynthSlider* temporarily_set_synth_slider_;
    ModulationAmountKnob* temporarily_set_hover_slider_;
    bool temporarily_set_bipolar_;
    OpenGlQuad drag_quad_;
    ModulationExpansionBox modulation_expansion_box_;
    OpenGlQuad current_modulator_quad_;
    OpenGlQuad editing_rotary_amount_quad_;
    OpenGlQuad editing_linear_amount_quad_;
    std::map<Viewport*, std::unique_ptr<OpenGlMultiQuad>> rotary_destinations_;
    std::map<Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_destinations_;
    std::map<Viewport*, std::unique_ptr<OpenGlMultiQuad>> rotary_meters_;
    std::map<Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_meters_;
    Point<int> mouse_drag_start_;
    Point<int> mouse_drag_position_;
    bool modifying_;
    bool dragging_;
    bool changing_hover_modulation_;

    ModulationButton* current_modulator_;
    std::map<std::string, ModulationButton*> modulation_buttons_;
    std::map<std::string, std::unique_ptr<ExpandModulationButton>> modulation_callout_buttons_;
    std::map<std::string, const vital::StatusOutput*> modulation_source_readouts_;
    std::map<std::string, vital::poly_float> smooth_mod_values_;
    std::map<std::string, bool> active_mod_values_;
    const vital::StatusOutput* num_voices_readout_;
    long long last_milliseconds_;
    std::unique_ptr<BarRenderer> modulation_source_meters_;

    std::map<std::string, ModulationDestination*> destination_lookup_;
    std::map<std::string, SynthSlider*> slider_model_lookup_;
    std::map<std::string, ModulationAmountKnob*> modulation_amount_lookup_;

    std::vector<std::unique_ptr<ModulationDestination>> all_destinations_;
    std::map<std::string, std::unique_ptr<ModulationMeter>> meter_lookup_;
    std::map<int, int> aux_connections_from_to_;
    std::map<int, int> aux_connections_to_from_;
    std::unique_ptr<ModulationAmountKnob> modulation_amount_sliders_[vital::kMaxModulationConnections];
    std::unique_ptr<ModulationAmountKnob> modulation_hover_sliders_[vital::kMaxModulationConnections];
    std::unique_ptr<ModulationAmountKnob> selected_modulation_sliders_[vital::kMaxModulationConnections];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};

