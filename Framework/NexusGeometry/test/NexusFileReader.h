// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_NEXUSGEOMETRY_NEXUSFILEREADER_H_
#define MANTID_NEXUSGEOMETRY_NEXUSFILEREADER_H_

#include "MantidNexusGeometry/NexusGeometryDefinitions.h"

#include <H5Cpp.h>
#include <boost/filesystem.hpp>
#include <string>
#include <vector>

/*
 TODO: DOCUMENTATION

 *@author Takudzwa Makoni, RAL (UKRI), ISIS
 *@date 06/08/2019
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

// test utility used for validation of the structure of a nexus file as needed
// for unit tests in Nexus Geometry.
class NexusFileReader {

public:
  NexusFileReader(const std::string &fullPath) {
    boost::filesystem::path tmp = fullPath;

    if (!boost::filesystem::exists(tmp)) {
      throw std::invalid_argument("no such file.\n");
    } else {
      m_file.openFile(fullPath, H5F_ACC_RDONLY);
    }
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
  // child has expected CLASS_TYPE, and is in parent group with expected parent

  bool parentNXgroupHasChildNXgroup(const std::string &parentNX_CLASS_TYPE,
                                    const std::string &childNX_CLASS_TYPE) {

    H5::Group rootGroup = m_file.openGroup(DEFAULT_ROOT_PATH);

    // if specified parent NX class type is NX entry, check the top level of
    // file structure only. (dont take extra step to look for parent group)
    if (parentNX_CLASS_TYPE == NX_ENTRY) {

      for (hsize_t i = 0; i < rootGroup.getNumObjs(); ++i) {
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
    for (hsize_t i = 0; i < rootGroup.getNumObjs(); ++i) {
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
          for (hsize_t i = 0; i < childGroup.getNumObjs(); ++i) {
            if (childGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
              std::string grandchildPath = childGroup.getObjnameByIdx(i);
              // Open the sub group
              H5::Group grandchildGroup = childGroup.openGroup(grandchildPath);
              // check NX class
              H5::Attribute grandchildAttribute =
                  grandchildGroup.openAttribute(NX_CLASS);
              std::string grandchildAttrVal;
              grandchildAttribute.read(grandchildAttribute.getDataType(),
                                       grandchildAttrVal);
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

  double readDoubleFromDataset(const std::string &datasetName,
                               const FullNXPath &pathToGroup) {
    double value;
    int rank = 1;
    hsize_t dims[(hsize_t)1];
    dims[0] = (hsize_t)1;

    H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

    // open dataset and read.
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataset = parentGroup.openDataSet(datasetName);
    dataset.read(&value, H5::PredType::NATIVE_DOUBLE, space);
    return value;
  }

  // HERE
  std::vector<double>
  readDoubleVectorFrom_d_Attribute(const std::string &attrName,
                                   const std::string &datasetName,
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
  bool hasDatasetWithNXAttribute(const std::string &pathToGroup,
                                 const std::string &nx_attributeVal) {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    auto numOfChildren = parentGroup.getNumObjs();
    for (hsize_t i = 0; i < numOfChildren; i++) {
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
  bool hasDatasetWithAttribute(const std::string &pathToGroup,
                               const std::string &attributeVal,
                               const std::string &attrName) {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    auto numOfChildren = parentGroup.getNumObjs();
    for (hsize_t i = 0; i < numOfChildren; i++) {
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

  bool hasDataset(const std::string dsetName, const FullNXPath &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    auto numOfChildren = parentGroup.getNumObjs();
    for (hsize_t i = 0; i < numOfChildren; i++) {
      if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
        std::string dataSetName = parentGroup.getObjnameByIdx(i);
        if (dsetName == dataSetName) {
          return true;
        }
      }
    }
    return false;
  }

  bool groupHasNxClass(const std::string &attrVal,
                       const std::string &pathToGroup) const {

    H5::Attribute attribute;
    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    attribute = parentGroup.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool dataSetHasStrValue(
      const std::string &dataSetName, const std::string &dataSetValue,
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
  bool hasAttributeInGroup(const std::string &attrName,
                           const std::string &attrVal,
                           const FullNXPath &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(attrName);
    std::string attributeValue;
    auto type = attribute.getDataType();
    attribute.read(type, attributeValue);
    attributeValue.resize(type.getSize());
    return attributeValue == attrVal;
  }

  bool hasNXAttributeInGroup(const std::string &attrVal,
                             const FullNXPath &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasAttributeInDataSet(
      const std::string dataSetName, const std::string &attrName,
      const std::string &attrVal,
      const FullNXPath &pathToGroup /*where the dataset lives*/) {

    H5::Attribute attribute;
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
    attribute = dataSet.openAttribute(attrName);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasNXAttributeInDataSet(const std::string dataSetName,
                               const std::string &attrVal,
                               const FullNXPath &pathToGroup) {
    H5::Attribute attribute;
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
    attribute = dataSet.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

private:
  H5::H5File m_file;

}; // NexusFileReader
} // namespace NexusGeometry
} // namespace Mantid
#endif
