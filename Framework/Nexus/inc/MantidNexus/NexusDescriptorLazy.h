// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/UniqueID.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Mantid {
namespace Nexus {

class MANTID_NEXUS_DLL NexusDescriptorLazy {

public:
  /**
   * Unique constructor
   * @param filename input HDF5 Nexus file name
   */
  NexusDescriptorLazy(std::string const &filename);

  NexusDescriptorLazy() = delete;

  NexusDescriptorLazy &operator=(NexusDescriptorLazy const &nd) = default;

  NexusDescriptorLazy(NexusDescriptorLazy const &nd) = default;

  /**
   * Using RAII components, no need to deallocate explicitly
   */
  ~NexusDescriptorLazy() = default;

  /**
   * Returns a constant reference to the current file name
   * @return
   */
  inline std::string const &filename() const noexcept { return m_filename; }

  /**
   * Access the file extension. Defined as the string after and including the
   * last period character
   * @returns A reference to a const string containing the file extension
   */
  inline std::string const &extension() const noexcept { return m_extension; }

  /// Query if the given attribute exists on the root node
  bool hasRootAttr(std::string const &name);

  /**
   * Returns a const reference of the internal map holding all entries in the
   * Nexus HDF5 file
   * @return map holding all entries by group class
   * <pre>
   *   key: group_class (e.g. NXentry, NXlog)
   *   value: set with absolute entry names for the group_class key
   *          (e.g. /entry/log)
   * </pre>
   */
  std::unordered_map<std::string, std::string> const &getAllEntries() const noexcept { return m_allEntries; }

  /**
   * Checks if a full-address entry exists for a particular groupClass in a Nexus
   * dataset
   * @param entryName full address for an entry name /entry/NXlogs
   * @param groupClass e.g. NxLog , Nexus entry attribute
   * @return true: entryName exists for a groupClass, otherwise false
   */
  bool isEntry(std::string const &entryName, std::string const &groupClass) {
    if (isEntry(entryName)) {
      return m_allEntries.at(entryName) == groupClass;
    } else {
      return false;
    }
  }

  /**
   * Checks if a full-address entry exists in a Nexus dataset
   * @param entryName full address for an entry name /entry/NXlogs
   * @return true: entryName exists, otherwise false
   */
  bool isEntry(std::string const &entryName);

  /// Query if a given type exists somewhere in the file
  bool classTypeExists(std::string const &classType) const;

private:
  /**
   * Sets m_allEntries, called in HDF5 constructor.
   * m_filename must be set
   */
  std::unordered_map<std::string, std::string> initAllEntries();
  void loadGroups(std::unordered_map<std::string, std::string> &allEntries, std::string const &address,
                  unsigned int depth, const unsigned int maxDepth);

  /** Nexus HDF5 file name */
  std::string const m_filename;
  /// Extension
  std::string const m_extension;
  /// HDF5 File Handle
  UniqueID<&H5Fclose> m_fileID;
  /// Root attributes
  std::unordered_set<std::string> m_rootAttrs;

  /**
   * All entries metadata
   * <pre>
   *   key: group address
   *   value: group class (e.g. NXentry, NXlog)
   * </pre>
   */
  std::unordered_map<std::string, std::string> m_allEntries;
};

} // namespace Nexus
} // namespace Mantid
