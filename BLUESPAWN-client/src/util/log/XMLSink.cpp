#include "util/log/XMLSink.h"
#include "common/StringUtils.h"

#include <chrono>
#include <iostream>
#include <common\Utils.h>

namespace Log{

	std::wstring ToWstringPad(DWORD value, size_t length=2){
		wchar_t* buf = new wchar_t[length + 1];
		swprintf(buf, (L"%0" + std::to_wstring(length) + L"d").c_str(), value);
		std::wstring str = buf;
		delete[] buf;
		return str;
	}

	void UpdateLog(XMLSink* sink){
		HandleWrapper hRecordEvent{ CreateEventW(nullptr, false, false, L"Local\\FlushLogs") };
		while(true){
			WaitForSingleObject(hRecordEvent, INFINITE);
			sink->Flush();
		}
	}

	XMLSink::XMLSink() :
		hMutex{ CreateMutexW(nullptr, false, nullptr) } ,
		Root{ XMLDoc.NewElement("bluespawn") },
		thread{ CreateThread(nullptr, 0, PTHREAD_START_ROUTINE(UpdateLog), this, CREATE_SUSPENDED, nullptr) }{
		SYSTEMTIME time{};
		GetLocalTime(&time);
		wFileName = L"bluespawn-" + ToWstringPad(time.wMonth) + L"-" + ToWstringPad(time.wDay) + L"-" + ToWstringPad(time.wYear, 4) + L"-"
			+ ToWstringPad(time.wHour) + ToWstringPad(time.wMinute) + L"-" + ToWstringPad(time.wSecond) + L".xml";
		XMLDoc.InsertEndChild(Root);
		ResumeThread(thread);
	}

	XMLSink::XMLSink(const std::wstring& wFileName) :
		hMutex{ CreateMutexW(nullptr, false, nullptr) },
		Root { XMLDoc.NewElement("bluespawn") },
		wFileName{ wFileName },
		thread{ CreateThread(nullptr, 0, PTHREAD_START_ROUTINE(UpdateLog), this, 0, nullptr) }{
		XMLDoc.InsertEndChild(Root);
	}

	XMLSink::~XMLSink(){
		XMLDoc.SaveFile(WidestringToString(wFileName).c_str());
		TerminateThread(thread, 0);
	}

	tinyxml2::XMLElement* CreateDetctionXML(const std::shared_ptr<DETECTION>& detection, tinyxml2::XMLDocument& XMLDoc){
		auto detect = XMLDoc.NewElement("detection");
		if(detection->Type == DetectionType::Registry){
			detect->SetAttribute("type", "Registry");
			auto RegistryDetection = std::static_pointer_cast<REGISTRY_DETECTION>(detection);
			auto Key = XMLDoc.NewElement("key");
			auto Value = XMLDoc.NewElement("value");
			auto Data = XMLDoc.NewElement("data");
			Key->SetText(WidestringToString(RegistryDetection->value.key.ToString()).c_str());
			Value->SetText(WidestringToString(RegistryDetection->value.GetPrintableName()).c_str());
			Data->SetText(WidestringToString(RegistryDetection->value.ToString()).c_str());
			detect->InsertEndChild(Key);
			detect->InsertEndChild(Value);
			detect->InsertEndChild(Data);
		} else if(detection->Type == DetectionType::File){
			detect->SetAttribute("type", "File");
			auto FileDetection = std::static_pointer_cast<FILE_DETECTION>(detection);
			auto Name = XMLDoc.NewElement("name");
			auto Path = XMLDoc.NewElement("path");
			auto MD5 = XMLDoc.NewElement("md5");
			auto SHA1 = XMLDoc.NewElement("sha1");
			auto SHA256 = XMLDoc.NewElement("sha256");
			auto Created = XMLDoc.NewElement("created");
			auto Modified = XMLDoc.NewElement("modified");
			auto Accessed = XMLDoc.NewElement("accessed");
			Name->SetText(WidestringToString(FileDetection->wsFileName).c_str());
			Path->SetText(WidestringToString(FileDetection->wsFilePath).c_str());
			MD5->SetText(WidestringToString(FileDetection->md5).c_str());
			SHA1->SetText(WidestringToString(FileDetection->sha1).c_str());
			SHA256->SetText(WidestringToString(FileDetection->sha256).c_str());
			Created->SetText(WidestringToString(FileDetection->created).c_str());
			Modified->SetText(WidestringToString(FileDetection->modified).c_str());
			Accessed->SetText(WidestringToString(FileDetection->accessed).c_str());
			detect->InsertEndChild(Name);
			detect->InsertEndChild(Path);
			detect->InsertEndChild(MD5);
			detect->InsertEndChild(SHA1);
			detect->InsertEndChild(SHA256);
			detect->InsertEndChild(Created);
			detect->InsertEndChild(Modified);
			detect->InsertEndChild(Accessed);
		} else if(detection->Type == DetectionType::Process){
			detect->SetAttribute("type", "Process");
			auto ProcessDetection = std::static_pointer_cast<PROCESS_DETECTION>(detection);
			auto Path = XMLDoc.NewElement("path");
			auto Cmd = XMLDoc.NewElement("cmdline");
			auto Pid = XMLDoc.NewElement("pid");
			Path->SetText(WidestringToString(ProcessDetection->wsImagePath).c_str());
			Cmd->SetText(WidestringToString(ProcessDetection->wsCmdline).c_str());
			Pid->SetText(std::to_string(ProcessDetection->PID).c_str());
			detect->InsertEndChild(Path);
			detect->InsertEndChild(Cmd);
			detect->InsertEndChild(Pid);
		} else if(detection->Type == DetectionType::Service){
			detect->SetAttribute("type", "Service");
			auto ServiceDetection = std::static_pointer_cast<SERVICE_DETECTION>(detection);
			auto Name = XMLDoc.NewElement("name");
			auto Path = XMLDoc.NewElement("path");
			auto Dll = XMLDoc.NewElement("dll");
			auto Pid = XMLDoc.NewElement("pid");
			Name->SetText(WidestringToString(ServiceDetection->wsServiceName).c_str());
			Path->SetText(WidestringToString(ServiceDetection->wsServiceExecutablePath).c_str());
			Dll->SetText(WidestringToString(ServiceDetection->wsServiceDll).c_str());
			Pid->SetText(std::to_string(ServiceDetection->ServicePID).c_str());
			detect->InsertEndChild(Name);
			detect->InsertEndChild(Path);
			detect->InsertEndChild(Dll);
			detect->InsertEndChild(Pid);
		} else if(detection->Type == DetectionType::Event){
			detect->SetAttribute("type", "Event");
			auto EventDetection = std::static_pointer_cast<EVENT_DETECTION>(detection);
			auto ID = XMLDoc.NewElement("id");
			auto RecordID = XMLDoc.NewElement("recordid");
			auto Time = XMLDoc.NewElement("time");
			auto Channel = XMLDoc.NewElement("channel");
			auto Raw = XMLDoc.NewElement("raw");
			ID->SetText(std::to_string(EventDetection->eventID).c_str());
			RecordID->SetText(std::to_string(EventDetection->eventRecordID).c_str());
			Time->SetText(WidestringToString(EventDetection->timeCreated).c_str());
			Channel->SetText(WidestringToString(EventDetection->channel).c_str());
			Raw->SetText(WidestringToString(EventDetection->rawXML).c_str());
			detect->InsertEndChild(ID);
			detect->InsertEndChild(RecordID);
			detect->InsertEndChild(Time);
			detect->InsertEndChild(Channel);
			detect->InsertEndChild(Raw);
			for(auto key : EventDetection->params){
				auto name = WidestringToString(key.first);
				auto idx1 = name.find("'") + 1;
				auto tag = XMLDoc.NewElement(name.substr(idx1, name.find_last_of("'") - idx1).c_str());
				tag->SetText(WidestringToString(key.second).c_str());
				detect->InsertEndChild(tag);
			}
		}
		return detect;
	}

	void XMLSink::LogMessage(const LogLevel& level, const std::string& message, const std::optional<HuntInfo> info, const std::vector<std::shared_ptr<DETECTION>>& detections){
		auto mutex = AcquireMutex(hMutex);
		if(level.Enabled() && level.severity == Severity::LogHunt && info){
			auto hunt = XMLDoc.NewElement("hunt");
			hunt->SetAttribute("agressiveness", info->HuntAggressiveness == Aggressiveness::Intensive ? "Intensive" :
				info->HuntAggressiveness == Aggressiveness::Normal ? "Normal" : "Cursory");
			hunt->SetAttribute("categories", static_cast<int64_t>(info->HuntCategories));
			hunt->SetAttribute("datasources", static_cast<int64_t>(info->HuntDatasources));
			hunt->SetAttribute("tactics", static_cast<int64_t>(info->HuntTactics));
			hunt->SetAttribute("time", SystemTimeToInteger(info->HuntStartTime));
			hunt->SetAttribute("datetime", WidestringToString(FormatWindowsTime(info->HuntStartTime)).c_str());

			auto name = XMLDoc.NewElement("name");
			name->SetText(WidestringToString(info->HuntName).c_str());
			hunt->InsertFirstChild(name);

			if(message.length() > 0){
				auto msg = XMLDoc.NewElement("message");
				msg->SetText(message.c_str());
				hunt->InsertEndChild(msg);
			}
			for(auto detection : detections){
				hunt->InsertEndChild(CreateDetctionXML(detection, XMLDoc));
			}

			Root->InsertEndChild(hunt);
		} else if(level.Enabled()) {
			auto msg = XMLDoc.NewElement(MessageTags[static_cast<DWORD>(level.severity)].c_str());
			SYSTEMTIME st;
			GetSystemTime(&st);
			msg->SetAttribute("time", SystemTimeToInteger(st));
			msg->SetText(message.c_str());
			Root->InsertEndChild(msg);
		}
	}

	bool XMLSink::operator==(const LogSink& sink) const {
		return (bool) dynamic_cast<const XMLSink*>(&sink) && dynamic_cast<const XMLSink*>(&sink)->wFileName == wFileName;
	}

	void XMLSink::Flush(){
		XMLDoc.SaveFile(WidestringToString(wFileName).c_str());
	}
};
