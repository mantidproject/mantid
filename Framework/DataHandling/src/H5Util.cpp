#include "MantidDataHandling/H5Util.h"
#include "MantidKernel/System.h"
#include "MantidAPI/LogManager.h"

#include <H5Cpp.h>
#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>

using namespace H5;

namespace Mantid {
namespace DataHandling {
namespace H5Util {

namespace {
/// static logger object
Mantid::Kernel::Logger g_log("H5Util");

const std::string NX_ATTR_CLASS("NX_class");
} // anonymous

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

DataSpace getDataSpace(const size_t length) {
  hsize_t dims[] = {length};
  return DataSpace(1, dims);
}

template <typename NumT> DataSpace getDataSpace(const std::vector<NumT> &data) {
  return H5Util::getDataSpace(data.size());
}

DSetCreatPropList setCompressionAttributes(const std::size_t length,
                                           const int deflateLevel) {
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

void write(Group &group, const std::string &name, const std::string &value) {
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
std::vector<OutputNumT> convertingingRead(DataSet &dataset,
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
} // anonymous

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
    return convertingingRead<int32_t, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_UINT32 == dataType) {
    return convertingingRead<uint32_t, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_INT64 == dataType) {
    return convertingingRead<int64_t, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_UINT64 == dataType) {
    return convertingingRead<uint64_t, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_FLOAT == dataType) {
    return convertingingRead<float, NumT>(dataset, dataType);
  } else if (PredType::NATIVE_DOUBLE == dataType) {
    return convertingingRead<double, NumT>(dataset, dataType);
  }

  // not a supported type
  throw DataTypeIException();
}

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
