#ifndef MPLCPP_SIPUTILS_H
#define MPLCPP_SIPUTILS_H
/*
  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "MantidQtWidgets/MplCpp/Python/Object.h"
#include <sip.h>
#include <stdexcept>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
namespace Python {

namespace detail {
/**
 * @return A pointer to the C++ sip api object
 */
const sipAPIDef *sipAPI() {
  static const sipAPIDef *sip_API = nullptr;
  if (sip_API)
    return sip_API;
#if defined(SIP_USE_PYCAPSULE)
  sip_API = (const sipAPIDef *)PyCapsule_Import("sip._C_API", 0);
#else
  /* Import the SIP module. */
  PyObject *sip_module = PyImport_ImportModule("sip");
  if (sip_module == NULL)
    throw std::runtime_error("sip_api() - Error importing sip module");

  /* Get the module's dictionary. */
  PyObject *sip_module_dict = PyModule_GetDict(sip_module);

  /* Get the "_C_API" attribute. */
  PyObject *c_api = PyDict_GetItemString(sip_module_dict, "_C_API");
  if (c_api == NULL)
    throw std::runtime_error(
        "sip_api() - Unable to find _C_API attribute in sip dictionary");

  /* Sanity check that it is the right type. */
  if (!PyCObject_Check(c_api))
    throw std::runtime_error("sip_api() - _C_API type is not a CObject");

  /* Get the actual pointer from the object. */
  sip_API = (const sipAPIDef *)PyCObject_AsVoidPtr(c_api);
#endif
  return sip_API;
}
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
