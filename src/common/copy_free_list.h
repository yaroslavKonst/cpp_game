#ifndef COPY_FREE_LIST_H
#define COPY_FREE_LIST_H

template<typename T>
class CopyFreeList
{
	friend class Iterator;

	struct Item
	{
		T data;
		Item* next;
	};

	Item* _front;
	Item* _back;

	size_t _size;

public:
	class Iterator
	{
		friend class CopyFreeList;

		Item* current;
		Item* prev;

	public:
		T& operator*()
		{
			return current->data;
		}

		void operator++()
		{
			prev = current;
			current = current->next;
		}

		bool operator==(const Iterator& iter)
		{
			return current == iter.current &&
				prev == iter.prev;
		}

		bool operator!=(const Iterator& iter)
		{
			return current != iter.current ||
				prev != iter.prev;
		}
	};

	CopyFreeList()
	{
		_size = 0;
		_front = nullptr;
		_back = nullptr;
	}

	~CopyFreeList()
	{
		while(_front) {
			_back = _front;
			_front = _front->next;
			delete _back;
		}
	}

	CopyFreeList(const CopyFreeList& list) = delete;
	CopyFreeList& operator=(const CopyFreeList& list) = delete;

	T& add()
	{
		if (!_back) {
			_front = new Item;
			_back = _front;
		} else {
			_back->next = new Item;
			_back = _back->next;
		}

		_back->next = nullptr;

		++_size;

		return _back->data;
	}

	void erase(const Iterator& iter)
	{
		if (iter.prev) {
			iter.prev->next = iter.current->next;
		} else {
			_front = iter.current->next;
		}

		delete iter.current;

		--_size;

		if (_size == 0) {
			_front = nullptr;
			_back = nullptr;
		}
	}

	size_t size()
	{
		return _size;
	}

	T& front()
	{
		return _front->data;
	}

	T& back()
	{
		return _back->data;
	}

	Iterator begin()
	{
		Iterator it;
		it.current = _front;
		it.prev = nullptr;

		return it;
	}

	Iterator end()
	{
		Iterator it;
		it.current = nullptr;
		it.prev = _back;

		return it;
	}
};

#endif
