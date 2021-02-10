// Copyright 2019 Google Inc. All Rights Reserved.

#ifndef FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_PARAMETER_NAMES_H_
#define FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_PARAMETER_NAMES_H_

/// @brief Namespace that encompasses all Firebase APIs.
namespace firebase {
/// @brief Firebase Analytics API.
namespace analytics {

/// @defgroup parameter_names Analytics Parameters
///
/// Predefined event parameter names.
///
/// Params supply information that contextualize Events. You can associate
/// up to 25 unique Params with each Event type. Some Params are suggested
/// below for certain common Events, but you are not limited to these. You
/// may supply extra Params for suggested Events or custom Params for
/// Custom events. Param names can be up to 40 characters long, may only
/// contain alphanumeric characters and underscores ("_"), and must start
/// with an alphabetic character. Param values can be up to 100 characters
/// long. The "firebase_", "google_", and "ga_" prefixes are reserved and
/// should not be used.
/// @{

/// Game achievement ID (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterAchievementID, "10_matches_won"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterAchievementID = "achievement_id";

/// Ad Network Click ID (string). Used for network-specific click IDs
/// which vary in format.
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterAdNetworkClickID, "1234567"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterAdNetworkClickID = "aclid";

/// The store or affiliation from which this transaction occurred
/// (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterAffiliation, "Google Store"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterAffiliation = "affiliation";

/// The individual campaign name, slogan, promo code, etc. Some networks
/// have pre-defined macro to capture campaign information, otherwise can
/// be populated by developer. Highly Recommended (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCampaign, "winter_promotion"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCampaign = "campaign";

/// Character used in game (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCharacter, "beat_boss"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCharacter = "character";

/// The checkout step (1..N) (unsigned 64-bit integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCheckoutStep, "1"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCheckoutStep = "checkout_step";

/// Some option on a step in an ecommerce flow (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCheckoutOption, "Visa"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCheckoutOption = "checkout_option";

/// Campaign content (string).
static const char *const kParameterContent = "content";

/// Type of content selected (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterContentType, "news article"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterContentType = "content_type";

/// Coupon code for a purchasable item (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCoupon, "zz123"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCoupon = "coupon";

/// Campaign custom parameter (string). Used as a method of capturing
/// custom data in a campaign. Use varies by network.
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCP1, "custom_data"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCP1 = "cp1";

/// The name of a creative used in a promotional spot (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCreativeName, "Summer Sale"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCreativeName = "creative_name";

/// The name of a creative slot (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCreativeSlot, "summer_banner2"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCreativeSlot = "creative_slot";

/// Purchase currency in 3-letter <a href="http://en.wikipedia.org/wiki/ISO_4217#Active_codes">
/// ISO_4217</a> format (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterCurrency, "USD"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterCurrency = "currency";

/// Flight or Travel destination (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterDestination, "Mountain View, CA"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterDestination = "destination";

/// The arrival date, check-out date or rental end date for the item. This
/// should be in YYYY-MM-DD format (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterEndDate, "2015-09-14"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterEndDate = "end_date";

/// Flight number for travel events (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterFlightNumber, "ZZ800"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterFlightNumber = "flight_number";

/// Group/clan/guild ID (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterGroupID, "g1"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterGroupID = "group_id";

/// Index of an item in a list (signed 64-bit integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterIndex, 1),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterIndex = "index";

/// Item brand (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterItemBrand, "Google"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterItemBrand = "item_brand";

/// Item category (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterItemCategory, "t-shirts"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterItemCategory = "item_category";

/// Item ID (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterItemID, "p7654"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterItemID = "item_id";

/// The Google <a href="https://developers.google.com/places/place-id">Place ID</a> (string) that
/// corresponds to the associated item. Alternatively, you can supply your
/// own custom Location ID.
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterItemLocationID, "ChIJiyj437sx3YAR9kUWC8QkLzQ"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterItemLocationID = "item_location_id";

/// Item name (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterItemName, "abc"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterItemName = "item_name";

/// The list in which the item was presented to the user (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterItemList, "Search Results"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterItemList = "item_list";

/// Item variant (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterItemVariant, "Red"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterItemVariant = "item_variant";

/// Level in game (signed 64-bit integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterLevel, 42),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterLevel = "level";

/// Location (string). The Google <a href="https://developers.google.com/places/place-id">Place ID
/// </a> that corresponds to the associated event. Alternatively, you can supply your own custom
/// Location ID.
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterLocation, "ChIJiyj437sx3YAR9kUWC8QkLzQ"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterLocation = "location";

/// The advertising or marParameter(keting, cpc, banner, email), push.
/// Highly recommended (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterMedium, "email"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterMedium = "medium";

/// Number of nights staying at hotel (signed 64-bit integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterNumberOfNights, 3),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterNumberOfNights = "number_of_nights";

/// Number of passengers traveling (signed 64-bit integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterNumberOfPassengers, 11),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterNumberOfPassengers = "number_of_passengers";

/// Number of rooms for travel events (signed 64-bit integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterNumberOfRooms, 2),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterNumberOfRooms = "number_of_rooms";

/// Flight or Travel origin (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterOrigin, "Mountain View, CA"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterOrigin = "origin";

/// Purchase price (double).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterPrice, 1.0),
///    Parameter(kParameterCurrency, "USD"),  // e.g. $1.00 USD
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterPrice = "price";

/// Purchase quantity (signed 64-bit integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterQuantity, 1),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterQuantity = "quantity";

/// Score in game (signed 64-bit integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterScore, 4200),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterScore = "score";

/// The search string/keywords used (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterSearchTerm, "periodic table"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterSearchTerm = "search_term";

/// Shipping cost (double).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterShipping, 9.50),
///    Parameter(kParameterCurrency, "USD"),  // e.g. $9.50 USD
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterShipping = "shipping";

/// Sign up method (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterSignUpMethod, "google"),
///    // ...
///  };
/// @endcode
///
/// @endif
///
///
/// <b>This constant has been deprecated. Use Method constant instead.</b>
static const char *const kParameterSignUpMethod = "sign_up_method";

/// A particular approach used in an operation; for example, "facebook" or
/// "email" in the context of a sign_up or login event.  (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterMethod, "google"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterMethod = "method";

/// The origin of your traffic, such as an Ad network (for example,
/// google) or partner (urban airship). Identify the advertiser, site,
/// publication, etc. that is sending traffic to your property. Highly
/// recommended (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterSource, "InMobi"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterSource = "source";

/// The departure date, check-in date or rental start date for the item.
/// This should be in YYYY-MM-DD format (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterStartDate, "2015-09-14"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterStartDate = "start_date";

/// Tax amount (double).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterTax, 1.0),
///    Parameter(kParameterCurrency, "USD"),  // e.g. $1.00 USD
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterTax = "tax";

/// If you're manually tagging keyword campaigns, you should use utm_term
/// to specify the keyword (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterTerm, "game"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterTerm = "term";

/// A single ID for a ecommerce group transaction (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterTransactionID, "ab7236dd9823"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterTransactionID = "transaction_id";

/// Travel class (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterTravelClass, "business"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterTravelClass = "travel_class";

/// A context-specific numeric value which is accumulated automatically
/// for each event type. This is a general purpose parameter that is
/// useful for accumulating a key metric that pertains to an event.
/// Examples include revenue, distance, time and points. Value should be
/// specified as signed 64-bit integer or double. Notes: Values for
/// pre-defined currency-related events (such as @c kEventAddToCart)
/// should be supplied using double and must be accompanied by a @c
/// kParameterCurrency parameter. The valid range of accumulated values is
/// [-9,223,372,036,854.77, 9,223,372,036,854.77]. Supplying a non-numeric
/// value, omitting the corresponding @c kParameterCurrency parameter, or
/// supplying an invalid
/// <a href="https://goo.gl/qqX3J2">currency code</a> for conversion events will cause that
/// conversion to be omitted from reporting.
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterValue, 3.99),
///    Parameter(kParameterCurrency, "USD"),  // e.g. $3.99 USD
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterValue = "value";

/// Name of virtual currency type (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterVirtualCurrencyName, "virtual_currency_name"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterVirtualCurrencyName =
    "virtual_currency_name";

/// The name of a level in a game (string).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterLevelName, "room_1"),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterLevelName = "level_name";

/// The result of an operation. Specify 1 to indicate success and 0 to
/// indicate failure (unsigned integer).
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterSuccess, 1),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterSuccess = "success";

/// Indicates that the associated event should either extend the current
/// session or start a new session if no session was active when the event
/// was logged. Specify YES to extend the current session or to start a
/// new session; any other value will not extend or start a session.
///
/// @if cpp_examples
/// @code{.cpp}
/// using namespace firebase::analytics;
/// Parameter parameters[] = {
///    Parameter(kParameterExtendSession, @YES),
///    // ...
///  };
/// @endcode
///
/// @endif
static const char *const kParameterExtendSession = "extend_session";
/// @}

} // namespace analytics
} // namespace firebase

#endif // FIREBASE_ANALYTICS_CLIENT_CPP_INCLUDE_FIREBASE_ANALYTICS_PARAMETER_NAMES_H_
