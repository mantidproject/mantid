#include "MantidQtWidgets/MplCpp/SipUtils.h"

#include "sip.h"

#include <stdexcept>

namespace {
/**
 * @return A pointer to the C++ sip api object
 */
const sipAPIDef *sip_api() {
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
}

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Unwrap a C++ object from a PyObject
 * @param obj
 * @return An opaque pointer to the memory of the C++ object
 * The result will need a static_cast applied to it to be useful.
 */
void *SipUtils::unwrap(PyObject *obj_ptr) {
  if (!PyObject_TypeCheck(obj_ptr, sip_api()->api_wrapper_type)) {
    throw std::runtime_error("sipUnwrap() - Object is not a wrapped type.");
  }
  // transfer ownership from python to C++
  sip_api()->api_transfer_to(obj_ptr, 0);
  // reinterpret to sipWrapper
  auto wrapper = reinterpret_cast<sipSimpleWrapper *>(obj_ptr);
#if (SIP_API_MAJOR_NR == 8 && SIP_API_MINOR_NR >= 1) || SIP_API_MAJOR_NR > 8
  return sip_api()->api_get_address(wrapper);
#elif SIP_API_MAJOR_NR == 8
  return wrapper->data;
#else
  return wrapper->u.cppPtr;
#endif
}
}
}
}
