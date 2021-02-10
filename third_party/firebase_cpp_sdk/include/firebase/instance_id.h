// Copyright 2017 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FIREBASE_INSTANCE_ID_CLIENT_CPP_SRC_INCLUDE_FIREBASE_INSTANCE_ID_H_
#define FIREBASE_INSTANCE_ID_CLIENT_CPP_SRC_INCLUDE_FIREBASE_INSTANCE_ID_H_

#include <cstdint>
#include <string>

#include "firebase/app.h"
#include "firebase/future.h"
#include "firebase/internal/common.h"

FIREBASE_APP_REGISTER_CALLBACKS_REFERENCE(instance_id)

/// @brief Namespace that encompasses all Firebase APIs.
namespace firebase {

namespace instance_id {

/// InstanceId error codes.
enum Error {
  kErrorNone = 0,
  /// An unknown error occurred.
  kErrorUnknown,
  /// Request could not be validated from this client.
  kErrorAuthentication,
  /// Instance ID service cannot be accessed.
  kErrorNoAccess,
  /// Request to instance ID backend timed out.
  kErrorTimeout,
  /// No network available to reach the servers.
  kErrorNetwork,
  /// A similar operation is in progress so aborting this one.
  kErrorOperationInProgress,
  /// Some of the parameters of the request were invalid.
  kErrorInvalidRequest,
  // ID is invalid and should be reset.
  kErrorIdInvalid,
};

#if defined(INTERNAL_EXPERIMENTAL)
// TODO(b/69930393): Unfortunately, the Android implementation uses a service
// to notify the application of token refresh events.  It's a load of work
// to setup a service to forward token change events to the application so
// - in order to unblock A/B testing - we'll add support later.

/// @brief Can be registered by an application for notifications when an app's
/// instance ID changes.
class InstanceIdListener {
 public:
  InstanceIdListener();
  virtual ~InstanceIdListener();

  /// @brief Called when the system determines that the tokens need to be
  /// refreshed. The application should call GetToken and send the tokens to
  /// all application servers.
  ///
  /// This will not be called very frequently, it is needed for key rotation
  /// and to handle Instance ID changes due to:
  /// <ul>
  ///   <li>App deletes Instance ID
  ///   <li>App is restored on a new device
  ///   <li>User uninstalls/reinstall the app
  ///   <li>User clears app data
  /// </ul>
  ///
  /// The system will throttle the refresh event across all devices to
  /// avoid overloading application servers with token updates.
  virtual void onTokenRefresh() = 0;
};
#endif  // defined(INTERNAL_EXPERIMENTAL)

#if defined(INTERNAL_EXPERIMENTAL)
// Expose private members of InstanceId to the unit tests.
// See FRIEND_TEST() macro in gtest.h for the naming convention here.
class InstanceIdTest_TestGetTokenEntityScope_Test;
class InstanceIdTest_TestDeleteTokenEntityScope_Test;
#endif  // defined(INTERNAL_EXPERIMENTAL)

#if !defined(DOXYGEN)
namespace internal {
// Implementation specific data for an InstanceId.
class InstanceIdInternal;
}  // namespace internal
#endif  // !defined(DOXYGEN)

/// @brief Instance ID provides a unique identifier for each app instance and
/// a mechanism to authenticate and authorize actions (for example, sending a
/// FCM message).
///
/// An Instance ID is long lived, but might be reset if the device is not used
/// for a long time or the Instance ID service detects a problem.  If the
/// Instance ID has become invalid, the app can request a new one and send it to
/// the app server.  To prove ownership of Instance ID and to allow servers to
/// access data or services associated with the app, call GetToken.
///
/// @if INTERNAL_EXPERIMENTAL
/// If an Instance ID is reset, the app will be notified via InstanceIdListener.
/// @endif
class InstanceId {
 public:
  ~InstanceId();

  /// @brief Gets the App this object is connected to.
  ///
  /// @returns App this object is connected to.
  App& app() const { return *app_; }

#if defined(INTERNAL_EXPERIMENTAL)
  // TODO(b/69932424): Blocked by iOS implementation.

  /// @brief Get the time the instance ID was created.
  ///
  /// @returns Time (in milliseconds since the epoch) when the instance ID was
  /// created.
  int64_t creation_time() const;
#endif  // defined(INTERNAL_EXPERIMENTAL)

  /// @brief Returns a stable identifier that uniquely identifies the app
  /// instance.
  ///
  /// @returns Unique identifier for the app instance.
  Future<std::string> GetId() const;

  /// @brief Get the results of the most recent call to @ref GetId.
  Future<std::string> GetIdLastResult() const;

  /// @brief Delete the ID associated with the app, revoke all tokens and
  /// allocate a new ID.
  Future<void> DeleteId();

  /// @brief Get the results of the most recent call to @ref DeleteId.
  Future<void> DeleteIdLastResult() const;

  /// @brief Returns a token that authorizes an Entity to perform an action on
  /// behalf of the application identified by Instance ID.
  ///
  /// This is similar to an OAuth2 token except, it applies to the
  /// application instance instead of a user.
  ///
  /// For example, to get a token that can be used to send messages to an
  /// application via Firebase Messaging, set entity to the
  /// sender ID, and set scope to "FCM".
  ///
  /// @returns A token that can identify and authorize the instance of the
  /// application on the device.
  Future<std::string> GetToken();

  /// @brief Get the results of the most recent call to @ref GetToken.
  Future<std::string> GetTokenLastResult() const;

  /// @brief Revokes access to a scope (action)
  Future<void> DeleteToken();

  /// @brief Get the results of the most recent call to @ref DeleteToken.
  Future<void> DeleteTokenLastResult() const;

  /// @brief Returns the InstanceId object for an App creating the InstanceId
  /// if required.
  ///
  /// @param[in] app The App to create an InstanceId object from.
  /// On <b>iOS</b> this must be the default Firebase App.
  /// @param[out] init_result_out Optional: If provided, write the init result
  /// here. Will be set to kInitResultSuccess if initialization succeeded, or
  /// kInitResultFailedMissingDependency on Android if Google Play services is
  /// not available on the current device.
  ///
  /// @returns InstanceId object if successful, nullptr otherwise.
  static InstanceId* GetInstanceId(App* app,
                                   InitResult* init_result_out = nullptr);

#if defined(INTERNAL_EXPERIMENTAL)
  // TODO(b/69930393): Needs to be implemented on Android.

  /// @brief Set a listener for instance ID changes.
  ///
  /// @param listener Listener which is notified when instance ID changes.
  ///
  /// @returns Previously registered listener.
  static InstanceIdListener* SetListener(InstanceIdListener* listener);

  // Delete the instance_id_internal_.
  void DeleteInternal();
#endif  // defined(INTERNAL_EXPERIMENTAL)

 private:
  InstanceId(App* app, internal::InstanceIdInternal* instance_id_internal);

#if defined(INTERNAL_EXPERIMENTAL)
  // Can access GetToken() and DeleteToken() methods for testing.
  friend class InstanceIdTest_TestGetTokenEntityScope_Test;
  friend class InstanceIdTest_TestDeleteTokenEntityScope_Test;

  /// @brief Returns a token that authorizes an Entity to perform an action on
  /// behalf of the application identified by Instance ID.
  ///
  /// This is similar to an OAuth2 token except, it applies to the
  /// application instance instead of a user.
  ///
  /// For example, to get a token that can be used to send messages to an
  /// application via Firebase Messaging, set entity to the
  /// sender ID, and set scope to "FCM".
  ///
  /// @param entity Entity authorized by the token.
  /// @param scope Action authorized for entity.
  ///
  /// @returns A token that can identify and authorize the instance of the
  /// application on the device.
  Future<std::string> GetToken(const char* entity, const char* scope);

  /// @brief Revokes access to a scope (action)
  ///
  /// @param entity entity Entity that must no longer have access.
  /// @param scope Action that entity is no longer authorized to perform.
  Future<void> DeleteToken(const char* entity, const char* scope);
#endif  // defined(INTERNAL_EXPERIMENTAL)

 private:
  App* app_;
  internal::InstanceIdInternal* instance_id_internal_;
};

}  // namespace instance_id

}  // namespace firebase

#endif  // FIREBASE_INSTANCE_ID_CLIENT_CPP_SRC_INCLUDE_FIREBASE_INSTANCE_ID_H_
