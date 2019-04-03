// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_SIPUTILS_H
#define MPLCPP_SIPUTILS_H

#include "MantidQtWidgets/MplCpp/Python/Object.h"
#include <sip.h>
#include <stdexcept>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
namespace Python {

namespace detail {
const sipAPIDef *sipAPI();
} // namespace detail

/**
 * Extract a C++ object of type T from the Python object
 * @param obj A sip-wrapped Python object
 */
template <typename T> T *extract(const Object &obj) {
  const auto sipapi = detail::sipAPI();
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

} // namespace Python
} // namespace MplCpp
} // namespace Widgets

} // namespace MantidQt

#endif // MPLCPP_SIPUTILS_H
