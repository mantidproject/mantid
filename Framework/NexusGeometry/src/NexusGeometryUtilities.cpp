// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/NexusGeometryUtilities.h"
#include "MantidNexusGeometry/H5ForwardCompatibility.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"
#include <regex>
namespace Mantid::NexusGeometry::utilities {
using namespace H5;

/// Find a single dataset inside parent group (returns first match). Optional
/// wrapped - empty to indicate nothing found.
std::optional<H5::DataSet> findDataset(const H5::Group &parentGroup, const H5std_string &name) {
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      if (childPath == name) {
        auto childDataset = parentGroup.openDataSet(childPath);
        return std::optional<DataSet>(childDataset);
      }
    }
  }
  return std::optional<DataSet>{}; // Empty
}

std::optional<H5::Group> findGroupByName(const H5::Group &parentGroup, const H5std_string &name) {

  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      if (childPath == name) {
        return std::optional<H5::Group>(parentGroup.openGroup(childPath));
      }
    }
  }
  return std::optional<H5::Group>();
}

bool hasNXAttribute(const H5::Group &group, const std::string &attributeValue) {
  bool result = false;
  for (uint32_t attribute_index = 0; attribute_index < static_cast<uint32_t>(group.getNumAttrs()); ++attribute_index) {
    // Test attribute at current index for NX_class
    Attribute attribute = group.openAttribute(attribute_index);
    if (attribute.getName() == NX_CLASS) {
      // Get attribute data type
      DataType dataType = attribute.getDataType();
      // Get the NX_class type
      H5std_string classT;
      attribute.read(dataType, classT);
      // If group of correct type, return the childGroup
      if (classT == attributeValue) {
        result = true;
        break;
      }
    }
  }
  return result;
}

bool isNamed(const H5::H5Object &object, const std::string &name) {
  const auto objName = H5_OBJ_NAME(object);
  // resultName gives full path. We match the last name on the path
  return std::regex_match(objName, std::regex(".*/" + name + "$"));
}

/// Find a single group inside parent (returns first match). class type must
/// match NX_class. Optional wrapped - empty to indicate nothing found.
std::optional<H5::Group> findGroup(const H5::Group &parentGroup, const H5std_string &classType) {
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      auto childGroup = parentGroup.openGroup(childPath);
      // Iterate through attributes to find NX_class
      if (hasNXAttribute(childGroup, classType)) {
        return std::optional<Group>(childGroup);
      }
    }
  }
  return std::optional<Group>{}; // Empty
} // namespace utilities

/// Find all groups at the same level matching same class type. Returns first
/// item found.
std::vector<H5::Group> findGroups(const H5::Group &parentGroup, const H5std_string &classType) {
  std::vector<H5::Group> groups;
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      auto childGroup = parentGroup.openGroup(childPath);
      // Iterate through attributes to find NX_class
      if (hasNXAttribute(childGroup, classType))
        groups.emplace_back(childGroup);
    }
  }
  return groups; // Empty
}
H5::Group findGroupOrThrow(const H5::Group &parentGroup, const H5std_string &classType) {
  auto found = findGroup(parentGroup, classType);
  if (!found) {
    throw std::runtime_error(std::string("Could not find group class ") + classType);
  } else
    return *found;
}

} // namespace Mantid::NexusGeometry::utilities
