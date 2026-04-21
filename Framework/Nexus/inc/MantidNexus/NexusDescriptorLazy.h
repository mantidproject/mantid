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
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include <H5Cpp.h>

namespace Mantid {
namespace Nexus {

using CacheValue =
    std::variant<float, double, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, std::string>;

template <typename T>
concept entryTypes = std::integral<T> || std::floating_point<T> || std::same_as<T, std::string>;

template <typename T> hid_t getH5NativeType();

class MANTID_NEXUS_DLL NexusDescriptorLazy {

public:
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

  template <entryTypes T> bool checkEntry(const std::string &entryName, const T &value) const;
  template <typename T> std::optional<std::reference_wrapper<const T>> getCached(const std::string &key) const;

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

  mutable std::map<std::string, CacheValue> m_readEntries;
};

template <>
inline bool NexusDescriptorLazy::checkEntry<std::string>(const std::string &entryName, const std::string &value) const {
  if (H5Oexists_by_name(m_fileID, entryName.c_str(), H5P_DEFAULT) <= 0)
    return false;

  UniqueID<&H5Oclose> entryID(H5Oopen(m_fileID, entryName.c_str(), H5P_DEFAULT));
  hid_t datatype = H5Dget_type(entryID.get());

  if (H5Tget_class(datatype) == H5T_STRING) {
    if (H5Tis_variable_str(datatype)) {
      char *rdata = nullptr;
      H5Dread(entryID.get(), datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &rdata);
      const std::string s(rdata);
      H5free_memory(rdata);
      return s == value;
    } else {
      size_t size = H5Tget_size(datatype);
      std::vector<char> buffer(size + 1, '\0');
      H5Dread(entryID.get(), datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
      const std::string s(buffer.data());
      return s == value;
    }
  }
  return false;
}

template <> inline hid_t getH5NativeType<float>() { return H5T_NATIVE_FLOAT; }

template <> inline hid_t getH5NativeType<double>() { return H5T_NATIVE_DOUBLE; }

template <> inline hid_t getH5NativeType<int8_t>() { return H5T_NATIVE_INT8; }

template <> inline hid_t getH5NativeType<uint8_t>() { return H5T_NATIVE_UINT8; }

template <> inline hid_t getH5NativeType<int16_t>() { return H5T_NATIVE_INT16; }

template <> inline hid_t getH5NativeType<uint16_t>() { return H5T_NATIVE_UINT16; }

template <> inline hid_t getH5NativeType<int32_t>() { return H5T_NATIVE_INT32; }

template <> inline hid_t getH5NativeType<uint32_t>() { return H5T_NATIVE_UINT32; }

template <> inline hid_t getH5NativeType<int64_t>() { return H5T_NATIVE_INT64; }

template <> inline hid_t getH5NativeType<uint64_t>() { return H5T_NATIVE_UINT64; }

template <typename T>
std::optional<std::reference_wrapper<const T>> NexusDescriptorLazy::getCached(const std::string &key) const {
  auto it = m_readEntries.find(key);
  if (it == m_readEntries.end())
    return std::nullopt;

  if (auto ptr = std::get_if<T>(&it->second))
    return std::cref(*ptr);

  return std::nullopt;
}

template <entryTypes T> bool NexusDescriptorLazy::checkEntry(const std::string &entryName, const T &value) const {

  // Checks if the entry is cached and if so compare the value with the cached one
  auto cachedValueOpt = getCached<T>(entryName);
  if (cachedValueOpt) {
    T cachedValue = cachedValueOpt.value().get();
    if constexpr (std::same_as<T, std::string>) {
      return cachedValue == value;
    } else {
      if constexpr (std::floating_point<T>)
        return std::fabs(cachedValue - value) < 1e-12;
      else
        return cachedValue == value;
    }
  }

  // Otherwise fetch if possible the entry and performs the comparison
  if (H5Oexists_by_name(m_fileID, entryName.c_str(), H5P_DEFAULT) <= 0)
    return false;

  UniqueID<&H5Dclose> entryID(H5Dopen(m_fileID, entryName.c_str(), H5P_DEFAULT));

  hid_t datatype = H5Dget_type(entryID.get());

  // Case of a string (fixed or variable length) type
  if constexpr (std::same_as<T, std::string>) {

    if (H5Tget_class(datatype) != H5T_STRING)
      return false;

    // Variable-length string
    if (H5Tis_variable_str(datatype)) {

      char *rdata = nullptr;
      H5Dread(entryID.get(), datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &rdata);

      std::string s(rdata);
      H5free_memory(rdata);
      return s == value;

      // Fixed-length string
    } else {

      size_t size = H5Tget_size(datatype);
      std::vector<char> buffer(size + 1, '\0');

      H5Dread(entryID.get(), datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());

      std::string s(buffer.data());
      return s == value;
    }
    // Numeric type
  } else {

    if (H5Tget_class(datatype) != H5T_FLOAT && H5Tget_class(datatype) != H5T_INTEGER)
      return false;

    hid_t dataspace = H5Dget_space(entryID.get());
    int ndims = H5Sget_simple_extent_ndims(dataspace);

    // The ndims < 0 is for case when an error occured while fetching the dims
    if (ndims < 0 || ndims > 1) {
      H5Sclose(dataspace);
      return false;
    }

    hsize_t size = 1;
    hsize_t dims[1] = {1};

    if (ndims == 1) {
      H5Sget_simple_extent_dims(dataspace, dims, nullptr);
      size = dims[0];
    }
    H5Sclose(dataspace);

    // Read the entry
    std::vector<T> buffer(size);
    H5Dread(entryID.get(), getH5NativeType<T>(), H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());

    if (buffer.size() != 1)
      return false;

    // Update the cache
    m_readEntries[entryName] = value;

    if constexpr (std::floating_point<T>)
      return std::fabs(buffer[0] - value) < 1e-12;
    else
      return buffer[0] == value;
  }
}

} // namespace Nexus
} // namespace Mantid
