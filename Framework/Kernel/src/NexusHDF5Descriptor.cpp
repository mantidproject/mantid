// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/NexusHDF5Descriptor.h"

#include <boost/multi_index/detail/index_matcher.hpp>

#include "MantidKernel/NexusDescriptor.h"
#include <H5Cpp.h>

#include <cstdlib> // malloc, calloc
#include <cstring> // strcpy
#include <filesystem>
#include <stdexcept> // std::invalid_argument
#include <utility>

using boost::multi_index::detail::index_matcher::entry;

namespace Mantid::Kernel {

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

bool NexusHDF5Descriptor::isReadable(const std::string &filename) {
  // use existing function to do the work
  return NexusDescriptor::isReadable(filename, NexusDescriptor::Version::Version5);
}

NexusHDF5Descriptor::NexusHDF5Descriptor(std::string filename)
    : m_filename(std::move(filename)), m_extension(std::filesystem::path(filename).extension().string()),
      m_firstEntryNameType(), m_allEntries(initAllEntries()) {}

// PUBLIC
const std::string &NexusHDF5Descriptor::filename() const noexcept { return m_filename; }

const std::map<std::string, std::set<std::string>> &NexusHDF5Descriptor::getAllEntries() const noexcept {
  return m_allEntries;
}

// PRIVATE
std::map<std::string, std::set<std::string>> NexusHDF5Descriptor::initAllEntries() {
  H5::FileAccPropList access_plist;
  access_plist.setFcloseDegree(H5F_CLOSE_STRONG);

  H5::H5File fileID;
  try {
    fileID = H5::H5File(m_filename, H5F_ACC_RDONLY, H5::FileCreatPropList::DEFAULT, access_plist);
  } catch (const H5::FileIException &) {
    throw std::invalid_argument("ERROR: Kernel::NexusHDF5Descriptor couldn't open hdf5 file " + m_filename + "\n");
  }

  H5::Group groupID = fileID.openGroup("/");

  std::map<std::string, std::set<std::string>> allEntries;
  // scan file recursively starting with root group "/"
  getGroup(groupID, allEntries, m_firstEntryNameType, 0);

  // rely on move semantics
  return allEntries;
}

bool NexusHDF5Descriptor::isEntry(const std::string &entryName, const std::string &groupClass) const noexcept {

  auto itClass = m_allEntries.find(groupClass);
  if (itClass == m_allEntries.end()) {
    return false;
  }

  if (itClass->second.count(entryName) == 1) {
    return true;
  }

  return false;
}

bool NexusHDF5Descriptor::isEntry(const std::string &entryName) const noexcept {
  return std::any_of(m_allEntries.rbegin(), m_allEntries.rend(),
                     [&entryName](const auto &entry) { return entry.second.count(entryName) == 1; });
}

bool NexusHDF5Descriptor::classTypeExists(const std::string &classType) const {
  return m_allEntries.contains(classType);
}

} // namespace Mantid::Kernel
