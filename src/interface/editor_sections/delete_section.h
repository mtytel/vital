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
#include "overlay.h"

class DeleteSection : public Overlay {
  public:
    static constexpr int kDeleteWidth = 340;
    static constexpr int kDeleteHeight = 140;
    static constexpr int kTextHeight = 15;
    static constexpr int kPaddingX = 25;
    static constexpr int kPaddingY = 20;
    static constexpr int kButtonHeight = 30;

    class Listener {
      public:
        virtual ~Listener() = default;

        virtual void fileDeleted(File save_file) = 0;
    };

    DeleteSection(const String& name);
    virtual ~DeleteSection() = default;
    void resized() override;
    void setVisible(bool should_be_visible) override;

    void mouseUp(const MouseEvent& e) override;
    void buttonClicked(Button* clicked_button) override;

    void setFileToDelete(File file) {
      file_ = std::move(file); 
      preset_text_->setText(file_.getFileNameWithoutExtension());
    }

    Rectangle<int> getDeleteRect();

    void addDeleteListener(Listener* listener) { listeners_.add(listener); }
    void removeDeleteListener(Listener* listener) { listeners_.removeAllInstancesOf(listener); }

  private:
    File file_;

    OpenGlQuad body_;

    std::unique_ptr<PlainTextComponent> delete_text_;
    std::unique_ptr<PlainTextComponent> preset_text_;

    std::unique_ptr<OpenGlToggleButton> delete_button_;
    std::unique_ptr<OpenGlToggleButton> cancel_button_;

    Array<Listener*> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeleteSection)
};

