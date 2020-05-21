#include "hunt/hunts/HuntT1035.h"
#include "hunt/RegistryHunt.h"

#include "util/log/Log.h"
#include "util/configurations/Registry.h"
#include "util/filesystem/FileSystem.h"
#include "util/filesystem/YaraScanner.h"
#include "util/processes/ProcessUtils.h"
#include "util/processes/CheckLolbin.h"
#include "common/Utils.h"

using namespace Registry;

namespace Hunts {
	HuntT1035::HuntT1035() : Hunt(L"T1035 - Service Execution") {
		dwSupportedScans = (DWORD) Aggressiveness::Normal;
		dwCategoriesAffected = (DWORD) Category::Configurations | (DWORD) Category::Files | (DWORD) Category::Processes;
		dwSourcesInvolved = (DWORD) DataSource::Registry | (DWORD) DataSource::FileSystem;
		dwTacticsUsed = (DWORD) Tactic::Execution;
	}

	int HuntT1035::EvaluateService(Registry::RegistryKey key, Reaction& reaction) {
		int detections = 0;

		auto cmd{ *key.GetValue<std::wstring>(L"ImagePath") };

		if(IsLolbinMalicious(cmd)){
				reaction.RegistryKeyIdentified(std::make_shared<REGISTRY_DETECTION>(RegistryValue{ key, L"ImagePath", key.GetValue<std::wstring>(L"ImagePath").value() }));
				detections++;
		} else{
			auto filepath = GetImagePathFromCommand(cmd);

			FileSystem::File image = FileSystem::File(filepath);
			if(image.GetFileExists() && !image.GetFileSigned()){
				reaction.RegistryKeyIdentified(std::make_shared<REGISTRY_DETECTION>(RegistryValue{ key, L"ImagePath", key.GetValue<std::wstring>(L"ImagePath").value() }));

				auto& yara = YaraScanner::GetInstance();
				YaraScanResult result = yara.ScanFile(image);

				reaction.FileIdentified(std::make_shared<FILE_DETECTION>(image));

				detections += 2;
			}
		}

		RegistryKey subkey = RegistryKey{ key, L"Parameters" };

		for (auto regkey : { key, subkey }) {
			if (!regkey.Exists()) {
				continue;
			}

			std::wstring value = L"ServiceDll";
			if (regkey.ValueExists(value)) {
				auto filepath2 = FileSystem::SearchPathExecutable(regkey.GetValue<std::wstring>(value).value());
				if (filepath2 && FileSystem::CheckFileExists(*filepath2)) {
					FileSystem::File servicedll = FileSystem::File(*filepath2);

					if (!servicedll.GetFileSigned()) {
						reaction.RegistryKeyIdentified(std::make_shared<REGISTRY_DETECTION>(RegistryValue{ regkey, value, regkey.GetValue<std::wstring>(value).value() }));

						auto& yara = YaraScanner::GetInstance();
						YaraScanResult result = yara.ScanFile(servicedll);

						reaction.FileIdentified(std::make_shared<FILE_DETECTION>(servicedll));

						detections += 2;
					}
				}
			}
		}

		return detections;
	}

	int HuntT1035::ScanNormal(const Scope& scope, Reaction reaction) {
		LOG_INFO(L"Hunting for " << name << " at level Normal");
		reaction.BeginHunt(GET_INFO());

		int detections = 0;

		auto services = RegistryKey{ HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services" };

		for (auto service : services.EnumerateSubkeys()) {
			if (service.GetValue<DWORD>(L"Type") >= 0x10u) {
				detections += EvaluateService(service, reaction);
			}
		}

		reaction.EndHunt();
		return detections;
	}

	std::vector<std::shared_ptr<Event>> HuntT1035::GetMonitoringEvents() {
		std::vector<std::shared_ptr<Event>> events;

		ADD_ALL_VECTOR(events, Registry::GetRegistryEvents(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services", false, false, true));

		return events;
	}
}