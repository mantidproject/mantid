//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/EmptyValues.h"
#include <cfloat>
#include <climits>

namespace Mantid {

/**
 * Returns what we consider an "empty" integer within a property
 * @returns An flag value
 */
int EMPTY_INT() { return INT_MAX; }

/**
 * Returns what we consider an "empty" long within a property
 * @returns An flag value
 */
long EMPTY_LONG() { return LONG_MAX; }

/**
 * Returns what we consider an "empty" int64_t within a property
 * @returns An flag value
 */
int64_t EMPTY_INT64() { return INT64_MAX; }

/**
 * Returns what we consider an "empty" double within a property
 * @returns An flag value
 */
double EMPTY_DBL() { return DBL_MAX / 2; }

} // namespace Mantid
