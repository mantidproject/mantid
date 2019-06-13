// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/Python/Sip.h"
#include <sip.h>

namespace MantidQt {
namespace Widgets {
namespace Common {
namespace Python {

namespace Detail {
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
} // namespace Detail

} // namespace Python
} // namespace Common
} // namespace Widgets
} // namespace MantidQt