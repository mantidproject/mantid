// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"

#include <map>
#include <set>
#include <string>

namespace Mantid {
namespace Kernel {

class MANTID_KERNEL_DLL NexusHDF5Descriptor {

public:
  /**
   * Unique constructor
   * @param filename input HDF5 Nexus file name
   */
  NexusHDF5Descriptor(std::string filename);

  NexusHDF5Descriptor() = delete;

  /**
   * Using RAII components, no need to deallocate explicitly
   */
  ~NexusHDF5Descriptor() = default;

  /// Returns true if the file is considered to store data in a hierarchy
  static bool isReadable(const std::string &filename);

  /**
   * Returns a copy of the current file name
   * @return
   */
  const std::string &filename() const noexcept;

  /**
   * Access the file extension. Defined as the string after and including the
   * last period character
   * @returns A reference to a const string containing the file extension
   */
  inline const std::string &extension() const { return m_extension; }

  /**
   * Returns a const reference of the internal map holding all entries in the
   * NeXus HDF5 file
   * @return map holding all entries by group class
   * <pre>
   *   key: group_class (e.g. NXentry, NXlog)
   *   value: set with absolute entry names for the group_class key
   *          (e.g. /entry/log)
   * </pre>
   */
  const std::map<std::string, std::set<std::string>> &getAllEntries() const noexcept;

  /**
   * Checks if a full-path entry exists for a particular groupClass in a Nexus
   * dataset
   * @param groupClass e.g. NxLog , Nexus entry attribute
   * @param entryName full path for an entry name /entry/NXlogs
   * @return true: entryName exists for a groupClass, otherwise false
   */
  bool isEntry(const std::string &entryName, const std::string &groupClass) const noexcept;

  /**
   * Checks if a full-path entry exists in a Nexus dataset
   * @param entryName full path for an entry name /entry/NXlogs
   * @return true: entryName exists, otherwise false
   */
  bool isEntry(const std::string &entryName) const noexcept;

private:
  /**
   * Sets m_allEntries, called in HDF5 constructor.
   * m_filename must be set
   */
  std::map<std::string, std::set<std::string>> initAllEntries();

  /** NeXus HDF5 file name */
  std::string m_filename;
  /// Extension
  std::string m_extension;

  /**
   * All entries metadata
   * <pre>
   *   key: group_class (e.g. NXentry, NXlog)
   *   value: set with absolute entry names for the group_class key
   *          (e.g. /entry/log)
   * </pre>
   */
  const std::map<std::string, std::set<std::string>> m_allEntries;
};

} // namespace Kernel
} // namespace Mantid
