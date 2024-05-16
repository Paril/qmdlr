#include "Events.h"

#include <SDL_assert.h>

void EventDispatcher::Register(EventType type, EventHandlerFunc handler, EventContext context, bool catchRepeat)
{
	_handlers.emplace(type, EventHandler { handler, context, catchRepeat });
}

void EventDispatcher::Push(const Event &event)
{
	_queue.push(event);
}

void EventDispatcher::Dispatch()
{
    for (; !_queue.empty(); _queue.pop())
	{
		Event ev = _queue.front();

		auto range = _handlers.equal_range(ev.type);

		for (auto it = range.first; it != range.second; ++it)
		{
			if (ev.context != EventContext::Any &&
				it->second.context != EventContext::Any &&
				ev.context != it->second.context)
				continue;
			else if (!it->second.catchRepeat && ev.repeat)
				continue;

			it->second.func(ev);
		}
	}
}

EventDispatcher &events()
{
	static EventDispatcher dispatcher;
	return dispatcher;
}