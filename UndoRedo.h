#pragma once

#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <functional>
#include <iostream>

class ModelData;

using UndoRedoStatePtr = std::unique_ptr<class UndoRedoState>;
using UndoRedoStateList = std::list<UndoRedoStatePtr>;
using UndoRedoStateIterator = typename UndoRedoStateList::iterator;
using UndoRedoStatePointer = std::optional<UndoRedoStateIterator>;

// base abstract class for undo/redo operations.
class UndoRedoState
{
public:
	UndoRedoState() = default;
	UndoRedoState(UndoRedoState &&) = default;
	virtual ~UndoRedoState() { }

	UndoRedoState &operator=(UndoRedoState &&) = default;

	virtual void Undo(ModelData *data) = 0;

	virtual void Redo(ModelData *data) = 0;

	// name for this operation
	virtual const char *Name() const = 0;

	virtual void Read(std::istream &input) = 0;

	virtual void Write(std::ostream &output) const = 0;

	// nb: this must return the *full size* occupied
	// by this instance. It is used for calculating
	// buffer size spillage, and for serialization.
	virtual size_t Size() const = 0;

	inline const UndoRedoStateIterator &Iterator() const { return _iterator; }

	virtual const char *Id() const = 0;

protected:
	UndoRedoStateIterator _iterator;

	friend class UndoRedo;
};

class UndoRedoStorage
{
public:
	UndoRedoStorage(const char *clas, std::function<UndoRedoState *()> factory);

	const char *clas;
	std::function<UndoRedoState *()> factory;

	UndoRedoStorage *next = nullptr;

	static UndoRedoStorage *Find(const char *name);
};

#define SET_UNDO_REDO_ID(clas) \
	virtual const char *Id() const override { return #clas; }

#define REGISTER_UNDO_REDO_ID(clas) \
	static UndoRedoStorage clas ## _store { #clas, []() { return new clas; } }

#define REGISTER_UNDO_REDO_TPL(name, clas) \
	static UndoRedoStorage name ## _store { #name, []() { return new clas; } }

class UndoRedoCombinedState : public UndoRedoState
{
public:
	UndoRedoCombinedState() = default;
	UndoRedoCombinedState(UndoRedoCombinedState &&) = default;
	virtual ~UndoRedoCombinedState() { }

	UndoRedoCombinedState &operator=(UndoRedoCombinedState &&) = default;

	virtual void Undo(ModelData *data) override
	{
		for (auto it = _states.rbegin(); it != _states.rend(); it++)
			(*it)->Undo(data);
	}

	virtual void Redo(ModelData *data) override
	{
		for (auto it = _states.begin(); it != _states.end(); it++)
			(*it)->Redo(data);
	}

	virtual const char *Name() const override
	{
		return "Multiple Operations";
	}

	virtual void Read(std::istream &input) override;
	virtual void Write(std::ostream &output) const override;

	virtual size_t Size() const override
	{
		return _size;
	}

	void Push(UndoRedoState *state)
	{
		_size += state->Size();
		_states.emplace_back(state);
	}

	void Push(std::unique_ptr<UndoRedoState> &&state)
	{
		Push(state.release());
	}

	SET_UNDO_REDO_ID(UndoRedoCombinedState)

	const auto &getStates() const { return _states; }

protected:
	std::vector<std::unique_ptr<UndoRedoState>> _states;
	size_t _size = sizeof(*this);

	friend class UndoRedo;
};

class UndoRedo
{
public:
	UndoRedoStateList &List() { return _list; }
	// total byte size of current data
	const size_t &Size() const { return _size; }
	// pointer to where we are in the linked list; if this is
	// null, we have not performed any operations yet (are at the head).
	const UndoRedoStatePointer &Pointer() const { return _pointer; }

	// push a new state onto the head stack.
	// the undo/redo manager takes control of the pointer.
	void Push(UndoRedoState *state);
	
	// push a new state onto the head stack.
	// the undo/redo manager takes control of the pointer.
	inline void Push(std::unique_ptr<UndoRedoState> &&ptr)
	{
		Push(ptr.release());
	}

private:
	UndoRedoCombinedState _combinedTemp;
	bool _combining = false;

public:
	// potentially combine multiple pushes into one state
	void BeginCombined();

	// end combination state
	void EndCombined();

	// push a deferred undo state; it will not immediately add
	// to the undo/redo list, but will execute a supplied callback
	// to add to the undo/redo list when a timer has expired.
	// `handle` should be a static boolean linked to your deferred callback
	// so it knows whether the current callback is what's being deferred or not.
	// `constructState` should be a function that returns state
	// which is called only if the deferred callback has not been created yet;
	// this state is then passed into the push function.
	template<typename TState>
	void PushDeferred(bool &handle, std::function<TState()> constructState, std::function<void(const TState &)> push)
	{
		if (_disabled)
			return;

		// already waiting on this object
		if (handle)
		{
			_deferTime = 0;
			return;
		}

		// doing a defer currently, so run it now
		if (_deferredUndo)
			RunDeferred(true);

		_deferHandle = &handle;
		handle = true;

		_deferredUndo = [this, &handle, push, state = constructState()]() {
			handle = false;
			decltype(_deferredUndo) swapped;
			std::swap(swapped, _deferredUndo);
			push(state);
		};
	}

	// temporarily disable undo/redo pushing
	void BeginDisabled() { _disabled = true; }
	void EndDisabled() { _disabled = false; }

	// clear entire stack
	void Clear();

	// shrink to fit max undo/redo buffer size
	void Shrink();

	// perform an undo (if possible)
	void Undo();

	// perform a redo (if possible)
	void Redo();

	// change the pointer to the given iterator
	void SetPointer(UndoRedoStateIterator it, bool ahead);

	// check if we can perform an undo
	inline bool CanUndo() { return _list.size() && _pointer != _list.begin(); }

	// check if we can perform a redo
	inline bool CanRedo() { return _list.size() && _pointer.has_value(); }

	// check deferred handling; if force is true,
	// it will run immediately.
	void RunDeferred(bool force = false);

	// read/write
	void Write(std::ostream &stream);
	void Read(std::istream &stream);

private:
	// saved
	UndoRedoStateList _list;
	size_t _size = 0;
	UndoRedoStatePointer _pointer = std::nullopt;

	// not saved
	std::function<void()> _deferredUndo {};
	double _deferTime = 0;
	bool *_deferHandle = nullptr;
	bool _disabled = false;

	friend class UndoRedoState;
};

UndoRedo &undo();