// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/UniqueID.h"

#include <map>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Mantid {
namespace Nexus {

using SharedFileID = SharedID<&H5Fclose>;
using EntryMap = std::unordered_map<std::string, std::string>;

class MANTID_NEXUS_DLL NexusDescriptorLazy {

public:
  /**
   * Construct by opening the file internally.
   * @param filename input HDF5 Nexus file name
   */
  NexusDescriptorLazy(std::string const &filename);

  /**
   * Construct from an already-open HDF5 file handle. Increments the HDF5 reference
   * count so the handle can be closed independently by both owner and descriptor.
   * @param fileID open HDF5 file hid_t
   * @param filename file path (used for error messages and extension)
   */
  NexusDescriptorLazy(SharedFileID fileID, std::string const &filename);

  NexusDescriptorLazy() = delete;

  // there is no reason to copy this object
  NexusDescriptorLazy &operator=(NexusDescriptorLazy const &nd) = delete;
  NexusDescriptorLazy(NexusDescriptorLazy const &nd) = delete;
  // there is no reason to move this object
  NexusDescriptorLazy &operator=(NexusDescriptorLazy &&nd) = delete;
  NexusDescriptorLazy(NexusDescriptorLazy &&nd) = delete;

  /**
   * Using RAII components, no need to deallocate explicitly
   */
  ~NexusDescriptorLazy() = default;

  /**
   * Returns a constant reference to the current file name
   * @return A reference to a const string containing the file name
   */
  inline std::string const &filename() const noexcept { return m_filename; }

  /**
   * Access the file extension. Defined as the string after and including the
   * last period character
   * @returns A reference to a const string containing the file extension
   */
  inline std::string const &extension() const noexcept { return m_extension; }

  /// Returns the name & type of the first entry in the file
  std::pair<std::string, std::string> const &firstEntryNameType() const noexcept { return m_firstEntryNameType; };

  /// Query if the given attribute exists on the root node
  bool hasRootAttr(std::string const &name) const;

  /**
   * Returns a const reference of the internal map holding all entries in the
   * Nexus HDF5 file
   * @return map holding all entries by group class
   * <pre>
   *   key: group address (absolute entry name, e.g., /entry/log)
   *   value: group class (e.g., NXentry, NXlog)
   * </pre>
   */
  EntryMap const &getAllEntries() const noexcept { return m_allEntries; }

  /**
   * Checks if a full-address entry exists for a particular groupClass in a Nexus
   * dataset
   * @param entryName full address for an entry name /entry/NXlogs
   * @param groupClass e.g. NxLog , Nexus entry attribute
   * @return true: entryName exists for a groupClass, otherwise false
   */
  bool isEntry(std::string const &entryName, std::string const &groupClass) const {
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
  bool isEntry(std::string const &entryName) const;

  /// Query if a given type exists somewhere in the file
  bool classTypeExists(std::string const &classType) const;

  /// Query if a given type exists as a decendant of the supplied parentPath. It is expected to be used only to check
  /// for direct children.
  bool classTypeExistsChild(const std::string &parentPath, const std::string &classType) const;

  /**
   * Return the absolute addresses of every entry with the given NX_class (groups) or "SDS" (datasets).
   * Triggers a one-time full scan of the file on first use, then serves subsequent queries from the cache.
   * @param classType e.g. NXlog, NXentry, SDS
   * @return ordered set of absolute addresses, e.g. {/entry/DASlogs/log_0, /entry/DASlogs/log_1, ...}
   */
  std::set<std::string> allAddressesOfType(std::string const &classType) const;

  /**
   * Register a known entry — call after creating a group or dataset so the cache stays
   * consistent with the file. Also removes the path from the miss cache if present.
   * @param entryName absolute path of the new entry
   * @param groupClass NXclass for a group, or SDS for a dataset
   */
  void registerEntry(std::string const &entryName, std::string const &groupClass);

  /**
   * Return the group NX_class for a given entry name, if it exists.  Else, UNKNOWN_CLASS.
   * @param entryName absolute path of the entry
   * @return the group NX_class for a given entry name, if it exists.  Else, UNKNOWN_CLASS.
   */
  std::string operator[](std::string const &entryName) const;

  /// @brief Get string data from a dataset at address
  /// @param address Full HDF5 address of the dataset
  /// @return string data at this address, if it is string dataset, else empty
  std::string getStrData(std::string const &address);

private:
  /**
   * Sets m_allEntries, called in HDF5 constructor.
   * m_filename must be set
   */
  EntryMap initAllEntries();
  void loadGroups(EntryMap &allEntries, std::string const &address, unsigned int depth, const unsigned int maxDepth);

  /**
   * Perform a single full-file scan (cycle-safe, via H5Lvisit2) that caches every entry's class into
   * m_allEntries and sets m_fullyScanned. A no-op once the file has been fully scanned. Enumeration
   * queries (allAddressesOfType, classTypeExists) call this so they read from a complete map instead of
   * re-walking the file on every call.
   */
  void ensureAllEntries() const;

  /** Nexus HDF5 file name */
  std::string const m_filename;
  /// Extension
  std::string const m_extension;
  /// HDF5 File Handle
  SharedFileID m_fileID;

  /** Root attributes cache. This is mutable because it is modified in a const method. */
  mutable std::unordered_set<std::string> m_rootAttrs;

  std::pair<std::string, std::string> m_firstEntryNameType;

  /**
   * All entries metadata. The map is mutable because additional values can be added lazily.
   * <pre>
   *   key: group address
   *   value: group class (e.g. NXentry, NXlog)
   * </pre>
   */
  mutable EntryMap m_allEntries;

  /// mutex to protect reading from file after initialization in const methods
  mutable std::shared_mutex m_readNexusMutex;

  /// the set of non-existent entries that have been checked
  mutable std::unordered_set<std::string> m_allMisses;

  /// true once the whole file has been scanned into m_allEntries, so enumeration queries are complete.
  /// mutable because it is set from const enumeration methods via ensureAllEntries().
  mutable bool m_fullyScanned = false;
};

} // namespace Nexus
} // namespace Mantid
