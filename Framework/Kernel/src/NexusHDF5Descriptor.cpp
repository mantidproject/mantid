// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/NexusHDF5Descriptor.h"

#include <boost/multi_index/detail/index_matcher.hpp>

#include "MantidKernel/NexusDescriptor.h"

#include <hdf5.h>

#include <cstdlib>   // malloc, calloc
#include <cstring>   // strcpy
#include <stdexcept> // std::invalid_argument
#include <utility>

using boost::multi_index::detail::index_matcher::entry;

namespace Mantid::Kernel {

/// hdf5 specific functions, stay in anonymous namespace to make hdf5 linking
/// PRIVATE
namespace {

/**
 * populate a string attribute from HDF5 attribute handler
 * @param attr input HDF5 atttribute handler
 * @param data
 * @return
 */
herr_t readStringAttribute(hid_t attr, char **data) {
  herr_t iRet = 0;
  hsize_t thedims[H5S_MAX_RANK];

  hid_t atype = H5Aget_type(attr);
  hsize_t sdim = H5Tget_size(atype);
  hid_t space = H5Aget_space(attr);
  int ndims = H5Sget_simple_extent_dims(space, thedims, NULL);

  if (ndims == 0) {
    if (H5Tis_variable_str(atype)) {
      hid_t btype = H5Tget_native_type(atype, H5T_DIR_ASCEND);
      iRet = H5Aread(attr, btype, data);
      H5Tclose(btype);
    } else {
      *data = (char *)malloc(sdim + 1);
      iRet = H5Aread(attr, atype, *data);
      (*data)[sdim] = '\0';
    }
  } else if (ndims == 1) {
    unsigned int i;
    char **strings;

    strings = (char **)malloc(thedims[0] * sizeof(char *));

    if (!H5Tis_variable_str(atype)) {
      strings[0] = (char *)malloc(thedims[0] * sdim * sizeof(char));
      for (i = 1; i < thedims[0]; i++) {
        strings[i] = strings[0] + i * sdim;
      }
    }

    iRet = H5Aread(attr, atype, strings[0]);
    *data = (char *)calloc((sdim + 2) * thedims[0], sizeof(char));
    for (i = 0; i < thedims[0]; i++) {
      if (i == 0) {
        strncpy(*data, strings[i], sdim);
      } else {
        strcat(*data, ", ");
        strncat(*data, strings[i], sdim);
      }
    }
    if (H5Tis_variable_str(atype)) {
      H5Dvlen_reclaim(atype, space, H5P_DEFAULT, strings);
    } else {
      free(strings[0]);
    }

    free(strings);
  } else {
    *data = (char *)malloc(33);
    strcpy(*data, " higher dimensional string array\0");
  }

  H5Tclose(atype);
  H5Sclose(space);
  if (iRet < 0)
    return -1;
  return 0;
}

/**
 * Reads a string attribute of N-dimensions
 * @param attr input HDF5 attribute handler
 * @return
 */
std::pair<std::string, herr_t> readStringAttributeN(hid_t attr) {
  char *vdat = NULL;
  const auto iRet = readStringAttribute(attr, &vdat);
  std::string attrData;
  if (iRet >= 0) {
    attrData = vdat;
  }
  free(vdat);
  return std::make_pair(attrData, iRet);
}

void getGroup(hid_t groupID, std::map<std::string, std::set<std::string>> &allEntries) {

  /**
   * Return the NX_class attribute associate with objectName group entry
   */
  auto lf_getNxClassAttribute = [&](hid_t groupID, const char *objectName) -> std::string {
    std::string attribute = "";
    hid_t attributeID = H5Aopen_by_name(groupID, objectName, "NX_class", H5P_DEFAULT, H5P_DEFAULT);
    if (attributeID < 0) {
      H5Aclose(attributeID);
      return attribute;
    }

    auto pairData = readStringAttributeN(attributeID);
    // already null terminated in readStringAttributeN
    attribute = pairData.first;
    H5Aclose(attributeID);

    return attribute;
  };

  // using HDF5 C API
  constexpr std::size_t maxLength = 1024;
  char groupName[maxLength];
  char memberName[maxLength];
  std::size_t groupNameLength = static_cast<std::size_t>(H5Iget_name(groupID, groupName, maxLength));
  hsize_t nObjects = 0;
  H5Gget_num_objs(groupID, &nObjects);

  const std::string groupNameStr(groupName, groupNameLength);
  const std::string nxClass = (groupNameStr == "/") ? "" : lf_getNxClassAttribute(groupID, groupNameStr.c_str());

  if (!nxClass.empty()) {
    allEntries[nxClass].insert(groupNameStr);
  }

  for (unsigned int i = 0; i < nObjects; ++i) {

    const int type = H5Gget_objtype_by_idx(groupID, static_cast<size_t>(i));
    const std::size_t memberNameLength =
        static_cast<std::size_t>(H5Gget_objname_by_idx(groupID, static_cast<hsize_t>(i), memberName, maxLength));

    if (type == H5O_TYPE_GROUP) {
      hid_t subGroupID = H5Gopen2(groupID, memberName, H5P_DEFAULT);
      getGroup(subGroupID, allEntries);
      H5Gclose(subGroupID);

    } else if (type == H5O_TYPE_DATASET) {
      const std::string memberNameStr(memberName, memberNameLength);
      const std::string absoluteEntryName = groupNameStr + "/" + memberNameStr;
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
    : m_filename(std::move(filename)), m_allEntries(initAllEntries()) {}

// PUBLIC
std::string NexusHDF5Descriptor::getFilename() const noexcept { return m_filename; }

const std::map<std::string, std::set<std::string>> &NexusHDF5Descriptor::getAllEntries() const noexcept {
  return m_allEntries;
}

// PRIVATE
std::map<std::string, std::set<std::string>> NexusHDF5Descriptor::initAllEntries() {

  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);

  hid_t fileID = H5Fopen(m_filename.c_str(), H5F_ACC_RDONLY, fapl);
  if (fileID < 0) {

    throw std::invalid_argument("ERROR: Kernel::NexusHDF5Descriptor couldn't open hdf5 file " + m_filename +
                                " with fapl " + std::to_string(fapl) + "\n");
  }

  hid_t groupID = H5Gopen2(fileID, "/", H5P_DEFAULT);

  std::map<std::string, std::set<std::string>> allEntries;
  // scan file recursively starting with root group "/"
  getGroup(groupID, allEntries);
  H5Gclose(groupID);
  H5Fclose(fileID);

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

} // namespace Mantid::Kernel
