#pragma once

namespace NLB::Options
{

	inline bool ignoreLimits = true;

	inline float maxSurcharge = 0.5f;

	inline float maxDiscount = 0.15f;

	// TODO: Use fMinBuyMult instead of this option.
	inline float parityBaseline = 1.0f; // Baseline that offsets whole "fair" formula. This is the multiplier to be used when both partiers have the same skill.

	void Load();
}

namespace NLB::Settings
{
	// These are main settings that control overall range of barter multipliers
	inline float fBarterMin() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBarterMin")) {
			return setting->data.f;
		}
		return 2.0f;
	}

	inline float fBarterMax() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBarterMax")) {
			return setting->data.f;
		}
		return 3.3f;
	}

	// These were used to clamp multipliers before applying perks
	inline float fBarterBuyMin() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBarterBuyMin")) {
			return setting->data.f;
		}
		return 1.05f;
	}

	inline float fBarterSellMax() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBarterSellMax")) {
			return setting->data.f;
		}
		return 0.95f;
	}

	// These were used to clamp multipliers after applied perks
	inline float fMinBuyMult() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fMinBuyMult")) {
			return setting->data.f;
		}
		return 1.05f;
	}

	inline float fMaxSellMult() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fMaxSellMult")) {
			return setting->data.f;
		}
		return 1.0f;
	}
}
