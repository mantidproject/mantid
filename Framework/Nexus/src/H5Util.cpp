// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexus/H5Util.h"
#include "MantidAPI/LogManager.h"

#include <H5Cpp.h>

#include <algorithm>
#include <array>
#include <boost/numeric/conversion/cast.hpp>
#include <stdexcept>
#include <string>

using namespace H5;

namespace Mantid::NeXus::H5Util {

namespace {
/// static logger object
Mantid::Kernel::Logger g_log("H5Util");

const std::string NX_ATTR_CLASS("NX_class");
const std::string CAN_SAS_ATTR_CLASS("canSAS_class");
} // namespace

// -------------------------------------------------------------------
// convert primitives to HDF5 enum
// -------------------------------------------------------------------

template <typename NumT> DataType getType() { throw DataTypeIException(); }

template <> MANTID_NEXUS_DLL DataType getType<float>() { return PredType::NATIVE_FLOAT; }

template <> MANTID_NEXUS_DLL DataType getType<double>() { return PredType::NATIVE_DOUBLE; }

template <> MANTID_NEXUS_DLL DataType getType<int32_t>() { return PredType::NATIVE_INT32; }

template <> MANTID_NEXUS_DLL DataType getType<uint32_t>() { return PredType::NATIVE_UINT32; }

template <> MANTID_NEXUS_DLL DataType getType<int64_t>() { return PredType::NATIVE_INT64; }

template <> MANTID_NEXUS_DLL DataType getType<uint64_t>() { return PredType::NATIVE_UINT64; }

DataSpace getDataSpace(const size_t length) {
  hsize_t dims[] = {length};
  return DataSpace(1, dims);
}

template <typename NumT> DataSpace getDataSpace(const std::vector<NumT> &data) {
  return H5Util::getDataSpace(data.size());
}

namespace {

template <typename NumT> H5::DataSet writeScalarDataSet(Group &group, const std::string &name, const NumT &value) {
  static_assert(std::is_integral<NumT>::value || std::is_floating_point<NumT>::value,
                "The writeNumAttribute function only accepts integral of "
                "floating point values.");
  auto dataType = getType<NumT>();
  DataSpace dataSpace = getDataSpace(1);
  H5::DataSet data = group.createDataSet(name, dataType, dataSpace);
  data.write(&value, dataType);
  return data;
}

template <>
H5::DataSet writeScalarDataSet<std::string>(Group &group, const std::string &name, const std::string &value) {
  StrType dataType(0, value.length() + 1);
  DataSpace dataSpace = getDataSpace(1);
  H5::DataSet data = group.createDataSet(name, dataType, dataSpace);
  data.write(value, dataType);
  return data;
}

} // namespace

// -------------------------------------------------------------------
// write methods
// -------------------------------------------------------------------

Group createGroupNXS(H5File &file, const std::string &name, const std::string &nxtype) {
  auto group = file.createGroup(name);
  writeStrAttribute(group, NX_ATTR_CLASS, nxtype);
  return group;
}

Group createGroupNXS(Group &group, const std::string &name, const std::string &nxtype) {
  auto outGroup = group.createGroup(name);
  writeStrAttribute(outGroup, NX_ATTR_CLASS, nxtype);
  return outGroup;
}

Group createGroupCanSAS(H5File &file, const std::string &name, const std::string &nxtype, const std::string &cstype) {
  auto outGroup = createGroupNXS(file, name, nxtype);
  writeStrAttribute(outGroup, CAN_SAS_ATTR_CLASS, cstype);
  return outGroup;
}

Group createGroupCanSAS(Group &group, const std::string &name, const std::string &nxtype, const std::string &cstype) {
  auto outGroup = createGroupNXS(group, name, nxtype);
  writeStrAttribute(outGroup, CAN_SAS_ATTR_CLASS, cstype);
  return outGroup;
}

DSetCreatPropList setCompressionAttributes(const std::size_t length, const int deflateLevel) {
  DSetCreatPropList propList;
  hsize_t chunk_dims[1] = {length};
  propList.setChunk(1, chunk_dims);
  propList.setDeflate(deflateLevel);
  return propList;
}

void writeStrAttribute(const H5::H5Object &object, const std::string &name, const std::string &value) {
  StrType attrType(0, H5T_VARIABLE);
  DataSpace attrSpace(H5S_SCALAR);
  auto attribute = object.createAttribute(name, attrType, attrSpace);
  attribute.write(attrType, value);
}

template <typename NumT>
void writeNumAttribute(const H5::H5Object &object, const std::string &name, const NumT &value) {
  static_assert(std::is_integral<NumT>::value || std::is_floating_point<NumT>::value,
                "The writeNumAttribute function only accepts integral or "
                "floating point values.");
  auto attrType = getType<NumT>();
  DataSpace attrSpace(H5S_SCALAR);

  auto attribute = object.createAttribute(name, attrType, attrSpace);
  // Wrap the data set in an array
  std::array<NumT, 1> valueArray = {{value}};
  attribute.write(attrType, valueArray.data());
}

template <typename NumT>
void writeNumAttribute(const H5::H5Object &object, const std::string &name, const std::vector<NumT> &value) {
  static_assert(std::is_integral<NumT>::value || std::is_floating_point<NumT>::value,
                "The writeNumAttribute function only accepts integral of "
                "floating point values.");
  auto attrType = getType<NumT>();
  DataSpace attrSpace = getDataSpace(value);

  auto attribute = object.createAttribute(name, attrType, attrSpace);
  attribute.write(attrType, value.data());
}

void write(Group &group, const std::string &name, const std::string &value) { writeScalarDataSet(group, name, value); }

template <typename T>
void writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const T &value,
                                         const std::map<std::string, std::string> &attributes) {
  auto data = writeScalarDataSet(group, name, value);
  for (const auto &attribute : attributes) {
    writeStrAttribute(data, attribute.first, attribute.second);
  }
}

template <typename NumT> void writeArray1D(Group &group, const std::string &name, const std::vector<NumT> &values) {
  DataType dataType(getType<NumT>());
  DataSpace dataSpace = getDataSpace(values);

  DSetCreatPropList propList = setCompressionAttributes(values.size());

  auto data = group.createDataSet(name, dataType, dataSpace, propList);
  data.write(values.data(), dataType);
}

// -------------------------------------------------------------------
// read methods
// -------------------------------------------------------------------

std::string readString(H5::H5File &file, const std::string &path) {
  try {
    auto data = file.openDataSet(path);
    return readString(data);
  } catch (const H5::FileIException &e) {
    UNUSED_ARG(e);
    return "";
  } catch (const H5::GroupIException &e) {
    UNUSED_ARG(e);
    return "";
  }
}

std::string readString(H5::Group &group, const std::string &name) {
  try {
    auto data = group.openDataSet(name);
    return readString(data);
  } catch (const H5::GroupIException &e) {
    UNUSED_ARG(e);
    return "";
  }
}

std::string readString(const H5::DataSet &dataset) {
  std::string value;
  dataset.read(value, dataset.getDataType(), dataset.getSpace());

#if H5_VERSION_GE(1, 14, 0)
  // hdf5 >=1.14 puts the null terminator in the string
  // this strips that out
  if (const auto pos = value.rfind('\0'); pos != std::string::npos)
    value.erase(pos);
#endif

  return value;
}

/**
 * Returns 1D vector of variable length strings
 * @param group :: H5::Group already opened
 * @param name :: name of the dataset in the group (rank must be 1)
 * @return :: vector of strings
 */
std::vector<std::string> readStringVector(Group &group, const std::string &name) {
  hsize_t dims[1];
  char **rdata;
  std::vector<std::string> result;

  DataSet dataset = group.openDataSet(name);
  DataSpace dataspace = dataset.getSpace();
  DataType datatype = dataset.getDataType();

  dataspace.getSimpleExtentDims(dims, nullptr);

  rdata = new char *[dims[0]];
  dataset.read(rdata, datatype);

  for (size_t i = 0; i < dims[0]; ++i)
    result.emplace_back(std::string(rdata[i]));

  dataset.vlenReclaim(rdata, datatype, dataspace);
  dataset.close();
  delete[] rdata;

  return result;
}

bool hasAttribute(const H5::H5Object &object, const char *attributeName) {
  const htri_t exists = H5Aexists(object.getId(), attributeName);
  return exists > 0;
}

void readStringAttribute(const H5::H5Object &object, const std::string &attributeName, std::string &output) {
  const auto attribute = object.openAttribute(attributeName);
  attribute.read(attribute.getDataType(), output);
}

// This method avoids a copy on return so should be preferred to its sibling method
template <typename NumT>
void readArray1DCoerce(const H5::Group &group, const std::string &name, std::vector<NumT> &output) {
  try {
    DataSet dataset = group.openDataSet(name);
    readArray1DCoerce(dataset, output);
  } catch (const H5::GroupIException &e) {
    UNUSED_ARG(e);
    g_log.information("Failed to open dataset \"" + name + "\"\n");
  } catch (const H5::DataTypeIException &e) {
    UNUSED_ARG(e);
    g_log.information("DataSet \"" + name + "\" should be double" + "\n");
  }
}

template <typename NumT> std::vector<NumT> readArray1DCoerce(const H5::Group &group, const std::string &name) {
  std::vector<NumT> result;
  try {
    DataSet dataset = group.openDataSet(name);
    readArray1DCoerce(dataset, result);
  } catch (const H5::GroupIException &e) {
    UNUSED_ARG(e);
    g_log.information("Failed to open dataset \"" + name + "\"\n");
  } catch (const H5::DataTypeIException &e) {
    UNUSED_ARG(e);
    g_log.information("DataSet \"" + name + "\" should be double" + "\n");
  }

  return result;
}

namespace {
template <typename InputNumT, typename OutputNumT>
void convertingRead(const DataSet &dataset, const DataType &dataType, std::vector<OutputNumT> &output) {
  DataSpace dataSpace = dataset.getSpace();

  std::vector<InputNumT> temp(dataSpace.getSelectNpoints());
  dataset.read(temp.data(), dataType, dataSpace);

  output.resize(temp.size());

  std::transform(temp.begin(), temp.end(), output.begin(),
                 [](const InputNumT a) { // lambda
                   return boost::numeric_cast<OutputNumT>(a);
                 });
}

template <typename InputNumT, typename OutputNumT>
std::vector<OutputNumT> convertingNumArrayAttributeRead(Attribute &attribute, const DataType &dataType) {
  DataSpace dataSpace = attribute.getSpace();

  std::vector<InputNumT> temp(dataSpace.getSelectNpoints());
  attribute.read(dataType, temp.data());

  std::vector<OutputNumT> result;
  result.resize(temp.size());

  std::transform(temp.begin(), temp.end(), result.begin(),
                 [](const InputNumT a) { return boost::numeric_cast<OutputNumT>(a); });

  return result;
}

template <typename InputNumT, typename OutputNumT>
OutputNumT convertingRead(Attribute &attribute, const DataType &dataType) {
  InputNumT temp;
  attribute.read(dataType, &temp);
  auto result = boost::numeric_cast<OutputNumT>(temp);
  return result;
}

} // namespace

template <typename NumT> NumT readNumAttributeCoerce(const H5::H5Object &object, const std::string &attributeName) {
  auto attribute = object.openAttribute(attributeName);
  auto dataType = attribute.getDataType();

  NumT value;

  if (getType<NumT>() == dataType) {
    attribute.read(dataType, &value);
  } else if (PredType::NATIVE_INT32 == dataType) {
    value = convertingRead<int32_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_UINT32 == dataType) {
    value = convertingRead<uint32_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_INT64 == dataType) {
    value = convertingRead<int64_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_UINT64 == dataType) {
    value = convertingRead<uint64_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_FLOAT == dataType) {
    value = convertingRead<float, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_DOUBLE == dataType) {
    value = convertingRead<double, NumT>(attribute, dataType);
  } else {
    // not a supported type
    throw DataTypeIException();
  }
  return value;
}

template <typename NumT>
std::vector<NumT> readNumArrayAttributeCoerce(const H5::H5Object &object, const std::string &attributeName) {
  auto attribute = object.openAttribute(attributeName);
  auto dataType = attribute.getDataType();

  std::vector<NumT> value;

  if (getType<NumT>() == dataType) {
    DataSpace dataSpace = attribute.getSpace();
    value.resize(dataSpace.getSelectNpoints());
    attribute.read(dataType, value.data());
  } else if (PredType::NATIVE_INT32 == dataType) {
    value = convertingNumArrayAttributeRead<int32_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_UINT32 == dataType) {
    value = convertingNumArrayAttributeRead<uint32_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_INT64 == dataType) {
    value = convertingNumArrayAttributeRead<int64_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_UINT64 == dataType) {
    value = convertingNumArrayAttributeRead<uint64_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_FLOAT == dataType) {
    value = convertingNumArrayAttributeRead<float, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_DOUBLE == dataType) {
    value = convertingNumArrayAttributeRead<double, NumT>(attribute, dataType);
  } else {
    // not a supported type
    throw DataTypeIException();
  }
  return value;
}

template <typename NumT> void readArray1DCoerce(const DataSet &dataset, std::vector<NumT> &output) {
  DataType dataType = dataset.getDataType();

  if (getType<NumT>() == dataType) { // no conversion necessary
    DataSpace dataSpace = dataset.getSpace();
    output.resize(dataSpace.getSelectNpoints());
    dataset.read(output.data(), dataType, dataSpace);
  }

  if (PredType::NATIVE_INT32 == dataType) {
    convertingRead<int32_t>(dataset, dataType, output);
  } else if (PredType::NATIVE_UINT32 == dataType) {
    convertingRead<uint32_t>(dataset, dataType, output);
  } else if (PredType::NATIVE_INT64 == dataType) {
    convertingRead<int64_t>(dataset, dataType, output);
  } else if (PredType::NATIVE_UINT64 == dataType) {
    convertingRead<uint64_t>(dataset, dataType, output);
  } else if (PredType::NATIVE_FLOAT == dataType) {
    convertingRead<float>(dataset, dataType, output);
  } else if (PredType::NATIVE_DOUBLE == dataType) {
    convertingRead<double>(dataset, dataType, output);
  } else {
    // not a supported type
    throw DataTypeIException();
  }
}

/// Test if a group exists in an HDF5 file or parent group.
bool groupExists(H5::H5Object &h5, const std::string &groupPath) {
  bool status = true;
  // Unfortunately, this is actually the approach recommended by the HDF Group.
  try {
    h5.openGroup(groupPath);
  } catch (const H5::Exception &e) {
    UNUSED_ARG(e);
    status = false;
  }
  return status;
}

/// Test if an attribute is present on an HDF5 group or dataset and has a specific string value.
bool keyHasValue(H5::H5Object &h5, const std::string &key, const std::string &value) {
  bool status = true;
  try {
    Attribute attr = h5.openAttribute(key);
    std::string value_;
    attr.read(attr.getDataType(), value_);
    if (value_ != value)
      status = false;
  } catch (const H5::Exception &e) {
    UNUSED_ARG(e);
    status = false;
  }
  return status;
}

void copyGroup(H5::H5Object &dest, const std::string &destGroupPath, H5::H5Object &src,
               const std::string &srcGroupPath) {
  // Source group must exist, and destination group must not exist.
  if (!groupExists(src, srcGroupPath) || groupExists(dest, destGroupPath))
    throw std::invalid_argument(std::string("H5Util::copyGroup: source group '") + srcGroupPath + "' must exist and " +
                                "destination group '" + destGroupPath + "' must not exist.");

  // TODO: check that source file must have at least read access and destination file must have write access.

  // Note that in the HDF5 API:
  //   C++ API support for these HDF5 methods does not yet exist.

  // Create intermediate groups, if necessary
  hid_t lcpl = H5Pcreate(H5P_LINK_CREATE);
  if (H5Pset_create_intermediate_group(lcpl, 1) < 0)
    throw std::runtime_error("H5Util::copyGroup: 'H5Pset_create_intermediate_group' error return.");

  if (H5Ocopy(src.getId(), srcGroupPath.c_str(), dest.getId(), destGroupPath.c_str(), H5P_DEFAULT, lcpl) < 0)
    throw std::runtime_error("H5Util::copyGroup: 'H5Ocopy' error return.");
  H5Pclose(lcpl);
}

void deleteObjectLink(H5::H5Object &h5, const std::string &target) {
  // Note that in the HDF5 API:
  //   C++ API support for this HDF5 method does not yet exist.

  // Target object must exist
  if (H5Ldelete(h5.getId(), target.c_str(), H5P_DEFAULT) < 0)
    throw std::runtime_error("H5Util::deleteObjectLink: 'H5Ldelete' error return.");
}

// -------------------------------------------------------------------
// instantiations for writeNumAttribute
// -------------------------------------------------------------------

template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const float &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const double &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const int32_t &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const uint32_t &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const int64_t &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const uint64_t &value);

template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const std::vector<float> &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const std::vector<double> &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const std::vector<int32_t> &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const std::vector<uint32_t> &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const std::vector<int64_t> &value);
template MANTID_NEXUS_DLL void writeNumAttribute(const H5::H5Object &object, const std::string &name,
                                                 const std::vector<uint64_t> &value);

// -------------------------------------------------------------------
// instantiations for readNumAttributeCoerce
// -------------------------------------------------------------------
template MANTID_NEXUS_DLL float readNumAttributeCoerce(const H5::H5Object &object, const std::string &attributeName);
template MANTID_NEXUS_DLL double readNumAttributeCoerce(const H5::H5Object &object, const std::string &attributeName);
template MANTID_NEXUS_DLL int32_t readNumAttributeCoerce(const H5::H5Object &object, const std::string &attributeName);
template MANTID_NEXUS_DLL uint32_t readNumAttributeCoerce(const H5::H5Object &object, const std::string &attributeName);
template MANTID_NEXUS_DLL int64_t readNumAttributeCoerce(const H5::H5Object &object, const std::string &attributeName);
template MANTID_NEXUS_DLL uint64_t readNumAttributeCoerce(const H5::H5Object &object, const std::string &attributeName);

// -------------------------------------------------------------------
// instantiations for readNumArrayAttributeCoerce
// -------------------------------------------------------------------
template MANTID_NEXUS_DLL std::vector<float> readNumArrayAttributeCoerce(const H5::H5Object &object,
                                                                         const std::string &attributeName);
template MANTID_NEXUS_DLL std::vector<double> readNumArrayAttributeCoerce(const H5::H5Object &object,
                                                                          const std::string &attributeName);
template MANTID_NEXUS_DLL std::vector<int32_t> readNumArrayAttributeCoerce(const H5::H5Object &object,
                                                                           const std::string &attributeName);
template MANTID_NEXUS_DLL std::vector<uint32_t> readNumArrayAttributeCoerce(const H5::H5Object &object,
                                                                            const std::string &attributeName);
template MANTID_NEXUS_DLL std::vector<int64_t> readNumArrayAttributeCoerce(const H5::H5Object &object,
                                                                           const std::string &attributeName);
template MANTID_NEXUS_DLL std::vector<uint64_t> readNumArrayAttributeCoerce(const H5::H5Object &object,
                                                                            const std::string &attributeName);

// -------------------------------------------------------------------
// instantiations for writeArray1D
// -------------------------------------------------------------------
template MANTID_NEXUS_DLL void writeArray1D(H5::Group &group, const std::string &name,
                                            const std::vector<float> &values);
template MANTID_NEXUS_DLL void writeArray1D(H5::Group &group, const std::string &name,
                                            const std::vector<double> &values);
template MANTID_NEXUS_DLL void writeArray1D(H5::Group &group, const std::string &name,
                                            const std::vector<int32_t> &values);
template MANTID_NEXUS_DLL void writeArray1D(H5::Group &group, const std::string &name,
                                            const std::vector<uint32_t> &values);
template MANTID_NEXUS_DLL void writeArray1D(H5::Group &group, const std::string &name,
                                            const std::vector<int64_t> &values);
template MANTID_NEXUS_DLL void writeArray1D(H5::Group &group, const std::string &name,
                                            const std::vector<uint64_t> &values);

// -------------------------------------------------------------------
// Instantiations for writeScalarWithStrAttributes
// -------------------------------------------------------------------
template MANTID_NEXUS_DLL void
writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const std::string &value,
                                    const std::map<std::string, std::string> &attributes);
template MANTID_NEXUS_DLL void
writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const float &value,
                                    const std::map<std::string, std::string> &attributes);
template MANTID_NEXUS_DLL void
writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const double &value,
                                    const std::map<std::string, std::string> &attributes);
template MANTID_NEXUS_DLL void
writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const int32_t &value,
                                    const std::map<std::string, std::string> &attributes);
template MANTID_NEXUS_DLL void
writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const uint32_t &value,
                                    const std::map<std::string, std::string> &attributes);
template MANTID_NEXUS_DLL void
writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const int64_t &value,
                                    const std::map<std::string, std::string> &attributes);
template MANTID_NEXUS_DLL void
writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const uint64_t &value,
                                    const std::map<std::string, std::string> &attributes);

// -------------------------------------------------------------------
// instantiations for getDataSpace
// -------------------------------------------------------------------
template MANTID_NEXUS_DLL DataSpace getDataSpace(const std::vector<float> &data);
template MANTID_NEXUS_DLL DataSpace getDataSpace(const std::vector<double> &data);
template MANTID_NEXUS_DLL DataSpace getDataSpace(const std::vector<int32_t> &data);
template MANTID_NEXUS_DLL DataSpace getDataSpace(const std::vector<uint32_t> &data);
template MANTID_NEXUS_DLL DataSpace getDataSpace(const std::vector<int64_t> &data);
template MANTID_NEXUS_DLL DataSpace getDataSpace(const std::vector<uint64_t> &data);

// -------------------------------------------------------------------
// instantiations for readArray1DCoerce
// -------------------------------------------------------------------
template MANTID_NEXUS_DLL void readArray1DCoerce(const H5::Group &group, const std::string &name,
                                                 std::vector<float> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const H5::Group &group, const std::string &name,
                                                 std::vector<double> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const H5::Group &group, const std::string &name,
                                                 std::vector<int32_t> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const H5::Group &group, const std::string &name,
                                                 std::vector<uint32_t> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const H5::Group &group, const std::string &name,
                                                 std::vector<int64_t> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const H5::Group &group, const std::string &name,
                                                 std::vector<uint64_t> &output);

template MANTID_NEXUS_DLL std::vector<float> readArray1DCoerce(const H5::Group &group, const std::string &name);
template MANTID_NEXUS_DLL std::vector<double> readArray1DCoerce(const H5::Group &group, const std::string &name);
template MANTID_NEXUS_DLL std::vector<int32_t> readArray1DCoerce(const H5::Group &group, const std::string &name);
template MANTID_NEXUS_DLL std::vector<uint32_t> readArray1DCoerce(const H5::Group &group, const std::string &name);
template MANTID_NEXUS_DLL std::vector<int64_t> readArray1DCoerce(const H5::Group &group, const std::string &name);
template MANTID_NEXUS_DLL std::vector<uint64_t> readArray1DCoerce(const H5::Group &group, const std::string &name);

template MANTID_NEXUS_DLL void readArray1DCoerce(const DataSet &dataset, std::vector<float> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const DataSet &dataset, std::vector<double> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const DataSet &dataset, std::vector<int32_t> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const DataSet &dataset, std::vector<uint32_t> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const DataSet &dataset, std::vector<int64_t> &output);
template MANTID_NEXUS_DLL void readArray1DCoerce(const DataSet &dataset, std::vector<uint64_t> &output);
} // namespace Mantid::NeXus::H5Util
