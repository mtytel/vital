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
#include "default_look_and_feel.h"
#include "drag_drop_effect_order.h"
#include "open_gl_image.h"
#include "open_gl_multi_quad.h"
#include "synth_constants.h"
#include "synth_section.h"

class EffectsContainer;
class ChorusSection;
class CompressorSection;
class DelaySection;
class DistortionSection;
class DragDropEffectOrder;
class EqualizerSection;
class FilterSection;
class FlangerSection;
class PhaserSection;
class ReverbSection;

class EffectsViewport : public Viewport {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void effectsScrolled(int position) = 0;
    };

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void visibleAreaChanged(const Rectangle<int>& visible_area) override {
      for (Listener* listener : listeners_)
        listener->effectsScrolled(visible_area.getY());

      Viewport::visibleAreaChanged(visible_area);
    }

  private:
    std::vector<Listener*> listeners_;
};

class EffectsInterface : public SynthSection, public DragDropEffectOrder::Listener,
                         public ScrollBar::Listener, EffectsViewport::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void effectsMoved() = 0;
    };

    EffectsInterface(const vital::output_map& mono_modulations);
    virtual ~EffectsInterface();

    void paintBackground(Graphics& g) override;
    void paintChildrenShadows(Graphics& g) override { }
    void resized() override;
    void redoBackgroundImage();

    void orderChanged(DragDropEffectOrder* order) override;
    void effectEnabledChanged(int order_index, bool enabled) override;

    void setFocus() { grabKeyboardFocus(); }
    void setEffectPositions();
    
    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void effectsScrolled(int position) override {
      setScrollBarRange();
      scroll_bar_->setCurrentRange(position, viewport_.getHeight());

      for (Listener* listener : listeners_)
        listener->effectsMoved();
    }

  private:
    std::vector<Listener*> listeners_;
    EffectsViewport viewport_;
    std::unique_ptr<EffectsContainer> container_;
    OpenGlImage background_;
    CriticalSection open_gl_critical_section_;

    std::unique_ptr<ChorusSection> chorus_section_;
    std::unique_ptr<CompressorSection> compressor_section_;
    std::unique_ptr<DelaySection> delay_section_;
    std::unique_ptr<DistortionSection> distortion_section_;
    std::unique_ptr<EqualizerSection> equalizer_section_;
    std::unique_ptr<FlangerSection> flanger_section_;
    std::unique_ptr<PhaserSection> phaser_section_;
    std::unique_ptr<ReverbSection> reverb_section_;
    std::unique_ptr<FilterSection> filter_section_;
    std::unique_ptr<DragDropEffectOrder> effect_order_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;

    SynthSection* effects_list_[vital::constants::kNumEffects];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsInterface)
};

