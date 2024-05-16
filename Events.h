#pragma once

#include <queue>
#include <functional>
#include <unordered_map>
#include <algorithm>

// UI buttons and key shortcuts push to an event queue
// which is handled post-UI.
#define EVENT_IDS(x) \
	x(ChangeTool_Pan), \
	x(ChangeTool_Select), \
	x(ChangeTool_Move), \
	x(ChangeTool_Rotate), \
	x(ChangeTool_Scale), \
	x(ChangeTool_CreateVertex), \
	x(ChangeTool_CreateFace), \
 \
	x(Open), \
	x(Save), \
	x(SaveAs), \
 \
	x(SelectAll), \
	x(SelectNone), \
	x(SelectInverse), \
	x(SelectConnected), \
	x(SelectTouching), \
 \
	x(ToggleModifyX), \
	x(ToggleModifyY), \
	x(ToggleModifyZ), \
 \
	x(UV_VerticesNone), \
	x(UV_VerticesDot), \
	x(UV_VerticesCircle), \
 \
	x(UV_LineMode), \
 \
	x(ZoomIn), \
	x(ZoomOut), \
 \
	x(Editor_3D_SetRenderModeWireframe), \
	x(Editor_3D_SetRenderModeFlat), \
	x(Editor_3D_SetRenderModeTextured), \
	x(Editor_3D_SetRenderDrawBackfaces), \
	x(Editor_3D_SetRenderPerVertexNormals), \
	x(Editor_3D_SetRenderShading), \
	x(Editor_3D_SetRenderShowOverlay), \
	x(Editor_3D_SetRenderFiltering), \
	x(Editor_3D_SetRenderShowTicks), \
	x(Editor_3D_SetRenderShowNormals), \
	x(Editor_3D_SetRenderShowOrigin), \
	x(Editor_3D_SetRenderShowGrid), \
 \
	x(Editor_2D_SetRenderModeWireframe), \
	x(Editor_2D_SetRenderModeFlat), \
	x(Editor_2D_SetRenderModeTextured), \
	x(Editor_2D_SetRenderDrawBackfaces), \
	x(Editor_2D_SetRenderPerVertexNormals), \
	x(Editor_2D_SetRenderShading), \
	x(Editor_2D_SetRenderShowOverlay), \
	x(Editor_2D_SetRenderFiltering), \
	x(Editor_2D_SetRenderShowTicks), \
	x(Editor_2D_SetRenderShowNormals), \
	x(Editor_2D_SetRenderShowOrigin), \
	x(Editor_2D_SetRenderShowGrid), \
 \
	x(SelectMode_Vertex), \
	x(SelectMode_Face), \
 \
	x(Undo), \
	x(Redo), \
 \
	x(AddSkin), \
	x(DeleteSkin), \
	x(ImportSkin), \
	x(ExportSkin), \
 \
	x(SyncSelection), \
 \
	x(Last)

enum class EventType
{
#define EVENT_ID_HANDLER(x) x
	EVENT_IDS(EVENT_ID_HANDLER)
#undef EVENT_ID_HANDLER
};

constexpr const char *EventTypeNames[] = {
#define EVENT_ID_HANDLER(x) #x
	EVENT_IDS(EVENT_ID_HANDLER)
#undef EVENT_ID_HANDLER
};

static_assert(std::size(EventTypeNames) == ((size_t) EventType::Last) + 1);

enum class EventContext
{
	Any,
	Editor3D,
	EditorUV,

	Skip // special context for shortcut editor
};

struct Event
{
	EventType		type;
	EventContext	context = EventContext::Any;
	bool			repeat = false;
	union {
	}				args;
};

using EventHandlerFunc = std::function<void(Event&)>;

class EventDispatcher
{
	struct QueuedEvent
	{
		Event			event;
		EventContext	context;
	};

	struct EventHandler
	{
		EventHandlerFunc	func;
		EventContext		context;
		bool				catchRepeat;
	};

public:
	void Register(EventType type, EventHandlerFunc handler, EventContext context = EventContext::Any, bool catchRepeat = false);
	void Push(const Event &event);

	inline void Push(const EventType &type, EventContext context = EventContext::Any, bool isRepeat = false)
	{
		Push(Event { type, context, isRepeat });
	}

	void Dispatch();

private:
	std::queue<Event> _queue;
	std::unordered_multimap<EventType, EventHandler> _handlers;
};

EventDispatcher &events();