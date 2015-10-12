#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

using Mantid::coord_t;
using namespace Mantid::Geometry;

namespace Mantid {
namespace API {

//-----------------------------------------------------------------------------------------------
/** Empty constructor */
IMDEventWorkspace::IMDEventWorkspace()
    : IMDWorkspace(), MultipleExperimentInfos(), m_fileNeedsUpdating(false) {}

//-----------------------------------------------------------------------------------------------
/** Copy constructor */
IMDEventWorkspace::IMDEventWorkspace(const IMDEventWorkspace &other)
    : IMDWorkspace(other), MultipleExperimentInfos(other),
      m_fileNeedsUpdating(other.m_fileNeedsUpdating) {}

//-----------------------------------------------------------------------------------------------
/** @return the marker set to true when a file-backed workspace needs its
 * back-end file updated (by calling SaveMD(UpdateFileBackEnd=1) )
 */
bool IMDEventWorkspace::fileNeedsUpdating() const {
  return m_fileNeedsUpdating;
}

//-----------------------------------------------------------------------------------------------
/** Sets the marker set to true when a file-backed workspace needs its back-end
 * file updated (by calling SaveMD(UpdateFileBackEnd=1) )
 * @param value :: marker value
 */
void IMDEventWorkspace::setFileNeedsUpdating(bool value) {
  m_fileNeedsUpdating = value;
}

//-----------------------------------------------------------------------------------------------
/** Is the workspace thread-safe. For MDEventWorkspaces, this means operations
 * on separate boxes in separate threads. Don't try to write to the
 * same box on different threads.
 *
 * @return false if the workspace is file-backed.
 */
bool IMDEventWorkspace::threadSafe() const { return !this->isFileBacked(); }

//-----------------------------------------------------------------------------------------------

/**
 */
const std::string IMDEventWorkspace::toString() const {
  std::ostringstream os;
  os << IMDWorkspace::toString();

  // Now box controller details
  std::vector<std::string> stats = getBoxControllerStats();
  for (size_t i = 0; i < stats.size(); i++) {
    os << stats[i] << "\n";
  }

  os << MultipleExperimentInfos::toString() << "\n";

  os << "Events: " << getNPoints() << "\n";
  return os.str();
}

//-----------------------------------------------------------------------------------------------

} // namespace MDEvents

} // namespace Mantid

namespace Mantid {
namespace Kernel {
/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDEventWorkspace> is required */
template <>
MANTID_API_DLL Mantid::API::IMDEventWorkspace_sptr
IPropertyManager::getValue<Mantid::API::IMDEventWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::IMDEventWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IMDEventWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected IMDEventWorkspace.";
    throw std::runtime_error(message);
  }
}

/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDEventWorkspace> is required */
template <>
MANTID_API_DLL Mantid::API::IMDEventWorkspace_const_sptr
IPropertyManager::getValue<Mantid::API::IMDEventWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::IMDEventWorkspace_const_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::API::IMDEventWorkspace_const_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const IMDEventWorkspace.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
