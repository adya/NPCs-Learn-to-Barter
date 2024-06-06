#pragma once

namespace NLB
{
	void Install();
}

namespace fmt
{
	template <>
	struct formatter<RE::NiPoint3>
	{
		template <class ParseContext>
		constexpr auto parse(ParseContext& a_ctx) {
			return a_ctx.begin();
		}

		template <class FormatContext>
		constexpr auto format(const RE::NiPoint3& point, FormatContext& a_ctx) {
			return fmt::format_to(a_ctx.out(), "({:.4f}, {:.4f}, {:.4f})", point.x, point.y, point.z);
		}
	};
}

namespace fmt
{
	template <>
	struct formatter<RE::Actor>
	{
		template <class ParseContext>
		constexpr auto parse(ParseContext& a_ctx) {
			return a_ctx.begin();
		}

		template <class FormatContext>
		constexpr auto format(const RE::Actor& actor, FormatContext& a_ctx) {
			return fmt::format_to(a_ctx.out(), "'{}' [0x{:X}]", actor.GetActorBase()->GetName(), actor.formID);
		}
	};
}
