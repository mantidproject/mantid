// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/NexusHDF5Descriptor.h"

#include <H5Cpp.h>
#include <boost/multi_index/detail/index_matcher.hpp>

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

/// Size of HDF magic number
const size_t HDFMagicSize = 4;
/// HDF cookie that is stored in the first 4 bytes of the file.
const unsigned char HDFMagic[4] = {'\016', '\003', '\023', '\001'}; // From HDF4::hfile.h

/// Size of HDF5 signature
const size_t HDF5SignatureSize = 8;
/// signature identifying a HDF5 file.
const unsigned char HDF5Signature[8] = {137, 'H', 'D', 'F', '\r', '\n', '\032', '\n'};
//---------------------------------------------------------------------------------------------------------------------------
// Anonymous helper methods to use isReadable methods to use an open file handle
//---------------------------------------------------------------------------------------------------------------------------

NexusHDF5Descriptor::Version HDFversion(FILE *fileHandle) {
  if (!fileHandle)
    throw std::invalid_argument("HierarchicalFileDescriptor::isHierarchical - Invalid file handle");

  // HDF4 check requires 4 bytes,  HDF5 check requires 8 bytes
  // Use same buffer and waste a few bytes if only checking HDF4
  unsigned char buffer[8] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  if (HDF5SignatureSize !=
      std::fread(static_cast<void *>(&buffer), sizeof(unsigned char), HDF5SignatureSize, fileHandle)) {
    throw std::runtime_error("Error while reading file");
  }

  NexusHDF5Descriptor::Version result = NexusHDF5Descriptor::None;

  // Number of bytes read doesn't matter as if it is not enough then the memory
  // simply won't match
  // as the buffer has been "zeroed"
  if (std::memcmp(&buffer, &HDF5Signature, HDF5SignatureSize) == 0)
    result = NexusHDF5Descriptor::Version5;
  else if (std::memcmp(&buffer, &HDFMagic, HDFMagicSize) == 0)
    result = NexusHDF5Descriptor::Version4;

  // Return file stream to start of file
  std::rewind(fileHandle);
  return result;
}

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

//---------------------------------------------------------------------------------------------------------------------------
// static NexusHDF5Descriptor methods
//---------------------------------------------------------------------------------------------------------------------------

/**
 * Checks for the HDF signatures and returns true if one of them is found
 * @param filename A string filename to check
 * @param version One of the NexusHDF5Descriptor::Version enumerations specifying
 * the required version
 * @return True if the file is considered hierarchical, false otherwise
 */
bool NexusHDF5Descriptor::isReadable(const std::string &filename, const NexusHDF5Descriptor::Version version) {
  return getHDFVersion(filename) == version;
}

NexusHDF5Descriptor::Version NexusHDF5Descriptor::getHDFVersion(const std::string &filename) {
  FILE *fd = fopen(filename.c_str(), "rb");
  if (!fd) {
    throw std::invalid_argument("HierarchicalFileDescriptor::isHierarchical - Unable to open file '" + filename + "'");
  }
  const NexusHDF5Descriptor::Version result = HDFversion(fd); // use anonymous helper
  fclose(fd);
  return result;
}

NexusHDF5Descriptor::NexusHDF5Descriptor(std::string filename)
    : m_filename(std::move(filename)), m_extension(), m_firstEntryNameType(), m_allEntries(initAllEntries()) {}

// PUBLIC
const std::string &NexusHDF5Descriptor::filename() const noexcept { return m_filename; }

bool NexusHDF5Descriptor::hasRootAttr(const std::string &name) const { return (m_rootAttrs.count(name) == 1); }

const std::map<std::string, std::set<std::string>> &NexusHDF5Descriptor::getAllEntries() const noexcept {
  return m_allEntries;
}

// PRIVATE
std::map<std::string, std::set<std::string>> NexusHDF5Descriptor::initAllEntries() {
  m_extension = std::filesystem::path(m_filename).extension().string();
  H5::FileAccPropList access_plist;
  access_plist.setFcloseDegree(H5F_CLOSE_STRONG);

  std::map<std::string, std::set<std::string>> allEntries;

  H5::H5File fileID;
  try {
    fileID = H5::H5File(m_filename, H5F_ACC_RDONLY, H5::FileCreatPropList::DEFAULT, access_plist);
  } catch (const H5::FileIException &) {
    return allEntries;
  }

  H5::Group groupID = fileID.openGroup("/");

  // get root attributes
  for (int i = 0; i < groupID.getNumAttrs(); ++i) {
    H5::Attribute attr = groupID.openAttribute(i);
    m_rootAttrs.insert(attr.getName());
  }

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

std::vector<std::string> NexusHDF5Descriptor::allPathsOfType(const std::string &type) const {
  std::vector<std::string> result;
  if (auto itClass = m_allEntries.find(type); itClass != m_allEntries.end()) {
    result.assign(itClass->second.begin(), itClass->second.end());
  }

  return result;
}

bool NexusHDF5Descriptor::classTypeExists(const std::string &classType) const {
  return m_allEntries.contains(classType);
}

} // namespace Mantid::Kernel
