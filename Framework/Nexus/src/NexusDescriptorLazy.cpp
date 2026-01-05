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

#include <H5Cpp.h>
#include <boost/multi_index/detail/index_matcher.hpp>
#include <hdf5.h>

#include <cstdlib> // malloc, calloc
#include <cstring> // strcpy
#include <filesystem>
#include <stdexcept> // std::invalid_argument
#include <unordered_map>
#include <unordered_set>
#include <utility>

using boost::multi_index::detail::index_matcher::entry;

static unsigned int const INIT_DEPTH = 1;
static unsigned int const ENTRY_DEPTH = 2;
static unsigned int const INSTR_DEPTH = 5;
static std::unordered_set<std::string> const SPECIAL_ADDRESS{"/entry0", "/entry1"};
static std::string const NONEXISTENT = "NONEXISTENT"; // register failures as well
static std::string const UNKNOWN_CLASS = "UNKNOWN_CLASS";
static std::string const SCIENTIFIC_DATA_SET = "SDS";

namespace Mantid::Nexus {

// PUBLIC

NexusDescriptorLazy::NexusDescriptorLazy(std::string const &filename)
    : m_filename(filename), m_extension(std::filesystem::path(m_filename).extension().string()),
      m_allEntries(initAllEntries()) {}

bool NexusDescriptorLazy::isEntry(std::string const &entryName) {
  if (m_allEntries.contains(entryName)) {
    return m_allEntries[entryName] != NONEXISTENT;
  } else {
    UniqueID<&H5Oclose> entryID(H5Oopen(m_fileID, entryName.c_str(), H5P_DEFAULT));
    if (entryID.isValid()) {
      m_allEntries[entryName] = UNKNOWN_CLASS;
      H5O_info2_t oinfo;
      H5Oget_info(entryID, &oinfo, H5O_INFO_BASIC);
      if (oinfo.type == H5O_TYPE_DATASET) {
        m_allEntries[entryName] = SCIENTIFIC_DATA_SET;
      } else {
        // read NX_class attribute
        UniqueID<&H5Aclose> attrID = H5Aopen_name(entryID, "NX_class");
        if (attrID.isValid()) {
          H5A_info_t ainfo;
          H5Aget_info(attrID, &ainfo);
          m_allEntries[entryName].resize(ainfo.data_size + 1); // +1 for null terminator
          H5Aread(attrID, H5Aget_type(attrID), m_allEntries[entryName].data());
        }
      }
      return true;
    } else {
      // register failure
      m_allEntries[entryName] = NONEXISTENT;
      return false;
    }
  }
  return false;
}

bool NexusDescriptorLazy::classTypeExists(std::string const &classType) const {
  throw std::logic_error("NexusDescriptorLazy::classTypeExists not implemented yet");
}

bool NexusDescriptorLazy::hasRootAttr(std::string const &name) {
  if (m_rootAttrs.count(name) == 1) {
    return true;
  } else {
    if (H5Aexists(m_fileID, name.c_str()) > 0) {
      m_rootAttrs.emplace(name);
      return true;
    } else {
      return false;
    }
  }
}

// PRIVATE

void NexusDescriptorLazy::loadGroups(std::unordered_map<std::string, std::string> &allEntries,
                                     std::string const &address, unsigned int depth, const unsigned int maxDepth) {
  if (depth >= maxDepth)
    return;

  UniqueID<&H5Gclose> groupID(H5Gopen(m_fileID, address.c_str(), H5P_DEFAULT));
  if (!groupID.isValid())
    return;

  // get NX_class attribute
  std::string nxClass = UNKNOWN_CLASS;
  if (H5Aexists(groupID.get(), "NX_class")) {
    UniqueID<&H5Aclose> attrID = H5Aopen_name(groupID, "NX_class");
    if (attrID.isValid()) {
      int strlen = H5Aread(attrID, H5T_C_S1, nullptr);
      nxClass.resize(strlen + 1); // +1 for null terminator
      H5Aread(attrID, H5T_C_S1, nxClass.data());
    }
  }
  allEntries[address] = nxClass;

  // iterate over members
  hsize_t numObjs;
  H5Gget_num_objs(groupID.get(), &numObjs);
  for (hsize_t i = 0; i < numObjs; i++) {
    H5G_obj_t type = H5Gget_objtype_by_idx(groupID, i);
    ssize_t name_len = H5Gget_objname_by_idx(groupID, i, nullptr, 0);
    if (name_len <= 0)
      continue;
    std::string memberName(name_len + 1, 'X'); // +1 for null terminator, fill with X for obvious errors
    H5Gget_objname_by_idx(groupID, i, memberName.data(), name_len + 1);
    std::string memberAddress = address;
    if (!memberAddress.ends_with("/"))
      memberAddress += "/";
    memberAddress += memberName;

    if (type == H5G_GROUP) {
      loadGroups(allEntries, memberAddress, depth + 1, maxDepth);
    } else if (type == H5G_DATASET) {
      allEntries[memberAddress] = "SDS";
    }
  }
}

std::unordered_map<std::string, std::string> NexusDescriptorLazy::initAllEntries() {

  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

  std::unordered_map<std::string, std::string> allEntries;

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

    // for levels beyond 2, only load special entries
    depth = 1;
    for (std::string const &specialAddress : SPECIAL_ADDRESS) {
      if (isEntry(specialAddress))
        loadGroups(allEntries, specialAddress, depth, ENTRY_DEPTH);
    }

    // get instrument up to a depth of 5
    depth = 2;
    for (std::string const &specialAddress : SPECIAL_ADDRESS) {
      if (isEntry(specialAddress)) {
        std::string instrumentAddress = specialAddress + "/instrument";
        if (isEntry(instrumentAddress)) {
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

} // namespace Mantid::Nexus
