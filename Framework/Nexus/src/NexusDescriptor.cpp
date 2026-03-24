// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexus/NexusDescriptor.h"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NexusException.h"

#include <H5Cpp.h>
#include <boost/multi_index/detail/index_matcher.hpp>

#include <cstdlib> // malloc, calloc
#include <cstring> // strcpy
#include <filesystem>
#include <stdexcept> // std::invalid_argument
#include <utility>

using boost::multi_index::detail::index_matcher::entry;

namespace Mantid::Nexus {

/// hdf5 specific functions, stay in anonymous namespace to make hdf5 linking
/// PRIVATE
namespace {

void getGroup(H5::Group groupID, std::map<std::string, std::set<std::string>> &allEntries,
              std::pair<std::string, std::string> &firstEntryNameType, size_t level) {

  /**
   * Return the NX_class attribute associate with objectName group entry
   */
  auto lf_getNxClassAttribute = [&](H5::Group groupID) -> std::string {
    std::string attribute = UNKNOWN_GROUP_SPEC;

    if (groupID.attrExists(GROUP_CLASS_SPEC)) {
      const auto attributeID = groupID.openAttribute(GROUP_CLASS_SPEC);
      attributeID.read(attributeID.getDataType(), attribute);
    }

    return attribute;
  };

  // using HDF5 C++ API
  const std::string groupNameStr = groupID.getObjName();
  const std::string nxClass = (groupNameStr == "/") ? "" : lf_getNxClassAttribute(groupID);

  if (!nxClass.empty()) {
    allEntries[nxClass].insert(groupNameStr);
  }

  for (hsize_t i = 0; i < groupID.getNumObjs(); ++i) {

    H5G_obj_t type = groupID.getObjTypeByIdx(i);
    H5std_string memberName = groupID.getObjnameByIdx(i);

    if (type == H5G_GROUP) {
      H5::Group subGroupID = groupID.openGroup(memberName);
      if (level == 0)
        firstEntryNameType = std::make_pair(memberName, lf_getNxClassAttribute(subGroupID));
      getGroup(subGroupID, allEntries, firstEntryNameType, level + 1);
    } else if (type == H5G_DATASET) {
      allEntries["SDS"].emplace(groupNameStr + "/" + memberName);
    }
  }
}
} // namespace

// PUBLIC

NexusDescriptor::NexusDescriptor(std::string const &filename)
    : m_filename(filename), m_extension(std::filesystem::path(m_filename).extension().string()), m_firstEntryNameType(),
      m_allEntries(initAllEntries()) {}

NexusDescriptor::NexusDescriptor(std::string const &filename, NXaccess access)
    : m_filename(filename), m_extension(std::filesystem::path(m_filename).extension().string()),
      m_firstEntryNameType() {
  // if we are creating a file and it already exists, then delete it first
  if (access == NXaccess::CREATE5) {
    if (std::filesystem::exists(m_filename)) {
      std::filesystem::remove(m_filename);
    }
  }
  m_allEntries = initAllEntries();
}

const std::string &NexusDescriptor::filename() const noexcept { return m_filename; }

bool NexusDescriptor::hasRootAttr(const std::string &name) const { return (m_rootAttrs.count(name) == 1); }

void NexusDescriptor::addRootAttr(const std::string &name) { m_rootAttrs.insert(name); }

void NexusDescriptor::addEntry(const std::string &entryName, const std::string &groupClass) {
  // simple checks
  if (entryName.empty())
    throw Exception("Cannot add empty path", "", m_filename);
  if (groupClass.empty())
    throw Exception("Cannot add empty class", "", m_filename);
  if (!entryName.starts_with("/"))
    throw Exception("Address must be absolute: " + entryName, "", m_filename);

  // do not add address twice
  if (this->isEntry(entryName))
    throw Exception("Cannot add an entry twice: " + entryName, "", m_filename);

  // verify the parent exists
  const auto lastPos = entryName.rfind("/");
  const auto parentAddress = entryName.substr(0, lastPos);
  if (parentAddress != "" && !this->isEntry(parentAddress))
    throw Exception("Parent address " + parentAddress + " does not exist", "", m_filename);

  // add the address
  m_allEntries[groupClass].insert(entryName);
}

// PRIVATE
std::map<std::string, std::set<std::string>> NexusDescriptor::initAllEntries() {

  std::map<std::string, std::set<std::string>> allEntries;

  // if the file exists read it
  if (std::filesystem::exists(m_filename)) {
    // if the file exists but cannot be opened, throw invalid
    // NOTE must be std::invalid_argument for expected errors to be raised in python API
    if (!H5::H5File::isAccessible(m_filename, Mantid::Nexus::H5Util::defaultFileAcc())) {
      throw std::invalid_argument("ERROR: Kernel::NexusDescriptor couldn't open hdf5 file " + m_filename + "\n");
    }

    H5::H5File fileID(m_filename, H5F_ACC_RDONLY, Mantid::Nexus::H5Util::defaultFileAcc());
    H5::Group groupID = fileID.openGroup("/");

    // get root attributes
    for (int i = 0; i < groupID.getNumAttrs(); ++i) {
      H5::Attribute attr = groupID.openAttribute(i);
      m_rootAttrs.insert(attr.getName());
    }

    // scan file recursively starting with root group "/"
    getGroup(groupID, allEntries, m_firstEntryNameType, 0);

    // handle going out of scope should automatically close
    fileID.close();
  } else {
    // if the file does not exist, then leave allEntries empty
  }

  // rely on move semantics
  return allEntries;
}

bool NexusDescriptor::isEntry(const std::string &entryName, const std::string &groupClass) const noexcept {

  auto itClass = m_allEntries.find(groupClass);
  if (itClass == m_allEntries.end()) {
    return false;
  }

  if (itClass->second.count(entryName) == 1) {
    return true;
  }

  return false;
}

bool NexusDescriptor::isEntry(const std::string &entryName) const noexcept {
  return std::any_of(m_allEntries.rbegin(), m_allEntries.rend(),
                     [&entryName](const auto &entry) { return entry.second.count(entryName) == 1; });
}

std::map<std::string, std::string> NexusDescriptor::allAddressesAtLevel(const std::string &level) const {
  std::map<std::string, std::string> result;
  for (auto itClass = m_allEntries.cbegin(); itClass != m_allEntries.cend(); itClass++) {
    for (auto itEntry = itClass->second.cbegin(); itEntry != itClass->second.cend(); itEntry++) {
      if (itEntry->size() <= level.size()) {
        continue;
      }
      if (itEntry->starts_with(level)) {
        int offset = (level == "/" ? 0 : 1);
        std::string address = itEntry->substr(level.size() + offset, itEntry->find("/", level.size() + offset));
        if (itEntry->ends_with(address)) {
          result[address] = itClass->first;
        }
      }
    }
  }
  return result;
}

bool NexusDescriptor::classTypeExists(const std::string &classType) const { return m_allEntries.contains(classType); }

std::string NexusDescriptor::classTypeForName(std::string const &entryName) const {
  std::string groupClass;
  auto it = m_allEntries.cbegin();
  for (; it != m_allEntries.cend(); it++) {
    if (it->second.count(entryName) == 1) {
      groupClass = it->first;
      break;
    }
  }
  if (it == m_allEntries.cend()) {
    throw Exception("Cannot find entry " + entryName, "classTypeForName", m_filename);
  }
  return groupClass;
}

} // namespace Mantid::Nexus
