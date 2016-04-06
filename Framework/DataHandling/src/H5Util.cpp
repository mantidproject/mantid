#include "MantidDataHandling/H5Util.h"

#include <H5Cpp.h>

using namespace H5;

namespace Mantid {
namespace DataHandling {
namespace H5Util {

// -------------------------------------------------------------------
// convert primitives to HDF5 enum
// -------------------------------------------------------------------

template <typename NumT>
DataType getType() {
  throw DataTypeIException();
}

template <>
MANTID_DATAHANDLING_DLL DataType getType<float>() {
  return PredType::NATIVE_FLOAT;
}

template <>
MANTID_DATAHANDLING_DLL DataType getType<double>() {
  return PredType::NATIVE_DOUBLE;
}

template <>
MANTID_DATAHANDLING_DLL DataType getType<int32_t>() {
  return PredType::NATIVE_INT32;
}

template <>
MANTID_DATAHANDLING_DLL DataType getType<uint32_t>() {
  return PredType::NATIVE_UINT32;
}

template <>
MANTID_DATAHANDLING_DLL DataType getType<int64_t>() {
  return PredType::NATIVE_INT64;
}

template <>
MANTID_DATAHANDLING_DLL DataType getType<uint64_t>() {
  return PredType::NATIVE_UINT64;
}

// -------------------------------------------------------------------
// write methods
// -------------------------------------------------------------------

DataSpace getDataSpace(const size_t length) {
  hsize_t dims[] = {length};
  return DataSpace(1, dims);
}

template <typename NumT> DataSpace getDataSpace(const std::vector<NumT> &data) {
  return H5Util::getDataSpace(data.size());
}

DSetCreatPropList getPropList(const std::size_t length, const int deflateLevel) {
  DSetCreatPropList propList;
  hsize_t chunk_dims[1] = {length};
  propList.setChunk(1, chunk_dims);
  propList.setDeflate(deflateLevel);
  return propList;
}

void writeStrAttribute(Group &location, const std::string &name,
                       const std::string &value) {
  StrType attrType(0, H5T_VARIABLE);
  DataSpace attrSpace(H5S_SCALAR);
  auto groupAttr = location.createAttribute(name, attrType, attrSpace);
  groupAttr.write(attrType, value);
}

void write(Group &group, const std::string &name,
                const std::string &value) {
  StrType dataType(0, value.length() + 1);
  DataSpace dataSpace = getDataSpace(1);
  H5::DataSet data = group.createDataSet(name, dataType, dataSpace);
  data.write(value, dataType);
}

template <typename NumT>
void writeArray1D(Group &group, const std::string &name,
                const std::vector<NumT> &values) {
  DataType dataType(getType<NumT>());
  DataSpace dataSpace = getDataSpace(values);

  DSetCreatPropList propList = getPropList(values.size());

  auto data = group.createDataSet(name, dataType, dataSpace, propList);
  data.write(&(values[0]), dataType);
}

// -------------------------------------------------------------------
// read methods
// -------------------------------------------------------------------

std::string readString(H5::H5File &file, const std::string &path) {
  try {
    DataSet data = file.openDataSet(path);
    std::string value;
    data.read(value, data.getDataType(), data.getSpace());
    return value;
  } catch (H5::FileIException &e) {
    UNUSED_ARG(e);
    return "";
  } catch (H5::GroupIException &e) {
    UNUSED_ARG(e);
    return "";
  }
}

template <typename NumT>
std::vector<NumT> readArray1DCoerce(DataSet &dataset,
                                  const DataType &desiredDataType) {
  std::vector<NumT> result;
  DataType dataType = dataset.getDataType();
  DataSpace dataSpace = dataset.getSpace();

  if (desiredDataType == dataType) {
    result.resize(dataSpace.getSelectNpoints());
    dataset.read(&result[0], dataType, dataSpace);
  } else if (PredType::NATIVE_UINT32 == dataType) {
    std::vector<uint32_t> temp(dataSpace.getSelectNpoints());
    dataset.read(&temp[0], dataType, dataSpace);
    result.assign(temp.begin(), temp.end());
  } else if (PredType::NATIVE_FLOAT == dataType) {
    std::vector<float> temp(dataSpace.getSelectNpoints());
    dataset.read(&temp[0], dataType, dataSpace);
    result.assign(temp.begin(), temp.end());
  } else {
    throw DataTypeIException();
  }

  return result;
}

// -------------------------------------------------------------------
// instantiations for writeArray1D
// -------------------------------------------------------------------
template
MANTID_DATAHANDLING_DLL void writeArray1D(H5::Group &group, const std::string &name, const std::vector<float> &values);
template
MANTID_DATAHANDLING_DLL void writeArray1D(H5::Group &group, const std::string &name, const std::vector<double> &values);
template
MANTID_DATAHANDLING_DLL void writeArray1D(H5::Group &group, const std::string &name, const std::vector<int32_t> &values);
template
MANTID_DATAHANDLING_DLL void writeArray1D(H5::Group &group, const std::string &name, const std::vector<uint32_t> &values);
template
MANTID_DATAHANDLING_DLL void writeArray1D(H5::Group &group, const std::string &name, const std::vector<int64_t> &values);
template
MANTID_DATAHANDLING_DLL void writeArray1D(H5::Group &group, const std::string &name, const std::vector<uint64_t> &values);

// -------------------------------------------------------------------
// instantiations for getDataSpace
// -------------------------------------------------------------------
template
MANTID_DATAHANDLING_DLL DataSpace getDataSpace(const std::vector<float> &data);
template
MANTID_DATAHANDLING_DLL DataSpace getDataSpace(const std::vector<double> &data);
template
MANTID_DATAHANDLING_DLL DataSpace getDataSpace(const std::vector<int32_t> &data);
template
MANTID_DATAHANDLING_DLL DataSpace getDataSpace(const std::vector<uint32_t> &data);
template
MANTID_DATAHANDLING_DLL DataSpace getDataSpace(const std::vector<int64_t> &data);
template
MANTID_DATAHANDLING_DLL DataSpace getDataSpace(const std::vector<uint64_t> &data);

// -------------------------------------------------------------------
// instantiations for readArray1DCoerce
// -------------------------------------------------------------------
template
MANTID_DATAHANDLING_DLL std::vector<float> readArray1DCoerce(DataSet &dataset,
                                  const DataType &desiredDataType);
template
MANTID_DATAHANDLING_DLL std::vector<double> readArray1DCoerce(DataSet &dataset,
                                  const DataType &desiredDataType);
template
MANTID_DATAHANDLING_DLL std::vector<int32_t> readArray1DCoerce(DataSet &dataset,
                                  const DataType &desiredDataType);
template
MANTID_DATAHANDLING_DLL std::vector<uint32_t> readArray1DCoerce(DataSet &dataset,
                                  const DataType &desiredDataType);
template
MANTID_DATAHANDLING_DLL std::vector<int64_t> readArray1DCoerce(DataSet &dataset,
                                  const DataType &desiredDataType);
template
MANTID_DATAHANDLING_DLL std::vector<uint64_t> readArray1DCoerce(DataSet &dataset,
                                  const DataType &desiredDataType);
} // namespace H5Util
} // namespace DataHandling
} // namespace Mantid
