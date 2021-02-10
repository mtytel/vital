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

#include <climits>

class AudioFileDropSource : public FileDragAndDropTarget {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void audioFileLoaded(const File& file) = 0;
    };

    AudioFileDropSource() {
      format_manager_.registerBasicFormats();
    }

    bool isInterestedInFileDrag(const StringArray& files) override {
      if (files.size() != 1)
        return false;

      String file = files[0];
      StringArray wildcards;
      wildcards.addTokens(getExtensions(), ";", "\"");
      for (const String& wildcard : wildcards) {
        if (file.matchesWildcard(wildcard, true))
          return true;
      }
      return false;
    }

    void filesDropped(const StringArray& files, int x, int y) override {
      if (files.size() == 0)
        return;

      File file(files[0]);
      audioFileLoaded(file);
      for (Listener* listener : listeners_)
        listener->audioFileLoaded(file);
    }

    virtual void audioFileLoaded(const File& file) = 0;
    void addListener(Listener* listener) { listeners_.push_back(listener); }

    String getExtensions() { return format_manager_.getWildcardForAllFormats(); }

    AudioFormatManager& formatManager() { return format_manager_; }

  protected:
    AudioFormatManager format_manager_;

  private:
    std::vector<Listener*> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFileDropSource)
};

