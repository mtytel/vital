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

#ifndef FIREBASE_ADMOB_CLIENT_CPP_SRC_INCLUDE_FIREBASE_ADMOB_REWARDED_VIDEO_H_
#define FIREBASE_ADMOB_CLIENT_CPP_SRC_INCLUDE_FIREBASE_ADMOB_REWARDED_VIDEO_H_

#include <queue>
#include <string>

#include "firebase/admob/types.h"
#include "firebase/future.h"

namespace firebase {

// Forward declaration of Firebase's internal Mutex.
class Mutex;

namespace admob {

/// @brief Loads and displays rewarded video ads via AdMob mediation.
///
/// The rewarded_video namespace contains methods to load and display rewarded
/// video ads via the Google Mobile Ads SDK. The underlying SDK objects for
/// rewarded video on Android and iOS are singletons, so there are no objects
/// to represent individual ads here. Instead, methods in the rewarded_video
/// namespace are invoked to initialize, load, and show.
///
/// The basic steps for loading and displaying an ad are:
///
/// 1. Call @ref Initialize to init the library and mediation adapters.
/// 2. Call @ref LoadAd to load an ad (some SDKs may have cached an ad at init
///     time).
/// 3. Call @ref Show to show the ad to the user.
/// 4. Repeat steps 2 and 3 as desired.
/// 5. Call @ref Destroy when your app is completely finished showing rewarded
///     video ads.
///
/// Note that Initialize must be the very first thing called, and @ref Destroy
/// must be the very last.
///
/// The library maintains a presentation state that indicates whether or not an
/// ad is currently onscreen, but otherwise provides information about its
/// current state through Futures. @ref Initialize, @ref LoadAd, and so on each
/// have a corresponding @ref Future from which apps can determine the result of
/// the previous call.
///
/// In addition, applications can create their own subclasses of @ref Listener,
/// pass an instance to the @ref SetListener method, and receive callbacks
/// whenever the presentation state changes or an ad has been viewed in full and
/// the user is due a reward.
///
/// Here's how one might initialize, load, and show a rewarded video ad while
/// checking against the result of the previous action at each step:
///
/// @code
/// firebase::admob::rewarded_video::Initialize();
/// @endcode
///
/// Then, later:
///
/// @code
/// if (firebase::admob::rewarded_video::InitializeLastResult().status() ==
///     firebase::kFutureStatusComplete &&
///     firebase::admob::rewarded_video::InitializeLastResult().error() ==
///     firebase::admob::kAdMobErrorNone) {
///   firebase::admob::rewarded_video::LoadAd(my_ad_unit_str, my_ad_request);
/// }
/// @endcode
///
/// And after that:
///
/// @code
/// if (firebase::admob::rewarded_video::LoadAdLastResult().status() ==
///     firebase::kFutureStatusComplete &&
///     firebase::admob::rewarded_video::LoadAdLastResult().error() ==
///     firebase::admob::kAdMobErrorNone) {
///   firebase::admob::rewarded_video::Show(my_ad_parent);
/// }
/// @endcode
namespace rewarded_video {

/// The possible presentation states for rewarded video.
enum PresentationState {
  /// No ad is currently being shown.
  kPresentationStateHidden = 0,
  /// A rewarded video ad is completely covering the screen or has caused
  /// focus to leave the application (for example, when opening an external
  /// browser during a clickthrough), but the video associated with the ad has
  /// yet to begin playing.
  kPresentationStateCoveringUI,
  /// All of the above conditions are true *except* that the video associated
  /// with the ad began playing at some point in the past.
  kPresentationStateVideoHasStarted,
  /// The rewarded video has played and completed.
  kPresentationStateVideoHasCompleted,
};

/// @brief A reward to be given to the user in exchange for watching a rewarded
/// video ad.
struct RewardItem {
  /// The reward amount.
  float amount;
  /// A string description of the type of reward (such as "coins" or "points").
  std::string reward_type;
};

/// A listener class that developers can extend and pass to @ref SetListener
/// to be notified of rewards and changes to the presentation state.
class Listener {
 public:
  /// Invoked when the user should be given a reward for watching an ad.
  /// @param[in] reward The user's reward.
  virtual void OnRewarded(RewardItem reward) = 0;

  /// Invoked when the presentation state of the ad changes.
  /// @param[in] state The new presentation state.
  virtual void OnPresentationStateChanged(PresentationState state) = 0;

  virtual ~Listener();
};

/// A polling-based listener that developers can instantiate and pass to
/// @ref SetListener in order to queue rewards for later retrieval.
///
/// The @ref PollReward method should be used to retrieve awards granted by the
/// Mobile Ads SDK and queued by this class.
/// @ref rewarded_video::presentation_state can be used to poll the current
/// presentation state, so no additional method has been added for it.
class PollableRewardListener : public Listener {
 public:
  PollableRewardListener();
  ~PollableRewardListener();
  void OnRewarded(RewardItem reward);
  void OnPresentationStateChanged(PresentationState state);

  /// Pop the oldest queued reward, and copy its data into the provided
  /// RewardItem. If no reward is available, the struct is unchanged.
  /// @param reward Pointer to a struct that reward data can be copied into.
  /// @returns true if a reward was popped and data was copied, false otherwise.
  bool PollReward(RewardItem* reward);

 private:
  Mutex* mutex_;

  // Rewards granted by the Mobile Ads SDK.
  std::queue<RewardItem> rewards_;
};

/// Initializes rewarded video. This must be the first method invoked in
/// this namespace.
Future<void> Initialize();

/// Returns a @ref Future that has the status of the last call to
/// @ref Initialize.
Future<void> InitializeLastResult();

/// Begins an asynchronous request for an ad.
/// @param[in] ad_unit_id The ad unit ID to use in the request.
/// @param[in] request An AdRequest struct with information about the request
///                    to be made (such as targeting info).
Future<void> LoadAd(const char* ad_unit_id, const AdRequest& request);

/// Returns a @ref Future containing the status of the last call to
/// @ref LoadAd.
Future<void> LoadAdLastResult();

/// Shows an ad, assuming one has loaded. @ref LoadAd must be called before this
/// method.
/// @param[in] parent An @ref AdParent that is a reference to an iOS
///                   UIView or an Android Activity.
Future<void> Show(AdParent parent);

/// Returns a @ref Future containing the status of the last call to
/// @ref Show.
Future<void> ShowLastResult();

/// Pauses any background processing associated with rewarded video. Should
/// be called whenever the C++ engine pauses or the application loses focus.
Future<void> Pause();

/// Returns a @ref Future containing the status of the last call to
/// @ref Pause.
Future<void> PauseLastResult();

/// Resumes the rewarded video system after pausing.
Future<void> Resume();

/// Returns a @ref Future containing the status of the last call to
/// @ref Resume.
Future<void> ResumeLastResult();

/// Cleans up and deallocates any resources used by rewarded video.
/// No other methods in rewarded_video should be called once this method has
/// been invoked. The system is closed for business at that point.
void Destroy();

/// Returns the current presentation state, indicating if an ad is visible or
/// if a video has started playing.
///
/// @return The current presentation state.
PresentationState presentation_state();

/// Sets the @ref Listener that should receive callbacks.
/// @param[in] listener A valid Listener.
void SetListener(Listener* listener);

}  // namespace rewarded_video
}  // namespace admob
}  // namespace firebase

#endif  // FIREBASE_ADMOB_CLIENT_CPP_SRC_INCLUDE_FIREBASE_ADMOB_REWARDED_VIDEO_H_
