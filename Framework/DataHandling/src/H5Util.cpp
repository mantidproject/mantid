#include "MantidDataHandling/H5Util.h"
#include "MantidAPI/LogManager.h"
#include "MantidKernel/System.h"

#include <H5Cpp.h>
#include <algorithm>
#include <array>
#include <boost/numeric/conversion/cast.hpp>
#include <iostream>

using namespace H5;

namespace Mantid {
namespace DataHandling {
namespace H5Util {

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

template <> MANTID_DATAHANDLING_DLL DataType getType<float>() {
  return PredType::NATIVE_FLOAT;
}

template <> MANTID_DATAHANDLING_DLL DataType getType<double>() {
  return PredType::NATIVE_DOUBLE;
}

template <> MANTID_DATAHANDLING_DLL DataType getType<int32_t>() {
  return PredType::NATIVE_INT32;
}

template <> MANTID_DATAHANDLING_DLL DataType getType<uint32_t>() {
  return PredType::NATIVE_UINT32;
}

template <> MANTID_DATAHANDLING_DLL DataType getType<int64_t>() {
  return PredType::NATIVE_INT64;
}

template <> MANTID_DATAHANDLING_DLL DataType getType<uint64_t>() {
  return PredType::NATIVE_UINT64;
}

DataSpace getDataSpace(const size_t length) {
  hsize_t dims[] = {length};
  return DataSpace(1, dims);
}

template <typename NumT> DataSpace getDataSpace(const std::vector<NumT> &data) {
  return H5Util::getDataSpace(data.size());
}

namespace {

template <typename NumT>
H5::DataSet writeScalarDataSet(Group &group, const std::string &name,
                               const NumT &value) {
  static_assert(std::is_integral<NumT>::value ||
                    std::is_floating_point<NumT>::value,
                "The writeNumAttribute function only accepts integral of "
                "floating point values.");
  auto dataType = getType<NumT>();
  DataSpace dataSpace = getDataSpace(1);
  H5::DataSet data = group.createDataSet(name, dataType, dataSpace);
  data.write(&value, dataType);
  return data;
}

template <>
H5::DataSet writeScalarDataSet<std::string>(Group &group,
                                            const std::string &name,
                                            const std::string &value) {
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

Group createGroupNXS(H5File &file, const std::string &name,
                     const std::string &nxtype) {
  auto group = file.createGroup(name);
  writeStrAttribute(group, NX_ATTR_CLASS, nxtype);
  return group;
}

Group createGroupNXS(Group &group, const std::string &name,
                     const std::string &nxtype) {
  auto outGroup = group.createGroup(name);
  writeStrAttribute(outGroup, NX_ATTR_CLASS, nxtype);
  return outGroup;
}

Group createGroupCanSAS(H5File &file, const std::string &name,
                        const std::string &nxtype, const std::string &cstype) {
  auto outGroup = createGroupNXS(file, name, nxtype);
  writeStrAttribute(outGroup, CAN_SAS_ATTR_CLASS, cstype);
  return outGroup;
}

Group createGroupCanSAS(Group &group, const std::string &name,
                        const std::string &nxtype, const std::string &cstype) {
  auto outGroup = createGroupNXS(group, name, nxtype);
  writeStrAttribute(outGroup, CAN_SAS_ATTR_CLASS, cstype);
  return outGroup;
}

DSetCreatPropList setCompressionAttributes(const std::size_t length,
                                           const int deflateLevel) {
  DSetCreatPropList propList;
  hsize_t chunk_dims[1] = {length};
  propList.setChunk(1, chunk_dims);
  propList.setDeflate(deflateLevel);
  return propList;
}

template <typename LocationType>
void writeStrAttribute(LocationType &location, const std::string &name,
                       const std::string &value) {
  StrType attrType(0, H5T_VARIABLE);
  DataSpace attrSpace(H5S_SCALAR);
  auto attribute = location.createAttribute(name, attrType, attrSpace);
  attribute.write(attrType, value);
}

template <typename NumT, typename LocationType>
void writeNumAttribute(LocationType &location, const std::string &name,
                       const NumT &value) {
  static_assert(std::is_integral<NumT>::value ||
                    std::is_floating_point<NumT>::value,
                "The writeNumAttribute function only accepts integral or "
                "floating point values.");
  auto attrType = getType<NumT>();
  DataSpace attrSpace(H5S_SCALAR);

  auto attribute = location.createAttribute(name, attrType, attrSpace);
  // Wrap the data set in an array
  std::array<NumT, 1> valueArray = {{value}};
  attribute.write(attrType, valueArray.data());
}

template <typename NumT, typename LocationType>
void writeNumAttribute(LocationType &location, const std::string &name,
                       const std::vector<NumT> &value) {
  static_assert(std::is_integral<NumT>::value ||
                    std::is_floating_point<NumT>::value,
                "The writeNumAttribute function only accepts integral of "
                "floating point values.");
  auto attrType = getType<NumT>();
  DataSpace attrSpace = getDataSpace(value);

  auto attribute = location.createAttribute(name, attrType, attrSpace);
  attribute.write(attrType, value.data());
}

void write(Group &group, const std::string &name, const std::string &value) {
  writeScalarDataSet(group, name, value);
}

template <typename T>
void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const T &value,
    const std::map<std::string, std::string> &attributes) {
  auto data = writeScalarDataSet(group, name, value);
  for (const auto &attribute : attributes) {
    writeStrAttribute(data, attribute.first, attribute.second);
  }
}

template <typename NumT>
void writeArray1D(Group &group, const std::string &name,
                  const std::vector<NumT> &values) {
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
  } catch (H5::FileIException &e) {
    UNUSED_ARG(e);
    return "";
  } catch (H5::GroupIException &e) {
    UNUSED_ARG(e);
    return "";
  }
}

std::string readString(H5::Group &group, const std::string &name) {
  try {
    auto data = group.openDataSet(name);
    return readString(data);
  } catch (H5::GroupIException &e) {
    UNUSED_ARG(e);
    return "";
  }
}

std::string readString(H5::DataSet &dataset) {
  std::string value;
  dataset.read(value, dataset.getDataType(), dataset.getSpace());
  return value;
}

/**
 * Returns 1D vector of variable length strings
 * @param group :: H5::Group already opened
 * @param name :: name of the dataset in the group (rank must be 1)
 * @return :: vector of strings
 */
std::vector<std::string> readStringVector(Group &group,
                                          const std::string &name) {
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

template <typename LocationType>
std::string readAttributeAsString(LocationType &location,
                                  const std::string &attributeName) {
  auto attribute = location.openAttribute(attributeName);
  std::string value;
  attribute.read(attribute.getDataType(), value);
  return value;
}

template <typename NumT>
std::vector<NumT> readArray1DCoerce(H5::Group &group, const std::string &name) {
  std::vector<NumT> result;

  try {
    DataSet dataset = group.openDataSet(name);
    result = readArray1DCoerce<NumT>(dataset);
  } catch (H5::GroupIException &e) {
    UNUSED_ARG(e);
    g_log.information() << "Failed to open dataset \"" << name << "\"\n";
  } catch (H5::DataTypeIException &e) {
    UNUSED_ARG(e);
    g_log.information() << "DataSet \"" << name << "\" should be double"
                        << "\n";
  }

  return result;
}

namespace {
template <typename InputNumT, typename OutputNumT>
std::vector<OutputNumT> convertingRead(DataSet &dataset,
                                       const DataType &dataType) {
  DataSpace dataSpace = dataset.getSpace();

  std::vector<InputNumT> temp(dataSpace.getSelectNpoints());
  dataset.read(temp.data(), dataType, dataSpace);

  std::vector<OutputNumT> result;
  result.resize(temp.size());

  std::transform(temp.begin(), temp.end(), result.begin(),
                 [](const InputNumT a) { // lambda
                   return boost::numeric_cast<OutputNumT>(a);
                 });

  return result;
}

template <typename InputNumT, typename OutputNumT>
std::vector<OutputNumT>
convertingNumArrayAttributeRead(Attribute &attribute,
                                const DataType &dataType) {
  DataSpace dataSpace = attribute.getSpace();

  std::vector<InputNumT> temp(dataSpace.getSelectNpoints());
  attribute.read(dataType, temp.data());

  std::vector<OutputNumT> result;
  result.resize(temp.size());

  std::transform(
      temp.begin(), temp.end(), result.begin(),
      [](const InputNumT a) { return boost::numeric_cast<OutputNumT>(a); });

  return result;
}

template <typename InputNumT, typename OutputNumT>
OutputNumT convertingRead(Attribute &attribute, const DataType &dataType) {
  InputNumT temp;
  attribute.read(dataType, &temp);
  OutputNumT result = boost::numeric_cast<OutputNumT>(temp);
  return result;
}

} // namespace

template <typename NumT, typename LocationType>
NumT readNumAttributeCoerce(LocationType &location,
                            const std::string &attributeName) {
  auto attribute = location.openAttribute(attributeName);
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

template <typename NumT, typename LocationType>
std::vector<NumT>
readNumArrayAttributeCoerce(LocationType &location,
                            const std::string &attributeName) {
  auto attribute = location.openAttribute(attributeName);
  auto dataType = attribute.getDataType();

  std::vector<NumT> value;

  if (getType<NumT>() == dataType) {
    DataSpace dataSpace = attribute.getSpace();
    value.resize(dataSpace.getSelectNpoints());
    attribute.read(dataType, value.data());
  } else if (PredType::NATIVE_INT32 == dataType) {
    value = convertingNumArrayAttributeRead<int32_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_UINT32 == dataType) {
    value =
        convertingNumArrayAttributeRead<uint32_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_INT64 == dataType) {
    value = convertingNumArrayAttributeRead<int64_t, NumT>(attribute, dataType);
  } else if (PredType::NATIVE_UINT64 == dataType) {
    value =
        convertingNumArrayAttributeRead<uint64_t, NumT>(attribute, dataType);
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

template <typename NumT> std::vector<NumT> readArray1DCoerce(DataSet &dataset) {
  DataType dataType = dataset.getDataType();

  if (getType<NumT>() == dataType) { // no conversion necessary
    std::vector<NumT> result;
    DataSpace dataSpace = dataset.getSpace();
    result.resize(dataSpace.getSelectNpoints());
    dataset.read(result.data(), dataType, dataSpace);
    return result;
  }

  if (PredType::NATIVE_INT32 == dataType) {
    return convertingRead<int32_t, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_UINT32 == dataType) {
    return convertingRead<uint32_t, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_INT64 == dataType) {
    return convertingRead<int64_t, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_UINT64 == dataType) {
    return convertingRead<uint64_t, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_FLOAT == dataType) {
    return convertingRead<float, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_DOUBLE == dataType) {
    return convertingRead<double, NumT>(dataset, dataType);
  }

  // not a supported type
  throw DataTypeIException();
}

// -------------------------------------------------------------------
// instantiations for writeStrAttribute
// -------------------------------------------------------------------
template MANTID_DATAHANDLING_DLL void
writeStrAttribute(H5::Group &location, const std::string &name,
                  const std::string &value);

template MANTID_DATAHANDLING_DLL void
writeStrAttribute(H5::DataSet &location, const std::string &name,
                  const std::string &value);

// -------------------------------------------------------------------
// instantiations for writeNumAttribute
// -------------------------------------------------------------------

template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::Group &location,
                                                        const std::string &name,
                                                        const float &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::DataSet &location,
                                                        const std::string &name,
                                                        const float &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::Group &location,
                                                        const std::string &name,
                                                        const double &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::DataSet &location,
                                                        const std::string &name,
                                                        const double &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::Group &location,
                                                        const std::string &name,
                                                        const int32_t &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::DataSet &location,
                                                        const std::string &name,
                                                        const int32_t &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::Group &location,
                                                        const std::string &name,
                                                        const uint32_t &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::DataSet &location,
                                                        const std::string &name,
                                                        const uint32_t &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::Group &location,
                                                        const std::string &name,
                                                        const int64_t &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::DataSet &location,
                                                        const std::string &name,
                                                        const int64_t &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::Group &location,
                                                        const std::string &name,
                                                        const uint64_t &value);
template MANTID_DATAHANDLING_DLL void writeNumAttribute(H5::DataSet &location,
                                                        const std::string &name,
                                                        const uint64_t &value);

template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::Group &location, const std::string &name,
                  const std::vector<float> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::DataSet &location, const std::string &name,
                  const std::vector<float> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::Group &location, const std::string &name,
                  const std::vector<double> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::DataSet &location, const std::string &name,
                  const std::vector<double> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::Group &location, const std::string &name,
                  const std::vector<int32_t> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::DataSet &location, const std::string &name,
                  const std::vector<int32_t> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::Group &location, const std::string &name,
                  const std::vector<uint32_t> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::DataSet &location, const std::string &name,
                  const std::vector<uint32_t> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::Group &location, const std::string &name,
                  const std::vector<int64_t> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::DataSet &location, const std::string &name,
                  const std::vector<int64_t> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::Group &location, const std::string &name,
                  const std::vector<uint64_t> &value);
template MANTID_DATAHANDLING_DLL void
writeNumAttribute(H5::DataSet &location, const std::string &name,
                  const std::vector<uint64_t> &value);

// -------------------------------------------------------------------
// instantiations for readAttributeAsString
// -------------------------------------------------------------------
template MANTID_DATAHANDLING_DLL std::string
readAttributeAsString(H5::Group &location, const std::string &attributeName);

template MANTID_DATAHANDLING_DLL std::string
readAttributeAsString(H5::DataSet &location, const std::string &attributeName);

// -------------------------------------------------------------------
// instantiations for readNumAttributeCoerce
// -------------------------------------------------------------------
template MANTID_DATAHANDLING_DLL float
readNumAttributeCoerce(H5::Group &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL float
readNumAttributeCoerce(H5::DataSet &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL double
readNumAttributeCoerce(H5::Group &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL double
readNumAttributeCoerce(H5::DataSet &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL int32_t
readNumAttributeCoerce(H5::Group &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL int32_t
readNumAttributeCoerce(H5::DataSet &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL uint32_t
readNumAttributeCoerce(H5::Group &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL uint32_t
readNumAttributeCoerce(H5::DataSet &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL int64_t
readNumAttributeCoerce(H5::Group &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL int64_t
readNumAttributeCoerce(H5::DataSet &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL uint64_t
readNumAttributeCoerce(H5::Group &location, const std::string &attributeName);
template MANTID_DATAHANDLING_DLL uint64_t
readNumAttributeCoerce(H5::DataSet &location, const std::string &attributeName);

// -------------------------------------------------------------------
// instantiations for readNumArrayAttributeCoerce
// -------------------------------------------------------------------
template MANTID_DATAHANDLING_DLL std::vector<float>
readNumArrayAttributeCoerce(H5::Group &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<float>
readNumArrayAttributeCoerce(H5::DataSet &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<double>
readNumArrayAttributeCoerce(H5::Group &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<double>
readNumArrayAttributeCoerce(H5::DataSet &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<int32_t>
readNumArrayAttributeCoerce(H5::Group &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<int32_t>
readNumArrayAttributeCoerce(H5::DataSet &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<uint32_t>
readNumArrayAttributeCoerce(H5::Group &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<uint32_t>
readNumArrayAttributeCoerce(H5::DataSet &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<int64_t>
readNumArrayAttributeCoerce(H5::Group &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<int64_t>
readNumArrayAttributeCoerce(H5::DataSet &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<uint64_t>
readNumArrayAttributeCoerce(H5::Group &location,
                            const std::string &attributeName);
template MANTID_DATAHANDLING_DLL std::vector<uint64_t>
readNumArrayAttributeCoerce(H5::DataSet &location,
                            const std::string &attributeName);

// -------------------------------------------------------------------
// instantiations for writeArray1D
// -------------------------------------------------------------------
template MANTID_DATAHANDLING_DLL void
writeArray1D(H5::Group &group, const std::string &name,
             const std::vector<float> &values);
template MANTID_DATAHANDLING_DLL void
writeArray1D(H5::Group &group, const std::string &name,
             const std::vector<double> &values);
template MANTID_DATAHANDLING_DLL void
writeArray1D(H5::Group &group, const std::string &name,
             const std::vector<int32_t> &values);
template MANTID_DATAHANDLING_DLL void
writeArray1D(H5::Group &group, const std::string &name,
             const std::vector<uint32_t> &values);
template MANTID_DATAHANDLING_DLL void
writeArray1D(H5::Group &group, const std::string &name,
             const std::vector<int64_t> &values);
template MANTID_DATAHANDLING_DLL void
writeArray1D(H5::Group &group, const std::string &name,
             const std::vector<uint64_t> &values);

// -------------------------------------------------------------------
// Instantiations for writeScalarWithStrAttributes
// -------------------------------------------------------------------
template MANTID_DATAHANDLING_DLL void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const std::string &value,
    const std::map<std::string, std::string> &attributes);
template MANTID_DATAHANDLING_DLL void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const float &value,
    const std::map<std::string, std::string> &attributes);
template MANTID_DATAHANDLING_DLL void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const double &value,
    const std::map<std::string, std::string> &attributes);
template MANTID_DATAHANDLING_DLL void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const int32_t &value,
    const std::map<std::string, std::string> &attributes);
template MANTID_DATAHANDLING_DLL void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const uint32_t &value,
    const std::map<std::string, std::string> &attributes);
template MANTID_DATAHANDLING_DLL void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const int64_t &value,
    const std::map<std::string, std::string> &attributes);
template MANTID_DATAHANDLING_DLL void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const uint64_t &value,
    const std::map<std::string, std::string> &attributes);

// -------------------------------------------------------------------
// instantiations for getDataSpace
// -------------------------------------------------------------------
template MANTID_DATAHANDLING_DLL DataSpace
getDataSpace(const std::vector<float> &data);
template MANTID_DATAHANDLING_DLL DataSpace
getDataSpace(const std::vector<double> &data);
template MANTID_DATAHANDLING_DLL DataSpace
getDataSpace(const std::vector<int32_t> &data);
template MANTID_DATAHANDLING_DLL DataSpace
getDataSpace(const std::vector<uint32_t> &data);
template MANTID_DATAHANDLING_DLL DataSpace
getDataSpace(const std::vector<int64_t> &data);
template MANTID_DATAHANDLING_DLL DataSpace
getDataSpace(const std::vector<uint64_t> &data);

// -------------------------------------------------------------------
// instantiations for readArray1DCoerce
// -------------------------------------------------------------------
template MANTID_DATAHANDLING_DLL std::vector<float>
readArray1DCoerce(H5::Group &group, const std::string &name);
template MANTID_DATAHANDLING_DLL std::vector<double>
readArray1DCoerce(H5::Group &group, const std::string &name);
template MANTID_DATAHANDLING_DLL std::vector<int32_t>
readArray1DCoerce(H5::Group &group, const std::string &name);
template MANTID_DATAHANDLING_DLL std::vector<uint32_t>
readArray1DCoerce(H5::Group &group, const std::string &name);
template MANTID_DATAHANDLING_DLL std::vector<int64_t>
readArray1DCoerce(H5::Group &group, const std::string &name);
template MANTID_DATAHANDLING_DLL std::vector<uint64_t>
readArray1DCoerce(H5::Group &group, const std::string &name);

template MANTID_DATAHANDLING_DLL std::vector<float>
readArray1DCoerce<float>(DataSet &dataset);
template MANTID_DATAHANDLING_DLL std::vector<double>
readArray1DCoerce<double>(DataSet &dataset);
template MANTID_DATAHANDLING_DLL std::vector<int32_t>
readArray1DCoerce<int32_t>(DataSet &dataset);
template MANTID_DATAHANDLING_DLL std::vector<uint32_t>
readArray1DCoerce<uint32_t>(DataSet &dataset);
template MANTID_DATAHANDLING_DLL std::vector<int64_t>
readArray1DCoerce<int64_t>(DataSet &dataset);
template MANTID_DATAHANDLING_DLL std::vector<uint64_t>
readArray1DCoerce<uint64_t>(DataSet &dataset);
} // namespace H5Util
} // namespace DataHandling
} // namespace Mantid
