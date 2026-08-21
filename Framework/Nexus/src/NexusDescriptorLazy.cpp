// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexus/NexusDescriptorLazy.h"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NexusException.h"
#include "MantidNexus/UniqueID.h"

#include "MantidNexus/NexusFile_fwd.h"
#include <hdf5.h>

#include <algorithm>
#include <cstdlib> // malloc, calloc
#include <cstring> // strcpy
#include <filesystem>
#include <map>
#include <stdexcept> // std::invalid_argument
#include <unordered_set>
#include <utility>

static unsigned int const INIT_DEPTH = 1;
static unsigned int const ENTRY_DEPTH = 2;
static unsigned int const INSTR_DEPTH = 5;
static std::unordered_set<std::string> const SPECIAL_ADDRESS{"/entry", "/entry0", "/entry1", "/raw_data_1"};
static std::string const UNKNOWN_CLASS = "UNKNOWN_CLASS";

namespace {
template <herr_t (*H5Xclose)(hid_t)> std::string readNXClass(Mantid::Nexus::UniqueID<H5Xclose> const &oid) {
  std::string nxClass = UNKNOWN_CLASS;
  if (H5Aexists(oid, Mantid::Nexus::GROUP_CLASS_SPEC.c_str()) > 0) {
    Mantid::Nexus::UniqueID<&H5Aclose> attrID = H5Aopen(oid, Mantid::Nexus::GROUP_CLASS_SPEC.c_str(), H5P_DEFAULT);
    if (attrID.isValid()) {
      Mantid::Nexus::UniqueID<&H5Tclose> atype(H5Aget_type(attrID));
      if (H5Tis_variable_str(atype)) {
        // variable length string
        char *rdata = nullptr;
        if (H5Aread(attrID, atype, &rdata) >= 0) {
          nxClass = std::string(rdata);
        }
        // reclaim memory allocated for rdata by HDF5
        H5free_memory(rdata);
      } else {
        // fixed length string
        std::size_t size = H5Tget_size(atype);
        nxClass.resize(size);
        H5Aread(attrID, atype, nxClass.data());
      }
    }
  }
  return nxClass;
}
} // namespace

namespace Mantid::Nexus {

// PUBLIC

NexusDescriptorLazy::NexusDescriptorLazy(std::string const &filename)
    : m_filename(filename), m_extension(std::filesystem::path(m_filename).extension().string()), m_firstEntryNameType(),
      m_allEntries(initAllEntries()), m_allMisses() {}

// open the object to determine its type
bool NexusDescriptorLazy::isEntry(std::string const &entryName) const {
  bool known_miss = false, known_hit = false;
  {
    // wait for any writes to m_allMisses to end
    std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
    known_miss = m_allMisses.contains(entryName);
    known_hit = m_allEntries.contains(entryName);
  }
  if (known_miss) {
    // if we know this doesn't exist, return early
    return false;
  } else if (known_hit) {
    // if we know it does exist, return
    return true;
  } else {
    if (H5Oexists_by_name(m_fileID, entryName.c_str(), H5P_DEFAULT) > 0) {
      // if it is there, save the correct class type for it
      std::string nxclass;
      H5O_info_t oinfo;
      // otherwise, try to open this group and see if it is there
      UniqueID<&H5Oclose> entryID(H5Oopen(m_fileID, entryName.c_str(), H5P_DEFAULT));
      H5Oget_info(entryID, &oinfo, H5O_INFO_BASIC);
      if (oinfo.type == H5O_TYPE_DATASET) {
        nxclass = SCIENTIFIC_DATA_SET;
      } else {
        // read NX_class attribute
        nxclass = readNXClass(entryID);
      }
      // modifying m_allEntries, need write lock
      std::lock_guard<std::shared_mutex> lock(m_readNexusMutex);
      m_allEntries[entryName] = std::move(nxclass);
      return true;
    } else {
      // otherwise register failure, need write lock
      std::lock_guard<std::shared_mutex> lock(m_readNexusMutex);
      m_allMisses.insert(entryName);
      return false;
    }
  }
}

/// @brief Check if a class type exists in the file
/// @param classType the NX_class type to check for
/// @return true if the class type exists anywhere in the file
bool NexusDescriptorLazy::classTypeExists(std::string const &classType) const {
  // wait for writes to end
  std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
  return std::any_of(m_allEntries.begin(), m_allEntries.end(),
                     [&classType](auto const &entry) { return entry.second == classType; });
}

bool NexusDescriptorLazy::classTypeExistsChild(const std::string &parentPath, const std::string &classType) const {
  // if the parent doesn't exist, the child doesn't either
  if (!this->isEntry(parentPath))
    return false;

  // wait for writes to end
  std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);

  // linear search through all entries - stop at first match
  const auto delimitedEntryName = parentPath + '/';
  for (auto const &[name, cls] : m_allEntries) {
    // match the class first since that limits the list more
    if (cls == classType && name.starts_with(delimitedEntryName)) {
      return true;
    }
  }
  return false;
}

bool NexusDescriptorLazy::hasRootAttr(std::string const &name) const {
  bool known_hit = false;
  { // wait for writes to end
    std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
    known_hit = m_rootAttrs.contains(name);
  }
  if (known_hit) {
    return true;
  } else {
    // check the file since it wasn't in the cache
    if (H5Aexists(m_fileID, name.c_str()) > 0) {
      // mutex has the wrong name, but it's what we have
      std::lock_guard<std::shared_mutex> lock(m_readNexusMutex);
      m_rootAttrs.emplace(name);
      return true;
    } else {
      return false;
    }
  }
}

/// Get string data from a dataset at address
std::string NexusDescriptorLazy::getStrData(std::string const &address) {
  std::string strData;
  if (isEntry(address, SCIENTIFIC_DATA_SET)) {
    // open the data set and get its string data
    // using H5Cpp interface because trying to read string data is an absolute nightmare with the C API
    UniqueID<&H5Dclose> did(H5Dopen(m_fileID, address.c_str(), H5P_DEFAULT));
    H5::DataSet dataset(did);
    H5::DataType dtype = dataset.getDataType();
    if (dtype.isVariableStr() || dtype.getClass() == H5T_STRING) {
      dataset.read(strData, dtype, dataset.getSpace());
    }
  }
  return strData;
}

// PRIVATE

void NexusDescriptorLazy::loadGroups(std::map<std::string, std::string> &allEntries, std::string const &address,
                                     unsigned int depth, const unsigned int maxDepth) {
  UniqueID<&H5Gclose> groupID(H5Gopen(m_fileID, address.c_str(), H5P_DEFAULT));
  if (!groupID.isValid()) {
    return;
  }

  // get NX_class attribute
  allEntries[address] = readNXClass(groupID);

  if (depth >= maxDepth)
    return;

  // iterate over members
  hsize_t numObjs = 0;
  H5Gget_num_objs(groupID.get(), &numObjs);
  for (hsize_t i = 0; i < numObjs; i++) {
    H5G_obj_t type = H5Gget_objtype_by_idx(groupID, i);
    size_t name_len = H5Gget_objname_by_idx(groupID, i, nullptr, 0);
    if (name_len == 0)
      continue;
    std::string memberName(name_len, 'X');                              // fill with X for obvious errors
    H5Gget_objname_by_idx(groupID, i, memberName.data(), name_len + 1); // +1 for null terminator,
    std::string memberAddress = address;
    if (!memberAddress.ends_with("/"))
      memberAddress += "/";
    memberAddress += memberName;

    if (type == H5G_GROUP) {
      loadGroups(allEntries, memberAddress, depth + 1, maxDepth);
    } else if (type == H5G_DATASET) {
      allEntries[memberAddress] = SCIENTIFIC_DATA_SET;
    }
  }
}

std::map<std::string, std::string> NexusDescriptorLazy::initAllEntries() {

  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

  std::map<std::string, std::string> allEntries;

  // if the file exists read it
  if (std::filesystem::exists(m_filename)) {
    // if the file exists but cannot be opened, throw invalid
    // NOTE must be std::invalid_argument for expected errors to be raised in python API
    if (!H5::H5File::isAccessible(m_filename, Mantid::Nexus::H5Util::defaultFileAcc())) {
      throw std::invalid_argument("ERROR: NexusDescriptorLazy couldn't open hdf5 file " + m_filename + "\n");
    } else {
      m_fileID = H5Fopen(m_filename.c_str(), H5F_ACC_RDONLY, Mantid::Nexus::H5Util::defaultFileAcc().getId());
    }
    if (!m_fileID.isValid()) {
      throw std::invalid_argument("ERROR: NexusDescriptorLazy couldn't open hdf5 file " + m_filename + "\n");
    }

    // get all top-level entries
    unsigned int depth = 0;
    loadGroups(allEntries, "/", depth, INIT_DEPTH);
    // set the first entry name/type
    if (allEntries.size() > 1) {
      m_firstEntryNameType = *(++allEntries.begin());
      m_firstEntryNameType.first = m_firstEntryNameType.first.substr(1); // remove leading /
    } else {
      m_firstEntryNameType = std::make_pair("", UNKNOWN_CLASS);
    }

    // for levels beyond 2, only load special entries
    depth = INIT_DEPTH;
    for (std::string const &specialAddress : SPECIAL_ADDRESS) {
      if (allEntries.contains(specialAddress))
        loadGroups(allEntries, specialAddress, depth, ENTRY_DEPTH);
    }

    // get instrument up to a depth of 5
    depth = ENTRY_DEPTH;
    for (std::string const &specialAddress : SPECIAL_ADDRESS) {
      if (allEntries.contains(specialAddress)) {
        std::string instrumentAddress = specialAddress + "/instrument";
        if (allEntries.contains(instrumentAddress)) {
          loadGroups(allEntries, instrumentAddress, depth, INSTR_DEPTH);
        }
      }
    }
  } else {
    // if the file does not exist, then leave allEntries empty
  }
  // rely on move semantics for single return
  return allEntries;
}

using CRS_t = NexusDescriptorLazy::CacheReturnStatus_t;

const NexusDescriptorLazy::CacheValue_t NexusDescriptorLazy::_getEntryValue(const std::string &entryName) const {
  // Otherwise fetch if possible the entry and performs the comparison
  if (H5Oexists_by_name(m_fileID, entryName.c_str(), H5P_DEFAULT) <= 0)
    return CRS_t::DATASET_NOT_FOUND;

  UniqueID<&H5Dclose> entryID(H5Dopen(m_fileID, entryName.c_str(), H5P_DEFAULT));

  hid_t datatype = H5Dget_type(entryID.get());

  // Case of a string (fixed or variable length) type

  if (H5Tget_class(datatype) == H5T_STRING) {

    // Variable-length string
    if (H5Tis_variable_str(datatype)) {

      char *rdata = nullptr;
      H5Dread(entryID.get(), datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &rdata);

      std::string s(rdata);
      H5free_memory(rdata);

      return s; // return std::string

      // Fixed-length string
    } else {

      size_t size = H5Tget_size(datatype);
      std::vector<char> buffer(size + 1, '\0');

      H5Dread(entryID.get(), datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());

      std::string s(buffer.data());
      return s; // return std::string
    }
    // Numeric type
  } else {
    if (H5Tget_class(datatype) != H5T_FLOAT && H5Tget_class(datatype) != H5T_INTEGER)
      return CRS_t::WRONG_TYPE;

    hid_t dataspace = H5Dget_space(entryID.get());
    int ndims = H5Sget_simple_extent_ndims(dataspace);

    // The ndims < 0 is for case when an error occured while fetching the dims
    if (ndims < 0 || ndims > 1) {
      H5Sclose(dataspace);
      return CRS_t::ERROR;
    }
    hsize_t size = 1;
    hsize_t dims[1] = {1};

    // if (ndims == 1) { // ensured by the previous if
    H5Sget_simple_extent_dims(dataspace, dims, nullptr);
    size = dims[0];
    //}
    H5Sclose(dataspace);

    // what about unsigned int?
    if (H5Tget_class(datatype) == H5T_FLOAT) {
      // Read the entry
      std::vector<float> buffer(size);
      H5Dread(entryID.get(), H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());

      if (buffer.size() != 1)
        return CRS_t::ERROR;
      else
        return buffer[0];
    } else if (H5Tget_class(datatype) == H5T_INTEGER) {
      // Read the entry
      std::vector<int> buffer(size);
      H5Dread(entryID.get(), H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());

      if (buffer.size() != 1)
        return CRS_t::ERROR;
      else
        return buffer[0];
    }

  } // else numeric

  return CRS_t::ERROR;
}

template <typename T>
std::pair<T, NexusDescriptorLazy::CacheReturnStatus_t>
NexusDescriptorLazy::getEntryValue(const std::string &entryName) const {

  T value = T{};
  NexusDescriptorLazy::CacheReturnStatus_t returnStatus = NexusDescriptorLazy::CacheReturnStatus_t::CACHED;
  // Checks if the entry is cached and if so compare the value with the cached one
  auto it = m_readEntries.find(entryName);
  if (it == m_readEntries.end()) {
    auto result = _getEntryValue(entryName);

    std::tie(it, std::ignore) = m_readEntries.insert(std::pair{entryName, result});
    returnStatus = NexusDescriptorLazy::CacheReturnStatus_t::FOUND;
  }

  // value not found or not available
  if (auto ptr = std::get_if<NexusDescriptorLazy::CacheReturnStatus_t>(&it->second))
    returnStatus = *ptr;
  else if (auto ptr = std::get_if<T>(&it->second))
    value = *ptr;
  else
    returnStatus = NexusDescriptorLazy::CacheReturnStatus_t::WRONG_TYPE;

  // the value type is wrong
  return std::pair<T, NexusDescriptorLazy::CacheReturnStatus_t>{value, returnStatus};
}

template std::pair<std::string, NexusDescriptorLazy::CacheReturnStatus_t>
NexusDescriptorLazy::getEntryValue<std::string>(const std::string &) const;

template std::pair<int, NexusDescriptorLazy::CacheReturnStatus_t>
NexusDescriptorLazy::getEntryValue<int>(const std::string &) const;

template std::pair<float, NexusDescriptorLazy::CacheReturnStatus_t>
NexusDescriptorLazy::getEntryValue<float>(const std::string &) const;

} // namespace Mantid::Nexus
