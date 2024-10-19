#pragma once
#include <ntddk.h>

typedef long LONG;

struct AtomicInt {
    volatile LONG value;

    LONG load() const {
        return InterlockedCompareExchange((LONG*)&value, 0, 0);
    }

    void store(LONG newVal) {
        InterlockedExchange((LONG*)&value, newVal);
    }

    LONG increment() {
        return InterlockedIncrement((LONG*)&value);
    }

    LONG decrement() {
        return InterlockedDecrement((LONG*)&value);
    }
};

struct AtomicBool {
    volatile LONG value;

    bool load() const {
        return InterlockedCompareExchange((LONG*)&value, 0, 0) != 0;
    }

    void store(bool newVal) {
        InterlockedExchange((LONG*)&value, newVal ? 1 : 0);
    }

    bool toggle() {
        return InterlockedXor((LONG*)&value, 1) != 0;
    }
};