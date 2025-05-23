// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexus/NexusDescriptor.h"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NeXusException.hpp"

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
    std::string attribute = "";

    if (groupID.attrExists("NX_class")) {
      const auto attributeID = groupID.openAttribute("NX_class");
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
      const std::string absoluteEntryName = groupNameStr + "/" + memberName;
      allEntries["SDS"].insert(absoluteEntryName);
    }
  }
}
} // namespace

// PUBLIC

NexusDescriptor::NexusDescriptor(std::string filename)
    : m_filename(std::move(filename)), m_extension(std::filesystem::path(m_filename).extension().string()),
      m_firstEntryNameType(), m_allEntries(initAllEntries()) {}

const std::string &NexusDescriptor::filename() const noexcept { return m_filename; }

bool NexusDescriptor::hasRootAttr(const std::string &name) const { return (m_rootAttrs.count(name) == 1); }

const std::map<std::string, std::set<std::string>> &NexusDescriptor::getAllEntries() const noexcept {
  return m_allEntries;
}

void NexusDescriptor::addRootAttr(const std::string &name) { m_rootAttrs.insert(name); }

void NexusDescriptor::addEntry(const std::string &entryName, const std::string &groupClass) {
  // simple checks
  if (entryName.empty())
    throw ::NeXus::Exception("Cannot add empty path", "", m_filename);
  if (groupClass.empty())
    throw ::NeXus::Exception("Cannot add empty class", "", m_filename);
  if (!entryName.starts_with("/"))
    throw ::NeXus::Exception("Paths must be absolute: " + entryName, "", m_filename);

  // do not add path twice
  if (this->isEntry(entryName))
    throw ::NeXus::Exception("Cannot add an entry twice: " + entryName, "", m_filename);

  // verify the parent exists
  const auto lastPos = entryName.rfind("/");
  const auto parentPath = entryName.substr(0, lastPos);
  if (parentPath != "" && !this->isEntry(parentPath))
    throw ::NeXus::Exception("Parent path " + parentPath + " does not exist", "", m_filename);

  // add the path
  m_allEntries[groupClass].insert(entryName);
}

// PRIVATE
std::map<std::string, std::set<std::string>> NexusDescriptor::initAllEntries() {
  H5::FileAccPropList access_plist;
  access_plist.setFcloseDegree(H5F_CLOSE_STRONG);

  std::map<std::string, std::set<std::string>> allEntries;

  // if the file exists read it
  if (std::filesystem::exists(m_filename)) {
    if (!H5::H5File::isAccessible(m_filename, access_plist)) {
      throw std::runtime_error("REALLY BAD");
    }

    H5::H5File fileID(m_filename, H5F_ACC_RDONLY, H5::FileCreatPropList::DEFAULT, access_plist);
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

std::vector<std::string> NexusDescriptor::allPathsOfType(const std::string &type) const {
  std::vector<std::string> result;
  if (auto itClass = m_allEntries.find(type); itClass != m_allEntries.end()) {
    result.assign(itClass->second.begin(), itClass->second.end());
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
    throw ::NeXus::Exception("Cannot find entry " + entryName, "classTypeForName", m_filename);
  }
  return groupClass;
}

} // namespace Mantid::Nexus
