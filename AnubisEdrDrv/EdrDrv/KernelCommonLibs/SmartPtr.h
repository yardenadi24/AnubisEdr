#pragma once

#include <ntddk.h>

#include "../Common/AnbsCommons.h"

#define SMART_PTR_MEM_TAG 'rtpS'

typedef struct _SMART_PTR_DATA
{
	PVOID pData_ = NULL;
	volatile LONG Count_ = 0;
	BOOLEAN Initialized_ = FALSE;

	VOID Init(PVOID pData)
	{
		pData_ = pData;
		Count_ = 1;
		Initialized_ = TRUE;
	}

	LONG IncRef() {
		if (!Initialized_) return -1;
		return InterlockedIncrement(&Count_);
	}

	LONG DecRef() {
		if (!Initialized_) return -1;
		return InterlockedDecrement(&Count_);
	}
}SMART_PTR_DATA, *PSMART_PTR_DATA;



template <typename Data>
class SmartPtr {

public:
	// New smart ptr
	// construct the struct itself
	SmartPtr(Data* pData)
	{
		pDataStruct_ = (PSMART_PTR_DATA)ExAllocatePoolWithTag(NonPagedPool, sizeof(SMART_PTR_DATA), SMART_PTR_MEM_TAG);
		if (pDataStruct_ = NULL)
		{
			DbgError("Failed allocating smart pointer for data at: (0xp)", pData);
			return;
		}

		// Allocated the struct now initialize it
		pDataStruct_->Init(pData);
	}

	// Copy constructor
	SmartPtr(const SmartPtr<Data>& Other)
	{
		// We want to increase count and copy struct pointer
		pDataStruct_ = Other.pDataStruct_;
		if (pDataStruct_)
		{
			pDataStruct_->IncRef();
		}
	}

	// Copy assignment
	SmartPtr<Data>& void operator=(const SmartPtr<Data>& Other)
	{
		// Check for self assignment
		if (this != &Other)
		{
			if (pDataStruct_  && (1 > pDataStruct_->DecRef()))
			{
				// We need to clean this struct as we 
				// were the last owner of this ptr
				ExFreePool(pDataStruct_);
			}

			pDataStruct_ = Other->pDataStruct_;
			if (pDataStruct_)
			{
				pDataStruct_->IncRef();
			}
		}

		return *this;
	}

	// Move	constructor
	SmartPtr(SmartPtr<Data>&& other)
	{
		// On move we dont need to increment the ref count
		pDataStruct_ = other.pDataStruct_;
		other.pDataStruct_ = NULL;
	}

	// Move assignment operator
	SmartPtr<Data>& void operator=(SmartPtr<Data>&& other)
	{
		// Check for self assignment
		if (this != &Other)
		{
			if (pDataStruct_ && (1 > pDataStruct_->DecRef()))
			{
				// We need to clean this struct as we 
				// were the last owner of this ptr
				ExFreePool(pDataStruct_);
			}

			pDataStruct_ = Other->pDataStruct_;
			Other->pDataStruct_ = NULL;
		}


		return *this;
	}

private:
	PSMART_PTR_DATA pDataStruct_;
};