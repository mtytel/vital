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

#include "header_section.h"

#include "fonts.h"
#include "preset_selector.h"
#include "load_save.h"
#include "oscilloscope.h"
#include "tab_selector.h"
#include "volume_section.h"

class LogoButton : public Button {
  public:
    LogoButton(const String& name) : Button(name) {
      image_component_.setComponent(this);
    }

    void setPaths(const Path& letter, const Path& ring) {
      letter_ = letter;
      ring_ = ring;
    }

    void resized() override {
      const DropShadow shadow(Colours::white, 5, Point<int>(0, 0));

      if (shadow_.getWidth() == getWidth() && shadow_.getHeight() == getHeight())
        return;

      Rectangle<float> bounds = getLocalBounds().toFloat();
      letter_.applyTransform(letter_.getTransformToScaleToFit(bounds, true));
      ring_.applyTransform(ring_.getTransformToScaleToFit(bounds, true));

      shadow_ = Image(Image::SingleChannel, getWidth(), getHeight(), true);

      Graphics shadow_g(shadow_);
      shadow.drawForPath(shadow_g, letter_);
      shadow.drawForPath(shadow_g, ring_);

      redoImage();
    }

    void paintButton(Graphics& g, bool hover, bool down) override {
      Rectangle<float> bounds = getLocalBounds().toFloat();
      letter_.applyTransform(letter_.getTransformToScaleToFit(bounds, true));
      ring_.applyTransform(ring_.getTransformToScaleToFit(bounds, true));

      g.setColour(findColour(Skin::kShadow, true));
      g.drawImageAt(shadow_, 0, 0, true);

      ColourGradient letter_gradient(letter_top_color_, 0.0f, 0.0f, letter_bottom_color_, 0.0f, getHeight(), false);
      ColourGradient ring_gradient(ring_top_color_, 0.0f, 0.0f, ring_bottom_color_, 0.0f, getHeight(), false);
      g.setGradientFill(letter_gradient);
      g.fillPath(letter_);

      g.setGradientFill(ring_gradient);
      g.fillPath(ring_);

      if (hover) {
        g.setColour(findColour(Skin::kLightenScreen, true));
        g.fillEllipse(getLocalBounds().toFloat());
      }
      else if (down) {
        g.setColour(findColour(Skin::kOverlayScreen, true));
        g.fillEllipse(getLocalBounds().toFloat());
      }
    }

    void setLetterColors(Colour top, Colour bottom) {
      letter_top_color_ = top;
      letter_bottom_color_ = bottom;
      redoImage();
    }

    void setRingColors(Colour top, Colour bottom) {
      ring_top_color_ = top;
      ring_bottom_color_ = bottom;
      redoImage();
    }

    void mouseEnter(const MouseEvent& e) override {
      Button::mouseEnter(e);
      image_component_.setColor(Colour(0xffdddddd));
    }

    void mouseExit(const MouseEvent& e) override {
      Button::mouseExit(e);
      image_component_.setColor(Colours::white);
    }

    OpenGlImageComponent* getImageComponent() { return &image_component_; }
    void redoImage() { image_component_.redrawImage(true); }

  private:
    OpenGlImageComponent image_component_;

    Path letter_;
    Path ring_;

    Image shadow_;

    Colour letter_top_color_;
    Colour letter_bottom_color_;

    Colour ring_top_color_;
    Colour ring_bottom_color_;
};

LogoSection::LogoSection() : SynthSection("logo_section") {
#if !defined(NO_TEXT_ENTRY)
  logo_button_ = std::make_unique<LogoButton>("logo");
  logo_button_->setPaths(Paths::vitalV(), Paths::vitalRing());
  addAndMakeVisible(logo_button_.get());
  addOpenGlComponent(logo_button_->getImageComponent());
  logo_button_->addListener(this);
#endif

  setSkinOverride(Skin::kLogo);
}

void LogoSection::resized() {
  int logo_padding_y = kLogoPaddingY * size_ratio_;
  int logo_height = getHeight() - 2 * logo_padding_y;
  int logo_padding_x = (getWidth() - logo_height) / 2;
  if (logo_button_)
    logo_button_->setBounds(logo_padding_x, logo_padding_y, logo_height, logo_height);
}

void LogoSection::paintBackground(Graphics& g) {
  if (logo_button_) {
    logo_button_->setRingColors(findColour(Skin::kWidgetPrimary1, true), findColour(Skin::kWidgetPrimary2, true));
    logo_button_->setLetterColors(findColour(Skin::kWidgetSecondary1, true), findColour(Skin::kWidgetSecondary2, true));
  }
}

void LogoSection::buttonClicked(Button* clicked_button) {
  for (Listener* listener : listeners_)
    listener->showAboutSection();
}

HeaderSection::HeaderSection() : SynthSection("header_section"), tab_offset_(0) {
  logo_section_ = std::make_unique<LogoSection>();
  addSubSection(logo_section_.get());
  logo_section_->addListener(this);

  tab_selector_ = std::make_unique<TabSelector>("tab_selector");
  addAndMakeVisible(tab_selector_.get());
  addOpenGlComponent(tab_selector_->getImageComponent());
  tab_selector_->setSliderStyle(Slider::LinearBar);
  tab_selector_->setRange(0, 3);
  tab_selector_->addListener(this);
  tab_selector_->setNames({"VOICE", "EFFECTS", "MATRIX", "ADVANCED"});
  tab_selector_->setScrollWheelEnabled(false);

  synth_preset_selector_ = std::make_unique<SynthPresetSelector>();
  addSubSection(synth_preset_selector_.get());
  synth_preset_selector_->addListener(this);

  
  volume_section_ = std::make_unique<VolumeSection>("VOLUME");
  addSubSection(volume_section_.get());

  oscilloscope_ = std::make_unique<Oscilloscope>();
  addOpenGlComponent(oscilloscope_.get());

  spectrogram_ = std::make_unique<Spectrogram>();
  addOpenGlComponent(spectrogram_.get());
  spectrogram_->setVisible(false);
  spectrogram_->paintBackgroundLines(false);
  spectrogram_->setMinDb(-40.0f);
  spectrogram_->setMaxDb(0.0f);

  view_spectrogram_ = std::make_unique<SynthButton>("view_spectrogram");
  addButton(view_spectrogram_.get());
  view_spectrogram_->getGlComponent()->setVisible(false);

  exit_temporary_button_ = std::make_unique<OpenGlShapeButton>("Exit");
  addChildComponent(exit_temporary_button_.get());
  addOpenGlComponent(exit_temporary_button_->getGlComponent());
  exit_temporary_button_->addListener(this);
  exit_temporary_button_->setShape(Paths::exitX());

  temporary_tab_ = std::make_unique<PlainTextComponent>("Temporary", "");
  addOpenGlComponent(temporary_tab_.get());
  temporary_tab_->setFontType(PlainTextComponent::kLight);
  temporary_tab_->setJustification(Justification::centredLeft);

  setSkinOverride(Skin::kHeader);
}

void HeaderSection::paintBackground(Graphics& g) {
  paintContainer(g);

  g.setColour(findColour(Skin::kBody, true));
  int logo_section_width = getStandardKnobSize() + getPadding();
  g.fillRect(0, 0, logo_section_width, getHeight());

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);

  g.saveState();
  Rectangle<int> bounds = getLocalArea(synth_preset_selector_.get(), synth_preset_selector_->getLocalBounds());
  g.reduceClipRegion(bounds);
  g.setOrigin(bounds.getTopLeft());
  synth_preset_selector_->paintBackground(g);
  g.restoreState();

  if (LoadSave::doesExpire()) {
    String countdown = "Beta expires in: " + String(LoadSave::getDaysToExpire()) + " days";
    int countdown_height = volume_section_->getY() + volume_section_->getBuffer();
    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(countdown_height / 2.0f));
    g.setColour(findColour(Skin::kTextComponentText, true));
    g.drawText(countdown, volume_section_->getX(), 0,
               volume_section_->getWidth(), countdown_height, Justification::centred);
  }
}

void HeaderSection::resized() {
  static constexpr float kTextHeightRatio = 0.3f;
  static constexpr float kPaddingLeft = 0.25f;

  oscilloscope_->setColour(Skin::kBody, findColour(Skin::kBackground, true));
  spectrogram_->setColour(Skin::kBody, findColour(Skin::kBackground, true));
  int height = getHeight();
  int width = getWidth();
  int widget_margin = findValue(Skin::kWidgetMargin);
  int large_padding = findValue(Skin::kLargePadding);
  int logo_width = findValue(Skin::kModulationButtonWidth);
  logo_section_->setBounds(large_padding, 0, logo_width, height);

  int preset_selector_width = width / 3;
  int preset_selector_height = height * 0.6f;
  int preset_selector_buffer = (height - preset_selector_height) * 0.5f;
  int preset_selector_x = (getWidth() - preset_selector_width + 2 * preset_selector_height) / 2;
  synth_preset_selector_->setBounds(preset_selector_x, preset_selector_buffer,
                                    preset_selector_width, preset_selector_height);

  int component_padding = 12 * size_ratio_;
  int volume_width = (width - synth_preset_selector_->getRight() - 2 * component_padding) / 2;
  int oscilloscope_width = volume_width - widget_margin;
  int oscilloscope_x = width - oscilloscope_width - large_padding;
  volume_section_->setBounds(synth_preset_selector_->getRight() + component_padding, 0, volume_width, height);
  oscilloscope_->setBounds(oscilloscope_x, widget_margin, oscilloscope_width, height - 2 * widget_margin);
  view_spectrogram_->setBounds(oscilloscope_->getBounds());
  spectrogram_->setBounds(oscilloscope_->getBounds());

  int tabs_width = preset_selector_x - component_padding - tab_offset_;
  tab_selector_->setBounds(tab_offset_, 0, tabs_width, height);
  exit_temporary_button_->setBounds(tab_offset_, 0, height, height);

  float temporary_height = getHeight() * kTextHeightRatio;
  temporary_tab_->setTextSize(temporary_height);

  int temporary_x = exit_temporary_button_->getRight() + getHeight() * kPaddingLeft;
  int temporary_width = synth_preset_selector_->getX() - temporary_x;
  temporary_tab_->setBounds(temporary_x, 0, temporary_width, getHeight());

  SynthSection::resized();
}

void HeaderSection::reset() {
  if (preset_selector_)
    synth_preset_selector_->resetText();
}

void HeaderSection::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  bool view_spectrogram = view_spectrogram_->getToggleState();
  oscilloscope_->setVisible(!view_spectrogram);
  spectrogram_->setVisible(view_spectrogram);
}

void HeaderSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == exit_temporary_button_.get()) {
    for (Listener* listener : listeners_)
      listener->clearTemporaryTab(tab_selector_->getValue());
    setTemporaryTab("");
  }
  else if (clicked_button == view_spectrogram_.get()) {
    bool view_spectrogram = view_spectrogram_->getToggleState();
    oscilloscope_->setVisible(!view_spectrogram);
    spectrogram_->setVisible(view_spectrogram);
    SynthSection::buttonClicked(clicked_button);
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void HeaderSection::sliderValueChanged(Slider* slider) {
  if (slider == tab_selector_.get()) {
    int index = tab_selector_->getValue();
    for (Listener* listener : listeners_)
      listener->tabSelected(index);
  }
  else
    SynthSection::sliderValueChanged(slider);
}

void HeaderSection::setPresetBrowserVisibility(bool visible) {
  for (Listener* listener : listeners_)
    listener->setPresetBrowserVisibility(visible, tab_selector_->getValue());
}

void HeaderSection::setBankExporterVisibility(bool visible) {
  for (Listener* listener : listeners_)
    listener->setBankExporterVisibility(visible, tab_selector_->getValue());
}

void HeaderSection::deleteRequested(File preset) {
  for (Listener* listener : listeners_)
    listener->deleteRequested(preset);
}

void HeaderSection::bankImported() {
  for (Listener* listener : listeners_)
    listener->bankImported();
}

void HeaderSection::save(File preset) {
  synth_preset_selector_->resetText();
  synth_preset_selector_->setModified(false);
}

void HeaderSection::setTemporaryTab(String name) {
  temporary_tab_->setText(name);
  tab_selector_->setVisible(name.isEmpty());
  exit_temporary_button_->setVisible(!name.isEmpty());
  repaint();
  repaintBackground();
}

void HeaderSection::setOscilloscopeMemory(const vital::poly_float* memory) {
  oscilloscope_->setOscilloscopeMemory(memory);
}

void HeaderSection::setAudioMemory(const vital::StereoMemory* memory) {
  spectrogram_->setAudioMemory(memory);
}

void HeaderSection::notifyChange() {
  synth_preset_selector_->setModified(true);
}

void HeaderSection::notifyFresh() {
  synth_preset_selector_->resetText();
  synth_preset_selector_->setModified(false);
}
