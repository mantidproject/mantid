#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/System.h"

#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------------------------

const std::string IMDHistoWorkspace::toString() const {
  std::ostringstream os;
  os << IMDWorkspace::toString();

  os << MultipleExperimentInfos::toString() << "\n";

  return os.str();
}
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDEventWorkspace> is required */
template <>
MANTID_API_DLL Mantid::API::IMDHistoWorkspace_sptr
IPropertyManager::getValue<Mantid::API::IMDHistoWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::IMDHistoWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IMDHistoWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IMDHistoWorkspace>.";
    throw std::runtime_error(message);
  }
}

/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDWorkspace_const_sptr> is required */
template <>
MANTID_API_DLL Mantid::API::IMDHistoWorkspace_const_sptr
IPropertyManager::getValue<Mantid::API::IMDHistoWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::IMDHistoWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IMDHistoWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<IMDHistoWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
