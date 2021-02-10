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

#include "authentication_section.h"

#include "authentication.h"
#include "load_save.h"
#include "fonts.h"
#include "skin.h"

#if NDEBUG && !NO_AUTH

namespace {
  
  constexpr int kMsAuthPoll = 1000;

  void onTokenResult(const firebase::Future<std::string>& completed_future, void* ref_data) {
    const MessageManagerLock lock(Thread::getCurrentThread());
    if (!lock.lockWasGained())
      return;
    
    Component::SafePointer<AuthenticationSection>* reference = (Component::SafePointer<AuthenticationSection>*)ref_data;
    
    if (completed_future.status() != firebase::kFutureStatusComplete) {
      LoadSave::writeErrorLog("Firebase getting initial token error: not complete");
      delete reference;
      return;
    }
    
    if (completed_future.error()) {
      std::string error = "Firebase getting initial token error: error code ";
      LoadSave::writeErrorLog(error + std::to_string(completed_future.error()));
      delete reference;
      return;
    }

    if (reference->getComponent()) {
      reference->getComponent()->auth()->setToken(*completed_future.result());
      reference->getComponent()->notifyLoggedIn();
    }

    delete reference;
  }

  void onLoginResult(const firebase::Future<firebase::auth::User*>& completed_future, void* ref_data) {
    const MessageManagerLock lock(Thread::getCurrentThread());
    if (!lock.lockWasGained())
      return;
    
    Component::SafePointer<AuthenticationSection>* reference = (Component::SafePointer<AuthenticationSection>*)ref_data;
    
    if (completed_future.status() != firebase::kFutureStatusComplete) {
      LoadSave::writeErrorLog("Firebase login error: not complete");
      delete reference;
      return;
    }
    
    if (completed_future.error()) {
      std::string error = "Firebase login error: error code ";
      LoadSave::writeErrorLog(error + std::to_string(completed_future.error()));
      delete reference;
      return;
    }

    if (reference->getComponent()) {
      if (completed_future.error())
        reference->getComponent()->setError(completed_future.error_message());
      else {
        firebase::Future<std::string> future = (*completed_future.result())->GetToken(false);
        Component::SafePointer<AuthenticationSection>* ref =
            new Component::SafePointer<AuthenticationSection>(reference->getComponent());
        future.OnCompletion(onTokenResult, ref);

        LoadSave::saveAuthenticated(true);
        reference->getComponent()->finishLogin();
      }

      reference->getComponent()->setButtonSettings(true, "Sign in");
    }

    delete reference;
  }
}

void AuthInitThread::run() {
  ref_->createAuth();
}

AuthenticationSection::AuthenticationSection(Authentication* auth) : Overlay("Auth"), auth_(auth),
                                                                     body_(Shaders::kRoundedRectangleFragment) {
  addOpenGlComponent(&body_);

  logo_ = std::make_unique<AppLogo>("logo");
  addOpenGlComponent(logo_.get());
                                                   
  sign_in_text_ = std::make_unique<PlainTextComponent>("Sign in", "Sign in");
  addOpenGlComponent(sign_in_text_.get());
  sign_in_text_->setFontType(PlainTextComponent::kLight);
  sign_in_text_->setTextSize(kTextHeight * size_ratio_ * 0.6f);
  sign_in_text_->setJustification(Justification::centred);

  error_text_ = std::make_unique<PlainTextComponent>("Error", "");
  addOpenGlComponent(error_text_.get());
  error_text_->setFontType(PlainTextComponent::kLight);
  error_text_->setTextSize(kTextHeight * size_ratio_ * 0.4f);
  error_text_->setJustification(Justification::centredRight);

#if !defined(NO_TEXT_ENTRY)
  email_ = std::make_unique<OpenGlTextEditor>("Email");
  email_->addListener(this);
  addAndMakeVisible(email_.get());
  addOpenGlComponent(email_->getImageComponent());
  
  password_ = std::make_unique<OpenGlTextEditor>("Password", 0x2022);
  password_->addListener(this);
  addAndMakeVisible(password_.get());
  addOpenGlComponent(password_->getImageComponent());
#endif
  
  sign_in_button_ = std::make_unique<OpenGlToggleButton>("Sign in");
  sign_in_button_->setText("Sign in");
  sign_in_button_->setUiButton(true);
  sign_in_button_->addListener(this);
  addAndMakeVisible(sign_in_button_.get());
  addOpenGlComponent(sign_in_button_->getGlComponent());

  forgot_password_ = std::make_unique<ForgotPasswordLink>();
  addOpenGlComponent(forgot_password_.get());
  forgot_password_->setFontType(PlainTextComponent::kLight);
  forgot_password_->setTextSize(kTextHeight * size_ratio_ * 0.4f);
  forgot_password_->setJustification(Justification::centredLeft);

  work_offline_ = std::make_unique<WorkOffline>();
  work_offline_->addListener(this);
  addOpenGlComponent(work_offline_.get());
  work_offline_->setFontType(PlainTextComponent::kLight);
  work_offline_->setTextSize(kTextHeight * size_ratio_ * 0.4f);
  work_offline_->setJustification(Justification::centredRight);

  setWantsKeyboardFocus(true);
  setSkinOverride(Skin::kOverlay);
}

AuthenticationSection::~AuthenticationSection() = default;

void AuthenticationSection::init() {
  if (firebase::App::GetInstance() == nullptr) {
    startTimer(kMsAuthPoll);
    return;
  }
}

void AuthenticationSection::create() {
  Authentication::create();
  createAuth();
  // TODO: Putting this on background thread hangs sometimes.
  // auth_init_thread_ = std::make_unique<AuthInitThread>(this);
  // auth_init_thread_->startThread();
}

void AuthenticationSection::createAuth() {
  auth_->init();
  checkAuth();
}

void AuthenticationSection::checkAuth() {
  if (!auth_->hasAuth())
    return;

  firebase::auth::User* user = auth_->auth()->current_user();
  if (user) {
    firebase::Future<std::string> future = user->GetToken(true);
    SafePointer<AuthenticationSection>* self_reference = new SafePointer<AuthenticationSection>(this);
    future.OnCompletion(onTokenResult, self_reference);
  }
  
  const MessageManagerLock lock(Thread::getCurrentThread());
  if (!lock.lockWasGained())
    return;
  
  setVisible(user == nullptr);
  if (user) {
    signed_in_email_ = user->email();
    LoadSave::saveAuthenticated(true);
  }
  else
    startTimer(kMsAuthPoll);
}

void AuthenticationSection::timerCallback() {
  if (!auth_->hasAuth())
    init();
  else if (isVisible())
    checkAuth();
  else
    stopTimer();
}

void AuthenticationSection::mouseUp(const MouseEvent &e) {
  if (!body_.getBounds().contains(e.getPosition()))
    setVisible(false);
}

void AuthenticationSection::resized() {
  Overlay::resized();

  Path border;

  int width = kWidth * size_ratio_;
  int height = kHeight * size_ratio_;
  int top = kY * size_ratio_;
  int padding = kPadding * size_ratio_;
  int logo_width = kImageWidth * size_ratio_;
  int text_height = kTextHeight * size_ratio_;

  int text_width = width - 2 * padding;
  int text_x = (getWidth() - text_width) / 2;
  int error_height = text_height * 0.5;
  int y = top + height - 2 * padding - 4 * text_height - error_height;
  if (email_ && password_) {
    Colour caret = findColour(Skin::kTextEditorCaret, true);
    Colour body_text = findColour(Skin::kTextEditorCaret, true);
    Colour selection = findColour(Skin::kTextEditorSelection, true);
    Colour empty_color = body_text.withMultipliedAlpha(0.5f);
    
    email_->setTextToShowWhenEmpty(TRANS("Email"), empty_color);
    password_->setTextToShowWhenEmpty(TRANS("Password"), empty_color);
    
    email_->setColour(CaretComponent::caretColourId, caret);
    email_->setColour(TextEditor::textColourId, body_text);
    email_->setColour(TextEditor::highlightedTextColourId, body_text);
    email_->setColour(TextEditor::highlightColourId, selection);
    password_->setColour(CaretComponent::caretColourId, caret);
    password_->setColour(TextEditor::textColourId, body_text);
    password_->setColour(TextEditor::highlightedTextColourId, body_text);
    password_->setColour(TextEditor::highlightColourId, selection);
    
    email_->setBounds(text_x, y, text_width, text_height);
    password_->setBounds(text_x, y + 1.25f * text_height, text_width, text_height);
  }

  int image_x = (getWidth() - logo_width) / 2;
  int image_y = top + padding;
  logo_->setBounds(image_x, image_y, logo_width, logo_width);
  
  Colour text_color = findColour(Skin::kBodyText, true);
  sign_in_text_->setColor(text_color);
  sign_in_text_->setBounds(text_x, image_y + logo_width, text_width, text_height);
  sign_in_text_->setTextSize(kTextHeight * size_ratio_ * 0.6f);
  sign_in_button_->setBounds(text_x, y + 3 * text_height + error_height + padding, text_width, text_height);

  forgot_password_->setColor(findColour(Skin::kWidgetAccent1, true));
  forgot_password_->setBounds(password_->getX(), password_->getBottom(), password_->getWidth() / 2, text_height);
  forgot_password_->setTextSize(kTextHeight * size_ratio_ * 0.4f);

  work_offline_->setColor(findColour(Skin::kWidgetAccent1, true));
  work_offline_->setBounds(forgot_password_->getRight(), forgot_password_->getY(),
                           password_->getRight() - forgot_password_->getRight(), text_height);
  work_offline_->setTextSize(kTextHeight * size_ratio_ * 0.4f);

  error_text_->setColor(text_color.interpolatedWith(Colours::red, 0.5f));
  error_text_->setBounds(password_->getX(), forgot_password_->getBottom(), password_->getWidth(), error_height);
  error_text_->setTextSize(kTextHeight * size_ratio_ * 0.4f);

  body_.setBounds((getWidth() - width) / 2, top, width, height);
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));

  if (isVisible()) {
    Image image(Image::ARGB, 1, 1, false);
    Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }
}

void AuthenticationSection::setVisible(bool should_be_visible) {
  Overlay::setVisible(should_be_visible);

  if (should_be_visible) {
    Image image(Image::ARGB, 1, 1, false);
    Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }
}

void AuthenticationSection::visibilityChanged() {
  Component::visibilityChanged();
  if (isShowing() && email_ && email_->isEmpty())
    email_->grabKeyboardFocus();
  
  Image image(Image::ARGB, 1, 1, false);
  Graphics g(image);
  paintOpenGlChildrenBackgrounds(g);
}

void AuthenticationSection::workOffline() {
  setVisible(false);
  LoadSave::saveWorkOffline(true);
}

void AuthenticationSection::signOut() {
  auth_->auth()->SignOut();
  setVisible(true);
  startTimer(kMsAuthPoll);
}

void AuthenticationSection::notifyLoggedIn() {
  for (Listener* listener : listeners_)
    listener->loggedIn();
}

void AuthenticationSection::setFocus() {
  if (isShowing() && email_ && email_->isEmpty())
    email_->grabKeyboardFocus();
}

void AuthenticationSection::finishLogin() {
  setVisible(false);
  for (Listener* listener : listeners_)
    listener->loggedIn();
}

void AuthenticationSection::tryLogin() {
  LoadSave::saveWorkOffline(false);

  if (!auth_->hasAuth()) {
    setVisible(false);
    return;
  }

  setError("");
  setButtonSettings(false, "Signing in...");
  signed_in_email_ = email_->getText().toStdString();
  std::string password = password_->getText().toStdString();
  firebase::Future<firebase::auth::User*> future =
      auth_->auth()->SignInWithEmailAndPassword(signed_in_email_.c_str(), password.c_str());

  SafePointer<AuthenticationSection>* self_reference = new SafePointer<AuthenticationSection>(this);
  future.OnCompletion(onLoginResult, self_reference);
}

#endif
