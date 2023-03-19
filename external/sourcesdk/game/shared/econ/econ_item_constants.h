
#pragma once

//-----------------------------------------------------------------------------
// Origin for an item for CEconItem
//-----------------------------------------------------------------------------
// WARNING!!! Values stored in DB.  DO NOT CHANGE EXISTING VALUES.  Add values to the end.
enum eEconItemOrigin
{
    kEconItemOrigin_Invalid = -1,				// should never be stored in the DB! used to indicate "invalid" for in-memory objects only

    kEconItemOrigin_Drop = 0,
    kEconItemOrigin_Achievement,
    kEconItemOrigin_Purchased,
    kEconItemOrigin_Traded,
    kEconItemOrigin_Crafted,
    kEconItemOrigin_StorePromotion,
    kEconItemOrigin_Gifted,
    kEconItemOrigin_SupportGranted,
    kEconItemOrigin_FoundInCrate,
    kEconItemOrigin_Earned,
    kEconItemOrigin_ThirdPartyPromotion,
    kEconItemOrigin_GiftWrapped,
    kEconItemOrigin_HalloweenDrop,
    kEconItemOrigin_PackageItem,
    kEconItemOrigin_Foreign,
    kEconItemOrigin_CDKey,
    kEconItemOrigin_CollectionReward,
    kEconItemOrigin_PreviewItem,
    kEconItemOrigin_SteamWorkshopContribution,
    kEconItemOrigin_PeriodicScoreReward,
    kEconItemOrigin_MvMMissionCompletionReward,			// includes loot from both "mission completed" and "tour completed" events
    kEconItemOrigin_MvMSquadSurplusReward,
    kEconItemOrigin_RecipeOutput,
    kEconItemOrigin_QuestDrop,
    kEconItemOrigin_QuestLoanerItem,
    kEconItemOrigin_TradeUp,
    kEconItemOrigin_ViralCompetitiveBetaPassSpread,

    kEconItemOrigin_Max,
};

enum EEconItemQuality
{
    AE_UNDEFINED = -1,

    AE_NORMAL = 0,
    AE_RARITY1 = 1,			// Genuine
    AE_RARITY2 = 2,			// Customized (unused)
    AE_VINTAGE = 3,			// Vintage has to stay at 3 for backwards compatibility
    AE_RARITY3,				// Artisan
    AE_UNUSUAL,				// Unusual
    AE_UNIQUE,
    AE_COMMUNITY,
    AE_DEVELOPER,
    AE_SELFMADE,
    AE_CUSTOMIZED,			// (unused)
    AE_STRANGE,
    AE_COMPLETED,
    AE_HAUNTED,
    AE_COLLECTORS,
    AE_PAINTKITWEAPON,

    AE_RARITY_DEFAULT,
    AE_RARITY_COMMON,
    AE_RARITY_UNCOMMON,
    AE_RARITY_RARE,
    AE_RARITY_MYTHICAL,
    AE_RARITY_LEGENDARY,
    AE_RARITY_ANCIENT,

    AE_MAX_TYPES,
    AE_DEPRECATED_UNIQUE = 3,
};
