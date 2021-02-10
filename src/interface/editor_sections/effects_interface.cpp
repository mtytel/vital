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

#include "effects_interface.h"

#include "chorus_section.h"
#include "compressor_section.h"
#include "delay_section.h"
#include "distortion_section.h"
#include "equalizer_section.h"
#include "full_interface.h"
#include "filter_section.h"
#include "flanger_section.h"
#include "phaser_section.h"
#include "reverb_section.h"

class EffectsContainer : public SynthSection {
  public:
    EffectsContainer(String name) : SynthSection(name) { }
    void paintBackground(Graphics& g) override {
      g.fillAll(findColour(Skin::kBackground, true));
      paintChildrenShadows(g);
      paintChildrenBackgrounds(g);
    }
};

EffectsInterface::EffectsInterface(const vital::output_map& mono_modulations) : SynthSection("effects") {
  container_ = std::make_unique<EffectsContainer>("container");

  addAndMakeVisible(viewport_);
  viewport_.setViewedComponent(container_.get());
  viewport_.addListener(this);
  viewport_.setScrollBarsShown(false, false, true, false);

  chorus_section_ = std::make_unique<ChorusSection>("CHORUS", mono_modulations);
  container_->addSubSection(chorus_section_.get());

  compressor_section_ = std::make_unique<CompressorSection>("COMPRESSOR");
  container_->addSubSection(compressor_section_.get());

  delay_section_ = std::make_unique<DelaySection>("DELAY", mono_modulations);
  container_->addSubSection(delay_section_.get());

  distortion_section_ = std::make_unique<DistortionSection>("DISTORTION", mono_modulations);
  container_->addSubSection(distortion_section_.get());

  equalizer_section_ = std::make_unique<EqualizerSection>("EQ", mono_modulations);
  container_->addSubSection(equalizer_section_.get());

  flanger_section_ = std::make_unique<FlangerSection>("FLANGER", mono_modulations);
  container_->addSubSection(flanger_section_.get());

  phaser_section_ = std::make_unique<PhaserSection>("PHASER", mono_modulations);
  container_->addSubSection(phaser_section_.get());

  reverb_section_ = std::make_unique<ReverbSection>("REVERB", mono_modulations);
  container_->addSubSection(reverb_section_.get());

  filter_section_ = std::make_unique<FilterSection>("fx", mono_modulations);
  container_->addSubSection(filter_section_.get());

  effect_order_ = std::make_unique<DragDropEffectOrder>("effect_chain_order");
  addSubSection(effect_order_.get());
  effect_order_->addListener(this);

  addSubSection(container_.get(), false);

  effects_list_[0] = chorus_section_.get();
  effects_list_[1] = compressor_section_.get();
  effects_list_[2] = delay_section_.get();
  effects_list_[3] = distortion_section_.get();
  effects_list_[4] = equalizer_section_.get();
  effects_list_[5] = filter_section_.get();
  effects_list_[6] = flanger_section_.get();
  effects_list_[7] = phaser_section_.get();
  effects_list_[8] = reverb_section_.get();

  scroll_bar_ = std::make_unique<OpenGlScrollBar>();
  scroll_bar_->setShrinkLeft(true);
  addAndMakeVisible(scroll_bar_.get());
  addOpenGlComponent(scroll_bar_->getGlComponent());
  scroll_bar_->addListener(this);

  setOpaque(false);
  setSkinOverride(Skin::kAllEffects);
}

EffectsInterface::~EffectsInterface() {
  chorus_section_ = nullptr;
  compressor_section_ = nullptr;
  delay_section_ = nullptr;
  distortion_section_ = nullptr;
  equalizer_section_ = nullptr;
  flanger_section_ = nullptr;
  phaser_section_ = nullptr;
  reverb_section_ = nullptr;
  effect_order_ = nullptr;
}

void EffectsInterface::paintBackground(Graphics& g) {
  Colour background = findColour(Skin::kBackground, true);
  g.setColour(background);
  g.fillRect(getLocalBounds().withRight(getWidth() - findValue(Skin::kLargePadding) / 2));
  paintChildBackground(g, effect_order_.get());

  redoBackgroundImage();
}

void EffectsInterface::redoBackgroundImage() {
  Colour background = findColour(Skin::kBackground, true);

  int height = std::max(container_->getHeight(), getHeight());
  int mult = getPixelMultiple();
  Image background_image = Image(Image::ARGB, container_->getWidth() * mult, height * mult, true);

  Graphics background_graphics(background_image);
  background_graphics.addTransform(AffineTransform::scale(mult));
  background_graphics.fillAll(background);
  container_->paintBackground(background_graphics);
  background_.setOwnImage(background_image);
}

void EffectsInterface::resized() {
  static constexpr float kEffectOrderWidthPercent = 0.2f;

  ScopedLock lock(open_gl_critical_section_);

  int order_width = getWidth() * kEffectOrderWidthPercent;
  effect_order_->setBounds(0, 0, order_width, getHeight());
  effect_order_->setSizeRatio(size_ratio_);
  int large_padding = findValue(Skin::kLargePadding);
  int shadow_width = getComponentShadowWidth();
  int viewport_x = order_width + large_padding - shadow_width;
  int viewport_width = getWidth() - viewport_x - large_padding + 2 * shadow_width;
  viewport_.setBounds(viewport_x, 0, viewport_width, getHeight());
  setEffectPositions();

  scroll_bar_->setBounds(getWidth() - large_padding + 1, 0, large_padding - 2, getHeight());
  scroll_bar_->setColor(findColour(Skin::kLightenScreen, true));

  SynthSection::resized();
}

void EffectsInterface::orderChanged(DragDropEffectOrder* order) {
  ScopedLock lock(open_gl_critical_section_);

  setEffectPositions();
  repaintBackground();
}

void EffectsInterface::effectEnabledChanged(int order_index, bool enabled) {
  ScopedLock lock(open_gl_critical_section_);

  if (enabled)
    effects_list_[order_index]->activator()->setToggleState(true, sendNotification);

  setEffectPositions();
  repaintBackground();
}

void EffectsInterface::setEffectPositions() {
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  int padding = getPadding();
  int large_padding = findValue(Skin::kLargePadding);
  int shadow_width = getComponentShadowWidth();
  int start_x = effect_order_->getRight() + large_padding;
  int effect_width = getWidth() - start_x - large_padding;
  int knob_section_height = getKnobSectionHeight();
  int widget_margin = findValue(Skin::kWidgetMargin);
  int effect_height = 2 * knob_section_height - widget_margin;
  int y = 0;

  Point<int> position = viewport_.getViewPosition();

  for (int i = 0; i < vital::constants::kNumEffects; ++i) {
    bool enabled = effect_order_->effectEnabled(i);
    effects_list_[effect_order_->getEffectIndex(i)]->setVisible(enabled);
    if (enabled) {
      effects_list_[effect_order_->getEffectIndex(i)]->setBounds(shadow_width, y, effect_width, effect_height);
      y += effect_height + padding;
    }
  }

  container_->setBounds(0, 0, viewport_.getWidth(), y - padding);
  viewport_.setViewPosition(position);

  for (Listener* listener : listeners_)
    listener->effectsMoved();

  container_->setScrollWheelEnabled(container_->getHeight() <= viewport_.getHeight());
  setScrollBarRange();
  repaintBackground();
}

void EffectsInterface::initOpenGlComponents(OpenGlWrapper& open_gl) {
  background_.init(open_gl);
  SynthSection::initOpenGlComponents(open_gl);
}

void EffectsInterface::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  ScopedLock lock(open_gl_critical_section_);

  OpenGlComponent::setViewPort(&viewport_, open_gl);

  float image_width = vital::utils::nextPowerOfTwo(background_.getImageWidth());
  float image_height = vital::utils::nextPowerOfTwo(background_.getImageHeight());
  int mult = getPixelMultiple();
  float width_ratio = image_width / (container_->getWidth() * mult);
  float height_ratio = image_height / (viewport_.getHeight() * mult);
  float y_offset = (2.0f * viewport_.getViewPositionY()) / getHeight();

  background_.setTopLeft(-1.0f, 1.0f + y_offset);
  background_.setTopRight(-1.0 + 2.0f * width_ratio, 1.0f + y_offset);
  background_.setBottomLeft(-1.0f, 1.0f - 2.0f * height_ratio + y_offset);
  background_.setBottomRight(-1.0 + 2.0f * width_ratio, 1.0f - 2.0f * height_ratio + y_offset);

  background_.setColor(Colours::white);
  background_.drawImage(open_gl);
  SynthSection::renderOpenGlComponents(open_gl, animate);
}

void EffectsInterface::destroyOpenGlComponents(OpenGlWrapper& open_gl) {
  background_.destroy(open_gl);
  SynthSection::destroyOpenGlComponents(open_gl);
}

void EffectsInterface::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
  viewport_.setViewPosition(Point<int>(0, range_start));
}

void EffectsInterface::setScrollBarRange() {
  scroll_bar_->setRangeLimits(0.0, container_->getHeight());
  scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getHeight(), dontSendNotification);
}
