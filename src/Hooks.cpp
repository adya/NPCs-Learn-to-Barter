#include "Hooks.h"
#include "Options.h"

namespace NLB
{
	static RE::Actor* GetMerchant() {
		REL::Relocation<RE::ActorHandle*> merchant{ RELOCATION_ID(519283, 405823) };
		if (auto m = *merchant; m) {
			return m.get().get();
		}
		return nullptr;
	}

	struct CalculateBaseFactor
	{
		static void thunk(float clampedSpeech, float* buyMult, float* sellMult) {
			if (auto merchant = GetMerchant(); merchant) {
				logger::info("");
				logger::info("{:*^40}", "BARTER");
				logger::info("Merchant: {}", *merchant);

				const float merchantSpeech = merchant->GetActorValue(RE::ActorValue::kSpeech);
				const float playerSpeech = RE::PlayerCharacter::GetSingleton()->GetActorValue(RE::ActorValue::kSpeech);

				logger::info("");
				logger::info("Merchant Speech: {}", merchantSpeech);
				logger::info("Player Speech: {}", playerSpeech);

				const float barterMin = Settings::fBarterMin();
				const float barterMax = Settings::fBarterMax();

				logger::info("");
				logger::info("fBarterMin: {}", barterMin);
				logger::info("fBarterMax: {}", barterMax);
#ifndef NDEBUG
				// To be able to quickly switch between vanilla and modded behavior in-game.
				if (Settings::fMinBuyMult() == 0.0f) {
					func(clampedSpeech, buyMult, sellMult);
					return;
				}
#endif
				if (barterMin >= 1.0f) {
					const float maxSurcharge = Options::maxSurcharge;
					const float maxDiscount = Options::maxDiscount;

					const float M1 = (1.0f - maxSurcharge) * barterMax;  // lowest BarterMax
					const float M2 = (1.0f + maxSurcharge) * barterMax;  // highest BarterMax
					const float m1 = (1.0f - maxDiscount) * barterMin;   // lowest BarterMin
					const float m2 = (1.0f + maxDiscount) * barterMin;   // highest BarterMin

					constexpr float D1 = -100.0f;  // Skill delta min
					constexpr float D2 = 100.0f;   // Skill delta max

					const float d = std::clamp(merchantSpeech - playerSpeech, 1.75f * D1, 1.75f * D2);

					const auto B1 = m1 + (d - D1) * (m2 - m1) / (D2 - D1);  // Scaled BarterMin
					const auto B2 = M1 + (d - D1) * (M2 - M1) / (D2 - D1);  // Scaled BarterMax

					logger::info("");
					logger::info("M1: {}", M1);
					logger::info("M2: {}", M2);
					logger::info("m1: {}", m1);
					logger::info("m2: {}", m2);
					logger::info("");
					logger::info("d: {}", d);
					logger::info("");
					logger::info("B1: {}", B1);
					logger::info("B2: {}", B2);

					*buyMult = B2 - (B2 - B1) * min(playerSpeech, 100.0f) * 0.01f;
					*sellMult = 1.0f / *buyMult;

					if (!Options::ignoreLimits) {
						*buyMult = max(*buyMult, Settings::fBarterBuyMin());
						*sellMult = min(*sellMult, Settings::fBarterSellMax());
					}
				} else {  // This uses "fair" formula where equal skills means that both parties get a baseline price (100% by default)

					const float M1 = barterMin;  // discountMax
					const float M2 = 1;          // equal
					const float M3 = barterMax;  // surchargeMax

					constexpr float D1 = -100.0f;  // Skill delta min
					constexpr float D2 = 0.0f;     // Skill delta max
					constexpr float D3 = 100.0f;   // Skill delta max

					const float d = merchantSpeech - playerSpeech;

					const float baseline = (Options::parityBaseline - 1.0f);  // Baseline that offsets whole formula

					logger::info("");
					logger::info("M1: {}", M1);
					logger::info("M2: {}", M2);
					logger::info("M3: {}", M3);
					logger::info("");
					logger::info("d: {}", d);

					if (d <= 0) {
						const float B1 = M1 + (d - D1) * (M2 - M1) / (D2 - D1);  // Scaled maxDiscount
						*buyMult = baseline + B1;
						*sellMult = baseline + 1.0f / B1;
					} else {
						const float B2 = M2 + (d - D2) * (M3 - M2) / (D3 - D2);  // Scaled maxSurcharge
						*buyMult = baseline + B2;
						*sellMult = baseline + 1.0f / B2;
					}
				}

				logger::info("{:-^40}", "");
			} else {
				func(clampedSpeech, buyMult, sellMult);
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct ApplyPerkEntries_Buy
	{
		static void thunk(RE::BGSPerkEntry::EntryPoint ep, RE::Actor* player, RE::Actor* merchant, float* mult) {
			logger::info("Player Buy Mult: {}", *mult);
			func(ep, player, merchant, mult);
			logger::info("Player Buy Mult with Perks: {}", *mult);
#ifndef NDEBUG
			if (Settings::fMaxSellMult() == 0.0f) {
				logger::info("{:-^40}", "");
				return;
			}
#endif
			float sellMult = 1.0f;

			func(RE::BGSPerkEntry::EntryPoint::kModSellPrices, merchant, player, &sellMult);
			logger::info("");
			logger::info("Merchant Sell Mult: {}", sellMult);

			func(RE::BGSPerkEntry::EntryPoint::kModSellPrices, merchant, player, mult);
			logger::info("");
			logger::info("Merged Buy Mult: {}", *mult);

			if (!Options::ignoreLimits) {
				*mult = max(*mult, Settings::fMinBuyMult());
			}
			logger::info("{:-^40}", "");
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct ApplyPerkEntries_Sell
	{
		static void thunk(RE::BGSPerkEntry::EntryPoint ep, RE::Actor* player, RE::Actor* merchant, float* mult) {
			logger::info("Player Sell Mult: {}", *mult);
			func(ep, player, merchant, mult);
			logger::info("Player Sell Mult with Perks: {}", *mult);
#ifndef NDEBUG
			if (Settings::fMaxSellMult() == 0.0f) {
				return;
			}
#endif
			float buyMult = 1.0f;

			func(RE::BGSPerkEntry::EntryPoint::kModBuyPrices, merchant, player, &buyMult);
			logger::info("");
			logger::info("Merchant Buy Mult: {}", buyMult);

			func(RE::BGSPerkEntry::EntryPoint::kModBuyPrices, merchant, player, mult);
			logger::info("");
			logger::info("Merged Sell Mult: {}", *mult);

			if (!Options::ignoreLimits) {
				*mult = min(*mult, Settings::fMaxSellMult());
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install() {
		logger::info("{:*^40}", "HOOKS");

		const REL::Relocation<std::uintptr_t> barterMenu{ RELOCATION_ID(50001, 50945) };

		stl::write_thunk_call<CalculateBaseFactor>(barterMenu.address() + OFFSET(0x4E8, 0x7B3));

		logger::info("Installed skill-based scaling logic");

		stl::write_thunk_call<ApplyPerkEntries_Buy>(barterMenu.address() + OFFSET(0x51C, 0x7EA));

		// Disable min/maxing of buyMult
#ifdef SKYRIM_AE
		// maxss xmm1, [rbp+0A0h] (F3 0F 5F 8D A0 00 00 00)
		//                               vv
		// movss xmm1, [rbp+0A0h] (F3 0F 10 8D A0 00 00 00)
		REL::safe_fill(barterMenu.address() + 0x814, 0x10, 1);

		// Erase this:
		// maxss xmm1, cs:gFloatOne (F3 0F 5F 0D 56 B3 D8 00)
		REL::safe_fill(barterMenu.address() + 0x81A, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x81B, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x81C, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x81D, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x81E, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x81F, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x820, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x821, 0x90, 1);
#else
		// In SE they don't use maxss instruction and instead do it with ifs, so disabling it is a bit more complicated.

		// Erase this:
		// movss   xmm0, cs:fMinBuyMult_141DE8458 (F3 0F 10 05 E7 62 59 01)
		REL::safe_fill(barterMenu.address() + 0x539, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x53A, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x53B, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x53C, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x53D, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x53E, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x53F, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x540, 0x90, 1);

		// Erase these:
		// comiss  xmm0, xmm1     (0F 2F C1)
		// ja short loc_14085217E (77 03)
		REL::safe_fill(barterMenu.address() + 0x546, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x547, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x548, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x549, 0x90, 1);

		// Erase these:
		// comiss  xmm0, cs:fOne_1415232D8 (0F 2F 05 53 11 CD 00)
		// jnb short loc_14085218F         (73 08)
		// movss xmm0, cs : fOne_1415232D8 (F3 0F 10 05 49 11 CD 00)
		REL::safe_fill(barterMenu.address() + 0x54A, 0x90, 1);

		REL::safe_fill(barterMenu.address() + 0x54E, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x54F, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x550, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x551, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x552, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x553, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x554, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x555, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x556, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x557, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x558, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x559, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x55A, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x55B, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x55C, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x55D, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x55E, 0x90, 1);

		// After the above, we should only have (where [rbp + 112] is our buyMult):
		// movss   xmm1, [rbp + 112]
		// movaps  xmm0, xmm1
#endif
		logger::info("Installed buying logic");

		stl::write_thunk_call<ApplyPerkEntries_Sell>(barterMenu.address() + OFFSET(0x534, 0x805));

		// Disable min/maxing of sellMult
#ifdef SKYRIM_AE
		// minss xmm0, [rbp+98h] (F3 0F 5D 85 98 00 00 00)
		//                              vv
		// movss xmm0, [rbp+98h] (F3 0F 10 85 98 00 00 00)
		REL::safe_fill(barterMenu.address() + 0x834, 0x10, 1);

		// Erase this:
		// minss xmm0, cs:gFloatOne (F3 0F 5D 05 36 B3 D8 00)
		REL::safe_fill(barterMenu.address() + 0x83A, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x83B, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x83C, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x83D, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x83E, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x83F, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x840, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x841, 0x90, 1);
#else
		// In SE they don't use maxss instruction and instead do it with ifs, so disabling it is a bit more complicated.

		// Erase this:
		// movss   xmm1, cs:fMaxSellMult_141DE8470 (F3 0F 10 0D D4 62 59 01)
		REL::safe_fill(barterMenu.address() + 0x564, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x565, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x566, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x567, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x568, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x569, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x56A, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x56B, 0x90, 1);

		// Erase these:
		// comiss  xmm1, xmm2			(0F 2F CA)
		// jb      short loc_1408521A9	(72 03)
		REL::safe_fill(barterMenu.address() + 0x571, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x572, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x573, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x574, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x575, 0x90, 1);

		// Erase these:
		// comiss  xmm1, cs:fOne_1415232D8	(0F 2F 0D 28 11 CD 00)
		// jbe short loc_1408521BB			(76 09)
		// mov[rbp + 50h + a3], 3F800000h	(C7 45 68 00 00 80 3F)
		// jmp short loc_1408521C0			(EB 05)
		REL::safe_fill(barterMenu.address() + 0x579, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x57A, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x57B, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x57C, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x57D, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x57E, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x57F, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x580, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x581, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x582, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x583, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x584, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x585, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x586, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x587, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x588, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x589, 0x90, 1);
		REL::safe_fill(barterMenu.address() + 0x58A, 0x90, 1);

		// After the above, we should only have (where [rbp+104] is our sellMult):
		// movss   xmm2, [rbp+104]
		// movaps  xmm1, xmm2
		// movss   [rbp+104], xmm1
#endif
		logger::info("Installed selling logic");
	}
}
