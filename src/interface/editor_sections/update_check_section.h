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

class UpdateMemory : public DeletedAtShutdown {
  public:
    UpdateMemory();
    virtual ~UpdateMemory();

    bool incrementChecker() {
      ScopedLock lock(lock_);
      bool should_check = checkers_ == 0;
      checkers_++;
      return should_check;
    }

    void decrementChecker() {
      ScopedLock lock(lock_);
      checkers_--;
    }

    JUCE_DECLARE_SINGLETON(UpdateMemory, false)

  private:
    CriticalSection lock_;
    int checkers_;
};

class UpdateCheckSection : public Overlay, public URL::DownloadTask::Listener {
  public:
    static constexpr int kUpdateCheckWidth = 340;
    static constexpr int kUpdateCheckHeight = 160;
    static constexpr int kPaddingX = 20;
    static constexpr int kPaddingY = 20;
    static constexpr int kButtonHeight = 30;

    class VersionRequestThread : public Thread {
      public:
        VersionRequestThread(UpdateCheckSection* ref) : Thread("Vial Download Thread"), ref_(ref) { }

        void run() override {
          ref_->checkUpdate();
        }

      private:
        UpdateCheckSection* ref_;
    };

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void needsUpdate() = 0;
    };

    UpdateCheckSection(String name);
    ~UpdateCheckSection() {
      version_request_.stopThread(350);
    }
    void resized() override;
    void setVisible(bool should_be_visible) override;
    void needsUpdate();

    void buttonClicked(Button* clicked_button) override;
    void mouseUp(const MouseEvent& e) override;

    void finished(URL::DownloadTask* task, bool success) override;
    void progress(URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override { }

    void startCheck() {
      version_request_.startThread();
    }
    void checkUpdate();
    void checkContentUpdate();

    Rectangle<int> getUpdateCheckRect();
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    void updateContent(String version);

    std::vector<Listener*> listeners_;

    VersionRequestThread version_request_;
    std::unique_ptr<URL::DownloadTask> download_task_;
    File version_file_;

    OpenGlQuad body_;
    std::unique_ptr<PlainTextComponent> notify_text_;
    std::unique_ptr<PlainTextComponent> version_text_;
    std::unique_ptr<OpenGlToggleButton> download_button_;
    std::unique_ptr<OpenGlToggleButton> nope_button_;

    String app_version_;
    String content_version_;
    bool content_update_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UpdateCheckSection)
};

