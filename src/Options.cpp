#include "Options.h"
#include "CLIBUtil/string.hpp"

namespace NLA::Options
{

	void Load() {
		logger::info("{:*^40}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsLearnToBarter.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		if (ini.LoadFile(options.string().c_str()) >= 0) {
			// TODO: Do loading here.
		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsLearnToAim.ini not found. Default options will be used.)");
			logger::info("");
		}

		// TODO: Log options here.

		logger::info("{:*^40}", "");
	}
}
