#pragma once
#include <ntddk.h>
#include "AnbsMemory.h"

template<typename Data>
class Element
{
public:
	Element(Data* pData) :pData_(pData) {}
	~Element()
	{
		delete pData_;
	}
	Data* pData_;
	LIST_ENTRY ListEntry_;
};

template<typename Data>
class List
{
public:
	List()
	{
		InitializeListHead(&Head_);
	}

	~List()
	{
		Clear();
	}

	BOOLEAN Add(Data* pValue)
	{
		Element<Data>* pElem = new Element<Data>(pValue);
		if (!pElem)
		{
			// TODO:: Log error
			return FALSE;
		}
		pElem->pData_ = pValue;
		InsertHeadList(&Head_, &pElem->ListEntry_);

		return TRUE;
	}

	BOOLEAN RemoveByData(Data* pData)
	{
		PLIST_ENTRY pListEntry = Head_.Flink;
		while (pListEntry != &Head_) {
			Element<Data>* Elem = CONTAINING_RECORD(pListEntry, Element<Data>, ListEntry_);
			if (Elem->pData_ == pData) {
				RemoveEntryList(pListEntry);
				delete Elem;
				return TRUE;
			}
			pListEntry = pListEntry->Flink;
		}
		// Could not find the element in the list
		return FALSE;
	}



private:
	
	VOID Clear()
	{
		PLIST_ENTRY ListEntry = Head_.Flink;
		while (ListEntry != &Head_)
		{
			PLIST_ENTRY NextEntry = ListEntry->Flink;
			Element<Data>* ListElement = CONTAINING_RECORD(ListEntry, Element<Data>, ListEntry_);
			delete ListElement;
			ListEntry = NextEntry;
		}
		InitializeListHead(&Head_);
	}

	LIST_ENTRY Head_;
};