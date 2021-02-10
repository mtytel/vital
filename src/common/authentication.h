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
#include "load_save.h"

#if defined(__APPLE__)
#include <firebase/app.h>
#include <firebase/auth.h>
#else
#include "firebase/app.h"
#include "firebase/auth.h"
#endif

class Authentication {
  public:
    static void onTokenRefreshResult(const firebase::Future<std::string>& completed_future, void* ref_data) {
      const MessageManagerLock lock(Thread::getCurrentThread());
      if (!lock.lockWasGained())
        return;
      
      if (completed_future.status() != firebase::kFutureStatusComplete) {
        LoadSave::writeErrorLog("Firebase getting token error: not complete");
        return;
      }
      
      if (completed_future.error()) {
        std::string error = "Firebase getting token error: error code ";
        LoadSave::writeErrorLog(error + std::to_string(completed_future.error()));
        return;
      }

      Authentication* reference = (Authentication*)ref_data;
      reference->setToken(*completed_future.result());
    }

    static void create() {
      if (firebase::App::GetInstance() != nullptr)
        return;

      firebase::AppOptions auth_app_options = firebase::AppOptions();
      auth_app_options.set_app_id("");
      auth_app_options.set_api_key("");
      auth_app_options.set_project_id("");
      
      firebase::App::Create(auth_app_options);
    }

    Authentication() : auth_(nullptr) { }

    void init() {
      if (auth_ == nullptr)
        auth_ = firebase::auth::Auth::GetAuth(firebase::App::GetInstance());
    }

    bool hasAuth() const { return auth_ != nullptr; }
    firebase::auth::Auth* auth() const { return auth_; }
    void setToken(const std::string& token) { token_ = token; }
    std::string token() const { return token_; }
    bool loggedIn() { return auth_ && auth_->current_user() != nullptr; }

    void refreshToken() {
      if (auth_ == nullptr || auth_->current_user() == nullptr)
        return;

      firebase::Future<std::string> future = auth_->current_user()->GetToken(false);
      future.OnCompletion(onTokenRefreshResult, this);
    }

  private:
    firebase::auth::Auth* auth_;
    std::string token_;
};

#else

class Authentication {
  public:
    static void create() { }

    std::string token() { return ""; }
    bool loggedIn() { return false; }
    void refreshToken() { }
};

#endif
