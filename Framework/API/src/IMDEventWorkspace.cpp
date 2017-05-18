#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
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
  for (auto &stat : stats) {
    os << stat << "\n";
  }

  os << MultipleExperimentInfos::toString() << "\n";

  os << "Events: " << getNPoints() << "\n";
  return os.str();
}

//-----------------------------------------------------------------------------------------------

} // namespace API

} // namespace Mantid

namespace Mantid {
namespace Kernel {

using Mantid::API::IMDEventWorkspace;
using Mantid::API::IMDEventWorkspace_sptr;
using Mantid::API::IMDEventWorkspace_const_sptr;
using Mantid::Indexing::SpectrumIndexSet;
using Mantid::API::WorkspacePropertyWithIndex;

/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDEventWorkspace> is required */
template <>
MANTID_API_DLL IMDEventWorkspace_sptr
IPropertyManager::getValue<IMDEventWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<IMDEventWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<IMDEventWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IMDEventWorkspace>.";
    throw std::runtime_error(message);
  }
}

/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDEventWorkspace> is required */
template <>
MANTID_API_DLL IMDEventWorkspace_const_sptr
IPropertyManager::getValue<IMDEventWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<IMDEventWorkspace_const_sptr> *prop =
      dynamic_cast<PropertyWithValue<IMDEventWorkspace_const_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    // Every other class with this behaviour allows you to get a shared_ptr<T>
    // property as a shared_ptr<const T>. This class should be consistent, so
    // try that:
    PropertyWithValue<IMDEventWorkspace_sptr> *nonConstProp =
        dynamic_cast<PropertyWithValue<IMDEventWorkspace_sptr> *>(
            getPointerToProperty(name));
    if (nonConstProp) {
      return nonConstProp->operator()();
    } else {
      std::string message =
          "Attempt to assign property " + name +
          " to incorrect type. Expected const shared_ptr<IMDEventWorkspace>.";
      throw std::runtime_error(message);
    }
  }
}

template <>
DLLExport std::tuple<IMDEventWorkspace_const_sptr, SpectrumIndexSet>
IPropertyManager::getValue<
    std::tuple<IMDEventWorkspace_const_sptr, SpectrumIndexSet>>(
    const std::string &name) const {
  WorkspacePropertyWithIndex<IMDEventWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IMDEventWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    return std::tuple<IMDEventWorkspace_const_sptr, SpectrumIndexSet>(*prop);
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IEventWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport std::tuple<IMDEventWorkspace_sptr, SpectrumIndexSet>
IPropertyManager::getValue<
    std::tuple<IMDEventWorkspace_sptr, SpectrumIndexSet>>(
    const std::string &name) const {
  WorkspacePropertyWithIndex<IMDEventWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IMDEventWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    return std::tuple<IMDEventWorkspace_sptr, SpectrumIndexSet>(*prop);
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IEventWorkspace>.";
    throw std::runtime_error(message);
  }
}

// Enable setTypedProperty for IMDEventWorkspace
template <>
DLLExport IPropertyManager *
IPropertyManager::setTypedProperty<IMDEventWorkspace_sptr, API::IndexType,
                                   std::vector<int>>(
    const std::string &name,
    const std::tuple<IMDEventWorkspace_sptr, API::IndexType, std::vector<int>>
        &value) {
  WorkspacePropertyWithIndex<IMDEventWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IMDEventWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    *prop = value;
  } else {
    throw std::invalid_argument("Attempt to assign to property (" + name +
                                ") of incorrect type");
  }
  return this;
}

template <>
DLLExport IPropertyManager *
IPropertyManager::setTypedProperty<IMDEventWorkspace_sptr, API::IndexType,
                                   std::string>(
    const std::string &name,
    const std::tuple<IMDEventWorkspace_sptr, API::IndexType, std::string>
        &value) {
  WorkspacePropertyWithIndex<IMDEventWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IMDEventWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    *prop = value;
  } else {
    throw std::invalid_argument("Attempt to assign to property (" + name +
                                ") of incorrect type");
  }
  return this;
}

} // namespace Kernel
} // namespace Mantid
