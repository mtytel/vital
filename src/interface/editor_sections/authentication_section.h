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

#if NDEBUG && !NO_AUTH

#include "JuceHeader.h"

#include "authentication.h"
#include "synth_button.h"
#include "overlay.h"
#include "open_gl_image_component.h"
#include "open_gl_multi_quad.h"

class AuthenticationSection;

class ForgotPasswordLink : public PlainTextComponent {
  public:
    ForgotPasswordLink() : PlainTextComponent("Forgot password?", "Forgot password?") {
      setInterceptsMouseClicks(true, false);
    }

    void mouseEnter(const MouseEvent& e) override {
      setColor(findColour(Skin::kWidgetAccent1, true).brighter(1.0f));
    }

    void mouseExit(const MouseEvent& e) override {
      setColor(findColour(Skin::kWidgetAccent1, true));
    }

    void mouseDown(const MouseEvent& e) override {
      URL url("");
      url.launchInDefaultBrowser();
    }
};

class AuthInitThread : public Thread {
  public:
    AuthInitThread(AuthenticationSection* ref) : Thread("Vial Auth Init Thread"), ref_(ref) { }
    virtual ~AuthInitThread() { }

    void run() override;

  private:
    AuthenticationSection* ref_;
};

class WorkOffline : public PlainTextComponent {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void workOffline() = 0;
    };

    WorkOffline() : PlainTextComponent("Work offline", "Work offline") {
      setInterceptsMouseClicks(true, false);
    }

    void mouseEnter(const MouseEvent& e) override {
      setColor(findColour(Skin::kWidgetAccent1, true).brighter(1.0f));
    }

    void mouseExit(const MouseEvent& e) override {
      setColor(findColour(Skin::kWidgetAccent1, true));
    }

    void mouseDown(const MouseEvent& e) override {
      for (Listener* listener: listeners_)
        listener->workOffline();
    }

    void addListener(Listener* listener) {
      listeners_.push_back(listener);
    }

  private:
    std::vector<Listener*> listeners_;
};

class AuthenticationSection : public Overlay, public TextEditor::Listener, public WorkOffline::Listener, public Timer {
  public:
    static constexpr int kWidth = 450;
    static constexpr int kHeight = 398;
    static constexpr int kY = 180;
    static constexpr int kPadding = 20;
    static constexpr int kTextHeight = 36;
    static constexpr int kImageWidth = 128;

    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void loggedIn() = 0;
    };

    AuthenticationSection(Authentication* auth);
    virtual ~AuthenticationSection();

    void init();
    void createAuth();
    void create();
    void checkAuth();
    Authentication* auth() { return auth_; }
  
    void timerCallback() override;
    void mouseUp(const MouseEvent& e) override;

    void paintBackground(Graphics& g) override { }
    void resized() override;
    void setVisible(bool should_be_visible) override;
    void visibilityChanged() override;
    void notifyLoggedIn();
    
    void textEditorReturnKeyPressed(TextEditor& editor) override {
      tryLogin();
    }
    
    void buttonClicked(Button* clicked_button) override {
      tryLogin();
    }

    std::string getSignedInName() {
      return signed_in_email_;
    }

    std::string getEmail() {
      return signed_in_email_;
    }

    void workOffline() override;
    void signOut();
    void setFocus();
    void setError(const std::string& error) { error_text_->setText(error); }
    void setButtonSettings(bool enabled, const std::string& text) {
      sign_in_button_->setEnabled(enabled);
      sign_in_button_->setText(text);
    }
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void finishLogin();

  private:
    void tryLogin();
    
    Authentication* auth_;
    std::vector<Listener*> listeners_;

    std::string signed_in_email_;
  
    OpenGlQuad body_;

    std::unique_ptr<AppLogo> logo_;
    std::unique_ptr<PlainTextComponent> sign_in_text_;
    std::unique_ptr<PlainTextComponent> error_text_;
    std::unique_ptr<OpenGlTextEditor> email_;
    std::unique_ptr<OpenGlTextEditor> password_;
    std::unique_ptr<OpenGlToggleButton> sign_in_button_;
    std::unique_ptr<ForgotPasswordLink> forgot_password_;
    std::unique_ptr<WorkOffline> work_offline_;
    std::unique_ptr<AuthInitThread> auth_init_thread_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuthenticationSection)
};

#else

class AuthenticationSection : public Component {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void loggedIn() = 0;
    };

    AuthenticationSection(Authentication* auth) { }

    std::string getSignedInName() { return ""; }
    void signOut() { }
    void create() { }
    void setFocus() { }
    void addListener(Listener* listener) { }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuthenticationSection)
};

#endif
