#include "MantidDataHandling/H5Util.h"

#include <H5Cpp.h>

using namespace H5;

namespace Mantid {
namespace DataHandling {
namespace H5Util {

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

void writeArray(H5::Group &group, const std::string &name,
                const std::string &value) {
  StrType dataType(0, value.length() + 1);
  DataSpace dataSpace = getDataSpace(1);
  H5::DataSet data = group.createDataSet(name, dataType, dataSpace);
  data.write(value, dataType);
}

void writeArray(H5::Group &group, const std::string &name,
                const std::vector<double> &values) {
  DataType dataType(PredType::NATIVE_DOUBLE);
  DataSpace dataSpace = getDataSpace(values);

  DSetCreatPropList propList = getPropList(values.size());

  auto data = group.createDataSet(name, dataType, dataSpace, propList);
  data.write(&(values[0]), dataType);
}

void writeArray(H5::Group &group, const std::string &name,
                const std::vector<int32_t> &values) {
  DataType dataType(PredType::NATIVE_INT32);
  DataSpace dataSpace = getDataSpace(values);

  DSetCreatPropList propList = getPropList(values.size());

  auto data = group.createDataSet(name, dataType, dataSpace, propList);
  data.write(&(values[0]), dataType);
}

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

} // namespace H5Util
} // namespace DataHandling
} // namespace Mantid
