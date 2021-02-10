// Copyright 2019 Google Inc. All Rights Reserved.

#ifndef FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_EVENT_NAMES_H_
#define FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_EVENT_NAMES_H_

/// @brief Namespace that encompasses all Firebase APIs.
namespace firebase {
/// @brief Firebase Analytics API.
namespace analytics {

/// @defgroup event_names Analytics Events
///
/// Predefined event names.
///
/// An Event is an important occurrence in your app that you want to
/// measure. You can report up to 500 different types of Events per app
/// and you can associate up to 25 unique parameters with each Event type.
/// Some common events are suggested below, but you may also choose to
/// specify custom Event types that are associated with your specific app.
/// Each event type is identified by a unique name. Event names can be up
/// to 40 characters long, may only contain alphanumeric characters and
/// underscores ("_"), and must start with an alphabetic character. The
/// "firebase_", "google_", and "ga_" prefixes are reserved and should not
/// be used.
/// @{

/// Add Payment Info event. This event signifies that a user has submitted
/// their payment information to your app.
static const char *const kEventAddPaymentInfo = "add_payment_info";

/// E-Commerce Add To Cart event. This event signifies that an item was
/// added to a cart for purchase. Add this event to a funnel with
/// kEventEcommercePurchase to gauge the effectiveness of your
/// checParameter(kout, If you supply the @c kParameterValue parameter),
/// you must also supply the @c kParameterCurrency parameter so that
/// revenue metrics can be computed accurately. Params:
///
/// <ul>
///  <li>@c kParameterQuantity (signed 64-bit integer)</li>
///  <li>@c kParameterItemID (string)</li>
///  <li>@c kParameterItemName (string)</li>
///  <li>@c kParameterItemCategory (string)</li>
///  <li>@c kParameterItemLocationID (string) (optional)</li>
///  <li>@c kParameterPrice (double) (optional)</li>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterValue (double) (optional)</li>
///  <li>@c kParameterOrigin (string) (optional)</li>
///  <li>@c kParameterDestination (string) (optional)</li>
///  <li>@c kParameterStartDate (string) (optional)</li>
///  <li>@c kParameterEndDate (string) (optional)</li>
/// </ul>
static const char *const kEventAddToCart = "add_to_cart";

/// E-Commerce Add To Wishlist event. This event signifies that an item
/// was added to a wishlist. Use this event to identify popular gift items
/// in your app. Note: If you supply the @c kParameterValue parameter, you
/// must also supply the @c kParameterCurrency parameter so that revenue
/// metrics can be computed accurately. Params:
///
/// <ul>
///  <li>@c kParameterQuantity (signed 64-bit integer)</li>
///  <li>@c kParameterItemID (string)</li>
///  <li>@c kParameterItemName (string)</li>
///  <li>@c kParameterItemCategory (string)</li>
///  <li>@c kParameterItemLocationID (string) (optional)</li>
///  <li>@c kParameterPrice (double) (optional)</li>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterValue (double) (optional)</li>
/// </ul>
static const char *const kEventAddToWishlist = "add_to_wishlist";

/// App Open event. By logging this event when an App becomes active,
/// developers can understand how often users leave and return during the
/// course of a Session. Although Sessions are automatically reported,
/// this event can provide further clarification around the continuous
/// engagement of app-users.
static const char *const kEventAppOpen = "app_open";

/// E-Commerce Begin Checkout event. This event signifies that a user has
/// begun the process of checking out. Add this event to a funnel with
/// your kEventEcommercePurchase event to gauge the effectiveness of your
/// checkout process. Note: If you supply the @c kParameterValue
/// parameter, you must also supply the @c kParameterCurrency parameter so
/// that revenue metrics can be computed accurately. Params:
///
/// <ul>
///  <li>@c kParameterValue (double) (optional)</li>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterTransactionID (string) (optional)</li>
///  <li>@c kParameterStartDate (string) (optional)</li>
///  <li>@c kParameterEndDate (string) (optional)</li>
///  <li>@c kParameterNumberOfNights (signed 64-bit integer) (optional) for
///      hotel bookings</li>
///  <li>@c kParameterNumberOfRooms (signed 64-bit integer) (optional) for
///      hotel bookings</li>
///  <li>@c kParameterNumberOfPassengers (signed 64-bit integer) (optional)
///      for travel bookings</li>
///  <li>@c kParameterOrigin (string) (optional)</li>
///  <li>@c kParameterDestination (string) (optional)</li>
///  <li>@c kParameterTravelClass (string) (optional) for travel bookings</li>
/// </ul>
static const char *const kEventBeginCheckout = "begin_checkout";

/// Campaign Detail event. Log this event to supply the referral details
/// of a re-engagement campaign. Note: you must supply at least one of the
/// required parameters kParameterSource, kParameterMedium or
/// kParameterCampaign. Params:
///
/// <ul>
///  <li>@c kParameterSource (string)</li>
///  <li>@c kParameterMedium (string)</li>
///  <li>@c kParameterCampaign (string)</li>
///  <li>@c kParameterTerm (string) (optional)</li>
///  <li>@c kParameterContent (string) (optional)</li>
///  <li>@c kParameterAdNetworkClickID (string) (optional)</li>
///  <li>@c kParameterCP1 (string) (optional)</li>
/// </ul>
static const char *const kEventCampaignDetails = "campaign_details";

/// Checkout progress. Params:
///
/// <ul>
///    <li>@c kParameterCheckoutStep (unsigned 64-bit integer)</li>
///    <li>@c kParameterCheckoutOption (string) (optional)</li>
/// </ul>
static const char *const kEventCheckoutProgress = "checkout_progress";

/// Earn Virtual Currency event. This event tracks the awarding of virtual
/// currency in your app. Log this along with @c
/// kEventSpendVirtualCurrency to better understand your virtual economy.
/// Params:
///
/// <ul>
///  <li>@c kParameterVirtualCurrencyName (string)</li>
///  <li>@c kParameterValue (signed 64-bit integer or double)</li>
/// </ul>
static const char *const kEventEarnVirtualCurrency = "earn_virtual_currency";

/// E-Commerce Purchase event. This event signifies that an item was
/// purchased by a user. Note: This is different from the in-app purchase
/// event, which is reported automatically for App Store-based apps. Note:
/// If you supply the @c kParameterValue parameter, you must also supply
/// the @c kParameterCurrency parameter so that revenue metrics can be
/// computed accurately. Params:
///
/// <ul>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterValue (double) (optional)</li>
///  <li>@c kParameterTransactionID (string) (optional)</li>
///  <li>@c kParameterTax (double) (optional)</li>
///  <li>@c kParameterShipping (double) (optional)</li>
///  <li>@c kParameterCoupon (string) (optional)</li>
///  <li>@c kParameterLocation (string) (optional)</li>
///  <li>@c kParameterStartDate (string) (optional)</li>
///  <li>@c kParameterEndDate (string) (optional)</li>
///  <li>@c kParameterNumberOfNights (signed 64-bit integer) (optional) for
///      hotel bookings</li>
///  <li>@c kParameterNumberOfRooms (signed 64-bit integer) (optional) for
///      hotel bookings</li>
///  <li>@c kParameterNumberOfPassengers (signed 64-bit integer) (optional)
///      for travel bookings</li>
///  <li>@c kParameterOrigin (string) (optional)</li>
///  <li>@c kParameterDestination (string) (optional)</li>
///  <li>@c kParameterTravelClass (string) (optional) for travel bookings</li>
/// </ul>
static const char *const kEventEcommercePurchase = "ecommerce_purchase";

/// Generate Lead event. Log this event when a lead has been generated in
/// the app to understand the efficacy of your install and re-engagement
/// campaigns. Note: If you supply the @c kParameterValue parameter, you
/// must also supply the @c kParameterCurrency parameter so that revenue
/// metrics can be computed accurately. Params:
///
/// <ul>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterValue (double) (optional)</li>
/// </ul>
static const char *const kEventGenerateLead = "generate_lead";

/// Join Group event. Log this event when a user joins a group such as a
/// guild, team or family. Use this event to analyze how popular certain
/// groups or social features are in your app. Params:
///
/// <ul>
///  <li>@c kParameterGroupID (string)</li>
/// </ul>
static const char *const kEventJoinGroup = "join_group";

/// Level Up event. This event signifies that a player has leveled up in
/// your gaming app. It can help you gauge the level distribution of your
/// userbase and help you identify certain levels that are difficult to
/// pass. Params:
///
/// <ul>
///  <li>@c kParameterLevel (signed 64-bit integer)</li>
///  <li>@c kParameterCharacter (string) (optional)</li>
/// </ul>
static const char *const kEventLevelUp = "level_up";

/// Login event. Apps with a login feature can report this event to
/// signify that a user has logged in.
static const char *const kEventLogin = "login";

/// Post Score event. Log this event when the user posts a score in your
/// gaming app. This event can help you understand how users are actually
/// performing in your game and it can help you correlate high scores with
/// certain audiences or behaviors. Params:
///
/// <ul>
///  <li>@c kParameterScore (signed 64-bit integer)</li>
///  <li>@c kParameterLevel (signed 64-bit integer) (optional)</li>
///  <li>@c kParameterCharacter (string) (optional)</li>
/// </ul>
static const char *const kEventPostScore = "post_score";

/// Present Offer event. This event signifies that the app has presented a
/// purchase offer to a user. Add this event to a funnel with the
/// kEventAddToCart and kEventEcommercePurchase to gauge your conversion
/// process. Note: If you supply the @c kParameterValue parameter, you
/// must also supply the @c kParameterCurrency parameter so that revenue
/// metrics can be computed accurately. Params:
///
/// <ul>
///  <li>@c kParameterQuantity (signed 64-bit integer)</li>
///  <li>@c kParameterItemID (string)</li>
///  <li>@c kParameterItemName (string)</li>
///  <li>@c kParameterItemCategory (string)</li>
///  <li>@c kParameterItemLocationID (string) (optional)</li>
///  <li>@c kParameterPrice (double) (optional)</li>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterValue (double) (optional)</li>
/// </ul>
static const char *const kEventPresentOffer = "present_offer";

/// E-Commerce Purchase Refund event. This event signifies that an item
/// purchase was refunded. Note: If you supply the @c kParameterValue
/// parameter, you must also supply the @c kParameterCurrency parameter so
/// that revenue metrics can be computed accurately. Params:
///
/// <ul>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterValue (double) (optional)</li>
///  <li>@c kParameterTransactionID (string) (optional)</li>
/// </ul>
static const char *const kEventPurchaseRefund = "purchase_refund";

/// Remove from cart event. Params:
///
/// <ul>
///  <li>@c kParameterQuantity (signed 64-bit integer)</li>
///  <li>@c kParameterItemID (string)</li>
///  <li>@c kParameterItemName (string)</li>
///  <li>@c kParameterItemCategory (string)</li>
///  <li>@c kParameterItemLocationID (string) (optional)</li>
///  <li>@c kParameterPrice (double) (optional)</li>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterValue (double) (optional)</li>
///  <li>@c kParameterOrigin (string) (optional)</li>
///  <li>@c kParameterDestination (string) (optional)</li>
///  <li>@c kParameterStartDate (string) (optional)</li>
///  <li>@c kParameterEndDate (string) (optional)</li>
/// </ul>
static const char *const kEventRemoveFromCart = "remove_from_cart";

/// Search event. Apps that support search features can use this event to
/// contextualize search operations by supplying the appropriate,
/// corresponding parameters. This event can help you identify the most
/// popular content in your app. Params:
///
/// <ul>
///  <li>@c kParameterSearchTerm (string)</li>
///  <li>@c kParameterStartDate (string) (optional)</li>
///  <li>@c kParameterEndDate (string) (optional)</li>
///  <li>@c kParameterNumberOfNights (signed 64-bit integer) (optional) for
///      hotel bookings</li>
///  <li>@c kParameterNumberOfRooms (signed 64-bit integer) (optional) for
///      hotel bookings</li>
///  <li>@c kParameterNumberOfPassengers (signed 64-bit integer) (optional)
///      for travel bookings</li>
///  <li>@c kParameterOrigin (string) (optional)</li>
///  <li>@c kParameterDestination (string) (optional)</li>
///  <li>@c kParameterTravelClass (string) (optional) for travel bookings</li>
/// </ul>
static const char *const kEventSearch = "search";

/// Select Content event. This general purpose event signifies that a user
/// has selected some content of a certain type in an app. The content can
/// be any object in your app. This event can help you identify popular
/// content and categories of content in your app. Params:
///
/// <ul>
///  <li>@c kParameterContentType (string)</li>
///  <li>@c kParameterItemID (string)</li>
/// </ul>
static const char *const kEventSelectContent = "select_content";

/// Set checkout option. Params:
///
/// <ul>
///    <li>@c kParameterCheckoutStep (unsigned 64-bit integer)</li>
///    <li>@c kParameterCheckoutOption (string)</li>
/// </ul>
static const char *const kEventSetCheckoutOption = "set_checkout_option";

/// Share event. Apps with social features can log the Share event to
/// identify the most viral content. Params:
///
/// <ul>
///  <li>@c kParameterContentType (string)</li>
///  <li>@c kParameterItemID (string)</li>
/// </ul>
static const char *const kEventShare = "share";

/// Sign Up event. This event indicates that a user has signed up for an
/// account in your app. The parameter signifies the method by which the
/// user signed up. Use this event to understand the different behaviors
/// between logged in and logged out users. Params:
///
/// <ul>
///  <li>@c kParameterSignUpMethod (string)</li>
/// </ul>
static const char *const kEventSignUp = "sign_up";

/// Spend Virtual Currency event. This event tracks the sale of virtual
/// goods in your app and can help you identify which virtual goods are
/// the most popular objects of purchase. Params:
///
/// <ul>
///  <li>@c kParameterItemName (string)</li>
///  <li>@c kParameterVirtualCurrencyName (string)</li>
///  <li>@c kParameterValue (signed 64-bit integer or double)</li>
/// </ul>
static const char *const kEventSpendVirtualCurrency = "spend_virtual_currency";

/// Tutorial Begin event. This event signifies the start of the
/// on-boarding process in your app. Use this in a funnel with
/// kEventTutorialComplete to understand how many users complete this
/// process and move on to the full app experience.
static const char *const kEventTutorialBegin = "tutorial_begin";

/// Tutorial End event. Use this event to signify the user's completion of
/// your app's on-boarding process. Add this to a funnel with
/// kEventTutorialBegin to gauge the completion rate of your on-boarding
/// process.
static const char *const kEventTutorialComplete = "tutorial_complete";

/// Unlock Achievement event. Log this event when the user has unlocked an
/// achievement in your game. Since achievements generally represent the
/// breadth of a gaming experience, this event can help you understand how
/// many users are experiencing all that your game has to offer. Params:
///
/// <ul>
///  <li>@c kParameterAchievementID (string)</li>
/// </ul>
static const char *const kEventUnlockAchievement = "unlock_achievement";

/// View Item event. This event signifies that some content was shown to
/// the user. This content may be a product, a webpage or just a simple
/// image or text. Use the appropriate parameters to contextualize the
/// event. Use this event to discover the most popular items viewed in
/// your app. Note: If you supply the @c kParameterValue parameter, you
/// must also supply the @c kParameterCurrency parameter so that revenue
/// metrics can be computed accurately. Params:
///
/// <ul>
///  <li>@c kParameterItemID (string)</li>
///  <li>@c kParameterItemName (string)</li>
///  <li>@c kParameterItemCategory (string)</li>
///  <li>@c kParameterItemLocationID (string) (optional)</li>
///  <li>@c kParameterPrice (double) (optional)</li>
///  <li>@c kParameterQuantity (signed 64-bit integer) (optional)</li>
///  <li>@c kParameterCurrency (string) (optional)</li>
///  <li>@c kParameterValue (double) (optional)</li>
///  <li>@c kParameterStartDate (string) (optional)</li>
///  <li>@c kParameterEndDate (string) (optional)</li>
///  <li>@c kParameterFlightNumber (string) (optional) for travel bookings</li>
///  <li>@c kParameterNumberOfPassengers (signed 64-bit integer) (optional)
///      for travel bookings</li>
///  <li>@c kParameterNumberOfNights (signed 64-bit integer) (optional) for
///      travel bookings</li>
///  <li>@c kParameterNumberOfRooms (signed 64-bit integer) (optional) for
///      travel bookings</li>
///  <li>@c kParameterOrigin (string) (optional)</li>
///  <li>@c kParameterDestination (string) (optional)</li>
///  <li>@c kParameterSearchTerm (string) (optional) for travel bookings</li>
///  <li>@c kParameterTravelClass (string) (optional) for travel bookings</li>
/// </ul>
static const char *const kEventViewItem = "view_item";

/// View Item List event. Log this event when the user has been presented
/// with a list of items of a certain category. Params:
///
/// <ul>
///  <li>@c kParameterItemCategory (string)</li>
/// </ul>
static const char *const kEventViewItemList = "view_item_list";

/// View Search Results event. Log this event when the user has been
/// presented with the results of a search. Params:
///
/// <ul>
///  <li>@c kParameterSearchTerm (string)</li>
/// </ul>
static const char *const kEventViewSearchResults = "view_search_results";

/// Level Start event. Log this event when the user starts a new level.
/// Params:
///
/// <ul>
///  <li>@c kParameterLevelName (string)</li>
/// </ul>
static const char *const kEventLevelStart = "level_start";

/// Level End event. Log this event when the user finishes a level.
/// Params:
///
/// <ul>
///  <li>@c kParameterLevelName (string)</li>
///  <li>@c kParameterSuccess (string)</li>
/// </ul>
static const char *const kEventLevelEnd = "level_end";
/// @}

} // namespace analytics
} // namespace firebase

#endif // FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_EVENT_NAMES_H_
