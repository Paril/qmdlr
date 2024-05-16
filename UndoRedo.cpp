#include <imgui.h>
#include <SDL_assert.h>

#include "UndoRedo.h"
#include "Stream.h"
#include "ModelLoader.h"

static UndoRedoStorage *head = nullptr;

UndoRedoStorage::UndoRedoStorage(const char *clas, std::function<UndoRedoState *()> factory) :
	clas(clas),
	factory(factory)
{
	for (UndoRedoStorage *p = head; p; p = p->next)
		SDL_assert(strcmp(p->clas, clas));

	if (!head)
		head = this;
	else
	{
		next = head;
		head = this;
	}
}

/*static*/ UndoRedoStorage *UndoRedoStorage::Find(const char *name)
{
	for (UndoRedoStorage *p = head; p; p = p->next)
		if (!strcmp(p->clas, name))
			return p;

	return nullptr;
}

REGISTER_UNDO_REDO_ID(UndoRedoCombinedState);

/*virtual*/ void UndoRedoCombinedState::Read(std::istream &input) /*override*/
{
	size_t count;
	input >= count;

	std::string id;

	for (size_t i = 0; i < count; i++)
	{
		UndoRedoStorage *store = UndoRedoStorage::Find(id.c_str());
		UndoRedoState *state = store->factory();

		state->Read(input);
		Push(state);
	}
}

/*virtual*/ void UndoRedoCombinedState::Write(std::ostream &output) const /*override*/
{
	output <= _states.size();

	for (auto &state : _states)
	{
		output <= state->Id();
		state->Write(output);
	}
}

void UndoRedo::Push(UndoRedoState *state)
{
	SDL_assert(UndoRedoStorage::Find(state->Id()));

	if (_disabled)
	{
		delete state;
		return;
	}

	if (_combining)
	{
		_combinedTemp.Push(state);
		return;
	}

	RunDeferred(true);

	// if we've moved the pointer, we have to slice off
	// the list from the pointer forward.
	if (_pointer)
	{
		_list.erase(*_pointer, _list.end());
		_pointer = std::nullopt;
	}

	state->_iterator = _list.insert(_list.end(), UndoRedoStatePtr(state));

	Shrink();

	_size += state->Size();
}

// potentially combine multiple pushes into one state
void UndoRedo::BeginCombined()
{
	_combinedTemp = {};
	_combining = true;
}

// end combination state
void UndoRedo::EndCombined()
{
	_combining = false;
	
	if (_disabled)
		_combinedTemp = {};
	else
	{
		if (_combinedTemp._states.size() == 1)
			Push(std::move(_combinedTemp._states[0]));
		else if (_combinedTemp._states.size() != 0)
			Push(std::make_unique<UndoRedoCombinedState>(std::move(_combinedTemp)));
	}
}

void UndoRedo::Clear()
{
	if (_deferHandle)
		*_deferHandle = false;
	*this = {};
}

void UndoRedo::Shrink()
{
	// todo
}

void UndoRedo::Undo()
{
	if (!CanUndo())
		return;

	RunDeferred(true);

	if (!_pointer.has_value())
		_pointer = --_list.end();
	else
		(*_pointer)--;

	(*_pointer)->get()->Undo(model().mutator().data);
}

void UndoRedo::Redo()
{
	if (!CanRedo())
		return;

	RunDeferred(true);

	(*_pointer)->get()->Redo(model().mutator().data);

	(*_pointer)++;

	if (_pointer == _list.end())
		_pointer = std::nullopt;
}

// change the pointer to the given iterator
void UndoRedo::SetPointer(UndoRedoStateIterator it, bool ahead)
{
	if (it == _pointer)
		return;

	auto current_it = _pointer.value_or(_list.end());
	auto distance = ahead ? 
		std::distance(it, current_it) :
		std::distance(current_it, it);

	if (!ahead)
	{
		for (auto i = current_it; i != it; i++)
		{
			(*i)->Redo(model().mutator().data);
		}
	}
	else
	{
		for (auto i = --current_it; i != it; i--)
		{
			(*i)->Undo(model().mutator().data);
		}

		(*it)->Undo(model().mutator().data);
	}

	_pointer = it;
}

void UndoRedo::RunDeferred(bool force)
{
	if (!_deferredUndo)
		return;

	bool runUndo = force;

	if (!force)
	{
		_deferTime += ImGui::GetIO().DeltaTime;
		runUndo = _deferTime >= 1;
	}

	if (runUndo)
	{
		_deferredUndo();
		_deferredUndo = {};
		_deferTime = 0;
		*_deferHandle = false;
		_deferHandle = nullptr;
	}
}

// read/write
void UndoRedo::Write(std::ostream &stream)
{
	RunDeferred(true);

	stream <= _list.size();

	for (auto &entry : _list)
	{
		stream <= entry->Id();
		entry->Write(stream);
	}

	if (!_pointer)
		stream <= false;
	else
	{
		stream <= true;
		stream <= std::distance(_list.begin(), _pointer.value());
	}
}

void UndoRedo::Read(std::istream &stream)
{
	size_t count;
	stream >= count;

	std::string id;

	for (size_t i = 0; i < count; i++)
	{
		stream >= id;

		UndoRedoStorage *store = UndoRedoStorage::Find(id.c_str());
		UndoRedoState *state = store->factory();

		state->Read(stream);
		Push(state);
	}

	bool has_ptr;
	stream >= has_ptr;

	if (has_ptr)
	{
		ptrdiff_t dist;
		stream >= dist;

		_pointer = _list.begin();
		std::advance(_pointer.value(), dist);
	}
}

UndoRedo &undo()
{
	static UndoRedo instance;
	return instance;
}