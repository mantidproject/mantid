// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include <sip.h>
#include <stdexcept>

namespace MantidQt {
namespace Widgets {
namespace Common {
namespace Python {

namespace Detail {
EXPORT_OPT_MANTIDQT_COMMON const sipAPIDef *sipAPI();
} // namespace Detail

/**
 * Extract a C++ object of type T from the Python object
 * @param obj A sip-wrapped Python object
 */
template <typename T> T *extract(const Object &obj) {
  const auto sipapi = Detail::sipAPI();
  if (!PyObject_TypeCheck(obj.ptr(), sipapi->api_wrapper_type)) {
    throw std::runtime_error("extract() - Object is not a sip-wrapped type.");
  }
  // transfer ownership from python to C++
  sipapi->api_transfer_to(obj.ptr(), 0);
  // reinterpret to sipWrapper
  auto wrapper = reinterpret_cast<sipSimpleWrapper *>(obj.ptr());
#if (SIP_API_MAJOR_NR == 8 && SIP_API_MINOR_NR >= 1) || SIP_API_MAJOR_NR > 8
  return static_cast<T *>(sipapi->api_get_address(wrapper));
#elif SIP_API_MAJOR_NR == 8
  return static_cast<T *>(wrapper->data);
#else
  return static_cast<T *>(wrapper->u.cppPtr);
#endif
}

/**
 * Convert a C++ object of type T to a Python object
 * @param obj A C++ object
 * @param nameOfType The type of the C++ object as a string. Allows SIP to find the right export.
 */
template <typename T> Object wrap(const T &obj, char const *nameOfType) {
  const auto sipapi = Detail::sipAPI();
  const auto type = sipapi->api_find_type(nameOfType);
  if (!type) {
    return Python::Object();
  }
  auto pyObj = sipapi->api_convert_from_type(obj, type, nullptr);
  return NewRef(pyObj);
}

} // namespace Python
} // namespace Common
} // namespace Widgets

} // namespace MantidQt
