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
#include <H5Cpp.h>
#include <hdf5.h>

#include <algorithm>
#include <cstdlib> // malloc, calloc
#include <cstring> // strcpy
#include <filesystem>
#include <optional>
#include <set>
#include <stdexcept> // std::invalid_argument
#include <unordered_map>
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
        // fixed length string -- the buffer has to cover every point of the attribute's dataspace
        Mantid::Nexus::UniqueID<&H5Sclose> aspace(H5Aget_space(attrID));
        hssize_t const npoints = H5Sget_simple_extent_npoints(aspace);
        std::size_t const size = H5Tget_size(atype) * static_cast<std::size_t>(npoints > 0 ? npoints : 1);
        std::string buffer(size, '\0');
        if (H5Aread(attrID, atype, buffer.data()) >= 0) {
          // a fixed-length string is null-padded out to the width of its type, and the padding
          // is not part of the class name -- keep only the characters before the first null
          std::size_t const terminator = buffer.find('\0');
          if (terminator != std::string::npos) {
            buffer.resize(terminator);
          }
          nxClass = std::move(buffer);
        }
      }
    }
  }
  return nxClass;
}

// H5Lvisit2 callback for the one-time full scan: records every visited link's absolute address and its
// class (NX_class for groups, "SDS" for datasets) into the EntryMap supplied via op_data.
herr_t fullScanCallback(hid_t loc_id, char const *name, H5L_info2_t const *, void *op_data) {
  auto *entries = static_cast<Mantid::Nexus::EntryMap *>(op_data);
  H5O_info2_t oinfo;
  if (H5Oget_info_by_name3(loc_id, name, &oinfo, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
    return 0;
  }
  // H5Lvisit2 supplies the path relative to the visit root (no leading '/')
  std::string const address = "/" + std::string(name);
  if (oinfo.type == H5O_TYPE_GROUP) {
    Mantid::Nexus::UniqueID<&H5Oclose> oid(H5Oopen(loc_id, name, H5P_DEFAULT));
    if (oid.isValid()) {
      (*entries)[address] = readNXClass(oid);
    }
  } else if (oinfo.type == H5O_TYPE_DATASET) {
    (*entries)[address] = Mantid::Nexus::SCIENTIFIC_DATA_SET;
  }
  return 0;
}

// Data for the early-exit class probe: search for the first entry of a target class.
struct ClassProbe {
  std::string const &target;
  bool found;
};

// H5Literate2/H5Lvisit2 callback that stops (returns 1) at the first entry matching the target class,
// so an existence query need not scan the whole tree. Used by classTypeExists / classTypeExistsChild.
herr_t classProbeCallback(hid_t loc_id, char const *name, H5L_info2_t const *, void *op_data) {
  auto *probe = static_cast<ClassProbe *>(op_data);
  H5O_info2_t oinfo;
  if (H5Oget_info_by_name3(loc_id, name, &oinfo, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
    return 0;
  }
  if (oinfo.type == H5O_TYPE_GROUP) {
    Mantid::Nexus::UniqueID<&H5Oclose> oid(H5Oopen(loc_id, name, H5P_DEFAULT));
    if (oid.isValid() && readNXClass(oid) == probe->target) {
      probe->found = true;
      return 1; // non-zero stops iteration
    }
  } else if (oinfo.type == H5O_TYPE_DATASET && probe->target == Mantid::Nexus::SCIENTIFIC_DATA_SET) {
    probe->found = true;
    return 1;
  }
  return 0;
}
} // namespace

namespace Mantid::Nexus {

// PUBLIC

NexusDescriptorLazy::NexusDescriptorLazy(std::string const &filename)
    : m_filename(filename), m_extension(std::filesystem::path(m_filename).extension().string()), m_firstEntryNameType(),
      m_allEntries(initAllEntries()), m_allMisses() {}

NexusDescriptorLazy::NexusDescriptorLazy(SharedFileID fileID, std::string const &filename)
    : m_filename(filename), m_extension(std::filesystem::path(m_filename).extension().string()), m_fileID(fileID),
      m_firstEntryNameType(), m_allMisses() {
  m_allEntries = initAllEntries();
}

bool NexusDescriptorLazy::isEntry(std::string const &entryName, std::string const &groupClass) const {
  return (*this)[entryName] == groupClass;
}

// open the object to determine its type
bool NexusDescriptorLazy::isEntry(std::string const &entryName) const { return (*this)[entryName] != UNKNOWN_CLASS; }

bool NexusDescriptorLazy::isDataSet(std::string const &entryName) const {
  return (*this)[entryName] == SCIENTIFIC_DATA_SET;
}

/// @brief Check if a class type exists in the file
/// @param classType the NX_class type to check for
/// @return true if the class type exists anywhere in the file
bool NexusDescriptorLazy::classTypeExists(std::string const &classType) const {
  bool known_hit = false;
  {
    std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
    // a cache hit is always definitive; absence is definitive only once the whole file was scanned
    known_hit = std::any_of(m_allEntries.begin(), m_allEntries.end(),
                            [&classType](auto const &entry) { return entry.second == classType; });
  }
  if (known_hit) {
    return true;
  } else if (m_fullyScanned.load()) {
    return false;
  } else {
    // not cached and not fully scanned: walk with early-exit rather than forcing a full scan
    ClassProbe probe{classType, false};
    H5Lvisit2(m_fileID, H5_INDEX_NAME, H5_ITER_NATIVE, &classProbeCallback, &probe);
    return probe.found;
  }
}

bool NexusDescriptorLazy::classTypeExistsInCache(std::string const &classType) const {
  // cache-only: never walk the file. Absence here means "not in the bounded init scan", which is the
  // intended answer for confidence() checks — the discriminating classes are shallow by construction.
  std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
  return std::any_of(m_allEntries.begin(), m_allEntries.end(),
                     [&classType](auto const &entry) { return entry.second == classType; });
}

bool NexusDescriptorLazy::classTypeExistsChild(const std::string &parentPath, const std::string &classType) const {
  // if the parent doesn't exist, the child doesn't either
  if (!this->isEntry(parentPath)) {
    return false;
  }
  const auto delimitedEntryName = parentPath + '/';
  bool known_hit = false;
  {
    std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
    // a cached descendant of the right class is definitive; absence is definitive only once fully scanned
    known_hit =
        std::any_of(m_allEntries.begin(), m_allEntries.end(), [&classType, &delimitedEntryName](auto const &entry) {
          return entry.second == classType && entry.first.starts_with(delimitedEntryName);
        });
  }
  if (known_hit) {
    return true;
  } else if (m_fullyScanned.load()) {
    return false;
  } else {
    // not cached and not fully scanned: probe the parent subtree with early-exit
    UniqueID<&H5Gclose> parentID(H5Gopen(m_fileID, parentPath.c_str(), H5P_DEFAULT));
    if (!parentID.isValid()) {
      return false;
    }
    ClassProbe probe{classType, false};
    H5Lvisit2(parentID, H5_INDEX_NAME, H5_ITER_NATIVE, &classProbeCallback, &probe);
    return probe.found;
  }
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

void NexusDescriptorLazy::registerEntry(std::string const &entryName, std::string const &groupClass) const {
  std::lock_guard<std::shared_mutex> lock(m_readNexusMutex);
  m_allMisses.erase(entryName);
  m_allEntries[entryName] = groupClass;
}

void NexusDescriptorLazy::registerDataSet(std::string const &entryName) const {
  registerEntry(entryName, SCIENTIFIC_DATA_SET);
}

std::string NexusDescriptorLazy::operator[](std::string const &entryName) const {
  bool known_miss = true;
  {
    std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
    known_miss = m_allMisses.contains(entryName);
  }
  if (known_miss) {
    return UNKNOWN_CLASS;
  } else {
    EntryMap::iterator it, iend;
    {
      std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
      it = m_allEntries.find(entryName);
      iend = m_allEntries.end();
    }
    if (it != iend) {
      // if it is found in the cache, use it
      return it->second;
    } else {
      // otherwise check if it exists in the file
      if (H5Oexists_by_name(m_fileID, entryName.c_str(), H5P_DEFAULT) > 0) {
        // if it exists in file, read and save the correct class type for it
        std::string nxclass;
        H5O_info_t oinfo;
        UniqueID<H5Gclose> entryID(H5Oopen(m_fileID, entryName.c_str(), H5P_DEFAULT));
        H5Oget_info(entryID, &oinfo, H5O_INFO_BASIC);
        if (oinfo.type == H5O_TYPE_DATASET) {
          nxclass = SCIENTIFIC_DATA_SET;
        } else {
          // read NX_class attribute
          nxclass = readNXClass(entryID);
        }
        // modifying m_allEntries, need write lock. Do NOT move nxclass here — it is returned below.
        std::lock_guard<std::shared_mutex> lock(m_readNexusMutex);
        m_allEntries[entryName] = nxclass;
        return nxclass;
      } else {
        // if it does not exist in the file, cache the miss
        std::lock_guard<std::shared_mutex> lock(m_readNexusMutex);
        m_allMisses.insert(entryName);
        return UNKNOWN_CLASS;
      }
    }
  }
}

/// Get string data from a dataset at address
std::string NexusDescriptorLazy::getStrData(std::string const &address) {
  std::string strData{};
  if (isDataSet(address)) {
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

std::set<std::string> NexusDescriptorLazy::allAddressesOfType(std::string const &classType) const {
  // ensure the whole file has been cached (one-time full scan), then filter by class from memory
  ensureAllEntries();
  std::set<std::string> result;
  std::shared_lock<std::shared_mutex> lock(m_readNexusMutex);
  for (auto const &[address, cls] : m_allEntries) {
    if (cls == classType) {
      result.insert(address);
    }
  }
  return result;
}

// PRIVATE

void NexusDescriptorLazy::ensureAllEntries() const {
  // fast path: already fully scanned
  if (m_fullyScanned.load()) {
    return;
  } else {
    // Walk the whole file once. H5Lvisit2 is cycle-safe, so hard-linked groups cannot cause infinite
    // recursion. Build into a local map with no lock held, then merge — readers are not blocked during I/O.
    std::lock_guard<std::shared_mutex> lock(m_readNexusMutex);
    EntryMap scanned;
    H5Lvisit2(m_fileID, H5_INDEX_NAME, H5_ITER_NATIVE, &fullScanCallback, &scanned);
    for (auto &entry : scanned) {
      m_allEntries.insert_or_assign(entry.first, std::move(entry.second));
    }
    m_fullyScanned.store(true);
  }
}

void NexusDescriptorLazy::loadGroups(EntryMap &allEntries, std::string const &address, unsigned int depth,
                                     const unsigned int maxDepth) {
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
    ssize_t name_len = H5Gget_objname_by_idx(groupID, i, nullptr, 0);
    if (name_len <= 0)
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

EntryMap NexusDescriptorLazy::initAllEntries() {

  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

  EntryMap allEntries;

  // open the file if not already open (may have been provided via the hid_t constructor)
  if (!m_fileID.isValid()) {
    if (!std::filesystem::exists(m_filename))
      return allEntries;
    if (!H5::H5File::isAccessible(m_filename, Mantid::Nexus::H5Util::defaultFileAcc())) {
      throw std::invalid_argument("ERROR: NexusDescriptorLazy couldn't open hdf5 file " + m_filename + "\n");
    }
    m_fileID = H5Fopen(m_filename.c_str(), H5F_ACC_RDONLY, Mantid::Nexus::H5Util::defaultFileAcc().getId());
    if (!m_fileID.isValid()) {
      throw std::invalid_argument("ERROR: NexusDescriptorLazy couldn't open hdf5 file " + m_filename + "\n");
    }
  }

  // get all top-level entries
  {
    unsigned int depth = 0;
    loadGroups(allEntries, "/", depth, INIT_DEPTH);
    // set the first entry name/type — find the first direct child of root
    // (unordered_map has no ordering, so we can't rely on iterator arithmetic)
    m_firstEntryNameType = std::make_pair("", UNKNOWN_CLASS);
    for (auto const &[path, cls] : allEntries) {
      if (path.size() > 1 && path.find('/', 1) == std::string::npos) {
        m_firstEntryNameType = {path.substr(1), cls};
        break;
      }
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
  }
  // rely on move semantics for single return
  return allEntries;
}

} // namespace Mantid::Nexus
