#include "hunt/hunts/HuntT1122.h"
#include "hunt/RegistryHunt.h"

#include "util/configurations/Registry.h"
#include "util/filesystem/Filesystem.h"
#include "util/log/Log.h"
#include "util/log/HuntLogMessage.h"
#include "util/processes/ProcessUtils.h"
#include "util/processes/CheckLolbin.h"

#include "common/Utils.h"

#include <algorithm>

using namespace Registry;

namespace Hunts{

	HuntT1122::HuntT1122() : Hunt(L"T1122 - COM Hijacking"){
		dwSupportedScans = (DWORD) Aggressiveness::Intensive;
		dwCategoriesAffected = (DWORD) Category::Configurations;
		dwSourcesInvolved = (DWORD) DataSource::Registry;
		dwTacticsUsed = (DWORD) Tactic::Persistence | (DWORD) Tactic::DefenseEvasion;
	}

#define ADD_FILE(file, ...)                                             \
	if(files.find(file) != files.end()){                                \
	    files.at(file).emplace_back(__VA_ARGS__);                       \
	} else {                                                            \
		files.emplace(file, std::vector<RegistryValue>{ __VA_ARGS__ }); \
	}

	int HuntT1122::ScanIntensive(const Scope& scope, Reaction reaction){
		LOG_INFO("Hunting for " << name << " at level Intensive");
		reaction.BeginHunt(GET_INFO());

		int detections = 0;

		std::map<std::wstring, std::vector<RegistryValue>> files{};

		for(auto key : CheckSubkeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID", true, true)){
			RegistryKey subkey{ key, L"InprocServer32" };
			if(subkey.Exists() && subkey.ValueExists(L"")){
				auto filename{ *subkey.GetValue<std::wstring>(L"") };
				auto path{ FileSystem::SearchPathExecutable(filename) };
				if(path){
					ADD_FILE(*path, RegistryValue{ subkey, L"", std::move(filename) });
				}
			}
			subkey = { key, L"InprocServer" };
			if(subkey.Exists() && subkey.ValueExists(L"")){
				auto filename{ *subkey.GetValue<std::wstring>(L"") };
				auto path{ FileSystem::SearchPathExecutable(filename) };
				if(path){
					ADD_FILE(*path, RegistryValue{ subkey, L"", std::move(filename) });
				}
			}
			if(key.ValueExists(L"InprocHandler32")){
				auto filename{ *key.GetValue<std::wstring>(L"InprocHandler32") };
				auto path{ FileSystem::SearchPathExecutable(filename) };
				if(path){
					ADD_FILE(*path, RegistryValue{ key, L"InprocHandler32", std::move(filename) });
				}
			}
			if(key.ValueExists(L"InprocHandler")){
				auto filename{ *key.GetValue<std::wstring>(L"InprocHandler") };
				auto path{ FileSystem::SearchPathExecutable(filename) };
				if(path){
					ADD_FILE(*path, RegistryValue{ key, L"InprocHandler", std::move(filename) });
				}
			}
			if(key.ValueExists(L"LocalServer")){
				auto filename{ *key.GetValue<std::wstring>(L"LocalServer") };
				ADD_FILE(filename, RegistryValue{ key, L"LocalServer", *subkey.GetValue<std::wstring>(L"") });
			}
			subkey = { key, L"LocalServer32" };
			if(subkey.Exists() && subkey.ValueExists(L"")){
				auto filename{ *subkey.GetValue<std::wstring>(L"") };
				ADD_FILE(filename, RegistryValue{ subkey, L"", *subkey.GetValue<std::wstring>(L"") });
			}
			if(subkey.Exists() && subkey.ValueExists(L"ServerExecutable")){
				auto filename{ *subkey.GetValue<std::wstring>(L"ServerExecutable") };
				ADD_FILE(filename, RegistryValue{ subkey, L"ServerExecutable", *subkey.GetValue<std::wstring>(L"") });
			}
		}

		for(auto pair : files){
			auto path{ pair.first };
			if(!FileSystem::CheckFileExists(path)){
				path = GetImagePathFromCommand(path);
				if(!FileSystem::CheckFileExists(path)){
					continue;
				}
			}

			FileSystem::File file{ path };
			if((!CompareIgnoreCaseW(file.GetFileAttribs().extension, L".dll") && IsLolbinMalicious(pair.first)) || !file.GetFileSigned()){
				for(auto& value : pair.second){
					reaction.RegistryKeyIdentified(std::make_shared<REGISTRY_DETECTION>(value));
					detections++;
				}
				reaction.FileIdentified(std::make_shared<FILE_DETECTION>(file));
				detections++;
			}
		}

		reaction.EndHunt();
		return detections;
	}

	std::vector<std::shared_ptr<Event>> HuntT1122::GetMonitoringEvents(){
		std::vector<std::shared_ptr<Event>> events;

		ADD_ALL_VECTOR(events, Registry::GetRegistryEvents(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID", true, true, true));

		return events;
	}
}