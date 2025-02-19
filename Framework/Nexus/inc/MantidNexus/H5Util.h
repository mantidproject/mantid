// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/DllConfig.h"

#include <limits>
#include <map>
#include <string>
#include <vector>

// forward declarations
namespace H5 {
class DataSpace;
class DataSet;
class DSetCreatPropList;
class DataType;
class FileAccPropList;
class Group;
class H5File;
class H5Object;
} // namespace H5

namespace Mantid {
namespace NeXus {
namespace H5Util {
/** H5Util : TODO: DESCRIPTION
 */

/** Default file access is H5F_CLOSE_STRONG. This should be set consistently for all access of a file. */
MANTID_NEXUS_DLL H5::FileAccPropList defaultFileAcc();

/// Create a 1D data-space to hold data of length.
MANTID_NEXUS_DLL H5::DataSpace getDataSpace(const size_t length);

/// Create a 1D data-space that will hold the supplied vector.
template <typename NumT> H5::DataSpace getDataSpace(const std::vector<NumT> &data);

/// Convert a primitive type to the appropriate H5::DataType.
template <typename NumT> H5::DataType getType();

MANTID_NEXUS_DLL H5::Group createGroupNXS(H5::H5File &file, const std::string &name, const std::string &nxtype);

MANTID_NEXUS_DLL H5::Group createGroupNXS(H5::Group &group, const std::string &name, const std::string &nxtype);

MANTID_NEXUS_DLL H5::Group createGroupCanSAS(H5::Group &group, const std::string &name, const std::string &nxtype,
                                             const std::string &cstype);

MANTID_NEXUS_DLL H5::Group createGroupCanSAS(H5::H5File &file, const std::string &name, const std::string &nxtype,
                                             const std::string &cstype);

/**
 * Sets up the chunking and compression rate.
 * @param length
 * @param deflateLevel
 * @return The configured property list
 */
MANTID_NEXUS_DLL H5::DSetCreatPropList setCompressionAttributes(const std::size_t length, const int deflateLevel = 6);

MANTID_NEXUS_DLL void writeStrAttribute(const H5::H5Object &object, const std::string &name, const std::string &value);

template <typename NumT> void writeNumAttribute(const H5::H5Object &object, const std::string &name, const NumT &value);

template <typename NumT>
void writeNumAttribute(const H5::H5Object &object, const std::string &name, const std::vector<NumT> &value);

MANTID_NEXUS_DLL void write(H5::Group &group, const std::string &name, const std::string &value);

template <typename T>
void writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const T &value,
                                         const std::map<std::string, std::string> &attributes);

template <typename NumT> void writeArray1D(H5::Group &group, const std::string &name, const std::vector<NumT> &values);

MANTID_NEXUS_DLL std::string readString(H5::H5File &file, const std::string &path);

MANTID_NEXUS_DLL std::string readString(H5::Group &group, const std::string &name);

MANTID_NEXUS_DLL std::string readString(const H5::DataSet &dataset);

MANTID_NEXUS_DLL std::vector<std::string> readStringVector(H5::Group &, const std::string &);

MANTID_NEXUS_DLL bool hasAttribute(const H5::H5Object &object, const char *attributeName);

MANTID_NEXUS_DLL void readStringAttribute(const H5::H5Object &object, const std::string &attributeName,
                                          std::string &output);

template <typename NumT> NumT readNumAttributeCoerce(const H5::H5Object &object, const std::string &attributeName);

template <typename NumT>
std::vector<NumT> readNumArrayAttributeCoerce(const H5::H5Object &object, const std::string &attributeName);

template <typename NumT>
void readArray1DCoerce(const H5::Group &group, const std::string &name, std::vector<NumT> &output);

template <typename NumT> std::vector<NumT> readArray1DCoerce(const H5::Group &group, const std::string &name);

template <typename NumT>
void readArray1DCoerce(const H5::DataSet &dataset, std::vector<NumT> &output,
                       const size_t length = std::numeric_limits<size_t>::max(),
                       const size_t offset = static_cast<size_t>(0));

/// Test if a group already exists within an HDF5 file or parent group.
MANTID_NEXUS_DLL bool groupExists(H5::H5Object &h5, const std::string &groupPath);

/// Test if an attribute is present and has a specific string value for an HDF5 group or dataset.
MANTID_NEXUS_DLL bool keyHasValue(H5::H5Object &h5, const std::string &key, const std::string &value);

/// Copy a group and all of its contents, between the same or different HDF5 files or groups.
MANTID_NEXUS_DLL void copyGroup(H5::H5Object &dest, const std::string &destGroupPath, H5::H5Object &src,
                                const std::string &srcGroupPath);

/**
 * Delete a target link for a group or dataset from a parent group.
 * If this is the last link to the target in the HDF5 graph, then it will be removed from the file.
 */
MANTID_NEXUS_DLL void deleteObjectLink(H5::H5Object &h5, const std::string &target);

} // namespace H5Util
} // namespace NeXus
} // namespace Mantid
