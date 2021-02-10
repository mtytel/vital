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

#include "load_save.h"
#include "open_gl_multi_quad.h"
#include "overlay.h"
#include "synth_constants.h"
#include "json/json.h"

using json = nlohmann::json;

class SaveSection : public Overlay, public TextEditor::Listener {
  public:
    static constexpr int kSaveWidth = 630;
    static constexpr int kSavePresetHeight = 450;
    static constexpr int kStylePaddingX = 4;
    static constexpr int kStylePaddingY = 4;
    static constexpr int kStyleButtonHeight = 24;
    static constexpr int kOverwriteWidth = 340;
    static constexpr int kOverwriteHeight = 160;
    static constexpr int kTextEditorHeight = 37;
    static constexpr int kLabelHeight = 15;
    static constexpr int kButtonHeight = 40;
    static constexpr int kAddFolderHeight = 20;
    static constexpr int kDivision = 150;
    static constexpr int kPaddingX = 25;
    static constexpr int kPaddingY = 20;
    static constexpr int kExtraTopPadding = 10;

    class Listener {
      public:
        virtual ~Listener() = default;

        virtual void save(File preset) = 0;
    };

    SaveSection(String name);
    virtual ~SaveSection() = default;
    void resized() override;
    void setVisible(bool should_be_visible) override;

    void setSaveBounds();
    void setOverwriteBounds();
    void setTextColors(OpenGlTextEditor* editor, String empty_string);

    void textEditorReturnKeyPressed(TextEditor& editor) override;
    void buttonClicked(Button* clicked_button) override;
    void mouseUp(const MouseEvent& e) override;
    void setFileType(const String& type) { file_type_ = type; repaint(); }
    void setFileExtension(const String& extension) { file_extension_ = extension; }
    void setDirectory(const File& directory) { file_directory_ = directory; }
    void setFileData(const json& data) { file_data_ = data; }
    void setIsPreset(bool preset) {
      saving_preset_ = preset;

      if (preset) {
        setFileExtension(vital::kPresetExtension);
        setFileType("Preset");
        setDirectory(LoadSave::getUserPresetDirectory());
      }
    }

    Rectangle<int> getSaveRect();
    Rectangle<int> getOverwriteRect();

    void addSaveListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    void save();

    bool overwrite_;
    bool saving_preset_;

    String file_type_;
    String file_extension_;
    File file_directory_;
    json file_data_;

    OpenGlQuad body_;

    std::unique_ptr<OpenGlTextEditor> name_;
    std::unique_ptr<OpenGlTextEditor> author_;
    std::unique_ptr<OpenGlTextEditor> comments_;

    std::unique_ptr<OpenGlToggleButton> save_button_;
    std::unique_ptr<OpenGlToggleButton> overwrite_button_;
    std::unique_ptr<OpenGlToggleButton> cancel_button_;

    std::unique_ptr<OpenGlToggleButton> style_buttons_[LoadSave::kNumPresetStyles];

    std::unique_ptr<PlainTextComponent> preset_text_;
    std::unique_ptr<PlainTextComponent> author_text_;
    std::unique_ptr<PlainTextComponent> style_text_;
    std::unique_ptr<PlainTextComponent> comments_text_;
    std::unique_ptr<PlainTextComponent> overwrite_text_;

    std::vector<Listener*> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaveSection)
};

