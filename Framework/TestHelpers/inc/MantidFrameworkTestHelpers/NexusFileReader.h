// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/H5Util.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"

#include <Eigen/Dense>
#include <H5Cpp.h>
#include <filesystem>
#include <string>
#include <vector>

/*
 * NexusFileReader: Test utility for unit testing in
 * NexusGeometrySave::saveInstrument.
 *
 * @author Takudzwa Makoni, RAL (UKRI), ISIS
 * @date 06/08/2019
 */

namespace Mantid {
namespace NexusGeometry {
using FullNXPath = std::vector<std::string>;
// get Nexus file path as string. Used in Nexus Geometry unit tests.
std::string toNXPathString(FullNXPath &path) {
  std::string pathString = "";
  for (const std::string &grp : path) {
    pathString += "/" + grp;
  }
  return pathString;
}

// ported from NexusGeometryParser, for validating storage type of dataset
// before reading its contents into a container
template <typename ExpectedT> void validateStorageType(const H5::DataSet &data) {

  const auto typeClass = data.getTypeClass();
  const size_t sizeOfType = data.getDataType().getSize();
  // Early check to prevent reinterpretation of underlying data.
  if (std::is_floating_point<ExpectedT>::value) {
    if (H5T_FLOAT != typeClass) {
      throw std::runtime_error("Storage type mismatch. Expecting to extract a "
                               "floating point number");
    }
    if (sizeOfType != sizeof(ExpectedT)) {
      throw std::runtime_error("Storage type mismatch for floats. This operation "
                               "is dangerous. Nexus stored has byte size:" +
                               std::to_string(sizeOfType));
    }
  } else if (std::is_integral<ExpectedT>::value) {
    if (H5T_INTEGER != typeClass) {
      throw std::runtime_error("Storage type mismatch. Expecting to extract a integer");
    }
    if (sizeOfType > sizeof(ExpectedT)) {
      // endianness not checked
      throw std::runtime_error("Storage type mismatch for integer. Result "
                               "would result in truncation. Nexus stored has byte size:" +
                               std::to_string(sizeOfType));
    }
  }
}

// test utility used for validation of the structure of a nexus file as needed
// for unit tests in Nexus Geometry.
class NexusFileReader {

  bool m_open = false;

public:
  NexusFileReader(const std::string &fullPath) {
    std::filesystem::path tmp = fullPath;

    if (!std::filesystem::exists(tmp)) {
      throw std::invalid_argument("no such file.\n");
    } else {
      m_file.openFile(fullPath, H5F_ACC_RDONLY, NeXus::H5Util::defaultFileAcc());
      m_open = true;
    }
  }

  int countNXgroup(const FullNXPath &pathToGroup, const std::string &nxClass) {
    int counter = 0;
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
      if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
        H5std_string childPath = parentGroup.getObjnameByIdx(i);
        // Open the sub group
        auto childGroup = parentGroup.openGroup(childPath);
        // Iterate through attributes to find NX_class
        for (uint32_t attribute_index = 0; attribute_index < static_cast<uint32_t>(childGroup.getNumAttrs());
             ++attribute_index) {
          // Test attribute at current index for NX_class
          H5::Attribute attribute = childGroup.openAttribute(attribute_index);
          if (attribute.getName() == NX_CLASS) {
            // Get attribute data type
            H5::DataType dataType = attribute.getDataType();
            // Get the NX_class type
            H5std_string classType;
            attribute.read(dataType, classType);
            // If group is of the correct type, include the group in the count
            if (classType == nxClass) {
              counter++;
            }
          }
        }
      }
    }
    return counter;
  }

  // read a multidimensional dataset and return a vector containing the data
  template <typename T>
  std::vector<T> readDataSetMultidimensional(FullNXPath &pathToGroup, const std::string &dataSetName) {

    std::vector<T> dataInFile;

    // open dataset and read.
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataset = parentGroup.openDataSet(dataSetName);

    validateStorageType<T>(dataset);
    auto space = dataset.getSpace();

    dataInFile.resize(space.getSelectNpoints());
    dataset.read(dataInFile.data(), dataset.getDataType(), space);
    return dataInFile;
  }

  /* safely open a HDF5 group path with additional helpful
   debug information to output where open fails) */
  H5::Group openfullH5Path(const FullNXPath &pathList) const {

    H5::Group child;
    H5::Group parent = m_file.openGroup(pathList[0]);

    for (size_t i = 1; i < pathList.size(); ++i) {
      child = parent.openGroup(pathList[i]);
      parent = child;
    }
    return child;
  }

  // moves down the index through groups starting at root, and if
  // child has expected CLASS_TYPE, and is in parent group with expected
  // parent

  bool parentNXgroupHasChildNXgroup(const std::string &parentNX_CLASS_TYPE, const std::string &childNX_CLASS_TYPE) {

    H5::Group rootGroup = m_file.openGroup(DEFAULT_ROOT_ENTRY_NAME);

    // if specified parent NX class type is NX entry, check the top level of
    // file structure only. (dont take extra step to look for parent group)
    if (parentNX_CLASS_TYPE == NX_ENTRY) {

      for (size_t i = 0; i < rootGroup.getNumObjs(); ++i) {
        if (rootGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
          std::string childPath = rootGroup.getObjnameByIdx(i);
          // Open the sub group
          H5::Group childGroup = rootGroup.openGroup(childPath);
          // Test attribute at current index for NX_class
          H5::Attribute attribute = childGroup.openAttribute(NX_CLASS);
          std::string attrVal;
          attribute.read(attribute.getDataType(), attrVal);
          if (attrVal == childNX_CLASS_TYPE) {
            return true;
          }
        }
      }
    }

    // Iterate over children of root group, and determine if a group
    for (size_t i = 0; i < rootGroup.getNumObjs(); ++i) {
      if (rootGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
        std::string childPath = rootGroup.getObjnameByIdx(i);
        // Open the sub group
        H5::Group childGroup = rootGroup.openGroup(childPath);
        // check current child group going down from root has the specified
        // NX_CLASS parent group
        H5::Attribute parentAttribute = childGroup.openAttribute(NX_CLASS);
        std::string parentAttrVal;
        parentAttribute.read(parentAttribute.getDataType(), parentAttrVal);
        if (parentAttrVal == parentNX_CLASS_TYPE) {
          for (size_t i = 0; i < childGroup.getNumObjs(); ++i) {
            if (childGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
              std::string grandchildPath = childGroup.getObjnameByIdx(i);
              // Open the sub group
              H5::Group grandchildGroup = childGroup.openGroup(grandchildPath);
              // check NX class
              H5::Attribute grandchildAttribute = grandchildGroup.openAttribute(NX_CLASS);
              std::string grandchildAttrVal;
              grandchildAttribute.read(grandchildAttribute.getDataType(), grandchildAttrVal);
              if (childNX_CLASS_TYPE == grandchildAttrVal) {
                return true;
              }
            }
          }
        }
      }
    }

    return false;
  } // namespace

  double readDoubleFromDataset(const std::string &datasetName, const FullNXPath &pathToGroup) {
    double value;
    int rank = 1;
    hsize_t dims[static_cast<hsize_t>(1)];
    dims[0] = static_cast<hsize_t>(1);

    H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

    // open dataset and read.
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataset = parentGroup.openDataSet(datasetName);
    dataset.read(&value, H5::PredType::NATIVE_DOUBLE, space);
    return value;
  }

  // HERE
  std::vector<double> readDoubleVectorFrom_d_Attribute(const std::string &attrName, const std::string &datasetName,
                                                       const FullNXPath &pathToGroup) {

    // open dataset and read.
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataset = parentGroup.openDataSet(datasetName);

    H5::Attribute attribute = dataset.openAttribute(attrName);

    H5::DataType dataType = attribute.getDataType();
    H5::DataSpace dataSpace = attribute.getSpace();

    std::vector<double> value;
    value.resize(dataSpace.getSelectNpoints());

    attribute.read(dataType, value.data());

    return value;
  }

  // HERE
  bool hasDatasetWithNXAttribute(const std::string &pathToGroup, const std::string &nx_attributeVal) {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    auto numOfChildren = parentGroup.getNumObjs();
    for (size_t i = 0; i < numOfChildren; i++) {
      if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
        std::string dSetName = parentGroup.getObjnameByIdx(i);
        H5::DataSet dSet = parentGroup.openDataSet(dSetName);
        if (dSet.attrExists(NX_CLASS)) {
          H5::Attribute attribute = dSet.openAttribute(NX_CLASS);
          std::string attributeValue;
          attribute.read(attribute.getDataType(), attributeValue);
          if (attributeValue == nx_attributeVal)
            return true;
        }
      }
    }
    return false;
  }

  // HERE
  bool hasDatasetWithAttribute(const std::string &pathToGroup, const std::string &attributeVal,
                               const std::string &attrName) {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    auto numOfChildren = parentGroup.getNumObjs();
    for (size_t i = 0; i < numOfChildren; i++) {
      if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
        std::string dSetName = parentGroup.getObjnameByIdx(i);
        H5::DataSet dSet = parentGroup.openDataSet(dSetName);
        if (dSet.attrExists(NX_CLASS)) {
          H5::Attribute attribute = dSet.openAttribute(attrName);
          std::string attributeValue;
          attribute.read(attribute.getDataType(), attributeValue);
          if (attributeValue == attributeVal)
            return true;
        }
      }
    }
    return false;
  }

  bool hasDataset(const std::string &dsetName, const FullNXPath &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    auto numOfChildren = parentGroup.getNumObjs();
    for (size_t i = 0; i < numOfChildren; i++) {
      if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
        std::string dataSetName = parentGroup.getObjnameByIdx(i);
        if (dsetName == dataSetName) {
          return true;
        }
      }
    }
    return false;
  }

  bool groupHasNxClass(const std::string &attrVal, const std::string &pathToGroup) const {

    H5::Attribute attribute;
    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    attribute = parentGroup.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool dataSetHasStrValue(const std::string &dataSetName, const std::string &dataSetValue,
                          const FullNXPath &pathToGroup /*where the dataset lives*/) const {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    try {
      H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
      std::string dataSetVal;
      auto type = dataSet.getDataType();
      dataSet.read(dataSetVal, type);
      dataSetVal.resize(type.getSize());
      return dataSetVal == dataSetValue;
    } catch (H5::DataSetIException &) {
      return false;
    }
  }

  // check if dataset or group has name-specific attribute
  bool hasAttributeInGroup(const std::string &attrName, const std::string &attrVal, const FullNXPath &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(attrName);
    std::string attributeValue;
    auto type = attribute.getDataType();
    attribute.read(type, attributeValue);
    attributeValue.resize(type.getSize());
    return attributeValue == attrVal;
  }

  bool hasNXAttributeInGroup(const std::string &attrVal, const FullNXPath &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasAttributeInDataSet(const std::string &dataSetName, const std::string &attrName, const std::string &attrVal,
                             const FullNXPath &pathToGroup /*where the dataset lives*/) {

    H5::Attribute attribute;
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
    attribute = dataSet.openAttribute(attrName);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasNXAttributeInDataSet(const std::string &dataSetName, const std::string &attrVal,
                               const FullNXPath &pathToGroup) {
    H5::Attribute attribute;
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
    attribute = dataSet.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  void close() {
    if (m_open) {
      m_file.close();
    }
    m_open = false;
  }

  ~NexusFileReader() { close(); }

private:
  H5::H5File m_file;

}; // NexusFileReader
} // namespace NexusGeometry
} // namespace Mantid
