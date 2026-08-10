//
// Copyright (C) Microsoft Corporation
// All rights reserved.
//

#include "pch.h"
#include <wrl\wrappers\corewrappers.h>
#pragma hdrstop

namespace Platform { namespace Details {

	typedef struct {
		SRWLOCK targetsLock;
		SRWLOCK addRemoveLock;
	} EventLockInternal;

	static_assert(sizeof(EventLock) == sizeof(EventLockInternal), "EventLock struct size must be the same as EventLockInternal");

	CPPCLI_FUNC void __stdcall EventSourceInitialize(void** targets)
	{
		*targets = nullptr;
	}

	CPPCLI_FUNC void __stdcall EventSourceUninitialize(void** targets)
	{
		// Release targets array
		::Platform::Array<Platform::Delegate^>^* targetsList = reinterpret_cast<::Platform::Array<Platform::Delegate^>^*>(targets);
		*targetsList = nullptr;
	}

	CPPCLI_FUNC void* __stdcall EventSourceGetTargetArray(void* targets, EventLock* lock)
	{
		auto lockData = reinterpret_cast<EventLockInternal*>(lock);

		auto targetsPointerLock = ::Microsoft::WRL::Wrappers::SRWLock::LockShared(&lockData->targetsLock);
		// AddRef on targetsList pointer must happen on lock
		auto targetsList = reinterpret_cast< ::Platform::Array<Platform::Delegate^>^ >(targets);
		return __detach_as_voidptr(reinterpret_cast<void**>(&targetsList));
	}

	CPPCLI_FUNC ::Windows::Foundation::EventRegistrationToken __stdcall EventSourceAdd(void** targets, EventLock* lock, ::Platform::Delegate^ delegateInterface)
	{
		// lock add/remove
		//    pNewList = _targets
		//    pNewList[last] = delegateInterface
		//    lock pointer exhange
		//        pTmpList = _targets
		//        _targets = pNewList
		//    unlock pointer exhange
		// unlock add/remove

		::Windows::Foundation::EventRegistrationToken token = { 0 };
		auto lockData = reinterpret_cast<EventLockInternal*>(lock);

		// Make sure that delegate interface pointer is not null
		if (delegateInterface == nullptr)
		{
			__abi_WinRTraiseInvalidArgumentException();
		}

		// This must be defined outside of the scope where the addRemoveLock is taken
		// to ensure that it's destructor is called after the lock is released
		::Platform::Array<Platform::Delegate^>^ pTempList = nullptr;
		{ // lock scope for addRemoveLock

			// Casting void* _targets to array type
			::Platform::Array<Platform::Delegate^>^* targetsList = reinterpret_cast< ::Platform::Array<Platform::Delegate^> ^* >(targets);

			// We are doing "copy to new list and add" so as not to disturb the list that may be
			// currently undergoing a walk and fire (invoke).

			// The _addRemoveLock prevents multiple threads from doing simultaneous adds.
			// An invoke may be occurring during an add or remove operation.        
			auto addRemovePointerLock = ::Microsoft::WRL::Wrappers::SRWLock::LockExclusive(&lockData->addRemoveLock);

			unsigned int index = 0;
			unsigned int size = 1;
			if (*targetsList != nullptr)
			{
				size = (*targetsList)->Length + 1;
			}

			// Allocate event array        
			::Platform::Array<Platform::Delegate^>^ pNewList = ref new ::Platform::Array<Platform::Delegate^>(size);

			// The list may not exist if nobody has registered
			for (; index < size - 1; index++)
			{
				// The T^ contained in the current targetsList node
				// is assigned to a T^ of a new node in pNewList
				// the net result is an addref on the interface.                
				pNewList->set(index, (*targetsList)->get(index));
			}

			// Get unique token value
#pragma warning(push)
			// Conversion from 'type1 ' to 'type_2' is sign-extended
#pragma warning(disable: 4826)
			token.Value = reinterpret_cast<__int64>(reinterpret_cast<void*>(delegateInterface));
#pragma warning(pop)

			// AddTail operation will take a reference which will result in 
			// this function adding one reference count on delegateInterface.
			pNewList->set(index, delegateInterface);

			{ // lock scope for targetsPointerLock
				// The _targetsPointerLock protects the exchanging of the new list (with the addition)
				// for the old list (which could be used currently for firing events)
				auto targetsPointerLock = ::Microsoft::WRL::Wrappers::SRWLock::LockExclusive(&lockData->targetsLock);
				// We move _targets to pTempList so that we can actually delete the list while 
				// not holding any locks. The InvokeAll method may still have a reference to _targets so 
				// even when pTempList releases, this might not delete what was in _targets.
				pTempList = *targetsList;                
				// We're done with pNewList, so just move it to targetsList.
				*targetsList = pNewList;
				// Assigning back local pointer to global data
				*targets = reinterpret_cast<void*>(*targetsList);
			} // end lock scope for targetsPointerLock
		} // end lock scope for addRemoveLock

		// Destroys pTempList here (this is the old _targets)        
		return token;
	}

	CPPCLI_FUNC void __stdcall EventSourceRemove(void** targets, EventLock* lock, ::Windows::Foundation::EventRegistrationToken token)
	{
		// lock add/remove
		//    pNewList = _targets - delegate[token]
		//    lock pointer exhange
		//        pTmpList = _targets
		//        _targets = pNewList
		//    unlock pointer exhange
		// unlock add/remove
		auto lockData = reinterpret_cast<EventLockInternal*>(lock);

		// Used for deleting the current array without holding the addRemoveLock.
		::Platform::Array<Platform::Delegate^>^ pTempList = nullptr;
		{ // lock scope for _addRemoveLock
			// Casting void* _targets to array type
			::Platform::Array<Platform::Delegate^>^* targetsList = reinterpret_cast< ::Platform::Array<Platform::Delegate^>^* >(targets);

			// The _addRemoveLock prevents multiple threads from doing simultaneous adds/removes.
			// An invoke may be occurring during an add or remove operation.
			auto addRemovePointerLock = ::Microsoft::WRL::Wrappers::SRWLock::LockExclusive(&lockData->addRemoveLock);

			if (*targetsList == nullptr)
			{
				return; // List is currently empty - thus token wasn't found, just return
			}

			const unsigned int size = (*targetsList)->Length;
			const unsigned int availableSlots = size - 1;
			// Allocate event array
			::Platform::Array<Platform::Delegate^>^ pNewList = nullptr;
			if (availableSlots > 0)
			{
				pNewList = ref new ::Platform::Array<Platform::Delegate^>(availableSlots);            
			}

			unsigned int dstIndex = 0;
			bool removed = false;
			for(unsigned int index = 0; index < size; index++)
			{
				auto element = (*targetsList)->get(index);
#pragma warning(push)
				// Conversion from 'type1 ' to 'type_2' is sign-extended
#pragma warning(disable: 4826)
				if (!removed && token.Value == reinterpret_cast<__int64>(reinterpret_cast<void*>(element)))
#pragma warning(pop)
				{
					removed = true;
					continue;
				}

				// The T^ contained in p is assigned to a T^ of a new node in pNewList. The net result is 
				// an addref on the interface.
				if (availableSlots == dstIndex)
				{
					// We don't have any availableSlots left in the target array, hence every item was copied
					// from the source array. This means we didn't find the item in the list - just return
					return;
				}

				// Copy every registrant from old list except the item being removed
				// The T^ contained in p is assigned to a T^ of a new node in pNewList.
				// The net result is an addref on the interface.                
				pNewList->set(dstIndex, element);
				dstIndex++;
			}

			{
				// lock scope for targetsPointerLock
				// The _targetsPointerLock protects the exchanging of the new list (with the removal)
				// for the old list (which could be used currently for firing events)
				auto targetsPointerLock = ::Microsoft::WRL::Wrappers::SRWLock::LockExclusive(&lockData->targetsLock);
				// We move _targets to pTempList so that we can actually delete the list while 
				// not holding any locks. The InvokeAll method may still have a reference to _targets so 
				// even when pTempList releases, this might not delete what was in _targets.
				pTempList = *targetsList;                
				// We still have some items left inside pNewList, so move it to _target.
				*targetsList = pNewList;

				// Assigning back local pointer to global data
				*targets = reinterpret_cast<void*>(*targetsList);
			} // end lock scope for targetsPointerLock
		} // end lock scope for addRemoveLock

		// Destroys pTempList here (this is the old _targets)
	}

} } // namespace Platform::Details
