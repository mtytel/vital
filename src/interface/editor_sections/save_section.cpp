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

#include "save_section.h"

#include "skin.h"
#include "fonts.h"
#include "load_save.h"
#include "synth_gui_interface.h"
#include "synth_strings.h"

class FileNameInputFilter : public TextEditor::InputFilter {
  public:
    FileNameInputFilter() : TextEditor::InputFilter() { }

    String filterNewText(TextEditor& editor, const String& new_input) override {
      return new_input.removeCharacters("<>?*/|\\[]\":");
    }
};

SaveSection::SaveSection(String name) : Overlay(name), overwrite_(false), saving_preset_(true),
                                        body_(Shaders::kRoundedRectangleFragment) {
  addOpenGlComponent(&body_);

#if !defined(NO_TEXT_ENTRY)
  name_ = std::make_unique<OpenGlTextEditor>("Name");
  name_->addListener(this);
  name_->setInputFilter(new FileNameInputFilter(), true);
  addAndMakeVisible(name_.get());
  addOpenGlComponent(name_->getImageComponent());

  author_ = std::make_unique<OpenGlTextEditor>("Author");
  author_->addListener(this);
  author_->setText(LoadSave::getAuthor());
  addAndMakeVisible(author_.get());
  addOpenGlComponent(author_->getImageComponent());

  comments_ = std::make_unique<OpenGlTextEditor>("Comments");
  comments_->addListener(this);
  comments_->setReturnKeyStartsNewLine(true);
  comments_->setInputRestrictions(LoadSave::kMaxCommentLength);
  addAndMakeVisible(comments_.get());
  addOpenGlComponent(comments_->getImageComponent());
  comments_->setMultiLine(true);
#endif

  save_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Save"));
  save_button_->setButtonText("Save");
  save_button_->setUiButton(true);
  save_button_->addListener(this);
  addAndMakeVisible(save_button_.get());
  addOpenGlComponent(save_button_->getGlComponent());

  overwrite_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Overwrite"));
  overwrite_button_->setButtonText("Overwrite");
  overwrite_button_->setUiButton(true);
  overwrite_button_->addListener(this);
  addAndMakeVisible(overwrite_button_.get());
  addOpenGlComponent(overwrite_button_->getGlComponent());

  cancel_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Cancel"));
  cancel_button_->setButtonText("Cancel");
  cancel_button_->setUiButton(false);
  cancel_button_->addListener(this);
  addAndMakeVisible(cancel_button_.get());
  addOpenGlComponent(cancel_button_->getGlComponent());

  preset_text_ = std::make_unique<PlainTextComponent>("Preset", "NAME");
  addOpenGlComponent(preset_text_.get());
  preset_text_->setFontType(PlainTextComponent::kLight);
  preset_text_->setTextSize(kLabelHeight * size_ratio_);
  preset_text_->setJustification(Justification::centredRight);

  author_text_ = std::make_unique<PlainTextComponent>("Author", "AUTHOR");
  addOpenGlComponent(author_text_.get());
  author_text_->setFontType(PlainTextComponent::kLight);
  author_text_->setTextSize(kLabelHeight * size_ratio_);
  author_text_->setJustification(Justification::centredRight);

  style_text_ = std::make_unique<PlainTextComponent>("Style", "STYLE");
  addOpenGlComponent(style_text_.get());
  style_text_->setFontType(PlainTextComponent::kLight);
  style_text_->setTextSize(kLabelHeight * size_ratio_);
  style_text_->setJustification(Justification::centredRight);

  comments_text_ = std::make_unique<PlainTextComponent>("Comments", "COMMENTS");
  addOpenGlComponent(comments_text_.get());
  comments_text_->setFontType(PlainTextComponent::kLight);
  comments_text_->setTextSize(kLabelHeight);
  comments_text_->setJustification(Justification::centredRight);

  overwrite_text_ = std::make_unique<PlainTextComponent>("overwrite", "Overwrite existing file?");
  addOpenGlComponent(overwrite_text_.get());
  overwrite_text_->setFontType(PlainTextComponent::kLight);
  overwrite_text_->setTextSize(kLabelHeight);
  overwrite_text_->setJustification(Justification::centred);

  for (int i = 0; i < LoadSave::kNumPresetStyles; ++i) {
    style_buttons_[i] = std::make_unique<OpenGlToggleButton>(strings::kPresetStyleNames[i]);
    style_buttons_[i]->addListener(this);
    addAndMakeVisible(style_buttons_[i].get());
    addOpenGlComponent(style_buttons_[i]->getGlComponent());
  }
}

void SaveSection::resized() {
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));

  Colour text_color = findColour(Skin::kBodyText, true);
  preset_text_->setColor(text_color);
  author_text_->setColor(text_color);
  style_text_->setColor(text_color);
  comments_text_->setColor(text_color);
  overwrite_text_->setColor(text_color);

  Overlay::resized();

  if (overwrite_)
    setOverwriteBounds();
  else
    setSaveBounds();

  if (name_) {
    name_->redoImage();
    author_->redoImage();
    comments_->redoImage();
  }

  float label_height = kLabelHeight * size_ratio_;
  preset_text_->setTextSize(label_height);
  author_text_->setTextSize(label_height);
  comments_text_->setTextSize(label_height);
  style_text_->setTextSize(label_height);
  overwrite_text_->setTextSize(label_height);
}

void SaveSection::setSaveBounds() {
  Rectangle<int> save_rect = getSaveRect();
  body_.setBounds(save_rect);
  int padding_x = size_ratio_ * kPaddingX;
  int padding_y = size_ratio_ * kPaddingX;
  int style_padding_x = size_ratio_ * kStylePaddingX;
  int style_padding_y = size_ratio_ * kStylePaddingY;
  int division = size_ratio_ * kDivision;
  int extra_top_padding = size_ratio_ * kExtraTopPadding;
  int button_height = size_ratio_ * kButtonHeight;
  int style_button_height = size_ratio_ * kStyleButtonHeight;

  save_button_->setVisible(true);
  overwrite_button_->setVisible(false);
  float button_width = (save_rect.getWidth() - 3 * padding_x) / 2.0f;
  save_button_->setBounds(save_rect.getX() + button_width + 2 * padding_x,
                          save_rect.getBottom() - padding_y - button_height,
                          button_width, button_height);
  cancel_button_->setBounds(save_rect.getX() + padding_x,
                            save_rect.getBottom() - padding_y - button_height,
                            button_width, button_height);

  int text_x = save_rect.getX() + padding_x;
  int text_y = save_rect.getY() + extra_top_padding;

  preset_text_->setVisible(true);
  author_text_->setVisible(true);
  style_text_->setVisible(saving_preset_);
  comments_text_->setVisible(saving_preset_);
  overwrite_text_->setVisible(false);

  if (name_ == nullptr || author_ == nullptr || comments_ == nullptr)
    return;

  name_->setVisible(true);
  author_->setVisible(true);
  comments_->setVisible(saving_preset_);
  for (int i = 0; i < LoadSave::kNumPresetStyles; ++i)
    style_buttons_[i]->setVisible(saving_preset_);

  int editor_height = kTextEditorHeight * size_ratio_;
  int editor_x = save_rect.getX() + padding_x + division;
  int editor_width = save_rect.getWidth() - 2 * padding_x - division;
  name_->setBounds(editor_x, save_rect.getY() + padding_y + extra_top_padding,
                   editor_width, editor_height);
  author_->setBounds(editor_x, save_rect.getY() + 2 * padding_y + editor_height + extra_top_padding,
                     editor_width, editor_height);

  int style_width = editor_width + style_padding_x;
  int style_y = save_rect.getY() + 3 * padding_y + 2 * editor_height + extra_top_padding;
  int num_in_row = LoadSave::kNumPresetStyles / 3;
  for (int i = 0; i < LoadSave::kNumPresetStyles; ++i) {
    int column = i % num_in_row;
    int x = editor_x + (style_width * column) / num_in_row;
    int next_x = editor_x + (style_width * (column + 1)) / num_in_row;
    int width = next_x - x - style_padding_x;
    int y = style_y + (i / num_in_row) * (style_button_height + style_padding_y);
    style_buttons_[i]->setBounds(x, y, width, style_button_height);
  }

  int comments_y = style_y + 3 * style_button_height + 2 * style_padding_y + padding_y;
  int comments_height = save_button_->getY() - comments_y - padding_y;
  comments_->setBounds(editor_x, comments_y, editor_width, comments_height);
  
  float text_width = division - 10 * size_ratio_;
  preset_text_->setBounds(text_x, text_y + padding_y, text_width, name_->getHeight());
  author_text_->setBounds(text_x, text_y + 2 * padding_y + name_->getHeight(),
                          text_width, name_->getHeight());

  int style_height = 3 * style_button_height + 2 * style_padding_y;
  style_text_->setBounds(text_x, text_y + 3 * padding_y + 2 * name_->getHeight(),
                         text_width, style_height);
  comments_text_->setBounds(text_x, text_y + 4 * padding_y + style_height + 2 * name_->getHeight(),
                            text_width, author_->getHeight());

  Font editor_font = Fonts::instance()->proportional_light().withPointHeight(editor_height * 0.6f);
  setTextColors(name_.get(), file_type_ + " " + TRANS("Name"));
  setTextColors(author_.get(), TRANS("Author"));
  setTextColors(comments_.get(), TRANS("Comments"));
  name_->applyFontToAllText(editor_font, true);
  author_->applyFontToAllText(editor_font, true);
  comments_->applyFontToAllText(editor_font, true);
}

void SaveSection::setOverwriteBounds() {
  preset_text_->setVisible(false);
  author_text_->setVisible(false);
  style_text_->setVisible(false);
  comments_text_->setVisible(false);
  overwrite_text_->setVisible(true);
  name_->setVisible(false);
  author_->setVisible(false);
  comments_->setVisible(false);

  for (int i = 0; i < LoadSave::kNumPresetStyles; ++i)
    style_buttons_[i]->setVisible(false);

  Rectangle<int> overwrite_rect = getOverwriteRect();
  body_.setBounds(overwrite_rect);

  save_button_->setVisible(false);
  overwrite_button_->setVisible(true);

  overwrite_text_->setText(String("Overwrite existing file?"));
  int padding_x = kPaddingX * size_ratio_;
  int padding_y = kPaddingY * size_ratio_;
  int button_height = kButtonHeight * size_ratio_;
  overwrite_text_->setBounds(overwrite_rect.getX() + padding_x,
                             overwrite_rect.getY() + (kExtraTopPadding + 24) * size_ratio_,
                             overwrite_rect.getWidth() - 2 * padding_x, 24 * size_ratio_);
  overwrite_text_->redrawImage(true);

  float button_width = (overwrite_rect.getWidth() - 3 * padding_x) / 2.0f;
  cancel_button_->setBounds(overwrite_rect.getX() + padding_x,
                            overwrite_rect.getBottom() - padding_y - button_height,
                            button_width, button_height);
  overwrite_button_->setBounds(overwrite_rect.getX() + button_width + 2 * padding_x,
                               overwrite_rect.getBottom() - padding_y - button_height,
                               button_width, button_height);
}

void SaveSection::setTextColors(OpenGlTextEditor* editor, String empty_string) {
  editor->setColour(CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
  editor->setColour(TextEditor::textColourId, findColour(Skin::kPresetText, true));
  editor->setColour(TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
  editor->setColour(TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
  Colour empty_color = findColour(Skin::kBodyText, true);
  empty_color = empty_color.withAlpha(0.5f * empty_color.getFloatAlpha());
  editor->setTextToShowWhenEmpty(empty_string, empty_color);
  editor->redoImage();
}

void SaveSection::setVisible(bool should_be_visible) {
  overwrite_ = false;

  if (should_be_visible) {
    setSaveBounds();
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent && name_) {
      name_->setText(parent->getSynth()->getPresetName());
      comments_->setText(parent->getSynth()->getComments());
      String style = parent->getSynth()->getStyle();
      for (int i = 0; i < LoadSave::kNumPresetStyles; ++i)
        style_buttons_[i]->setToggleState(style == String(strings::kPresetStyleNames[i]), dontSendNotification);
    }
  }

  Image image(Image::ARGB, 1, 1, false);
  Graphics g(image);
  paintOpenGlChildrenBackgrounds(g);

  Overlay::setVisible(should_be_visible);
  
  if (name_ && name_->isShowing())
    name_->grabKeyboardFocus();
}

void SaveSection::mouseUp(const MouseEvent &e) {
  if (!getSaveRect().contains(e.getPosition()))
    setVisible(false);
}

void SaveSection::textEditorReturnKeyPressed(TextEditor& editor) {
  save();
}

void SaveSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == save_button_.get() || clicked_button == overwrite_button_.get())
    save();
  else if (clicked_button == cancel_button_.get())
    setVisible(false);
  else {
    for (int i = 0; i < LoadSave::kNumPresetStyles; ++i) {
      if (style_buttons_[i].get() != clicked_button)
        style_buttons_[i]->setToggleState(false, dontSendNotification);
    }
  }
}

Rectangle<int> SaveSection::getSaveRect() {
  int save_width = kSaveWidth * size_ratio_;
  int save_height = kSavePresetHeight * size_ratio_;
  if (!saving_preset_)
    save_height = (kTextEditorHeight * 2 + kButtonHeight + kPaddingY * 4 + kExtraTopPadding) * size_ratio_;

  int x = (getWidth() - save_width) / 2;
  int y = (getHeight() - save_height) / 2;
  return Rectangle<int>(x, y, save_width, save_height);
}

Rectangle<int> SaveSection::getOverwriteRect() {
  int x = (getWidth() - kOverwriteWidth * size_ratio_) / 2;
  int y = (getHeight() - kOverwriteHeight * size_ratio_) / 2;
  return Rectangle<int>(x, y, kOverwriteWidth * size_ratio_, kOverwriteHeight * size_ratio_);
}

void SaveSection::save() {
  if (name_ == nullptr || name_->getText().trim() == "")
    return;

  String file_name = name_->getText().trim() + String(".") + file_extension_;
  File save_file = file_directory_.getChildFile(File::createLegalFileName(file_name));
  if (!overwrite_ && save_file.exists()) {
    overwrite_ = true;
    setOverwriteBounds();
    repaint();
    return;
  }

  if (saving_preset_) {
    LoadSave::saveAuthor(author_->getText().trim().toStdString());
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent) {
      parent->getSynth()->setAuthor(author_->getText().trim().removeCharacters("\""));
      parent->getSynth()->setComments(comments_->getText().trim().removeCharacters("\""));
      String style = "";
      for (int i = 0; i < LoadSave::kNumPresetStyles; ++i) {
        if (style_buttons_[i]->getToggleState())
          style = style_buttons_[i]->getName();
      }
      parent->getSynth()->setStyle(style.removeCharacters("\""));
      parent->getSynth()->saveToFile(save_file);

      for (Listener* listener : listeners_)
        listener->save(save_file);
    }
  }
  else {
    file_data_["name"] = name_->getText().trim().toStdString();
    file_data_["author"] = author_->getText().trim().toStdString();
    save_file.replaceWithText(file_data_.dump());
    for (Listener* listener : listeners_)
      listener->save(save_file);
  }
  setVisible(false);
}
