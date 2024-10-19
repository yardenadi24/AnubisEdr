#pragma once
#include <ntddk.h>
#include "AnbsNew.h"

// An template list class
// TODO:: Decide on the exact API for the list

template<typename Data>
class Element
{
public:
	Element(Data* pData, ULONG Id) :pData_(pData), Id_(Id) {}
	VOID FreeElement()
	{
		delete pData_;
	}
	Data* pData_;
	ULONG Id_;
	LIST_ENTRY ListEntry_ = {};
};

template<typename Data>
class List
{
private:
	LIST_ENTRY Head_ = {};
	PFAST_MUTEX Mutex_;
	SIZE_T Size_ = 0;
public:
	List() = delete;
	List(PFAST_MUTEX Mutex) :Mutex_(Mutex)
	{
		InitializeListHead(&Head_);
		ExInitializeFastMutex(Mutex_);
	}

	VOID FreeList()
	{	
		Clear();
	}

	// Delete copy
	List(const List& other) = delete;
	List& operator=(const List& other) = delete;

	// Allow move
	List(List&& other)
	{
		// Init Head
		InitializeListHead(&Head_);
		MoveElementFromList(other);
	}

	List& operator=(List&& other)
	{
		// Clean current data
		// Then move list
		Clear();
		MoveElementFromList(other);
		return *this;
	}

	VOID MoveElementFromList(List& other)
	{
		if (other.IsEmpty())
			return; // Nothing to move

		// Remove other head, and then re-initialize it,
		// Results in other head to point to an empty list.
		// at this point we can point our head to the rest of other
		// past list.

		ExAcquireFastMutex(other.Mutex_);
		SIZE_T OtherSize = other.Size_;
		PLIST_ENTRY Entry = other.Head_.Flink;
		RemoveEntryList(other.Head_);
		InitializeListHead(other.Head_);
		ExReleaseFastMutex(other.Mutex_);

		ExAcquireFastMutex(Mutex_);
		AppendTailList(&Head_, Entry);
		Size_ += OtherSize;
		ExReleaseFastMutex(Mutex_);
	}

	BOOLEAN IsEmpty()
	{
		return IsListEmpty(&Head_);
	}

	// Add element to the list with the data provided
	BOOLEAN Add(Data* pValue, ULONG Id)
	{
		if (Size_ >= MAX_PROCESSES)
		{
			DbgError("Reached %u process in list wont add new process", Size_);
			return FALSE;
		}

		Element<Data>* pElem = new  (NonPagedPool) Element<Data>(pValue, Id);
		if (!pElem)
		{
			// TODO:: Log error
			return FALSE;
		}
		pElem->pData_ = pValue;
		ExAcquireFastMutex(Mutex_);
		InsertHeadList(&Head_, &pElem->ListEntry_);
		Size_++;
		ExReleaseFastMutex(Mutex_);
		return TRUE;
	}

	Data* GetDataById(ULONG Id)
	{
		ExAcquireFastMutex(Mutex_);
		PLIST_ENTRY pListEntry = Head_.Flink;
		Data* pRetData = NULL;
		while (pListEntry1 != &Head_) {
			Element<Data>* pElem = CONTAINING_RECORD(pListEntry, Element<Data>, ListEntry_);
			if (pElem->Id_ == Id) {
				pRetData = pElem->pData_;
				ExReleaseFastMutex(Mutex_);
				return pRetData;
			}
			pListEntry = pListEntry->Flink;
		}
		ExReleaseFastMutex(Mutex_);
		// Could not find the element in the list
		return pRetData;
	}

	// Remove element by its data
	BOOLEAN RemoveByData(Data* pData)
	{
		ExAcquireFastMutex(Mutex_);
		PLIST_ENTRY pListEntry = Head_.Flink;
		while (pListEntry != &Head_) {
			Element<Data>* pElem = CONTAINING_RECORD(pListEntry, Element<Data>, ListEntry_);
			if (pElem->pData_ == pData) {
				RemoveEntryList(pListEntry);
				Size_--;
				pElem->FreeElement();
				delete pElem;
				ExReleaseFastMutex(Mutex_);
				return TRUE;
			}
			pListEntry = pListEntry->Flink;
		}
		ExReleaseFastMutex(Mutex_);
		// Could not find the element in the list
		return FALSE;
	}
	
	// Get the associated Element of the ListEntry
	static Element<Data>* GetElement(const PLIST_ENTRY pListEntry)
	{
		return (Element<Data>*)CONTAINING_RECORD(pListEntry, Element<Data>, ListEntry_);
	}

	// Iterator for the list
	class Iterator
	{
	
	private:
		// Let access list's private members
		friend List;

		PLIST_ENTRY pListEntry_ = NULL;

	public:

		Iterator() = default;
		Iterator(PLIST_ENTRY pListEntry) : pListEntry_(pListEntry) {}

		// Access values
		Data& operator* () const { return *GetElement(pListEntry_)->pData_; }
		Data* operator-> () const { return GetElement(pListEntry_)->pData_; }

		// Equality operator should check pListEntry
		BOOLEAN operator==(const Iterator& other) const
		{
			return pListEntry_ == other.pListEntry_;
		}

		BOOLEAN operator!=(const Iterator& other) const
		{
			return pListEntry_ != other.pListEntry_;
		}

		// Get next
		Iterator& operator++()
		{
			pListEntry_ = pListEntry_->Flink;
			return *this;
		}

		// Get prev
		Iterator& operator--()
		{
			pListEntry_ = pListEntry_->Blink;
			return *this;
		}

	};
	
	// Return the iterator pointing to data if found,
	// otherwise return end.
	template <typename This>
	static Iterator FindData(This* pThis, const Data& data)
	{
		Iterator EndIter = pThis->end();
		for (Iterator Iter = pThis->begin(); Iter != EndIter; ++Iter)
		{
			if (*Iter == data)
			{
				return Iter;
			}
		}
		return EndIter;
	}

	// Free specific entry
	VOID FreeListEntry(PLIST_ENTRY pListEntry)
	{
		Element<Data>* pElem = GetElement(pListEntry);
		pElem->FreeElement();
		delete pElem;
	}

	// Clear list
	// This also free memory of each element
	VOID Clear()
	{
		ExAcquireFastMutex(Mutex_);
		PLIST_ENTRY pListHead = &Head_;
		while (!IsListEmpty(pListHead))
			FreeListEntry(RemoveHeadList(pListHead));
		InitializeListHead(&Head_);
		Size_ = 0;
		ExReleaseFastMutex(Mutex_);
	}

	// Get the size of the list
	// TODO:: Improve to O(1) (Currently O(n))
	SIZE_T GetSize() const
	{
		return Size_;
	}

	// Iteration support
	Iterator begin() { return Iterator(Head_.Flink); }
	Iterator end() { return Iterator(&Head_); }

	// Return first element
	Data& GetFront()
	{
		return (*begin());
	}

	// Return last element
	Data& GetBack()
	{
		return (*(--end()));
	}

	// Find specific data in the list
	// Returns Iterator pointing to the data if found
	// Otherwise points to end()
	Iterator find(const Data& data)
	{
		return FindData<List<Data>>(this, data);
	}

	// Remove the data which the iterator currently on
	// Returns Iterator pointing to the next item
	//Iterator remove(const Iterator& Iter)
	//{
	//	Iterator Result = Iter;
	//	++Result;
	//	RemoveEntryList(Iter.pListEntry_);
	//	FreeListEntry(Iter.pListEntry_);
	//	return Result;
	//}

	// Look for specific data in the list.
	// Returns true if exists
	BOOLEAN has(const Data& data)
	{
		return find(data) != end();
	}

};