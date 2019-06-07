// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/Memory.h"

namespace Mantid {
namespace API {

Workspace::Workspace(const Parallel::StorageMode storageMode)
    : m_history(std::make_unique<WorkspaceHistory>()),
      m_storageMode(storageMode) {}

// Defined as default in source for forward declaration with std::unique_ptr.
Workspace::~Workspace() = default;

Workspace::Workspace(const Workspace &other)
    : Kernel::DataItem(other), m_title(other.m_title),
      m_comment(other.m_comment), m_name(),
      m_history(std::make_unique<WorkspaceHistory>(other.getHistory())),
      m_storageMode(other.m_storageMode) {}

/** Set the title of the workspace
 *
 *  @param t :: The title
 */
void Workspace::setTitle(const std::string &t) { m_title = t; }

/** Set the comment field of the workspace
 *
 *  @param c :: The comment
 */
void Workspace::setComment(const std::string &c) { m_comment = c; }

/** Set the name field of the workspace
 *
 *  @param name :: The name
 */
void Workspace::setName(const std::string &name) { m_name = name; }

/** Get the workspace title
 *
 *  @return The title
 */
const std::string Workspace::getTitle() const { return m_title; }

/** Get the workspace comment
 *
 *  @return The comment
 */
const std::string &Workspace::getComment() const { return m_comment; }

/** Get the workspace name
 *
 *  @return The name
 */
const std::string &Workspace::getName() const { return m_name; }

/**
 * Check whether other algorithms have been applied to the
 * workspace by checking the history length.
 *
 * By default a workspace is called dirty if its history is
 * longer than one. This default can be changed to allow for
 * workspace creation processes that necessitate more than
 * a single algorithm.
 *
 * @param n: number of algorithms defining a clean workspace
 */
bool Workspace::isDirty(const int n) const {
  return static_cast<int>(m_history->size()) > n;
}

/**
 * Returns the memory footprint in sensible units
 * @return A string with the
 */
std::string Workspace::getMemorySizeAsStr() const {
  return Mantid::Kernel::memToString<uint64_t>(
      static_cast<uint64_t>(getMemorySize()) / 1024);
}

/// Returns the storage mode (used for MPI runs)
Parallel::StorageMode Workspace::storageMode() const { return m_storageMode; }

/// Sets the storage mode (used for MPI runs)
void Workspace::setStorageMode(Parallel::StorageMode storageMode) {
  m_storageMode = storageMode;
}

} // namespace API
} // Namespace Mantid

///\cond TEMPLATE
namespace Mantid {
namespace Kernel {

template <>
MANTID_API_DLL Mantid::API::Workspace_sptr
IPropertyManager::getValue<Mantid::API::Workspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::Workspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::Workspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected shared_ptr<Workspace>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL Mantid::API::Workspace_const_sptr
IPropertyManager::getValue<Mantid::API::Workspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::Workspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::Workspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<Workspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
///\endcond TEMPLATE
