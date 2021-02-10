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

class Authentication;

class DownloadSection : public Overlay, public URL::DownloadTask::Listener, public Timer {
  public:
    static const std::string kFactoryDownloadPath;
    static constexpr int kY = 180;
    static constexpr int kDownloadWidth = 450;
    static constexpr int kDownloadInitialHeight = 380;
    static constexpr int kDownloadAdditionalHeight = 324;
    static constexpr int kTextHeight = 15;
    static constexpr int kPaddingX = 20;
    static constexpr int kPaddingY = 20;
    static constexpr int kButtonHeight = 36;
    static constexpr int kCompletionWaitMs = 1000;

    struct DownloadPack {
      DownloadPack(std::string n, std::string a, int i, URL u, File d) :
        name(std::move(n)), author(std::move(a)), id(i), url(u), download_location(d), finished(false) { }
      std::string name;
      std::string author;
      int id;
      URL url;
      File download_location;
      bool finished;
    };

    class Listener {
      public:
        virtual ~Listener() = default;

        virtual void dataDirectoryChanged() = 0;
        virtual void noDownloadNeeded() = 0;
    };

    class DownloadThread : public Thread {
      public:
        DownloadThread(DownloadSection* ref, URL url, File dest) :
            Thread("Vial Download Thread"), ref_(ref), url_(std::move(url)), dest_(std::move(dest)) { }
        virtual ~DownloadThread() { }

        void run() override {
          ref_->startDownload(this, url_, dest_);
        }

      private:
        DownloadSection* ref_;
        URL url_;
        File dest_;
    };

    class InstallThread : public Thread {
      public:
        InstallThread(DownloadSection* ref) : Thread("Vial Install Thread"), ref_(ref) { }
        virtual ~InstallThread() { }

        void run() override {
          ref_->startInstall(this);
        }

      private:
        DownloadSection* ref_;
    };

    DownloadSection(String name, Authentication* auth);
    virtual ~DownloadSection() {
      for (auto& thread : download_threads_)
        thread->stopThread(300);
    }

    void resized() override;
    void setVisible(bool should_be_visible) override;
    void timerCallback() override {
      stopTimer();
      setVisible(false);
    }

    void mouseUp(const MouseEvent& e) override;
    void buttonClicked(Button* clicked_button) override;

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;

    void finished(URL::DownloadTask* task, bool success) override;
    void progress(URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override;

    Rectangle<int> getDownloadRect();
    void triggerDownload();
    void triggerInstall();
    void startDownload(Thread* thread, URL& url, const File& dest);
    void startInstall(Thread* thread);
    void cancelDownload();
    void chooseInstallFolder();
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    Authentication* auth_;
    OpenGlQuad body_;
    bool cancel_;
    bool initial_download_;
    float progress_value_;
    OpenGlQuad download_progress_;
    OpenGlQuad download_background_;
    OpenGlQuad install_text_background_;
    std::unique_ptr<AppLogo> logo_;
    std::unique_ptr<LoadingWheel> loading_wheel_;

    std::vector<std::unique_ptr<DownloadThread>> download_threads_;
    InstallThread install_thread_;

    URL packs_url_;
    URL factory_download_url_;
    File available_packs_location_;
    std::vector<DownloadPack> awaiting_install_;
    std::vector<DownloadPack> awaiting_download_;
    std::vector<std::unique_ptr<URL::DownloadTask>> download_tasks_;
    File install_location_;
    std::vector<Listener*> listeners_;

    std::unique_ptr<OpenGlShapeButton> folder_button_;
    std::unique_ptr<PlainTextComponent> download_text_;
    std::unique_ptr<PlainTextComponent> install_location_text_;
    std::unique_ptr<OpenGlToggleButton> install_button_;
    std::unique_ptr<OpenGlToggleButton> cancel_button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DownloadSection)
};

