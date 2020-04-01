// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include <map>
#include <set>
#include <string>

namespace Mantid {
namespace NeXus {

class DLLExport NexusHDF5Descriptor {

public:
  /**
   * Unique constructor
   * @param filename input HDF5 Nexus file name
   */
  NexusHDF5Descriptor(const std::string &filename);

  NexusHDF5Descriptor() = delete;

  /**
   * Using RAII components, no need to deallocate explicitly
   */
  ~NexusHDF5Descriptor() = default;

  /**
   * Returns a copy of the current file name
   * @return
   */
  std::string getFilename() const noexcept;

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
  const std::map<std::string, std::set<std::string>> &getAllEntries() const
      noexcept;

private:
  /**
   * Sets m_allEntries, called in HDF5 constructor.
   * m_filename must be set
   */
  std::map<std::string, std::set<std::string>> initAllEntries();

  /** NeXus HDF5 file name */
  std::string m_filename;

  /**
   * All entries metadata
   * <pre>
   *   key: group_class (e.g. NXentry, NXlog)
   *   value: set with absolute entry names for the group_class key
   *          (e.g. /entry/log)
   * </pre>
   */
  std::map<std::string, std::set<std::string>> m_allEntries;
};

} // namespace NeXus
} // namespace Mantid
