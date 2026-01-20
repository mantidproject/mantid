// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/DllConfig.h"

#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

// from NexusFile_fwd.h
enum class NXaccess : unsigned int;

namespace Mantid {
namespace Nexus {

class MANTID_NEXUS_DLL NexusDescriptor {

public:
  /**
   * Unique constructor
   * @param filename input HDF5 Nexus file name
   */
  NexusDescriptor(std::string const &filename);

  NexusDescriptor(std::string const &filename, NXaccess access);

  NexusDescriptor() = delete;

  NexusDescriptor &operator=(NexusDescriptor const &nd) = default;

  NexusDescriptor(NexusDescriptor const &nd) = default;

  /**
   * Using RAII components, no need to deallocate explicitly
   */
  ~NexusDescriptor() = default;

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

  /// Returns the name & type of the first entry in the file
  const std::pair<std::string, std::string> &firstEntryNameType() const { return m_firstEntryNameType; };

  /// Query if the given attribute exists on the root node
  bool hasRootAttr(const std::string &name) const;

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
  const std::map<std::string, std::set<std::string>> &getAllEntries() const noexcept { return m_allEntries; }

  /**
   * Checks if a full-address entry exists for a particular groupClass in a Nexus
   * dataset
   * @param groupClass e.g. NxLog , Nexus entry attribute
   * @param entryName full address for an entry name /entry/NXlogs
   * @return true: entryName exists for a groupClass, otherwise false
   */
  bool isEntry(const std::string &entryName, const std::string &groupClass) const noexcept;

  /**
   * Checks if a full-address entry exists in a Nexus dataset
   * @param entryName full address for an entry name /entry/NXlogs
   * @return true: entryName exists, otherwise false
   */
  bool isEntry(const std::string &entryName) const noexcept;

  /**
   * @param type A string specifying the required type
   * @return pataddressh A vector of strings giving address using UNIX-style address
   * separators (/), e.g. /raw_data_1, /entry/bank1
   */
  std::set<std::string> allAddressesOfType(const std::string &type) const {return m_allEntries.at(type)}

  /**
   * @param level A string specifying the parent address
   * @return address A map of strings giving names within parent (mapped to class type)
   * e.g. group1 : NXentry, group2 : NXentry, data : NXdata
   */
  std::map<std::string, std::string> allAddressesAtLevel(const std::string &level) const;

  /// Query if a given type exists somewhere in the file
  bool classTypeExists(const std::string &classType) const;

  /**
   * @param name A address within a Nexus file to an entry
   * @return The class type associated with that address
   */
  std::string classTypeForName(std::string const &name) const;

  /**
   * Add an entry to the mapping for the file. Since there is no handle to the file after creation, it is up to the
   * caller to only add entries that exist. This should not be used for files that are read-only.
   *
   * @param entryName full address to the node in the file
   * @param groupClass NXclass (for group) or SDS (for dataset)
   */
  void addEntry(const std::string &entryName, const std::string &groupClass);

  void addRootAttr(const std::string &name);

private:
  /**
   * Sets m_allEntries, called in HDF5 constructor.
   * m_filename must be set
   */
  std::map<std::string, std::set<std::string>> initAllEntries();

  /** Nexus HDF5 file name */
  std::string m_filename;
  /// Extension
  std::string m_extension;
  /// First entry name/type
  std::pair<std::string, std::string> m_firstEntryNameType;
  /// Root attributes
  std::unordered_set<std::string> m_rootAttrs;

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

} // namespace Nexus
} // namespace Mantid
