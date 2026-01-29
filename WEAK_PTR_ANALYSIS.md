# Analysis: Converting Non-Owning shared_ptr to weak_ptr

## Problem Statement
"Since these references don't own the object, change them from a std::shared_ptr to a std::weak_ptr"

## Technical Impossibility

**The requirement as stated is technically impossible** because:

1. `std::weak_ptr<T>` can only be constructed from `std::shared_ptr<T>`
2. The current code uses raw pointers (`const IComponent*`) stored in member variables like `m_sourceCache` and `m_sampleCache`
3. These raw pointers cannot be directly converted to `weak_ptr`

```cpp
// Current code (after removing NoDeleting):
const IComponent *m_sourceCache = nullptr;  // Raw pointer

IComponent_const_sptr Instrument::getSource() const {
    return IComponent_const_sptr(m_sourceCache, [](auto *) {});  // shared_ptr with no-op deleter
}
```

## Why weak_ptr Won't Work

```cpp
// This is INVALID C++ - won't compile:
const IComponent *m_sourceCache = nullptr;
std::weak_ptr<const IComponent> wp = m_sourceCache;  // ERROR: Cannot convert raw pointer to weak_ptr

// weak_ptr requires an existing shared_ptr:
std::shared_ptr<const IComponent> sp = ...;  // Must exist somewhere
std::weak_ptr<const IComponent> wp = sp;      // OK - observes the shared_ptr
```

## Current Architecture

The Mantid codebase uses:
- **Raw pointers** for internal component storage (owned by component hierarchy)
- **shared_ptr** for API returns (for historical reasons and API stability)
- **No-op deleters** to wrap raw pointers in shared_ptr without taking ownership

## Possible Solutions

### Option 1: Keep Current Approach (RECOMMENDED)
The pattern of `shared_ptr` with a no-op deleter is a well-known C++ idiom for:
- Maintaining API compatibility
- Wrapping non-owned pointers in shared_ptr interfaces
- Avoiding raw pointer returns when API expects smart pointers

**Pros:**
- No API changes
- Standard C++ pattern
- Already implemented

**Cons:**
- Semantically misleading (shared_ptr implies ownership)
- Slight overhead

### Option 2: Change API to Return Raw Pointers
```cpp
const IComponent* getSource() const {
    return m_sourceCache;
}
```

**Pros:**
- Semantically correct (no ownership claimed)
- No overhead
- Clear intent

**Cons:**
- **BREAKS API** - Major breaking change
- All calling code must be updated
- May affect Python bindings

### Option 3: Restructure Ownership Model
Make components actually owned by shared_ptrs throughout:
```cpp
std::shared_ptr<IComponent> m_sourceCache;  // Actually owns

std::weak_ptr<const IComponent> getSource() const {
    return m_sourceCache;  // Return weak_ptr
}
```

**Pros:**
- Proper modern C++ ownership semantics
- Enables weak_ptr usage

**Cons:**
- **MASSIVE REFACTORING** required
- Changes fundamental architecture
- High risk of bugs
- Performance implications

### Option 4: Use std::observer_ptr (C++20+)
```cpp
std::experimental::observer_ptr<const IComponent> getSource() const {
    return std::experimental::observer_ptr<const IComponent>(m_sourceCache);
}
```

**Pros:**
- Semantically correct for non-owning references
- Part of C++ standard library extensions

**Cons:**
- Not widely available yet
- API breaking change
- Experimental status

## Recommendation

**Keep the current approach** (Option 1) because:
1. The problem statement is technically impossible as written
2. The current pattern is a well-known C++ idiom
3. Changing the API would be a massive breaking change
4. Restructuring ownership would be extremely risky

If the goal is to make ownership semantics clearer, consider:
- Adding documentation comments
- Using type aliases that make intent clear
- Adding static analysis suppressions if needed

## Affected Locations

All 25 locations using `[](auto *) {}` lambda deleter:
- Framework/Geometry/src/Instrument.cpp (12 uses)
- Framework/Geometry/src/Instrument/Component.cpp (1 use)
- Framework/Geometry/src/Instrument/CompAssembly.cpp (2 uses)
- Framework/Geometry/src/Instrument/ObjCompAssembly.cpp (1 use)
- Framework/Geometry/src/Instrument/ParameterMap.cpp (1 use)
- Framework/Geometry/src/Instrument/InstrumentVisitor.cpp (3 uses)
- Framework/Geometry/src/Instrument/ParComponentFactory.cpp (1 use)
- Framework/API/src/SpectrumInfo.cpp (1 use)
- Framework/Algorithms/src/CreateDetectorTable.cpp (2 uses)
- Framework/DataHandling/src/SaveAscii2.cpp (1 use)
