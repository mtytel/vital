/*
 * Copyright 2016 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_H_
#define FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "firebase/app.h"
#include "firebase/future.h"
#include "firebase/internal/common.h"
#include "firebase/variant.h"

FIREBASE_APP_REGISTER_CALLBACKS_REFERENCE(analytics)

/// @brief Namespace that encompasses all Firebase APIs.
namespace firebase {

/// @brief Firebase Analytics API.
///
/// See <a href="/docs/analytics">the developer guides</a> for general
/// information on using Firebase Analytics in your apps.
namespace analytics {

/// @brief Event parameter.
///
/// Parameters supply information that contextualize events (see @ref LogEvent).
/// You can associate up to 25 unique Parameters with each event type (name).
///
/// @if cpp_examples
/// Common event types (names) are suggested in @ref event_names
/// (%event_names.h) with parameters of common event types defined in
/// @ref parameter_names (%parameter_names.h).
///
/// You are not limited to the set of event types and parameter names suggested
/// in @ref event_names (%event_names.h) and  %parameter_names.h respectively.
/// Additional Parameters can be supplied for suggested event types or custom
/// Parameters for custom event types.
/// @endif
///
/// Parameter names must be a combination of letters and digits
/// (matching the regular expression [a-zA-Z0-9]) between 1 and 40 characters
/// long starting with a letter [a-zA-Z] character.  The "firebase_",
/// "google_" and "ga_" prefixes are reserved and should not be used.
///
/// Parameter string values can be up to 100 characters long.
///
/// @if cpp_examples
/// An array of this structure is passed to LogEvent in order to associate
/// parameter's of an event (Parameter::name) with values (Parameter::value)
/// where each value can be a double, 64-bit integer or string.
/// @endif
///
/// For example, a game may log an achievement event along with the
/// character the player is using and the level they're currently on:
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// int64_t current_level = GetCurrentLevel();
/// const Parameter achievement_parameters[] = {
///   Parameter(kParameterAchievementID,  "ultimate_wizard"),
///   Parameter(kParameterCharacter, "mysterion"),
///   Parameter(kParameterLevel, current_level),
/// };
/// LogEvent(kEventUnlockAchievement, achievement_parameters,
///          sizeof(achievement_parameters) /
///          sizeof(achievement_parameters[0]));
/// @endcode
/// @endif
///
struct Parameter {
  /// Construct an empty parameter.
  ///
  /// This is provided to allow initialization after construction.
  Parameter() : name(nullptr) {}

  /// Construct a parameter.
  ///
  /// @param parameter_name Name of the parameter (see Parameter::name).
  /// @param parameter_value Value for the parameter. Variants can
  /// hold numbers and strings.
  Parameter(const char* parameter_name, Variant parameter_value)
      : name(parameter_name) {
    value = parameter_value;
  }

  /// Construct a 64-bit integer parameter.
  ///
  /// @param parameter_name Name of the parameter.
  /// @if cpp_examples
  /// (see Parameter::name).
  /// @endif
  /// @param parameter_value Integer value for the parameter.
  Parameter(const char* parameter_name, int parameter_value)
      : name(parameter_name) {
    value = parameter_value;
  }

  /// Construct a 64-bit integer parameter.
  ///
  /// @param parameter_name Name of the parameter.
  /// @if cpp_examples
  /// (see Parameter::name).
  /// @endif
  /// @param parameter_value Integer value for the parameter.
  Parameter(const char* parameter_name, int64_t parameter_value)
      : name(parameter_name) {
    value = parameter_value;
  }

  /// Construct a floating point parameter.
  ///
  /// @param parameter_name Name of the parameter.
  /// @if cpp_examples
  /// (see Parameter::name).
  /// @endif
  /// @param parameter_value Floating point value for the parameter.
  Parameter(const char* parameter_name, double parameter_value)
      : name(parameter_name) {
    value = parameter_value;
  }

  /// Construct a string parameter.
  ///
  /// @param parameter_name Name of the parameter.
  /// @if cpp_examples
  /// (see Parameter::name).
  /// @endif
  /// @param parameter_value String value for the parameter, can be up to 100
  /// characters long.
  Parameter(const char* parameter_name, const char* parameter_value)
      : name(parameter_name) {
    value = parameter_value;
  }


  /// @brief Name of the parameter.
  ///
  /// Parameter names must be a combination of letters and digits
  /// (matching the regular expression [a-zA-Z0-9]) between 1 and 40 characters
  /// long starting with a letter [a-zA-Z] character.  The "firebase_",
  /// "google_" and "ga_" prefixes are reserved and should not be used.
  const char* name;
  /// @brief Value of the parameter.
  ///
  /// See firebase::Variant for usage information.
  /// @note String values can be up to 100 characters long.
  Variant value;
};

/// @brief Initialize the Analytics API.
///
/// This must be called prior to calling any other methods in the
/// firebase::analytics namespace.
///
/// @param[in] app Default @ref firebase::App instance.
///
/// @see firebase::App::GetInstance().
void Initialize(const App& app);

/// @brief Terminate the Analytics API.
///
/// Cleans up resources associated with the API.
void Terminate();

/// @brief Sets whether analytics collection is enabled for this app on this
/// device.
///
/// This setting is persisted across app sessions. By default it is enabled.
///
/// @param[in] enabled true to enable analytics collection, false to disable.
void SetAnalyticsCollectionEnabled(bool enabled);

/// @brief Log an event with one string parameter.
///
/// @param[in] name Name of the event to log. Should contain 1 to 40
/// alphanumeric characters or underscores. The name must start with an
/// alphabetic character. Some event names are reserved.
/// @if cpp_examples
/// See @ref event_names (%event_names.h) for the list of reserved event names.
/// @endif
/// The "firebase_" prefix is reserved and should not be used. Note that event
/// names are case-sensitive and that logging two events whose names differ
/// only in case will result in two distinct events.
/// @param[in] parameter_name Name of the parameter to log.
/// For more information, see @ref Parameter.
/// @param[in] parameter_value Value of the parameter to log.
///
/// @if cpp_examples
/// @see LogEvent(const char*, const Parameter*, size_t)
/// @endif
void LogEvent(const char* name, const char* parameter_name,
              const char* parameter_value);

/// @brief Log an event with one float parameter.
///
/// @param[in] name Name of the event to log. Should contain 1 to 40
/// alphanumeric characters or underscores. The name must start with an
/// alphabetic character. Some event names are reserved.
/// @if cpp_examples
/// See @ref event_names (%event_names.h) for the list of reserved event names.
/// @endif
/// The "firebase_" prefix is reserved and should not be used. Note that event
/// names are case-sensitive and that logging two events whose names differ
/// only in case will result in two distinct events.
/// @param[in] parameter_name Name of the parameter to log.
/// For more information, see @ref Parameter.
/// @param[in] parameter_value Value of the parameter to log.
///
/// @if cpp_examples
/// @see LogEvent(const char*, const Parameter*, size_t)
/// @endif
void LogEvent(const char* name, const char* parameter_name,
              const double parameter_value);

/// @brief Log an event with one 64-bit integer parameter.
///
/// @param[in] name Name of the event to log. Should contain 1 to 40
/// alphanumeric characters or underscores. The name must start with an
/// alphabetic character. Some event names are reserved.
/// @if cpp_examples
/// See @ref event_names (%event_names.h) for the list of reserved event names.
/// @endif
/// The "firebase_" prefix is reserved and should not be used. Note that event
/// names are case-sensitive and that logging two events whose names differ
/// only in case will result in two distinct events.
/// @param[in] parameter_name Name of the parameter to log.
/// For more information, see @ref Parameter.
/// @param[in] parameter_value Value of the parameter to log.
///
/// @if cpp_examples
/// @see LogEvent(const char*, const Parameter*, size_t)
/// @endif
void LogEvent(const char* name, const char* parameter_name,
              const int64_t parameter_value);

/// @brief Log an event with one integer parameter
/// (stored as a 64-bit integer).
///
/// @param[in] name Name of the event to log. Should contain 1 to 40
/// alphanumeric characters or underscores. The name must start with an
/// alphabetic character. Some event names are reserved.
/// @if cpp_examples
/// See @ref event_names (%event_names.h) for the list of reserved event names.
/// @endif
/// The "firebase_" prefix is reserved and should not be used. Note that event
/// names are case-sensitive and that logging two events whose names differ
/// only in case will result in two distinct events.
/// @param[in] parameter_name Name of the parameter to log.
/// For more information, see @ref Parameter.
/// @param[in] parameter_value Value of the parameter to log.
///
/// @if cpp_examples
/// @see LogEvent(const char*, const Parameter*, size_t)
/// @endif
void LogEvent(const char* name, const char* parameter_name,
              const int parameter_value);

/// @brief Log an event with no parameters.
///
/// @param[in] name Name of the event to log. Should contain 1 to 40
/// alphanumeric characters or underscores. The name must start with an
/// alphabetic character. Some event names are reserved.
/// @if cpp_examples
/// See @ref event_names (%event_names.h) for the list of reserved event names.
/// @endif
/// The "firebase_" prefix is reserved and should not be used. Note that event
/// names are case-sensitive and that logging two events whose names differ
/// only in case will result in two distinct events.
///
/// @if cpp_examples
/// @see LogEvent(const char*, const Parameter*, size_t)
/// @endif
void LogEvent(const char* name);

// clang-format off

/// @brief Log an event with associated parameters.
///
/// An Event is an important occurrence in your app that you want to
/// measure.  You can report up to 500 different types of events per app and
/// you can associate up to 25 unique parameters with each Event type.
///
/// Some common events are documented in @ref event_names (%event_names.h),
/// but you may also choose to specify custom event types that are associated
/// with your specific app.
///
/// @param[in] name Name of the event to log. Should contain 1 to 40
/// alphanumeric characters or underscores. The name must start with an
/// alphabetic character. Some event names are reserved. See @ref event_names
/// (%event_names.h) for the list of reserved event names. The "firebase_"
/// prefix is reserved and should not be used. Note that event names are
/// case-sensitive and that logging two events whose names differ only in
/// case will result in two distinct events.
/// @param[in] parameters Array of Parameter structures.
/// @param[in] number_of_parameters Number of elements in the parameters
/// array.
void LogEvent(const char* name, const Parameter* parameters,
              size_t number_of_parameters);
// clang-format on

/// @brief Set a user property to the given value.
///
/// Properties associated with a user allow a developer to segment users
/// into groups that are useful to their application.  Up to 25 properties
/// can be associated with a user.
///
/// Suggested property names are listed @ref user_property_names
/// (%user_property_names.h) but you're not limited to this set. For example,
/// the "gamertype" property could be used to store the type of player where
/// a range of values could be "casual", "mid_core", or "core".
///
/// @param[in] name Name of the user property to set.  This must be a
/// combination of letters and digits (matching the regular expression
/// [a-zA-Z0-9] between 1 and 40 characters long starting with a letter
/// [a-zA-Z] character.
/// @param[in] property Value to set the user property to.  Set this
/// argument to NULL or nullptr to remove the user property.  The value can be
/// between 1 to 100 characters long.
void SetUserProperty(const char* name, const char* property);

/// @brief Sets the user ID property.
///
/// This feature must be used in accordance with
/// <a href="https://www.google.com/policies/privacy">Google's Privacy
/// Policy</a>
///
/// @param[in] user_id The user ID associated with the user of this app on this
/// device.  The user ID must be non-empty and no more than 256 characters long.
/// Setting user_id to NULL or nullptr removes the user ID.
void SetUserId(const char* user_id);

/// @brief Sets the minimum engagement time required before starting a session.
///
/// @note The default value is 10000 (10 seconds).
///
/// @param milliseconds The minimum engagement time required to start a new
/// session.
///
/// @deprecated SetMinimumSessionDuration is deprecated and no longer
/// functional.
FIREBASE_DEPRECATED void SetMinimumSessionDuration(int64_t milliseconds);

/// @brief Sets the duration of inactivity that terminates the current session.
///
/// @note The default value is 1800000 (30 minutes).
///
/// @param milliseconds The duration of inactivity that terminates the current
/// session.
void SetSessionTimeoutDuration(int64_t milliseconds);

/// @brief Sets the current screen name and screen class, which specifies the
/// current visual context in your app. This helps identify the areas in your
/// app where users spend their time and how they interact with your app.
///
/// @param screen_name The name of the current screen. Set to nullptr to clear
/// the current screen name. Limited to 100 characters.
/// @param screen_class The name of the screen class. If you specify nullptr for
/// this, it will use the default. On Android, the default is the class name of
/// the current Activity. On iOS, the default is the class name of the current
/// UIViewController. Limited to 100 characters.
void SetCurrentScreen(const char* screen_name, const char* screen_class);

/// Clears all analytics data for this app from the device and resets the app
/// instance id.
void ResetAnalyticsData();

/// Get the instance ID from the analytics service.
///
/// @note This is *not* the same ID as the ID returned by
/// @if cpp_examples
/// firebase::instance_id::InstanceId.
/// @else
/// Firebase.InstanceId.FirebaseInstanceId.
/// @endif
///
/// @returns Object which can be used to retrieve the analytics instance ID.
Future<std::string> GetAnalyticsInstanceId();

/// Get the result of the most recent GetAnalyticsInstanceId() call.
///
/// @returns Object which can be used to retrieve the analytics instance ID.
Future<std::string> GetAnalyticsInstanceIdLastResult();

}  // namespace analytics
}  // namespace firebase

#endif  // FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_H_
