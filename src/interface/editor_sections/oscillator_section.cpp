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

#include "oscillator_section.h"

#include "bar_renderer.h"
#include "curve_look_and_feel.h"
#include "skin.h"
#include "paths.h"
#include "fonts.h"
#include "full_interface.h"
#include "load_save.h"
#include "preset_selector.h"
#include "producers_module.h"
#include "synth_strings.h"
#include "synth_button.h"
#include "synth_oscillator.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "text_look_and_feel.h"
#include "text_selector.h"
#include "wavetable_3d.h"

namespace {
  const std::string kSpectralMorphTypes[vital::SynthOscillator::kNumSpectralMorphTypes] = {
    "---",
    "VOCODE",
    "FORM SCALE",
    "HARM STRETCH",
    "INHARMONIC",
    "SMEAR",
    "RAND AMP",
    "LOW PASS",
    "HIGH PASS",
    "PHASE DISP",
    "SHEPARD TONE",
    "TIME SKEW",
  };

  const std::string kDistortionTypes[vital::SynthOscillator::kNumDistortionTypes] = {
    "---",
    "SYNC",
    "FORMANT",
    "QUANTIZE",
    "BEND",
    "SQUEEZE",
    "PULSE",
    "FM <- OSC",
    "FM <- OSC",
    "FM <- SAMPLE",
    "RM <- OSC",
    "RM <- OSC",
    "RM <- SAMPLE"
  };

  const std::string kLanguageNames[] = {
    "Arabic",
    "Czech",
    "Danish",
    "Dutch",
    "English (Aus)",
    "English (UK)",
    "English (US)",
    "Filipino",
    "Finnish",
    "French (Can)",
    "French (Fr)",
    "German",
    "Greek",
    "Hindi",
    "Hungarian",
    "Indonesian",
    "Italian",
    "Japanese",
    "Korean",
    "Mandarin Chinese",
    "Norwegian",
    "Polish",
    "Portuguese (Br)",
    "Portuguese (Po)",
    "Russian",
    "Slovak",
    "Spanish",
    "Swedish",
    "Turkish",
    "Ukrainian",
    "Vietnamese"
  };

  const std::string kLanguageCodes[] = {
    "ar-XA",
    "cs-CZ",
    "da-DK",
    "nl-NL",
    "en-AU",
    "en-GB",
    "en-US",
    "fil-PH",
    "fi-FI",
    "fr-CA",
    "fr-FR",
    "de-DE",
    "el-GR",
    "hi-IN",
    "hu-HU",
    "id-ID",
    "it-IT",
    "ja-JP",
    "ko-KR",
    "cmn-CN",
    "nb-NO",
    "pl-PL",
    "pt-BR",
    "pt-PT",
    "ru-RU",
    "sk-SK",
    "es-ES",
    "sv-SE",
    "tr-TR",
    "uk-UA",
    "vi-VN"
  };

  const std::string kUrlPrefix = "";
  const std::string kLanguageUrlQuery = "&language=";
  const std::string kTokenUrlQuery = "&idToken=";
  constexpr int kTtwtId = INT_MAX;
  constexpr int kAddCustomFolderId = INT_MAX - 1;
  constexpr int kMaxTTWTLength = 100;
  constexpr int kShowErrorMs = 2000;

  String getDistortionSuffix(int type, int index) {
    if (type == vital::SynthOscillator::kFmOscillatorA || type == vital::SynthOscillator::kRmOscillatorA)
      return " " + String(1 + vital::ProducersModule::getFirstModulationIndex(index));
    if (type == vital::SynthOscillator::kFmOscillatorB || type == vital::SynthOscillator::kRmOscillatorB)
      return " " + String(1 + vital::ProducersModule::getSecondModulationIndex(index));

    return "";
  }

  bool isSpectralMenuSeparator(int index) {
    return index == (vital::SynthOscillator::kNoSpectralMorph + 1);
  }

  bool isDistortionMenuSeparator(int index) {
    return index == (vital::SynthOscillator::kNone + 1) || index == vital::SynthOscillator::kFmOscillatorA;
  }

  String getDistortionMenuString(int type, int index) {
    return String(strings::kPhaseDistortionNames[type]) + getDistortionSuffix(type, index);
  }

  String getDistortionString(int type, int index) {
    return String(kDistortionTypes[type]) + getDistortionSuffix(type, index);
  }

  int getLanguageIndex(const std::string& language) {
    constexpr int kDefaultIndex = 4;

    for (int i = 0; i < sizeof(kLanguageNames) / sizeof(std::string); ++i) {
      if (kLanguageCodes[i] == language)
        return i;
    }

    return kDefaultIndex;
  }

  bool isBipolarDistortionType(int distortion_type) {
    return distortion_type == vital::SynthOscillator::kNone ||
           distortion_type == vital::SynthOscillator::kSqueeze ||
           distortion_type == vital::SynthOscillator::kSync ||
           distortion_type == vital::SynthOscillator::kFormant ||
           distortion_type == vital::SynthOscillator::kBend;
  }

  bool isBipolarSpectralMorphType(int morph_type) {
    return morph_type == vital::SynthOscillator::kNoSpectralMorph ||
           morph_type == vital::SynthOscillator::kVocode ||
           morph_type == vital::SynthOscillator::kFormScale ||
           morph_type == vital::SynthOscillator::kHarmonicScale ||
           morph_type == vital::SynthOscillator::kInharmonicScale ||
           morph_type == vital::SynthOscillator::kPhaseDisperse;
  }
} // namespace

class UnisonViewer : public BarRenderer {
  public:
    UnisonViewer(int index, const vital::output_map& mono_modulations, const vital::output_map& poly_modulations) :
        BarRenderer(vital::SynthOscillator::kMaxUnison) {
      std::string prefix = std::string("osc_") + std::to_string(index + 1);
      voices_ = { mono_modulations.at(prefix + "_unison_voices"), poly_modulations.at(prefix + "_unison_voices") };
      detune_ = { mono_modulations.at(prefix + "_unison_detune"), poly_modulations.at(prefix + "_unison_detune") };
      detune_power_ = { mono_modulations.at(prefix + "_detune_power"), poly_modulations.at(prefix + "_detune_power") };

      voices_slider_ = nullptr;
      detune_slider_ = nullptr;
      detune_power_slider_ = nullptr;
    }

    static inline vital::poly_float getOutputsTotal(std::pair<vital::Output*, vital::Output*> outputs,
                                                    vital::poly_float default_value, bool animate) {
      if (!outputs.first->owner->enabled() || !animate)
        return default_value;
      return outputs.first->trigger_value + outputs.second->trigger_value;
    }

    void setVoicesSlider(SynthSlider* slider) { voices_slider_ = slider; }
    void setDetuneSlider(SynthSlider* slider) { detune_slider_ = slider; }
    void setDetunePowerSlider(SynthSlider* slider) { detune_power_slider_ = slider; }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      static constexpr float kHeightRatio = 0.7f;
      static constexpr int kMaxUnison = vital::SynthOscillator::kMaxUnison;

      int voices = getOutputsTotal(voices_, voices_slider_->getValue(), animate)[0];
      voices = std::min(std::max(voices, 1), kMaxUnison);
      float detune = 0.1f * getOutputsTotal(detune_, detune_slider_->getValue(), animate)[0];
      float detune_power = getOutputsTotal(detune_power_, detune_power_slider_->getValue(), animate)[0];

      setColor(findColour(Skin::kWidgetPrimary1, true).withMultipliedAlpha(0.5f));
      setBarWidth(1.0f / getWidth());
      float percent_active = 1.0f - getHeight() / (2.0f * getWidth());

      for (int i = 0; i < kMaxUnison; ++i) {
        setBottom(i, -kHeightRatio);
        setY(i, kHeightRatio);
      }
      
      float offset = -1.0f / getWidth();
      if (voices == 1)
        setX(0, offset);
      else {
        for (int i = 0; i < voices; ++i) {
          float t = 2.0f * i / (voices - 1.0f) - 1.0f;
          float center_offset = fabsf(t);
          float power_scale = vital::futils::powerScale(center_offset, detune_power);
          if (t < 0.0f)
            power_scale = -power_scale;
          setX(i, power_scale * percent_active * detune + offset);
        }
      }
      for (int i = voices; i < kMaxUnison; ++i)
        setX(i, -2.0f);

      BarRenderer::render(open_gl, animate);
    }

  private:
    std::pair<vital::Output*, vital::Output*> voices_;
    std::pair<vital::Output*, vital::Output*> detune_;
    std::pair<vital::Output*, vital::Output*> detune_power_;

    SynthSlider* voices_slider_;
    SynthSlider* detune_slider_;
    SynthSlider* detune_power_slider_;
};

class InvisibleSlider : public SynthSlider {
  public:
    InvisibleSlider(String name) : SynthSlider(std::move(name)) { }

    void paint(Graphics& g) override { }
    void drawShadow(Graphics& g) override { }
};

OscillatorSection::OscillatorSection(int index,
                                     const vital::output_map& mono_modulations,
                                     const vital::output_map& poly_modulations) :
    SynthSection(String("OSC ") + String(index + 1)), index_(index),
    show_ttwt_error_(false), ttwt_overlay_(Shaders::kRoundedRectangleFragment) {
  std::string number = std::to_string(index + 1);
  wavetable_ = std::make_unique<Wavetable3d>(index, mono_modulations, poly_modulations);
  addOpenGlComponent(wavetable_.get());
  wavetable_->addListener(this);

  transpose_quantize_button_ = std::make_unique<TransposeQuantizeButton>();
  addOpenGlComponent(transpose_quantize_button_.get());
  transpose_quantize_button_->addQuantizeListener(this);

  transpose_ = std::make_unique<SynthSlider>("osc_" + number + "_transpose");
  addSlider(transpose_.get());
  transpose_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  transpose_->setLookAndFeel(TextLookAndFeel::instance());
  transpose_->setSensitivity(kTransposeMouseSensitivity);
  transpose_->setTextEntrySizePercent(1.0f, 0.7f);
  transpose_->setShiftIndexAmount(vital::kNotesPerOctave);
  transpose_->overrideValue(Skin::kTextComponentOffset, 0.0f);
  transpose_->setModulationBarRight(false);

  tune_ = std::make_unique<SynthSlider>("osc_" + number + "_tune");
  addSlider(tune_.get());
  tune_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tune_->setLookAndFeel(TextLookAndFeel::instance());
  tune_->setBipolar();
  tune_->setMaxDisplayCharacters(3);
  tune_->setMaxDecimalPlaces(0);
  tune_->setTextEntrySizePercent(1.0f, 0.7f);
  tune_->overrideValue(Skin::kTextComponentOffset, 0.0f);

  unison_detune_ = std::make_unique<SynthSlider>("osc_" + number + "_unison_detune");
  addSlider(unison_detune_.get());
  unison_detune_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  unison_detune_->setLookAndFeel(TextLookAndFeel::instance());
  unison_detune_->setMaxDisplayCharacters(3);
  unison_detune_->setMaxDecimalPlaces(0);
  unison_detune_->setTextEntrySizePercent(1.0f, 0.7f);
  unison_detune_->overrideValue(Skin::kTextComponentOffset, 0.0f);

  unison_detune_power_ = std::make_unique<InvisibleSlider>("osc_" + number + "_detune_power");
  addSlider(unison_detune_power_.get());
  unison_detune_power_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  unison_detune_power_->setLookAndFeel(CurveLookAndFeel::instance());
  unison_detune_power_->setPopupPrefix("Unison Detune Power: ");
  unison_detune_power_->setTextEntrySizePercent(1.0f, 0.7f);

  unison_voices_ = std::make_unique<SynthSlider>("osc_" + number + "_unison_voices");
  addSlider(unison_voices_.get());
  unison_voices_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  unison_voices_->setLookAndFeel(TextLookAndFeel::instance());
  unison_voices_->setTextEntrySizePercent(1.0f, 0.7f);
  unison_voices_->overrideValue(Skin::kTextComponentOffset, 0.0f);
  unison_voices_->setModulationBarRight(false);

  phase_ = std::make_unique<SynthSlider>("osc_" + number + "_phase");
  addSlider(phase_.get());
  phase_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  phase_->setLookAndFeel(TextLookAndFeel::instance());
  phase_->setTextEntrySizePercent(1.0f, 0.7f);
  phase_->overrideValue(Skin::kTextComponentOffset, 0.0f);
  phase_->setMaxDisplayCharacters(3);
  phase_->setMaxDecimalPlaces(0);

  random_phase_ = std::make_unique<SynthSlider>("osc_" + number + "_random_phase");
  addSlider(random_phase_.get());
  random_phase_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  random_phase_->setLookAndFeel(TextLookAndFeel::instance());
  random_phase_->setTextEntrySizePercent(1.0f, 0.7f);
  random_phase_->overrideValue(Skin::kTextComponentOffset, 0.0f);
  random_phase_->setMaxDisplayCharacters(3);
  random_phase_->setMaxDecimalPlaces(0);

  distortion_phase_ = std::make_unique<SynthSlider>("osc_" + number + "_distortion_phase");
  addSlider(distortion_phase_.get());
  distortion_phase_->setSliderStyle(Slider::LinearBar);
  distortion_phase_->setVisible(false);
  distortion_phase_->setBipolar(true);
  distortion_phase_->setModulationPlacement(BubbleComponent::above);

  level_ = std::make_unique<SynthSlider>("osc_" + number + "_level");
  addSlider(level_.get());
  level_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  pan_ = std::make_unique<SynthSlider>("osc_" + number + "_pan");
  addSlider(pan_.get());
  pan_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  pan_->setBipolar();

  distortion_control_name_ = "osc_" + number + "_distortion_type";
  current_distortion_type_ = 0;
  distortion_type_text_ = std::make_unique<PlainTextComponent>("Distortion Text", "---");
  addOpenGlComponent(distortion_type_text_.get());

  spectral_morph_control_name_ = "osc_" + number + "_spectral_morph_type";
  current_spectral_morph_type_ = 0;
  spectral_morph_type_text_ = std::make_unique<PlainTextComponent>("Frequency Morph Text", "---");
  addOpenGlComponent(spectral_morph_type_text_.get());

  quantize_control_name_ = "osc_" + number + "_transpose_quantize";

  distortion_amount_ = std::make_unique<SynthSlider>("osc_" + number + "_distortion_amount");
  addSlider(distortion_amount_.get());
  distortion_amount_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  distortion_amount_->setKnobSizeScale(1.5f);
  distortion_amount_->setPopupPrefix("Wave Morph: ");

  spectral_morph_amount_ = std::make_unique<SynthSlider>("osc_" + number + "_spectral_morph_amount");
  addSlider(spectral_morph_amount_.get());
  spectral_morph_amount_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  spectral_morph_amount_->setKnobSizeScale(1.5f);
  spectral_morph_amount_->setPopupPrefix("Spectral Morph: ");

  spectral_morph_type_selector_ = std::make_unique<ShapeButton>("Spectral Morph", Colour(0xff666666),
                                                                 Colour(0xffaaaaaa), Colour(0xff888888));
  addAndMakeVisible(spectral_morph_type_selector_.get());
  spectral_morph_type_selector_->addListener(this);
  spectral_morph_type_selector_->setTriggeredOnMouseDown(true);
  spectral_morph_type_selector_->setShape(Path(), true, true, true);

  distortion_type_selector_ = std::make_unique<ShapeButton>("Distortion", Colour(0xff666666),
                                                            Colour(0xffaaaaaa), Colour(0xff888888));
  addAndMakeVisible(distortion_type_selector_.get());
  distortion_type_selector_->addListener(this);
  distortion_type_selector_->setTriggeredOnMouseDown(true);
  distortion_type_selector_->setShape(Path(), true, true, true);

  wave_frame_ = std::make_unique<SynthSlider>("osc_" + number + "_wave_frame");
  addSlider(wave_frame_.get());
  wave_frame_->setSliderStyle(Slider::LinearBarVertical);
  wave_frame_->setPopupPlacement(BubbleComponent::right);
  wave_frame_->setMouseWheelMovement(8.0);
  wave_frame_->setModulationPlacement(BubbleComponent::left);
  wave_frame_->setExtraModulationTarget(wavetable_.get());
  wave_frame_->setPopupPrefix("Frame: ");

  destination_selector_ = std::make_unique<ShapeButton>("Destination", Colour(0xff666666),
                                                        Colour(0xffaaaaaa), Colour(0xff888888));

  destination_control_name_ = "osc_" + number + "_destination";
  current_destination_ = 0;
  destination_text_ = std::make_unique<PlainTextComponent>("Destination Text", "---");
  addOpenGlComponent(destination_text_.get());
  
  addAndMakeVisible(destination_selector_.get());
  destination_selector_->addListener(this);
  destination_selector_->setTriggeredOnMouseDown(true);
  destination_selector_->setShape(Path(), true, true, true);

  prev_destination_ = std::make_unique<OpenGlShapeButton>("Prev Destination");
  addAndMakeVisible(prev_destination_.get());
  addOpenGlComponent(prev_destination_->getGlComponent());
  prev_destination_->addListener(this);
  prev_destination_->setShape(Paths::prev());

  next_destination_ = std::make_unique<OpenGlShapeButton>("Next Destination");
  addAndMakeVisible(next_destination_.get());
  addOpenGlComponent(next_destination_->getGlComponent());
  next_destination_->addListener(this);
  next_destination_->setShape(Paths::next());

  preset_selector_ = std::make_unique<PresetSelector>();
  addSubSection(preset_selector_.get());
  preset_selector_->addListener(this);
  setPresetSelector(preset_selector_.get(), true);

  edit_button_ = std::make_unique<OpenGlShapeButton>("edit");
  addAndMakeVisible(edit_button_.get());
  addOpenGlComponent(edit_button_->getGlComponent());
  edit_button_->addListener(this);
  edit_button_->setShape(Paths::pencil());

  dimension_button_ = std::make_unique<SynthButton>("osc_" + number + "_dimension");
  addButton(dimension_button_.get());
  dimension_button_->setNoBackground();
  dimension_button_->setShowOnColors(false);
  dimension_button_->setLookAndFeel(TextLookAndFeel::instance());

  dimension_value_ = std::make_unique<SynthSlider>("osc_" + number + "_view_2d");
  addSlider(dimension_value_.get());
  dimension_value_->setVisible(false);

  ttwt_overlay_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
  addOpenGlComponent(&ttwt_overlay_);
  ttwt_overlay_.setVisible(false);

#if !defined(NO_TEXT_ENTRY)
  ttwt_ = std::make_unique<OpenGlTextEditor>("ttwt");
  ttwt_->addListener(this);
  ttwt_->setFont(Fonts::instance()->proportional_light().withPointHeight(16.0f));
  ttwt_->setMultiLine(false, false);
  ttwt_->setJustification(Justification::centred);
  addChildComponent(ttwt_.get());
  addOpenGlComponent(ttwt_->getImageComponent());
#endif

  showing_language_menu_ = false;
  ttwt_language_ = getLanguageIndex(LoadSave::getPreferredTTWTLanguage());
  ttwt_settings_ = std::make_unique<SynthButton>("Menu");
  ttwt_settings_->setNoBackground();
  addChildComponent(ttwt_settings_.get());
  addOpenGlComponent(ttwt_settings_->getGlComponent());
  ttwt_settings_->addListener(this);
  ttwt_settings_->setTriggeredOnMouseDown(true);
  ttwt_settings_->setText(kLanguageCodes[ttwt_language_]);

  std::string ttwt_error = "Error rendering speech. Check internet connection";
  ttwt_error_text_ = std::make_unique<PlainTextComponent>("ttwt error", ttwt_error);
  addOpenGlComponent(ttwt_error_text_.get());
  ttwt_error_text_->setVisible(false);

  oscillator_on_ = std::make_unique<SynthButton>("osc_" + number + "_on");
  addButton(oscillator_on_.get());
  setActivator(oscillator_on_.get());

  wavetable_->setFrameSlider(wave_frame_.get());
  wavetable_->setSpectralMorphSlider(spectral_morph_amount_.get());
  wavetable_->setDistortionSlider(distortion_amount_.get());
  wavetable_->setDistortionPhaseSlider(distortion_phase_.get());

  setSkinOverride(Skin::kOscillator);

  unison_viewer_ = std::make_unique<UnisonViewer>(index, mono_modulations, poly_modulations);
  addOpenGlComponent(unison_viewer_.get());
  unison_viewer_->setVoicesSlider(unison_voices_.get());
  unison_viewer_->setDetuneSlider(unison_detune_.get());
  unison_viewer_->setDetunePowerSlider(unison_detune_power_.get());
  unison_viewer_->setInterceptsMouseClicks(false, false);

  prev_spectral_ = std::make_unique<OpenGlShapeButton>("Prev Spectral");
  addAndMakeVisible(prev_spectral_.get());
  addOpenGlComponent(prev_spectral_->getGlComponent());
  prev_spectral_->addListener(this);
  prev_spectral_->setShape(Paths::prev());

  next_spectral_ = std::make_unique<OpenGlShapeButton>("Next Spectral");
  addAndMakeVisible(next_spectral_.get());
  addOpenGlComponent(next_spectral_->getGlComponent());
  next_spectral_->addListener(this);
  next_spectral_->setShape(Paths::next());

  prev_distortion_ = std::make_unique<OpenGlShapeButton>("Prev Distortion");
  addAndMakeVisible(prev_distortion_.get());
  addOpenGlComponent(prev_distortion_->getGlComponent());
  prev_distortion_->addListener(this);
  prev_distortion_->setShape(Paths::prev());

  next_distortion_ = std::make_unique<OpenGlShapeButton>("Next Distortion");
  addAndMakeVisible(next_distortion_.get());
  addOpenGlComponent(next_distortion_->getGlComponent());
  next_distortion_->addListener(this);
  next_distortion_->setShape(Paths::next());
}

OscillatorSection::~OscillatorSection() = default;

void OscillatorSection::setSkinValues(const Skin& skin, bool top_level) {
  SynthSection::setSkinValues(skin, top_level);
  float horizontal_angle = skin.getValue(Skin::kWavetableHorizontalAngle);
  float vertical_angle = skin.getValue(Skin::kWavetableVerticalAngle);
  float draw_width = skin.getValue(Skin::kWavetableDrawWidth);
  float wave_height = skin.getValue(Skin::kWavetableWaveHeight);
  float y_offset = skin.getValue(Skin::kWavetableYOffset);
  wavetable_->setViewSettings(horizontal_angle, vertical_angle, draw_width, wave_height, y_offset);
}

void OscillatorSection::paintBackground(Graphics& g) {
  if (getWidth() == 0)
    return;

  if (ttwt_) {
    ttwt_->setColour(CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
    ttwt_->setColour(TextEditor::textColourId, findColour(Skin::kBodyText, true));
    ttwt_->setColour(TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
    ttwt_->setColour(TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
    Colour empty_color = findColour(Skin::kBodyText, true);
    empty_color = empty_color.withAlpha(0.5f * empty_color.getFloatAlpha());
    ttwt_->setTextToShowWhenEmpty(TRANS("Text to wavetable"), empty_color);
  }

  paintContainer(g);
  paintHeadingText(g);

  paintChildrenBackgrounds(g);
  paintBorder(g);

  int title_width = findValue(Skin::kTitleWidth);
  setLabelFont(g);
  drawLabelConnectionForComponents(g, level_.get(), pan_.get());
  drawLabelForComponent(g, TRANS("PAN"), pan_.get());
  drawLabelForComponent(g, TRANS("LEVEL"), level_.get());

  int widget_margin = findValue(Skin::kWidgetMargin);
  int level_pan_x = title_width;
  int level_pan_width = getWidth() * kSectionWidthRatio;
  int knob_section_height = getKnobSectionHeight();
  int top_row_width = level_pan_width - 2 * widget_margin;
  int section2_x = getWidth() - 2 * top_row_width - 2 * widget_margin;
  int top_row_y = widget_margin;
  int top_row_height = level_->getY() - top_row_y;
  int phase_x = section2_x + top_row_width + widget_margin;
  int unison_x = section2_x;

  g.setColour(findColour(Skin::kTextComponentBackground, true));
  int label_rounding = findValue(Skin::kLabelBackgroundRounding);
  int morph_y = getHeight() - knob_section_height + widget_margin;
  Rectangle<int> spectral_normal_bounds(spectral_morph_amount_->getX(), morph_y,
                                        spectral_morph_amount_->getWidth(), knob_section_height - 2 * widget_margin);
  g.fillRoundedRectangle(getLabelBackgroundBounds(spectral_normal_bounds, false).toFloat(), label_rounding);
  Rectangle<int> distortion_normal_bounds(distortion_amount_->getX(), morph_y,
                                          distortion_amount_->getWidth(), knob_section_height - 2 * widget_margin);
  g.fillRoundedRectangle(getLabelBackgroundBounds(distortion_normal_bounds, false).toFloat(), label_rounding);
  g.fillRoundedRectangle(destination_selector_->getBounds().toFloat(), label_rounding);

  paintKnobShadows(g);

  paintJointControl(g, level_pan_x + widget_margin, top_row_y, top_row_width, top_row_height, "PITCH");
  paintJointControl(g, unison_x, top_row_y, top_row_width, top_row_height, "UNISON");
  paintJointControl(g, phase_x, top_row_y, top_row_width, top_row_height, "PHASE");
  wavetable_->setDirty();
}

void OscillatorSection::resized() {
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  SynthSection::resized();

  preset_selector_->setColour(Skin::kIconButtonOff, findColour(Skin::kUiButton, true));
  preset_selector_->setColour(Skin::kIconButtonOffHover, findColour(Skin::kUiButtonHover, true));
  preset_selector_->setColour(Skin::kIconButtonOffPressed, findColour(Skin::kUiButtonPressed, true));
  dimension_button_->setColour(Skin::kIconButtonOff, findColour(Skin::kUiButton, true));
  dimension_button_->setColour(Skin::kIconButtonOffHover, findColour(Skin::kUiButtonHover, true));
  dimension_button_->setColour(Skin::kIconButtonOffPressed, findColour(Skin::kUiButtonPressed, true));
  edit_button_->setColour(Skin::kIconButtonOff, findColour(Skin::kUiButton, true));
  edit_button_->setColour(Skin::kIconButtonOffHover, findColour(Skin::kUiButtonHover, true));
  edit_button_->setColour(Skin::kIconButtonOffPressed, findColour(Skin::kUiButtonPressed, true));

  int label_height = findValue(Skin::kLabelBackgroundHeight);
  float label_text_height = findValue(Skin::kLabelHeight);
  Colour body_text = findColour(Skin::kBodyText, true);
  destination_text_->setColor(body_text);
  spectral_morph_type_text_->setColor(body_text);
  distortion_type_text_->setColor(body_text);

  int title_width = getTitleWidth();
  int widget_margin = getWidgetMargin();
  int text_height = findValue(Skin::kTextButtonHeight);
  int slider_width = getSliderWidth();

  int knob_section_height = getKnobSectionHeight();
  int slider_overlap = getSliderOverlap();
  int overlap_with_space = getSliderOverlapWithSpace();
  int wave_section_height = getHeight() - 2 * widget_margin;
  if (distortion_phase_->isVisible())
    wave_section_height -= slider_width - slider_overlap - overlap_with_space;

  int level_pan_x = title_width;
  int level_pan_width = getWidth() * kSectionWidthRatio;
  int top_row_width = level_pan_width - 2 * widget_margin;
  int knob_y = getHeight() - label_height - widget_margin - knob_section_height;
  int big_knob_height = getHeight() - knob_y;
  placeKnobsInArea(Rectangle<int>(level_pan_x, knob_y, level_pan_width, knob_section_height),
                   { level_.get(), pan_.get() });

  int section2_x = getWidth() - 2 * top_row_width - 2 * widget_margin;
  int wave_frame_x = section2_x - slider_width + overlap_with_space;
  int wavetable_x = level_pan_width + level_pan_x;
  int wavetable_width = wave_frame_x - wavetable_x + overlap_with_space;

  wavetable_->setBounds(wavetable_x, widget_margin, wavetable_width, wave_section_height);
  preset_selector_->setBounds(wavetable_x, widget_margin, wavetable_width, title_width - 2 * widget_margin);
  dimension_button_->setBounds(wavetable_x, widget_margin + wave_section_height - text_height,
                               text_height, text_height);

  int wave_frame_height = wave_section_height + 2 * widget_margin;
  wave_frame_->setBounds(wave_frame_x, 0, slider_width, wave_frame_height);
  int edit_x = wavetable_->getRight() - text_height;
  edit_button_->setBounds(edit_x, widget_margin + wave_section_height - text_height, text_height, text_height);

  int top_row_y = widget_margin;
  int text_section_height = knob_y - widget_margin;
  placeJointControls(title_width + widget_margin, top_row_y, level_pan_width - 2 * widget_margin, text_section_height,
                     transpose_.get(), tune_.get(), transpose_quantize_button_.get());

  int section2_width = getWidth() - section2_x;
  int unison_x = section2_x;
  placeJointControls(unison_x, top_row_y, top_row_width, text_section_height,
                     unison_voices_.get(), unison_detune_.get(), unison_detune_power_.get());
  unison_viewer_->setBounds(unison_detune_power_->getBounds());

  int phase_x = unison_x + top_row_width + widget_margin;
  placeJointControls(phase_x, top_row_y, top_row_width, text_section_height,
                     phase_.get(), random_phase_.get(), nullptr);

  placeKnobsInArea(Rectangle<int>(section2_x - widget_margin, knob_y, section2_width + widget_margin, big_knob_height),
                   { spectral_morph_amount_.get(), distortion_amount_.get() });

  int morph_y = getHeight() - knob_section_height + widget_margin;
  Rectangle<int> spectral_normal_bounds(spectral_morph_amount_->getX(), morph_y,
                                        spectral_morph_amount_->getWidth(), knob_section_height - 2 * widget_margin);
  Rectangle<int> spectral_label_bounds = getLabelBackgroundBounds(spectral_normal_bounds);
  int browse_width = spectral_label_bounds.getHeight();
  int browse_y = spectral_label_bounds.getY();
  prev_spectral_->setBounds(spectral_morph_amount_->getX(), browse_y, browse_width, browse_width);
  next_spectral_->setBounds(spectral_morph_amount_->getRight() - browse_width, browse_y,
                            browse_width, browse_width);

  prev_distortion_->setBounds(distortion_amount_->getX(), browse_y, browse_width, browse_width);
  next_distortion_->setBounds(distortion_amount_->getRight() - browse_width, browse_y,
                              browse_width, browse_width);

  spectral_morph_type_text_->setBounds(spectral_label_bounds);
  spectral_morph_type_text_->setTextSize(label_text_height);
  int spectral_menu_x = prev_spectral_->getRight();
  spectral_morph_type_selector_->setBounds(spectral_menu_x, prev_spectral_->getY(),
                                           next_spectral_->getX() - spectral_menu_x, prev_spectral_->getHeight());

  Rectangle<int> distortion_normal_bounds(distortion_amount_->getX(), morph_y,
                                          distortion_amount_->getWidth(), knob_section_height - 2 * widget_margin);
  distortion_type_text_->setBounds(getLabelBackgroundBounds(distortion_normal_bounds));
  distortion_type_text_->setTextSize(label_text_height);
  int distortion_menu_x = prev_distortion_->getRight();
  distortion_type_selector_->setBounds(distortion_menu_x, prev_distortion_->getY(),
                                       next_distortion_->getX() - distortion_menu_x, prev_distortion_->getHeight());

  distortion_phase_->setBounds(wavetable_->getX() - widget_margin,
                               wavetable_->getBottom() - slider_overlap + widget_margin,
                               wavetable_->getWidth() + 2 * widget_margin, slider_width);

  int destination_x = level_pan_x + widget_margin;
  int destination_y = getHeight() - label_height - widget_margin;
  destination_selector_->setBounds(destination_x, destination_y, top_row_width, label_height);
  destination_text_->setBounds(destination_selector_->getBounds());
  destination_text_->setTextSize(label_text_height);

  prev_destination_->setBounds(destination_x, destination_y, browse_width, browse_width);
  next_destination_->setBounds(destination_x + top_row_width - browse_width, destination_y,
                               browse_width, browse_width);

  ttwt_overlay_.setRounding(findValue(Skin::kWidgetRoundedCorner));
  ttwt_overlay_.setBounds(wavetable_->getBounds());
  ttwt_overlay_.setColor(findColour(Skin::kOverlayScreen, true));

  if (ttwt_) {
    float ttwt_height = title_width;
    int settings_width = ttwt_height * 2.0f;
    int ttwt_y = (wavetable_->getHeight() - ttwt_height) / 2;
    int ttwt_x = wavetable_->getX() + widget_margin;
    int ttwt_width = wavetable_->getWidth() - 2 * widget_margin;
    ttwt_->setBounds(ttwt_x, ttwt_y, ttwt_width, ttwt_height);
    ttwt_->setFont(Fonts::instance()->proportional_light().withPointHeight(ttwt_height * 0.6f));
    ttwt_settings_->setBounds(ttwt_->getRight() - settings_width, ttwt_->getBottom(), settings_width, ttwt_height / 2);

    ttwt_error_text_->setTextSize(label_text_height);
    ttwt_error_text_->setBounds(ttwt_->getBounds());
    ttwt_error_text_->setColor(body_text);
  }
}

void OscillatorSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == edit_button_.get()) {
    FullInterface* parent = findParentComponentOfClass<FullInterface>();
    if (parent)
      parent->showWavetableEditSection(index_);
  }
  else if (clicked_button == spectral_morph_type_selector_.get()) {
    PopupItems options;

    for (int i = 0; i < vital::SynthOscillator::kNumSpectralMorphTypes; ++i) {
      if (isSpectralMenuSeparator(i))
        options.addItem(-1, "");

      options.addItem(i, strings::kSpectralMorphNames[i]);
    }

    Point<int> position(spectral_morph_type_selector_->getX(), spectral_morph_type_selector_->getBottom());
    showPopupSelector(this, position, options, [=](int selection) { setSpectralMorphSelected(selection); });
  }
  else if (clicked_button == distortion_type_selector_.get()) {
    PopupItems options;
    
    for (int i = 0; i < vital::SynthOscillator::kNumDistortionTypes; ++i) {
      if (isDistortionMenuSeparator(i))
        options.addItem(-1, "");

      options.addItem(i, getDistortionMenuString(i, index_).toStdString());
    }

    Point<int> position(distortion_type_selector_->getX(), distortion_type_selector_->getBottom());
    showPopupSelector(this, position, options, [=](int selection) { setDistortionSelected(selection); });
  }
  else if (clicked_button == destination_selector_.get()) {
    PopupItems options;

    int num_source_destinations = vital::constants::kNumSourceDestinations;
    for (int i = 0; i < num_source_destinations; ++i)
      options.addItem(i, strings::kDestinationMenuNames[i]);

    Point<int> position(destination_selector_->getX(), destination_selector_->getBottom());
    showPopupSelector(this, position, options, [=](int selection) { setDestinationSelected(selection); });
  }
  else if (clicked_button == ttwt_settings_.get())
    showTtwtSettings();
  else if (clicked_button == dimension_button_.get()) {
    int render_type = (wavetable_->getRenderType() + Wavetable3d::kNumRenderTypes - 1) % Wavetable3d::kNumRenderTypes;
    dimension_button_->setText(strings::kWavetableDimensionNames[render_type]);
    dimension_value_->setValue(render_type, sendNotificationSync);
    wavetable_->setRenderType(static_cast<Wavetable3d::RenderType>(render_type));
  }
  else if (clicked_button == prev_destination_.get()) {
    int new_destination = current_destination_ - 1 + vital::constants::kNumSourceDestinations;
    setDestinationSelected(new_destination % vital::constants::kNumSourceDestinations);
  }
  else if (clicked_button == next_destination_.get()) {
    int new_destination = (current_destination_ + 1) % vital::constants::kNumSourceDestinations;
    setDestinationSelected(new_destination);
  }
  else if (clicked_button == prev_spectral_.get()) {
    int new_morph_type = current_spectral_morph_type_ - 1 + vital::SynthOscillator::kNumSpectralMorphTypes;
    setSpectralMorphSelected(new_morph_type % vital::SynthOscillator::kNumSpectralMorphTypes);
  }
  else if (clicked_button == next_spectral_.get()) {
    int new_morph_type = (current_spectral_morph_type_ + 1) % vital::SynthOscillator::kNumSpectralMorphTypes;
    setSpectralMorphSelected(new_morph_type);
  }
  else if (clicked_button == prev_distortion_.get()) {
    int new_distortion_type = current_distortion_type_ - 1 + vital::SynthOscillator::kNumDistortionTypes;
    setDistortionSelected(new_distortion_type % vital::SynthOscillator::kNumDistortionTypes);
  }
  else if (clicked_button == next_distortion_.get()) {
    int new_distortion_type = (current_distortion_type_ + 1) % vital::SynthOscillator::kNumDistortionTypes;
    setDistortionSelected(new_distortion_type);
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void OscillatorSection::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  current_spectral_morph_type_ = controls[spectral_morph_control_name_]->value();
  current_distortion_type_ = controls[distortion_control_name_]->value();
  current_destination_ = controls[destination_control_name_]->value();
  transpose_quantize_button_->setValue(static_cast<int>(controls[quantize_control_name_]->value()));
  setupSpectralMorph();
  setupDistortion();
  setupDestination();
  vital::SynthOscillator::DistortionType type = (vital::SynthOscillator::DistortionType)current_distortion_type_;

  setDistortionPhaseVisible(vital::SynthOscillator::usesDistortionPhase(type));

  wavetable_->setSpectralMorphType(current_spectral_morph_type_);
  wavetable_->setDistortionType(current_distortion_type_);
  int render_type = dimension_value_->getValue();
  dimension_button_->setText(strings::kWavetableDimensionNames[render_type]);
  wavetable_->setRenderType(static_cast<Wavetable3d::RenderType>(render_type));
}

void OscillatorSection::textEditorReturnKeyPressed(TextEditor& text_editor) {
  String text = text_editor.getText();
  text = text.trim();
  show_ttwt_error_ = false;
  if (!text.isEmpty()) {
    std::string error = loadWavetableFromText(text);
    show_ttwt_error_ = !error.empty();
    if (show_ttwt_error_) {
      ttwt_error_text_->setText(error);
      ttwt_error_text_->redrawImage(true);
      ttwt_error_text_->setVisible(true);
      startTimer(kShowErrorMs);
    }
  }

  ttwt_->clear();
  ttwt_overlay_.setVisible(show_ttwt_error_);
  ttwt_->setVisible(false);
  ttwt_settings_->setVisible(false);
}

void OscillatorSection::textEditorFocusLost(TextEditor& text_editor) {
  if (showing_language_menu_)
    return;

  ttwt_overlay_.setVisible(show_ttwt_error_);
  ttwt_->setVisible(false);
  ttwt_settings_->setVisible(false);
}

void OscillatorSection::timerCallback() {
  show_ttwt_error_ = false;
  ttwt_error_text_->setVisible(false);
  ttwt_overlay_.setVisible(false);
  stopTimer();
}

void OscillatorSection::setActive(bool active) {
  wavetable_->setActive(active);
  SynthSection::setActive(active);
  spectral_morph_amount_->setActive(active && current_spectral_morph_type_ != vital::SynthOscillator::kNoSpectralMorph);
  distortion_amount_->setActive(active && current_distortion_type_ != vital::SynthOscillator::kNone);
}

void OscillatorSection::setName(String name) {
  preset_selector_->setText(name);
}

void OscillatorSection::showTtwtSettings() {
  showing_language_menu_ = true;
  PopupItems options;

  for (int i = 0; i < sizeof(kLanguageNames) / sizeof(std::string); ++i)
    options.addItem(i, kLanguageNames[i]);

  Point<int> position(ttwt_settings_->getX(), ttwt_settings_->getBottom());
  showPopupSelector(this, position, options, [=](int selection) { setLanguage(selection); });
}

std::string OscillatorSection::loadWavetableFromText(const String& text) {
  String clamped_text = text.substring(0, kMaxTTWTLength);
  String language_query = String(kLanguageUrlQuery) + URL::addEscapeChars(kLanguageCodes[ttwt_language_], true);
  URL ttwt_url(String(kUrlPrefix) + URL::addEscapeChars(clamped_text, true) + language_query);

  try {
    String result = ttwt_url.readEntireTextStream(false);
    json data = json::parse(result.toStdString());

    if (data.count("error"))
      return data["error"];

    std::string hex_encoded_buffer = data["buffer"];
    MemoryBlock audio_memory;
    audio_memory.loadFromHexString(hex_encoded_buffer);
    MemoryInputStream* audio_stream = new MemoryInputStream(audio_memory, false);

    bool succeeded = loadAudioAsWavetable("TTWT", audio_stream, WavetableCreator::kTtwt);
    if (succeeded)
      return "";
    return "Error converting speech to wavetable.";
  }
  catch (const std::exception& e) {
    return "Error rendering speech. Check internet connection";
  }
}

Slider* OscillatorSection::getWaveFrameSlider() {
  return wave_frame_.get();
}

void OscillatorSection::setDistortionSelected(int selection) {
  current_distortion_type_ = selection;
  wavetable_->setDistortionType(selection);
  vital::SynthOscillator::DistortionType type = (vital::SynthOscillator::DistortionType)current_distortion_type_;
  setDistortionPhaseVisible(vital::SynthOscillator::usesDistortionPhase(type));
  notifyDistortionTypeChange();
}

void OscillatorSection::setSpectralMorphSelected(int selection) {
  current_spectral_morph_type_ = selection;
  wavetable_->setSpectralMorphType(selection);
  notifySpectralMorphTypeChange();
}

void OscillatorSection::setDestinationSelected(int selection) {
  current_destination_ = selection;
  notifyDestinationChange();
}

void OscillatorSection::toggleFilterInput(int filter_index, bool on) {
  vital::constants::SourceDestination current_destination = (vital::constants::SourceDestination)current_destination_;
  if (filter_index == 0)
    current_destination_ = vital::constants::toggleFilter1(current_destination, on);
  else
    current_destination_ = vital::constants::toggleFilter2(current_destination, on);
  notifyDestinationChange();
}

void OscillatorSection::loadBrowserState() {
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  if (parent)
    preset_selector_->setText(parent->getWavetableName(index_));
}

void OscillatorSection::setIndexSelected() {
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  if (parent == nullptr)
    return;
}

void OscillatorSection::setLanguage(int index) {
  ttwt_language_ = index;
  LoadSave::savePreferredTTWTLanguage(kLanguageCodes[ttwt_language_]);
  showing_language_menu_ = false;
  if (ttwt_)
    ttwt_->grabKeyboardFocus();
  ttwt_settings_->setToggleState(false, dontSendNotification);
  ttwt_settings_->setText(kLanguageCodes[ttwt_language_]);
}

void OscillatorSection::languageSelectCancelled() {
  showing_language_menu_ = false;
  if (ttwt_)
    ttwt_->grabKeyboardFocus();
  ttwt_settings_->setToggleState(false, dontSendNotification);
}

void OscillatorSection::prevClicked() {
  File wavetable_file = LoadSave::getShiftedFile(LoadSave::kWavetableFolderName, vital::kWavetableExtensionsList,
                                                 LoadSave::kAdditionalWavetableFoldersName, current_file_, -1);
  if (wavetable_file.exists())
    loadFile(wavetable_file);
  
  updatePopupBrowser(this);
}

void OscillatorSection::nextClicked() {
  File wavetable_file = LoadSave::getShiftedFile(LoadSave::kWavetableFolderName, vital::kWavetableExtensionsList,
                                                 LoadSave::kAdditionalWavetableFoldersName, current_file_, 1);
  if (wavetable_file.exists())
    loadFile(wavetable_file);
  
  updatePopupBrowser(this);
}

void OscillatorSection::textMouseDown(const MouseEvent& e) {
  static constexpr int kBrowserWidth = 600;
  static constexpr int kBrowserHeight = 400;
  Rectangle<int> bounds(unison_voices_->getX(), preset_selector_->getY(),
                        kBrowserWidth * size_ratio_, kBrowserHeight * size_ratio_);
  bounds = getLocalArea(this, bounds);
  showPopupBrowser(this, bounds, LoadSave::getWavetableDirectories(), vital::kWavetableExtensionsList,
                   LoadSave::kWavetableFolderName, LoadSave::kAdditionalWavetableFoldersName);
}

void OscillatorSection::quantizeUpdated() {
  int value = transpose_quantize_button_->getValue();
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->getSynth()->valueChangedInternal(quantize_control_name_, value);
}

void OscillatorSection::resetOscillatorModulationDistortionType() {
  if (vital::SynthOscillator::isFirstModulation(current_distortion_type_) ||
      vital::SynthOscillator::isSecondModulation(current_distortion_type_)) {
    current_distortion_type_ = vital::SynthOscillator::kNone;
    notifyDistortionTypeChange();
  }
}

bool OscillatorSection::loadAudioAsWavetable(String name, InputStream* audio_stream,
                                             WavetableCreator::AudioFileLoadStyle style) {
  preset_selector_->setText(name);

  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  if (parent == nullptr) {
    delete audio_stream;
    return false;
  }

  wavetable_->setLoadingWavetable(true);
  bool success = parent->loadAudioAsWavetable(index_, name, audio_stream, style);
  wavetable_->setLoadingWavetable(false);
  wavetable_->repaintBackground();
  return success;
}

void OscillatorSection::loadWavetable(json& wavetable_data) {
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  if (parent == nullptr)
    return;

  wavetable_->setLoadingWavetable(true);
  parent->loadWavetable(index_, wavetable_data);
  wavetable_->setLoadingWavetable(false);
  std::string name = wavetable_data["name"];
  preset_selector_->setText(name);
}

void OscillatorSection::loadDefaultWavetable() {
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  wavetable_->setLoadingWavetable(true);
  if (parent)
    parent->loadDefaultWavetable(index_);
  wavetable_->setLoadingWavetable(false);

  preset_selector_->setText("Init");
}

void OscillatorSection::resynthesizeToWavetable() {
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  wavetable_->setLoadingWavetable(true);
  if (parent)
    parent->resynthesizeToWavetable(index_);
  wavetable_->setLoadingWavetable(false);
}

void OscillatorSection::saveWavetable() {
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  if (parent)
    parent->saveWavetable(index_);
}

void OscillatorSection::loadFile(const File& wavetable_file) {
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  if (parent == nullptr)
    return;

  current_file_ = wavetable_file;
  if (wavetable_file.getFileExtension() == String(".") + vital::kWavetableExtension) {
    wavetable_->setLoadingWavetable(true);
    parent->loadWavetableFile(index_, wavetable_file);
    wavetable_->setLoadingWavetable(false);
    preset_selector_->setText(wavetable_file.getFileNameWithoutExtension());
    wavetable_->repaintBackground();
  }
  else {
    FileInputStream* input_stream = new FileInputStream(wavetable_file);
    loadAudioAsWavetable(wavetable_file.getFileNameWithoutExtension(), input_stream,
                         WavetableCreator::kWavetableSplice);
  }
}

Rectangle<float> OscillatorSection::getWavetableRelativeBounds() {
  Rectangle<int> wavetable_bounds = wavetable_->getBounds();
  float width_ratio = 1.0f / getWidth();
  float height_ratio = 1.0f / getHeight();
  return Rectangle<float>(wavetable_bounds.getX() * width_ratio, wavetable_bounds.getY() * height_ratio,
                          wavetable_bounds.getWidth() * width_ratio, wavetable_bounds.getHeight() * height_ratio);
}

void OscillatorSection::setupSpectralMorph() {
  bool bipolar = isBipolarSpectralMorphType(current_spectral_morph_type_);
  spectral_morph_amount_->setBipolar(bipolar);
  spectral_morph_amount_->setDoubleClickReturnValue(true, bipolar ? 0.5f : 0.0f);
  spectral_morph_amount_->setActive(isActive() &&
                                    current_spectral_morph_type_ != vital::SynthOscillator::kNoSpectralMorph);
  spectral_morph_amount_->redoImage();
  spectral_morph_type_text_->setText(kSpectralMorphTypes[current_spectral_morph_type_]);
}

void OscillatorSection::setupDistortion() {
  bool bipolar = isBipolarDistortionType(current_distortion_type_);
  distortion_amount_->setBipolar(bipolar);
  distortion_amount_->setDoubleClickReturnValue(true, bipolar ? 0.5f : 0.0f);
  distortion_amount_->setActive(isActive() && current_distortion_type_ != vital::SynthOscillator::kNone);
  distortion_amount_->redoImage();
  distortion_type_text_->setText(getDistortionString(current_distortion_type_, index_));
}

void OscillatorSection::setupDestination() {
  for (Listener* listener : listeners_)
    listener->oscillatorDestinationChanged(this, current_destination_);

  destination_text_->setText(strings::kDestinationNames[current_destination_]);
}

void OscillatorSection::setDistortionPhaseVisible(bool visible) {
  if (visible == distortion_phase_->isVisible())
    return;
  distortion_phase_->setVisible(visible);
  resized();
  repaintBackground();
}

void OscillatorSection::notifySpectralMorphTypeChange() {
  setupSpectralMorph();

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->getSynth()->valueChangedInternal(spectral_morph_control_name_, current_spectral_morph_type_);
}

void OscillatorSection::notifyDistortionTypeChange() {
  setupDistortion();

  for (Listener* listener : listeners_)
    listener->distortionTypeChanged(this, current_distortion_type_);

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->getSynth()->valueChangedInternal(distortion_control_name_, current_distortion_type_);
}

void OscillatorSection::notifyDestinationChange() {
  setupDestination();

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->getSynth()->valueChangedInternal(destination_control_name_, current_destination_);
}

