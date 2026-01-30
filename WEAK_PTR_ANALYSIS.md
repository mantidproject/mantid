# Analysis: Converting Non-Owning shared_ptr to Raw Pointers

## Problem Statement
"Don't use shared_ptr for non-owning references - use raw pointers or references instead."

## Progress: 5/25 Locations Converted ✅

### Successfully Converted
Added raw pointer overloads for `ExperimentInfo` methods:
- `getEFixed(const IDetector*)`
- `getEFixedGivenEMode(const IDetector*, DeltaEMode::Type)`
- `getEFixedForIndirect(const IDetector*, vector<string>)`

Updated 5 call sites to use raw pointers instead of `shared_ptr` with no-op deleters.

## Remaining 20 Locations - Analysis

### Category 1: Return Types (11 locations)
**Challenge:** These methods return `shared_ptr` as part of public API.

**Files:**
- `Instrument::getSource()`, `getSample()` - 4 locations
- `Instrument::getComponentByID()` - 2 locations
- `Component::getParent()` - 1 location
- `CompAssembly::operator[]` - 1 location
- `ObjCompAssembly::operator[]` - 1 location
- `Instrument::getAllComponentsWithName()` - 1 location
- `CompAssembly::getComponentByName()` - 1 location

**Why difficult:**
- API is used throughout codebase
- Many callers expect `shared_ptr`
- Would require updating 100+ call sites

**Recommendation:** Keep as-is or change gradually with deprecation warnings.

### Category 2: Data Structure Storage (2 locations)
**Challenge:** `shared_ptr` stored in data structures.

**Files:**
- `Instrument::markAsDetector()` - 1 location
- `Instrument::markAsDetectorIncomplete()` - 1 location

**Why difficult:**
- `m_detectorCache` is `vector<tuple<detid_t, IDetector_const_sptr, bool>>`
- Accessed throughout Instrument class
- Would require changing data structure definition

**Recommendation:** Requires refactoring data structure design.

### Category 3: Mixed with API Calls (4 locations)
**Challenge:** Local variables used with APIs requiring `shared_ptr`.

**Files:**
- `Instrument::getAllComponentsWithName()` - uses `dynamic_pointer_cast` - 1 location
- `CompAssembly::getComponentByName()` - uses `dynamic_pointer_cast` - 1 location
- `Instrument::getDetectors()` - builds list of shared_ptr - 2 locations

**Why difficult:**
- `dynamic_pointer_cast` requires `shared_ptr`
- Building collections that hold `shared_ptr`
- Intertwined with shared_ptr-based APIs

**Recommendation:** Would require API redesign.

### Category 4: Factory/Constructor Parameters (4 locations)
**Challenge:** Passed to factory methods/constructors expecting `shared_ptr`.

**Files:**
- `ParComponentFactory::create()` - 1 location
- `ParComponentFactory::createInstrument()` - 1 location
- `InstrumentVisitor` - 3 locations

**Why difficult:**
- Factory APIs designed around `shared_ptr`
- Changing would break factory pattern

**Recommendation:** Requires factory API redesign.

### Category 5: Parent Chain Traversal (1 location)
**Challenge:** Variable reassigned during traversal.

**Files:**
- `ParameterMap::getRecursiveByType()` - 1 location

**Why difficult:**
```cpp
std::shared_ptr<const IComponent> compInFocus(comp, [](auto *) {});  // Start with raw
while (compInFocus != nullptr) {
    // ...
    compInFocus = compInFocus->getParent();  // Reassigned to shared_ptr from API
}
```
- Starts with raw pointer but gets reassigned to `shared_ptr` from `getParent()`
- Cannot use raw pointer unless `getParent()` API changes

**Recommendation:** Depends on Category 1 changes.

## Summary

### Feasible Changes: 5/25 ✅ DONE
Local variables passed to methods that now have raw pointer overloads.

### Difficult Changes: 20/25 ⚠️
Require extensive API changes affecting:
- 11 public method return types
- 2 data structure definitions
- 7 factory/collection building patterns

## Recommendations

### Option A: Stop Here (RECOMMENDED)
- **Completed:** 5/25 conversions (20%)
- **Benefit:** Most impactful changes done (local variables in hot paths)
- **Risk:** Low - backward compatible
- **Effort:** Already complete

### Option B: Continue with API Changes
- **Additional:** 11 return type changes
- **Benefit:** More consistent ownership semantics
- **Risk:** High - breaks API, requires updating 100+ call sites
- **Effort:** 2-3 weeks of careful refactoring

### Option C: Comprehensive Redesign
- **Complete:** All 25/25 conversions
- **Benefit:** Fully consistent ownership model
- **Risk:** Very high - major architecture change
- **Effort:** 1-2 months, high risk of bugs

## Conclusion

**Recommend Option A:** The 5 conversions completed provide the main benefit (clearer ownership for local variables) without the risk of breaking changes. The remaining 20 locations are structurally embedded in the API and changing them provides diminishing returns relative to the effort and risk involved.
