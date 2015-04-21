//------------------------------------------
// Includes
//------------------------------------------
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/EmptyValues.h"
#include <cmath>

namespace Mantid {
namespace Kernel {
namespace Detail {
/**
 * Specialization of checkIsEmpty for string
 * @param value :: A string object
 * @return True if the string is considered empty
 */
template <> DLLExport bool checkIsEmpty(const std::string &value) {
  return value.empty();
}
/**
 * Specialization of checkIsEmpty for double values
 * @param value :: A double
 * @return True if the value is considered empty, see EmptyValues.h
 */
template <> DLLExport bool checkIsEmpty(const double &value) {
  if (std::fabs(value - Mantid::EMPTY_DBL()) < 1e-08)
    return true;
  else
    return false;
}
/**
 * Specialization of checkIsEmpty for int
 * @param value :: A int value
 * @return True if the value is considered empty, see EmptyValues.h
 */
template <> DLLExport bool checkIsEmpty(const int &value) {
  return (value == Mantid::EMPTY_INT());
}
/**
 * Specialization of checkIsEmpty for long
 * @param value :: A long value
 * @return True if the value is considered empty, see EmptyValues.h
 */
template <> DLLExport bool checkIsEmpty(const long &value) {
  return (value == Mantid::EMPTY_LONG());
}
}
}
}
