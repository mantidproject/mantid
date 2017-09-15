#include "MantidAPI/IEventWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid {

namespace API {

/**
 */
const std::string IEventWorkspace::toString() const {
  std::ostringstream os;
  os << MatrixWorkspace::toString() << "\n";

  os << "Events: " + std::to_string(getNumberEvents());
  switch (getEventType()) {
  case WEIGHTED:
    os << " (weighted)\n";
    break;
  case WEIGHTED_NOTIME:
    os << " (weighted, no times)\n";
    break;
  case TOF:
    os << "\n";
    break;
  }
  return os.str();
}
}

///\cond TEMPLATE
/*
 * In order to be able to cast PropertyWithValue classes correctly a definition
 *for the PropertyWithValue<IEventWorkspace> is required
 *
 */
namespace Kernel {

template <>
MANTID_API_DLL Mantid::API::IEventWorkspace_sptr
IPropertyManager::getValue<Mantid::API::IEventWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::IEventWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IEventWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IEventWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL Mantid::API::IEventWorkspace_const_sptr
IPropertyManager::getValue<Mantid::API::IEventWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::IEventWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IEventWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<IEventWorkspace>.";
    throw std::runtime_error(message);
  }
}
}

// namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
