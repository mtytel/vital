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

#include "filter_section.h"

#include "skin.h"
#include "digital_svf.h"
#include "filter_response.h"
#include "preset_selector.h"
#include "fonts.h"
#include "synth_button.h"
#include "synth_constants.h"
#include "synth_slider.h"
#include "synth_strings.h"
#include "synth_gui_interface.h"
#include "text_look_and_feel.h"

namespace {
  constexpr int kBlendLabelWidth = 30;

  int getNumStyles(int int_model) {
    vital::constants::FilterModel model = static_cast<vital::constants::FilterModel>(int_model);
    switch (model) {
      case vital::constants::kAnalog:
        return 5;
      case vital::constants::kDirty:
        return 5;
      case vital::constants::kLadder:
        return 5;
      case vital::constants::kDigital:
        return 5;
      case vital::constants::kDiode:
        return 2;
      case vital::constants::kFormant:
        return 2;
      case vital::constants::kComb:
        return 6;
      case vital::constants::kPhase:
        return 2;
      default:
        return 0;
    }
  }

  std::string getStyleName(int int_model, int style) {
    vital::constants::FilterModel model = static_cast<vital::constants::FilterModel>(int_model);
    switch (model) {
      case vital::constants::kAnalog:
      case vital::constants::kDirty:
      case vital::constants::kLadder:
      case vital::constants::kDigital:
        return strings::kFilterStyleNames[style];
      case vital::constants::kDiode:
        return strings::kDiodeStyleNames[style];
      case vital::constants::kFormant:
        if (style == vital::FormantFilter::kVocalTract)
          return "The Mouth";
        else if (style == vital::FormantFilter::kAIUO)
          return "AIUO";
        else
          return "AOIE";
      case vital::constants::kComb:
        return strings::kCombStyleNames[style];
      case vital::constants::kPhase:
        if (style)
          return "Negative";
        else
          return "Positive";
      default:
        return "";
    }
  }
}

FilterSection::FilterSection(String name, String suffix) : 
    SynthSection(name), current_model_(0), current_style_(0), specify_input_(false) {
  std::string number = suffix.toStdString();
  model_name_ = "filter_" + number + "_model";
  style_name_ = "filter_" + number + "_style";

  cutoff_ = std::make_unique<SynthSlider>("filter_" + number + "_cutoff");
  addSlider(cutoff_.get());
  cutoff_->setSliderStyle(Slider::LinearBar);
  cutoff_->setPopupPlacement(BubbleComponent::below);
  cutoff_->setModulationPlacement(BubbleComponent::above);
  cutoff_->setPopupPrefix("Cutoff: ");
  setSliderHasHzAlternateDisplay(cutoff_.get());

  formant_x_ = std::make_unique<SynthSlider>("filter_" + number + "_formant_x");
  addSlider(formant_x_.get());
  formant_x_->setSliderStyle(Slider::LinearBar);
  formant_x_->setPopupPlacement(BubbleComponent::below);
  formant_x_->setModulationPlacement(BubbleComponent::above);
  formant_x_->setPopupPrefix("Formant X: ");

  mix_ = std::make_unique<SynthSlider>("filter_" + number + "_mix");
  addSlider(mix_.get());
  mix_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  blend_ = std::make_unique<SynthSlider>("filter_" + number + "_blend");
  addSlider(blend_.get());
  blend_->snapToValue(true, 1.0);
  blend_->setBipolar(true);
  blend_->setSliderStyle(Slider::LinearBar);
  blend_->setPopupPlacement(BubbleComponent::above);
  blend_->setPopupPrefix("Blend: ");

  preset_selector_ = std::make_unique<PresetSelector>();
  addSubSection(preset_selector_.get());
  preset_selector_->addListener(this);
  setPresetSelector(preset_selector_.get());
  setFilterText();

  formant_transpose_ = std::make_unique<SynthSlider>("filter_" + number + "_formant_transpose");
  addSlider(formant_transpose_.get());
  formant_transpose_->snapToValue(true, 0.0);
  formant_transpose_->setBipolar(true);
  formant_transpose_->setSliderStyle(Slider::LinearBar);
  formant_transpose_->setPopupPlacement(BubbleComponent::above);
  formant_transpose_->setPopupPrefix("Formant Transpose: ");

  resonance_ = std::make_unique<SynthSlider>("filter_" + number + "_resonance");
  addSlider(resonance_.get());
  resonance_->setSliderStyle(Slider::LinearBarVertical);
  resonance_->setPopupPlacement(BubbleComponent::right);
  resonance_->setModulationPlacement(BubbleComponent::left);
  resonance_->setPopupPrefix("Resonance: ");

  formant_y_ = std::make_unique<SynthSlider>("filter_" + number + "_formant_y");
  addSlider(formant_y_.get());
  formant_y_->setSliderStyle(Slider::LinearBarVertical);
  formant_y_->setPopupPlacement(BubbleComponent::right);
  formant_y_->setModulationPlacement(BubbleComponent::left);
  formant_y_->setPopupPrefix("Formant Y: ");

  drive_ = std::make_unique<SynthSlider>("filter_" + number + "_drive");
  addSlider(drive_.get());
  drive_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  formant_resonance_ = std::make_unique<SynthSlider>("filter_" + number + "_formant_resonance");
  addSlider(formant_resonance_.get());
  formant_resonance_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  formant_spread_ = std::make_unique<SynthSlider>("filter_" + number + "_formant_spread");
  addSlider(formant_spread_.get());
  formant_spread_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  formant_spread_->snapToValue(true, 0.0);
  formant_spread_->setBipolar();

  blend_transpose_ = std::make_unique<SynthSlider>("filter_" + number + "_blend_transpose");
  addSlider(blend_transpose_.get());
  blend_transpose_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  keytrack_ = std::make_unique<SynthSlider>("filter_" + number + "_keytrack");
  addSlider(keytrack_.get());
  keytrack_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  keytrack_->snapToValue(true, 0.0);
  keytrack_->setBipolar();

  filter_on_ = std::make_unique<SynthButton>("filter_" + number + "_on");
  addButton(filter_on_.get());
  setActivator(filter_on_.get());

  filter_label_1_ = std::make_unique<PlainTextComponent>("label1", "DRIVE");
  addOpenGlComponent(filter_label_1_.get());

  filter_label_2_ = std::make_unique<PlainTextComponent>("label2", "KEY TRK");
  addOpenGlComponent(filter_label_2_.get());

  formant_x_->setVisible(false);
  formant_y_->setVisible(false);
  formant_transpose_->setVisible(false);
  formant_resonance_->setVisible(false);
  formant_spread_->setVisible(false);
  blend_transpose_->setVisible(false);
}

FilterSection::FilterSection(String suffix, const vital::output_map& mono_modulations) :
    FilterSection("FILTER", suffix) {
  filter_response_ = std::make_unique<FilterResponse>(suffix, mono_modulations);
  addOpenGlComponent(filter_response_.get());
  setFilterResponseSliders();
  preset_selector_->setTextComponent(true);
  setSkinOverride(Skin::kFxFilter);
}

FilterSection::FilterSection(int index, const vital::output_map& mono_modulations,
                             const vital::output_map& poly_modulations) :
    FilterSection("FILTER " + String(index), String(index)) {
  setSidewaysHeading(false);

  std::string number = std::to_string(index);
  filter_response_ = std::make_unique<FilterResponse>(index, mono_modulations, poly_modulations);
  addOpenGlComponent(filter_response_.get());
  setFilterResponseSliders();

  cutoff_->setExtraModulationTarget(filter_response_.get());

  osc1_input_ = std::make_unique<OpenGlToggleButton>("OSC1");
  addAndMakeVisible(osc1_input_.get());
  addOpenGlComponent(osc1_input_->getGlComponent());
  osc1_input_->addListener(this);
  osc1_input_->setText("OSC1");
  osc1_input_->setLookAndFeel(TextLookAndFeel::instance());

  osc2_input_ = std::make_unique<OpenGlToggleButton>("OSC2");
  addAndMakeVisible(osc2_input_.get());
  addOpenGlComponent(osc2_input_->getGlComponent());
  osc2_input_->addListener(this);
  osc2_input_->setText("OSC2");
  osc2_input_->setLookAndFeel(TextLookAndFeel::instance());

  osc3_input_ = std::make_unique<OpenGlToggleButton>("OSC3");
  addAndMakeVisible(osc3_input_.get());
  addOpenGlComponent(osc3_input_->getGlComponent());
  osc3_input_->addListener(this);
  osc3_input_->setText("OSC3");
  osc3_input_->setLookAndFeel(TextLookAndFeel::instance());

  sample_input_ = std::make_unique<OpenGlToggleButton>("SMP");
  addAndMakeVisible(sample_input_.get());
  addOpenGlComponent(sample_input_->getGlComponent());
  sample_input_->addListener(this);
  sample_input_->setText("SMP");
  sample_input_->setLookAndFeel(TextLookAndFeel::instance());

  filter_input_ = std::make_unique<SynthButton>("filter_" + number + "_filter_input");
  addButton(filter_input_.get());
  filter_input_->setText(String("FIL") + String(3 - index));
  filter_input_->setLookAndFeel(TextLookAndFeel::instance());
  specify_input_ = true;

  preset_selector_->setTextComponent(false);
  setSkinOverride(Skin::kFilter);
}

FilterSection::~FilterSection() { }

void FilterSection::setFilterResponseSliders() {
  filter_response_->setCutoffSlider(cutoff_.get());
  filter_response_->setResonanceSlider(resonance_.get());
  filter_response_->setFormantXSlider(formant_x_.get());
  filter_response_->setFormantYSlider(formant_y_.get());
  filter_response_->setBlendSlider(blend_.get());
  filter_response_->setTransposeSlider(blend_transpose_.get());
  filter_response_->setFormantTransposeSlider(formant_transpose_.get());
  filter_response_->setFormantResonanceSlider(formant_resonance_.get());
  filter_response_->setFormantSpreadSlider(formant_spread_.get());
  filter_response_->setFilterMixSlider(mix_.get());

  cutoff_->toFront(false);
  resonance_->toFront(false);
  formant_x_->toFront(false);
  formant_y_->toFront(false);
}

void FilterSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);
  setLabelFont(g);
  drawLabelForComponent(g, TRANS("MIX"), mix_.get());

  int title_width = getTitleWidth();
  int blend_label_padding_y = size_ratio_ * kBlendLabelPaddingY;

  drawLabelBackgroundForComponent(g, drive_.get());
  drawLabelBackgroundForComponent(g, keytrack_.get());

  int widget_margin = findValue(Skin::kWidgetMargin);
  int blend_height = filter_response_->getY() - title_width + widget_margin;
  int morph_y = title_width - widget_margin + blend_label_padding_y;
  if (!specify_input_) {
    g.setColour(findColour(Skin::kBody, true));
    g.fillRect(preset_selector_->getBounds());
    drawTextComponentBackground(g, preset_selector_->getBounds(), true);
    drawLabelForComponent(g, TRANS("MODE"), preset_selector_.get(), true);
    morph_y = blend_label_padding_y;
    blend_height = filter_response_->getY();
  }

  g.setColour(findColour(Skin::kBodyText, true));
  int morph_width = size_ratio_ * kBlendLabelWidth;
  int morph_height = blend_height - 2 * blend_label_padding_y;
  int left_morph_x = blend_->getX() - morph_width + widget_margin;
  int right_morph_x = getWidth() - morph_width;

  Rectangle<float> left_morph_bounds(left_morph_x, morph_y, morph_width, morph_height);
  Path left_morph = getLeftMorphPath();
  g.fillPath(left_morph, left_morph.getTransformToScaleToFit(left_morph_bounds, true));

  Rectangle<float> right_morph_bounds(right_morph_x, morph_y, morph_width, morph_height);
  Path right_morph = getRightMorphPath();
  g.fillPath(right_morph, right_morph.getTransformToScaleToFit(right_morph_bounds, true));
}

void FilterSection::positionTopBottom() {
  int title_width = getTitleWidth();
  int knob_section_height = getKnobSectionHeight();
  int slider_width = getSliderWidth();
  int blend_label_width = size_ratio_ * kBlendLabelWidth;
  int widget_margin = getWidgetMargin();

  int slider_overlap = getSliderOverlap();
  int slider_overlap_space = getSliderOverlapWithSpace();
  int response_width = getWidth() - slider_width + slider_overlap + slider_overlap_space - 2 * widget_margin;
  int response_y = title_width + slider_width - slider_overlap_space - slider_overlap;
  int response_height = getHeight() - 2 * slider_width -
                        title_width - knob_section_height + 2 * slider_overlap_space + 2 * slider_overlap;

  int blend_y = title_width - slider_overlap;
  blend_->setBounds(blend_label_width - widget_margin, blend_y,
                    getWidth() - 2 * (blend_label_width - widget_margin), slider_width);

  filter_response_->setBounds(widget_margin, response_y, response_width, response_height);
  int resonance_x = filter_response_->getRight() - slider_overlap + widget_margin;
  resonance_->setBounds(resonance_x, response_y - widget_margin, slider_width, response_height + 2 * widget_margin);
  int cutoff_y = filter_response_->getBottom() - slider_overlap + widget_margin;
  cutoff_->setBounds(0, cutoff_y, response_width + 2 * widget_margin, slider_width);

  float component_width = getWidth() / 5.0f;
  int knob_y = getHeight() - knob_section_height;

  int inputs_width = 2 * component_width;
  int internal_margin = widget_margin / 2;
  int input_width = (inputs_width - 2 * widget_margin - internal_margin) / 2;
  float input_height = (knob_section_height - 2 * (widget_margin + internal_margin)) / 3.0f;
  int osc_y = knob_y + widget_margin;

  osc1_input_->setBounds(widget_margin, osc_y, input_width, input_height);
  osc2_input_->setBounds(inputs_width - widget_margin - input_width, osc_y, input_width, input_height);

  int other_y = knob_y + (knob_section_height - input_height) / 2.0f;
  osc3_input_->setBounds(widget_margin, other_y, input_width, input_height);
  sample_input_->setBounds(inputs_width - widget_margin - input_width, other_y, input_width, input_height);

  int filter_x = (inputs_width - input_width) / 2;
  int filter_y = getHeight() - input_height - widget_margin;
  filter_input_->setBounds(filter_x, filter_y, input_width, input_height);

  int knobs_x = 2.0f * component_width - widget_margin;
  Rectangle<int> knobs_area(knobs_x, knob_y, getWidth() - knobs_x, knob_section_height);
  placeKnobsInArea(knobs_area, { drive_.get(), mix_.get(), keytrack_.get() });
}

void FilterSection::positionLeftRight() {
  int title_width = getTitleWidth();
  int knob_section_height = getKnobSectionHeight();
  int slider_width = getSliderWidth();
  int blend_label_width = size_ratio_ * kBlendLabelWidth;
  int widget_margin = getWidgetMargin();
  
  Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
  Rectangle<int> widget_bounds = getDividedAreaUnbuffered(bounds, 2, 1, widget_margin);
  
  int slider_overlap = getSliderOverlapWithSpace();
  int response_x = widget_bounds.getX();
  int response_area_width = getWidth() - response_x;
  int response_width = response_area_width - slider_width + 2 * slider_overlap;
  int response_y = slider_width - 2 * slider_overlap;
  int response_height = getHeight() - 2 * slider_width + 4 * slider_overlap;

  filter_response_->setBounds(response_x, response_y, response_width, response_height);
  blend_->setBounds(response_x + blend_label_width - 2 * widget_margin, -slider_overlap,
                    response_area_width - 2 * (blend_label_width - widget_margin), slider_width);
  resonance_->setBounds(getWidth() - slider_width + slider_overlap, response_y - widget_margin, slider_width,
                        response_height + 2 * widget_margin);
  cutoff_->setBounds(response_x - widget_margin, filter_response_->getBottom() - slider_overlap,
                     response_width + 2 * widget_margin, slider_width);

  int knob_area_width = response_x - title_width;
  int knobs_y = getHeight() - knob_section_height;
  placeKnobsInArea(Rectangle<int>(title_width, knobs_y, knob_area_width, knob_section_height),
                   { drive_.get(), mix_.get(), keytrack_.get() });

  preset_selector_->setBounds(title_width + widget_margin, widget_margin,
                              knob_area_width - 2 * widget_margin, knob_section_height - 2 * widget_margin);
}

void FilterSection::resized() {
  SynthSection::resized();

  if (specify_input_)
    positionTopBottom();
  else
    positionLeftRight();

  formant_x_->setBounds(cutoff_->getBounds());
  formant_y_->setBounds(resonance_->getBounds());
  formant_transpose_->setBounds(blend_->getBounds());
  formant_resonance_->setBounds(drive_->getBounds());
  formant_spread_->setBounds(keytrack_->getBounds());
  blend_transpose_->setBounds(drive_->getBounds());

  filter_label_1_->setFontType(PlainTextComponent::kRegular);
  filter_label_2_->setFontType(PlainTextComponent::kRegular);
  float label_height = findValue(Skin::kLabelHeight);
  filter_label_1_->setTextSize(label_height);
  filter_label_2_->setTextSize(label_height);

  filter_label_1_->setBounds(getLabelBackgroundBounds(drive_.get()));
  filter_label_2_->setBounds(getLabelBackgroundBounds(keytrack_.get()));

  Colour body_text = findColour(Skin::kBodyText, true);
  filter_label_1_->setColor(body_text);
  filter_label_2_->setColor(body_text);
}

void FilterSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == filter_input_.get()) {
    if (filter_input_->getToggleState()) {
      for (Listener* listener : listeners_)
        listener->filterSerialSelected(this);
    }
    SynthSection::buttonClicked(clicked_button);
  }
  else if (clicked_button == osc1_input_.get()) {
    for (Listener* listener : listeners_)
      listener->oscInputToggled(this, 0, clicked_button->getToggleState());
  }
  else if (clicked_button == osc2_input_.get()) {
    for (Listener* listener : listeners_)
      listener->oscInputToggled(this, 1, clicked_button->getToggleState());
  }
  else if (clicked_button == osc3_input_.get()) {
    for (Listener* listener : listeners_)
      listener->oscInputToggled(this, 2, clicked_button->getToggleState());
  }
  else if (clicked_button == sample_input_.get()) {
    for (Listener* listener : listeners_)
      listener->sampleInputToggled(this, clicked_button->getToggleState());
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void FilterSection::setAllValues(vital::control_map& controls) {
  current_model_ = std::round(controls[model_name_]->value());
  current_style_ = std::round(controls[style_name_]->value());
  setFilterText();

  vital::constants::FilterModel model = static_cast<vital::constants::FilterModel>(current_model_);
  filter_response_->setModel(model);
  filter_response_->setStyle(current_style_);
  showModelKnobs();
  setLabelText();
}

void FilterSection::prevClicked() {
  current_style_--;
  if (current_style_ < 0) {
    current_model_ = (current_model_ + vital::constants::kNumFilterModels - 1) % vital::constants::kNumFilterModels;
    current_style_ = getNumStyles(current_model_) - 1;
  }
  showModelKnobs();
  notifyFilterChange();
}

void FilterSection::nextClicked() {
  current_style_++;
  if (current_style_ >= getNumStyles(current_model_)) {
    current_style_ = 0;
    current_model_ = (current_model_ + 1) % vital::constants::kNumFilterModels;
  }
  showModelKnobs();
  notifyFilterChange();
}

void FilterSection::textMouseDown(const MouseEvent& e) {
  PopupItems options;
  
  int index = 1;
  for (int i = 0; i < vital::constants::kNumFilterModels; ++i) {
    PopupItems sub_options(strings::kFilterModelNames[i]);
    sub_options.selected = i == current_model_;

    int num_styles = getNumStyles(i);
    for (int style = 0; style < num_styles; ++style)
      sub_options.addItem(index++, getStyleName(i, style), style == current_style_);

    options.addItem(sub_options);
  }

  Point<int> position(getWidth(), preset_selector_->getY());
  if (!specify_input_)
    position = Point<int>(preset_selector_->getRight() - getDualPopupWidth(), preset_selector_->getBottom());
  showDualPopupSelector(this, position, getDualPopupWidth(),
                        options, [=](int selection) { setFilterSelected(selection); });
}

void FilterSection::setFilterSelected(int menu_id) {
  int current_id = 1;
  for (int i = 0; i < vital::constants::kNumFilterModels; ++i) {
    int num_styles = getNumStyles(i);
    if (menu_id - current_id < num_styles) {
      current_model_ = i;
      current_style_ = menu_id - current_id;
      showModelKnobs();
      notifyFilterChange();
      return;
    }

    current_id += num_styles;
  }
}

void FilterSection::setOscillatorInput(int oscillator_index, bool input) {
  if (oscillator_index == 0)
    osc1_input_->setToggleState(input, dontSendNotification);
  else if (oscillator_index == 1)
    osc2_input_->setToggleState(input, dontSendNotification);
  else
    osc3_input_->setToggleState(input, dontSendNotification);
}

void FilterSection::setSampleInput(bool input) {
  sample_input_->setToggleState(input, dontSendNotification);
}

void FilterSection::showModelKnobs() {
  vital::constants::FilterModel model = static_cast<vital::constants::FilterModel>(current_model_);
  filter_response_->setModel(model);

  bool formant = model == vital::constants::kFormant;
  bool vocal_tract = formant && current_style_ == vital::FormantFilter::kVocalTract;
  bool comb = model == vital::constants::kComb;
  formant_x_->setVisible(formant);
  formant_y_->setVisible(formant);
  formant_transpose_->setVisible(formant && !vocal_tract);
  formant_resonance_->setVisible(formant);
  formant_spread_->setVisible(formant);

  blend_transpose_->setVisible(comb);

  cutoff_->setVisible(!formant);
  resonance_->setVisible(!formant);
  keytrack_->setVisible(!formant);
  blend_->setVisible(!formant || vocal_tract);
  drive_->setVisible(!formant && !comb);
}

void FilterSection::setFilterText() {
  std::string style = getStyleName(current_model_, current_style_);
  preset_selector_->setText(strings::kFilterModelNames[current_model_], ":", style);
}

void FilterSection::setLabelText() {
  if (current_model_ == vital::constants::kFormant) {
    filter_label_1_->setText("PEAK");
    filter_label_2_->setText("SPREAD");
  }
  else {
    filter_label_2_->setText("KEY TRK");
    if (current_model_ == vital::constants::kComb)
      filter_label_1_->setText("CUT");
    else
      filter_label_1_->setText("DRIVE");
  }
}

void FilterSection::notifyFilterChange() {
  filter_response_->setStyle(current_style_);
  filter_response_->setModel(static_cast<vital::constants::FilterModel>(current_model_));
  setFilterText();
  setLabelText();

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent) {
    parent->getSynth()->valueChangedInternal(model_name_, current_model_);
    parent->getSynth()->valueChangedInternal(style_name_, current_style_);
  }
}

void FilterSection::setActive(bool active) {
  SynthSection::setActive(active);
  if (filter_response_)
    filter_response_->setActive(active);
}

Path FilterSection::getLeftMorphPath() {
  if (current_model_ == vital::constants::kPhase)
    return Paths::phaser1();
  if (current_model_ == vital::constants::kFormant)
    return Paths::leftArrow();
  if (current_style_ == vital::SynthFilter::kDualNotchBand || current_style_ == vital::SynthFilter::kBandPeakNotch)
    return Paths::bandPass();
  if (current_model_ == vital::constants::kComb && current_style_)
    return Paths::narrowBand();

  return Paths::lowPass();
}

Path FilterSection::getRightMorphPath() {
  if (current_model_ == vital::constants::kPhase)
    return Paths::phaser3();
  if (current_model_ == vital::constants::kFormant)
    return Paths::rightArrow();
  if (current_style_ == vital::SynthFilter::kDualNotchBand || current_style_ == vital::SynthFilter::kBandPeakNotch)
    return Paths::notch();
  if (current_model_ == vital::constants::kComb && current_style_)
    return Paths::wideBand();
  if (current_model_ == vital::constants::kDiode)
    return Paths::bandPass();
  return Paths::highPass();
}
