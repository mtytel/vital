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
#include "open_gl_image_component.h"

class OpenGlHyperlink;

class ExpiredSection : public Overlay {
  public:
    static constexpr int kExpiredWidth = 340;
    static constexpr int kExpiredHeight = 85;
    static constexpr int kPaddingX = 25;
    static constexpr int kPaddingY = 20;
    static constexpr int kButtonHeight = 30;

    ExpiredSection(String name);
    virtual ~ExpiredSection() = default;

    void resized() override;
    void setVisible(bool should_be_visible) override;

    Rectangle<int> getExpiredRect();

  private:
    OpenGlQuad body_;
    std::unique_ptr<PlainTextComponent> text_;
    std::unique_ptr<OpenGlHyperlink> link_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpiredSection)
};

