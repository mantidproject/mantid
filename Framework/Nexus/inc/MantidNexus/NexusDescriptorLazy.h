// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/UniqueID.h"

#include <cmath>
#include <concepts>
#include <format>
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <unordered_set>
#include <variant>
#include <vector>

#include <H5Cpp.h>

namespace Mantid {
namespace Nexus {

template <typename T>
concept entryTypes = std::integral<T> || std::floating_point<T> || std::same_as<T, std::string>;

template <typename T> hid_t getH5NativeType();

class MANTID_NEXUS_DLL NexusDescriptorLazy {

public:
  enum class CacheReturnStatus_t {
    FOUND,
    CACHED,
    DATASET_NOT_FOUND,
    WRONG_TYPE,
    ERROR,
    UNSET,
  };

  using CacheValue_t = std::variant<float, double, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                                    uint64_t, std::string, CacheReturnStatus_t>;

  /**
   * Unique constructor
   * @param filename input HDF5 Nexus file name
   */
  NexusDescriptorLazy(std::string const &filename);

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
  std::map<std::string, std::string> const &getAllEntries() const noexcept { return m_allEntries; }

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

  /**
   * @brief Gets the value of an entry in the Nexus file.
   * Only single values are returned, either numeric or strings.
   * @param entryName full address for an entry name
   * @return pair<value, status> where value is valid only if the return status is FOUND or CACHED
   */
  template <typename T>
  std::pair<T, NexusDescriptorLazy::CacheReturnStatus_t> getEntryValue(const std::string &entryName) const;

  /// Query if a given type exists somewhere in the file
  bool classTypeExists(std::string const &classType) const;

  /// Query if a given type exists as a decendant of the supplied parentPath. It is expected to be used only to check
  /// for direct children.
  bool classTypeExistsChild(const std::string &parentPath, const std::string &classType) const;

  /// @brief Get string data from a dataset at address
  /// @param address Full HDF5 address of the dataset
  /// @return string data at this address, if it is string dataset, else empty
  std::string getStrData(std::string const &address);

private:
  /**
   * Sets m_allEntries, called in HDF5 constructor.
   * m_filename must be set
   */
  std::map<std::string, std::string> initAllEntries();
  void loadGroups(std::map<std::string, std::string> &allEntries, std::string const &address, unsigned int depth,
                  const unsigned int maxDepth);

  const CacheValue_t _getEntryValue(const std::string &entryName) const;

  /** Nexus HDF5 file name */
  std::string const m_filename;
  /// Extension
  std::string const m_extension;
  /// HDF5 File Handle
  UniqueID<&H5Fclose> m_fileID;

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
  mutable std::map<std::string, std::string> m_allEntries;

  /// mutex to protect reading from file after initialization in const methods
  mutable std::shared_mutex m_readNexusMutex;

  /// the set of non-existent entries that have been checked
  mutable std::unordered_set<std::string> m_allMisses;

  /// the map of all read entry values that have been checked
  mutable std::map<std::string, CacheValue_t> m_readEntries;
};

} // namespace Nexus
} // namespace Mantid
