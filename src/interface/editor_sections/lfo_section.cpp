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

#include "lfo_section.h"

#include "load_save.h"
#include "skin.h"
#include "paths.h"
#include "lfo_editor.h"
#include "modulation_button.h"
#include "synth_strings.h"
#include "tempo_selector.h"
#include "text_selector.h"
#include "text_look_and_feel.h"

LfoSection::LfoSection(String name, std::string value_prepend, LineGenerator* lfo_source,
                       const vital::output_map& mono_modulations, const vital::output_map& poly_modulations) :
                       SynthSection(std::move(name)), current_preset_(0) {
  static constexpr double kTempoDragSensitivity = 0.5;
  static constexpr int kDefaultGridSizeX = 8;
  static constexpr int kDefaultGridSizeY = 1;

  smooth_mode_control_name_ = value_prepend + "_smooth_mode";

  frequency_ = std::make_unique<SynthSlider>(value_prepend + "_frequency");
  addSlider(frequency_.get());
  frequency_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  frequency_->setLookAndFeel(TextLookAndFeel::instance());

  tempo_ = std::make_unique<SynthSlider>(value_prepend + "_tempo");
  addSlider(tempo_.get());
  tempo_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tempo_->setLookAndFeel(TextLookAndFeel::instance());
  tempo_->setSensitivity(kTempoDragSensitivity);
  tempo_->setTextEntrySizePercent(1.0f, 0.7f);

  keytrack_transpose_ = std::make_unique<SynthSlider>(value_prepend + "_keytrack_transpose");
  addSlider(keytrack_transpose_.get());
  keytrack_transpose_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  keytrack_transpose_->setLookAndFeel(TextLookAndFeel::instance());
  keytrack_transpose_->setSensitivity(kTransposeMouseSensitivity);
  keytrack_transpose_->setBipolar();
  keytrack_transpose_->setShiftIndexAmount(vital::kNotesPerOctave);

  keytrack_tune_ = std::make_unique<SynthSlider>(value_prepend + "_keytrack_tune");
  addSlider(keytrack_tune_.get());
  keytrack_tune_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  keytrack_tune_->setLookAndFeel(TextLookAndFeel::instance());
  keytrack_tune_->setBipolar();
  keytrack_tune_->setMaxDisplayCharacters(3);
  keytrack_tune_->setMaxDecimalPlaces(0);

  sync_ = std::make_unique<TempoSelector>(value_prepend + "_sync");
  addSlider(sync_.get());
  sync_->setSliderStyle(Slider::LinearBar);
  sync_->setTempoSlider(tempo_.get());
  sync_->setKeytrackTransposeSlider(keytrack_transpose_.get());
  sync_->setKeytrackTuneSlider(keytrack_tune_.get());
  sync_->setFreeSlider(frequency_.get());

  sync_type_ = std::make_unique<TextSelector>(value_prepend + "_sync_type");
  addSlider(sync_type_.get());
  sync_type_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  sync_type_->setLookAndFeel(TextLookAndFeel::instance());
  sync_type_->setLongStringLookup(strings::kSyncNames);

  paint_pattern_ = std::make_unique<PaintPatternSelector>("paint_pattern");
  addSlider(paint_pattern_.get());
  paint_pattern_->setRange(0.0f, kNumPaintPatterns - 1.0f, 1.0f);
  paint_pattern_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  paint_pattern_->setStringLookup(strings::kPaintPatternNames);
  paint_pattern_->setLookAndFeel(TextLookAndFeel::instance());
  paint_pattern_->setLongStringLookup(strings::kPaintPatternNames);
  paint_pattern_->setTextHeightPercentage(0.45f);
  paint_pattern_->setActive(false);
  paint_pattern_->overrideValue(Skin::kTextComponentOffset, 0.0f);

  transpose_tune_divider_ = std::make_unique<OpenGlQuad>(Shaders::kColorFragment);
  addOpenGlComponent(transpose_tune_divider_.get());
  transpose_tune_divider_->setInterceptsMouseClicks(false, false);

  phase_ = std::make_unique<SynthSlider>(value_prepend + "_phase");
  addSlider(phase_.get());
  phase_->setSliderStyle(Slider::LinearBar);
  phase_->setModulationPlacement(BubbleComponent::above);

  fade_ = std::make_unique<SynthSlider>(value_prepend + "_fade_time");
  addSlider(fade_.get());
  fade_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  fade_->setPopupPlacement(BubbleComponent::below);
  fade_->setVisible(false);

  smooth_ = std::make_unique<SynthSlider>(value_prepend + "_smooth_time");
  addSlider(smooth_.get());
  smooth_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  smooth_->setPopupPlacement(BubbleComponent::below);

  smooth_mode_text_ = std::make_unique<PlainTextComponent>("Smooth Mode Text", "---");
  addOpenGlComponent(smooth_mode_text_.get());
  smooth_mode_text_->setText(strings::kSmoothModeNames[0]);

  smooth_mode_type_selector_ = std::make_unique<ShapeButton>("Smooth Mode", Colours::black,
                                                             Colours::black, Colours::black);
  addAndMakeVisible(smooth_mode_type_selector_.get());
  smooth_mode_type_selector_->addListener(this);
  smooth_mode_type_selector_->setTriggeredOnMouseDown(true);

  delay_ = std::make_unique<SynthSlider>(value_prepend + "_delay_time");
  addSlider(delay_.get());
  delay_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  delay_->setPopupPlacement(BubbleComponent::below);

  stereo_ = std::make_unique<SynthSlider>(value_prepend + "_stereo");
  addSlider(stereo_.get());
  stereo_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  stereo_->setPopupPlacement(BubbleComponent::below);
  stereo_->setBipolar(true);
  stereo_->snapToValue(true, 0.0);

  grid_size_x_ = std::make_unique<SynthSlider>("grid_size_x");
  grid_size_x_->setRange(1.0, LfoEditor::kMaxGridSizeX, 1.0);
  grid_size_x_->setValue(kDefaultGridSizeX);
  grid_size_x_->setLookAndFeel(TextLookAndFeel::instance());
  grid_size_x_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  addSlider(grid_size_x_.get());
  grid_size_x_->setDoubleClickReturnValue(true, kDefaultGridSizeX);
  grid_size_x_->setMaxDecimalPlaces(0);
  grid_size_x_->setSensitivity(0.2f);
  grid_size_x_->overrideValue(Skin::kTextComponentOffset, 0.0f);
  grid_size_x_->setTextHeightPercentage(0.6f);
  grid_size_x_->setPopupPrefix("X Grid");

  grid_size_y_ = std::make_unique<SynthSlider>("grid_size_y");
  grid_size_y_->setRange(1.0, LfoEditor::kMaxGridSizeY, 1.0);
  grid_size_y_->setValue(kDefaultGridSizeY);
  grid_size_y_->setLookAndFeel(TextLookAndFeel::instance());
  grid_size_y_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  addSlider(grid_size_y_.get());
  grid_size_y_->setDoubleClickReturnValue(true, kDefaultGridSizeY);
  grid_size_y_->setMaxDecimalPlaces(0);
  grid_size_y_->setSensitivity(0.2f);
  grid_size_y_->overrideValue(Skin::kTextComponentOffset, 0.0f);
  grid_size_y_->setTextHeightPercentage(0.6f);
  grid_size_y_->setPopupPrefix("Y Grid");

  paint_ = std::make_unique<OpenGlShapeButton>("paint");
  paint_->useOnColors(true);
  paint_->setClickingTogglesState(true);
  addAndMakeVisible(paint_.get());
  addOpenGlComponent(paint_->getGlComponent());
  paint_->addListener(this);
  paint_->setShape(Paths::paintBrush());

  lfo_smooth_ = std::make_unique<OpenGlShapeButton>("smooth");
  lfo_smooth_->useOnColors(true);
  lfo_smooth_->setClickingTogglesState(true);
  addAndMakeVisible(lfo_smooth_.get());
  addOpenGlComponent(lfo_smooth_->getGlComponent());
  lfo_smooth_->addListener(this);
  lfo_smooth_->setShape(Paths::halfSinCurve());

  editor_ = std::make_unique<LfoEditor>(lfo_source, value_prepend, mono_modulations, poly_modulations);
  editor_->addListener(this);
  editor_->setGridSizeX(kDefaultGridSizeX);
  editor_->setGridSizeY(kDefaultGridSizeY);
  addOpenGlComponent(editor_.get());
  addOpenGlComponent(editor_->getTextEditorComponent());
  lfo_smooth_->setToggleState(editor_->getModel()->smooth(), dontSendNotification);

  paint_pattern_->setValue(kDown);

  preset_selector_ = std::make_unique<PresetSelector>();
  addSubSection(preset_selector_.get());
  preset_selector_->addListener(this);
  setPresetSelector(preset_selector_.get());
  preset_selector_->setText(editor_->getModel()->getName());

  setSkinOverride(Skin::kLfo);
}

LfoSection::~LfoSection() = default;

void LfoSection::paintBackground(Graphics& g) {
  if (getWidth() <= 0)
    return;

  float tempo_width = sync_->getRight() - tempo_->getX();

  drawTextComponentBackground(g, sync_type_->getBounds(), true);
  Rectangle<int> frequency_bounds(tempo_->getX(), tempo_->getY(), tempo_width, tempo_->getHeight());
  drawTextComponentBackground(g, frequency_bounds, true);
  drawTempoDivider(g, sync_.get());

  setLabelFont(g);
  drawLabel(g, TRANS("MODE"), sync_type_->getBounds(), true);
  drawLabel(g, TRANS("FREQUENCY"), frequency_bounds, true);

  drawLabelForComponent(g, "DELAY", delay_.get());
  drawLabelForComponent(g, "STEREO", stereo_.get());
  drawLabelForComponent(g, TRANS(""), fade_.get());
  int title_width = getTitleWidth();

  int widget_margin = getWidgetMargin();
  int rounding = getWidgetRounding();
  int grid_label_x = grid_size_x_->getX();
  int grid_size_width = grid_size_y_->getRight() - grid_label_x;
  g.setColour(findColour(Skin::kPopupSelectorBackground, true));
  int background_height = title_width - 2 * widget_margin;
  g.fillRoundedRectangle(grid_label_x, widget_margin, grid_size_width, background_height, rounding);
  g.fillRoundedRectangle(widget_margin, widget_margin, grid_label_x - 2 * widget_margin, background_height, rounding);

  Colour body_text = findColour(Skin::kBodyText, true);
  g.setColour(body_text);
  g.drawText("-", grid_label_x, widget_margin, grid_size_width, title_width - 2 * widget_margin,
             Justification::centred, false);

  transpose_tune_divider_->setColor(findColour(Skin::kLightenScreen, true));
  smooth_mode_text_->setColor(body_text);
  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void LfoSection::resized() {
  int title_width = getTitleWidth();
  int knob_section_height = getKnobSectionHeight();
  int slider_width = getSliderWidth();

  int slider_overlap = getSliderOverlap();

  int widget_margin = findValue(Skin::kWidgetMargin);
  int wave_height = getHeight() - slider_width - widget_margin -
                    title_width - knob_section_height + 2 * slider_overlap;
  int wave_width = getWidth() - 2 * widget_margin;
  editor_->setBounds(widget_margin, title_width, wave_width, wave_height);
  phase_->setBounds(0, editor_->getBottom() - slider_overlap + widget_margin, getWidth(), slider_width);

  int knobs_width = 4 * (int)findValue(Skin::kModulationButtonWidth) + widget_margin + findValue(Skin::kPadding);
  int style_width = getWidth() - knobs_width;

  int knob_y = getHeight() - knob_section_height;
  int text_component_width = style_width / 2 - widget_margin;
  sync_type_->setBounds(widget_margin, knob_y + widget_margin,
                        text_component_width, knob_section_height - 2 * widget_margin);
  int tempo_x = sync_type_->getRight() + widget_margin;
  placeTempoControls(tempo_x, knob_y + widget_margin, style_width - tempo_x, knob_section_height - 2 * widget_margin,
                     frequency_.get(), sync_.get());
  tempo_->setBounds(frequency_->getBounds());
  Rectangle<int> divider_bounds = frequency_->getModulationArea() + frequency_->getBounds().getTopLeft();
  divider_bounds = divider_bounds.reduced(divider_bounds.getHeight() / 4);
  divider_bounds.setX(divider_bounds.getCentreX());
  divider_bounds.setWidth(1);
  transpose_tune_divider_->setBounds(divider_bounds);
  tempo_->setModulationArea(frequency_->getModulationArea());

  Rectangle<int> frequency_bounds = frequency_->getBounds();
  keytrack_transpose_->setBounds(frequency_bounds.withWidth(frequency_bounds.getWidth() / 2));
  keytrack_tune_->setBounds(frequency_bounds.withLeft(keytrack_transpose_->getRight()));
  keytrack_transpose_->setModulationArea(frequency_->getModulationArea().withWidth(keytrack_transpose_->getWidth()));
  keytrack_tune_->setModulationArea(frequency_->getModulationArea().withWidth(keytrack_tune_->getWidth()));

  placeKnobsInArea(Rectangle<int>(style_width, knob_y, knobs_width, knob_section_height),
                   { fade_.get(), delay_.get(), stereo_.get() });
  smooth_->setBounds(fade_->getBounds());

  Rectangle<int> smooth_label_bounds = getLabelBackgroundBounds(fade_->getBounds());
  smooth_mode_text_->setBounds(smooth_label_bounds);
  smooth_mode_text_->setTextSize(findValue(Skin::kLabelHeight));
  smooth_mode_type_selector_->setBounds(smooth_label_bounds);

  Rectangle<int> browser_bounds = getPresetBrowserBounds();
  int top_height = title_width - 2 * widget_margin;

  lfo_smooth_->setBounds(browser_bounds.getX() - title_width - widget_margin, widget_margin, title_width, top_height);
  grid_size_y_->setBounds(lfo_smooth_->getX() - title_width - widget_margin, widget_margin, title_width, top_height);
  grid_size_x_->setBounds(grid_size_y_->getX() - title_width - widget_margin, widget_margin, title_width, top_height);

  paint_->setBounds(widget_margin, widget_margin, top_height, top_height);
  int pattern_width = grid_size_x_->getX() - paint_->getRight() - widget_margin;
  paint_pattern_->setPadding(getWidgetMargin());
  paint_pattern_->setBounds(paint_->getRight(), widget_margin, pattern_width, top_height);

  SynthSection::resized();
  editor_->setSizeRatio(getSizeRatio());
}

void LfoSection::reset() {
  SynthSection::reset();
  preset_selector_->setText(editor_->getModel()->getName());
  editor_->resetPositions();
}

void LfoSection::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  lfo_smooth_->setToggleState(editor_->getSmooth(), dontSendNotification);
  transpose_tune_divider_->setVisible(sync_->isKeytrack());

  int smooth_mode = controls[smooth_mode_control_name_]->value();
  smooth_mode_text_->setText(strings::kSmoothModeNames[smooth_mode]);
  smooth_->setVisible(smooth_mode);
  fade_->setVisible(smooth_mode == 0);
}

void LfoSection::sliderValueChanged(Slider* changed_slider) {
  if (changed_slider == grid_size_x_.get())
    editor_->setGridSizeX(grid_size_x_->getValue());
  else if (changed_slider == grid_size_y_.get())
    editor_->setGridSizeY(grid_size_y_->getValue());
  else if (changed_slider == paint_pattern_.get())
    editor_->setPaintPattern(getPaintPattern(paint_pattern_->getValue()));
  else
    SynthSection::sliderValueChanged(changed_slider);

  transpose_tune_divider_->setVisible(sync_->isKeytrack());
}

void LfoSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == paint_.get()) {
    editor_->setPaint(paint_->getToggleState());
    paint_pattern_->setActive(paint_->getToggleState());
  }
  else if (clicked_button == lfo_smooth_.get())
    editor_->setSmooth(lfo_smooth_->getToggleState());
  else if (clicked_button == smooth_mode_type_selector_.get()) {
    PopupItems options;

    for (int i = 0; i < 2; ++i)
      options.addItem(i, strings::kSmoothModeNames[i]);

    showPopupSelector(this, Point<int>(clicked_button->getX(), clicked_button->getBottom()), options,
                      [=](int selection) { setSmoothModeSelected(selection); });
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void LfoSection::lineEditorScrolled(const MouseEvent& e, const MouseWheelDetails& wheel) {
  if (paint_->getToggleState())
    paint_pattern_->mouseWheelMove(e, wheel);
  else
    grid_size_x_->mouseWheelMove(e, wheel);
}

void LfoSection::togglePaintMode(bool enabled, bool temporary_switch) {
  paint_->setToggleState(enabled != temporary_switch, dontSendNotification);
  paint_pattern_->setActive(enabled != temporary_switch);
}

void LfoSection::importLfo() {
  FileChooser import_box("Import LFO", LoadSave::getUserLfoDirectory(), String("*.") + vital::kLfoExtension);
  if (!import_box.browseForFileToOpen())
    return;

  File choice = import_box.getResult();
  loadFile(choice.withFileExtension(vital::kLfoExtension));
}

void LfoSection::exportLfo() {
  FileChooser export_box("Export LFO", LoadSave::getUserLfoDirectory(), String("*.") + vital::kLfoExtension);
  if (!export_box.browseForFileToSave(true))
    return;

  File choice = export_box.getResult();
  choice = choice.withFileExtension(vital::kLfoExtension);
  if (!choice.exists())
    choice.create();
  choice.replaceWithText(editor_->getModel()->stateToJson().dump());

  String name = choice.getFileNameWithoutExtension();
  editor_->getModel()->setName(name.toStdString());
  preset_selector_->setText(name);
}

void LfoSection::fileLoaded() {
  lfo_smooth_->setToggleState(editor_->getModel()->smooth(), dontSendNotification);
}

void LfoSection::prevClicked() {
  File lfo_file = LoadSave::getShiftedFile(LoadSave::kLfoFolderName, String("*.") + vital::kLfoExtension,
                                           "", getCurrentFile(), -1);
  if (lfo_file.exists())
    loadFile(lfo_file);
  
  updatePopupBrowser(this);
}

void LfoSection::nextClicked() {
  File lfo_file = LoadSave::getShiftedFile(LoadSave::kLfoFolderName, String("*.") + vital::kLfoExtension,
                                           "", getCurrentFile(), 1);
  if (lfo_file.exists())
    loadFile(lfo_file);
  
  updatePopupBrowser(this);
}

void LfoSection::textMouseDown(const MouseEvent& e) {
  static constexpr int kBrowserWidth = 500;
  static constexpr int kBrowserHeight = 250;

  int browser_width = kBrowserWidth * size_ratio_;
  int browser_height = kBrowserHeight * size_ratio_;
  Rectangle<int> bounds(preset_selector_->getRight() - browser_width, -browser_height,
                        browser_width, browser_height);
  bounds = getLocalArea(this, bounds);
  showPopupBrowser(this, bounds, LoadSave::getLfoDirectories(), String("*.") + vital::kLfoExtension,
                   LoadSave::kLfoFolderName, "");
}

void LfoSection::setSmoothModeSelected(int result) {
  smooth_mode_text_->setText(strings::kSmoothModeNames[result]);
  smooth_->setVisible(result);
  fade_->setVisible(result == 0);

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->getSynth()->valueChangedInternal(smooth_mode_control_name_, result);
}

void LfoSection::loadFile(const File& file) {
  if (!file.exists())
    return;

  current_file_ = file;
  try {
    json parsed_file = json::parse(file.loadFileAsString().toStdString(), nullptr, false);
    editor_->getModel()->jsonToState(parsed_file);
  }
  catch (const json::exception& e) {
    return;
  }

  String name = file.getFileNameWithoutExtension();
  editor_->getModel()->setName(name.toStdString());
  editor_->getModel()->setLastBrowsedFile(file.getFullPathName().toStdString());
  preset_selector_->setText(name);

  editor_->resetPositions();
  lfo_smooth_->setToggleState(editor_->getModel()->smooth(), dontSendNotification);
}
