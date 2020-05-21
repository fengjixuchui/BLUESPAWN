#pragma once

#include <functional>
#include <string>
#include "reaction/Reaction.h"
#include "hunt/Scope.h"
#include "Event.h"
#include "util/eventlogs/EventSubscription.h"

class EventManager {

	public:
		DWORD SubscribeToEvent(const std::shared_ptr<Event>& e, const std::function<void()>& callback);
		
		// EventManager is a singleton class; call GetInstance() to get an instance of it.
		static EventManager& GetInstance();

	private:

		// Make constructor private for singleton class
		EventManager();

		// Delete copy and move constructors
		EventManager(const EventManager&) = delete;
		EventManager(EventManager&&) = delete;
		EventManager operator=(const EventManager&) = delete;
		EventManager operator=(EventManager&&) = delete;

		static EventManager manager;
		std::vector<std::shared_ptr<Event>> vEventList;
};