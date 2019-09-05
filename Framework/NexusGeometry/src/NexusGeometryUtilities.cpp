// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/NexusGeometryUtilities.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"

namespace Mantid {
namespace NexusGeometry {
namespace utilities {
using namespace H5;

/// Find a single dataset inside parent group (returns first match). Optional
/// wrapped - empty to indicate nothing found.
boost::optional<H5::DataSet> findDataset(const H5::Group &parentGroup,
                                         const H5std_string &name) {
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      if (childPath == name) {
        auto childDataset = parentGroup.openDataSet(childPath);
        return boost::optional<DataSet>(childDataset);
      }
    }
  }
  return boost::optional<DataSet>{}; // Empty
}
/// Find a single group inside parent (returns first match). class type must
/// match NX_class. Optional wrapped - empty to indicate nothing found.
boost::optional<H5::Group> findGroup(const H5::Group &parentGroup,
                                     const H5std_string &classType) {
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      auto childGroup = parentGroup.openGroup(childPath);
      // Iterate through attributes to find NX_class
      for (uint32_t attribute_index = 0;
           attribute_index < static_cast<uint32_t>(childGroup.getNumAttrs());
           ++attribute_index) {
        // Test attribute at current index for NX_class
        Attribute attribute = childGroup.openAttribute(attribute_index);
        if (attribute.getName() == NX_CLASS) {
          // Get attribute data type
          DataType dataType = attribute.getDataType();
          // Get the NX_class type
          H5std_string classT;
          attribute.read(dataType, classT);
          // If group of correct type, return the childGroup
          if (classT == classType) {
            return boost::optional<Group>(childGroup);
          }
        }
      }
    }
  }
  return boost::optional<Group>{}; // Empty
}

/// Find all groups at the same level matching same class type. Returns first
/// item found.
std::vector<H5::Group> findGroups(const H5::Group &parentGroup,
                                  const H5std_string &classType) {
  std::vector<H5::Group> groups;
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      auto childGroup = parentGroup.openGroup(childPath);
      // Iterate through attributes to find NX_class
      for (uint32_t attribute_index = 0;
           attribute_index < static_cast<uint32_t>(childGroup.getNumAttrs());
           ++attribute_index) {
        // Test attribute at current index for NX_class
        Attribute attribute = childGroup.openAttribute(attribute_index);
        if (attribute.getName() == NX_CLASS) {
          // Get attribute data type
          DataType dataType = attribute.getDataType();
          // Get the NX_class type
          H5std_string classT;
          attribute.read(dataType, classT);
          // If group of correct type, return the childGroup
          if (classT == classType) {
            groups.push_back(childGroup);
          }
        }
      }
    }
  }
  return groups; // Empty
}
H5::Group findGroupOrThrow(const H5::Group &parentGroup,
                           const H5std_string &classType) {
  auto found = findGroup(parentGroup, classType);
  if (!found) {
    throw std::runtime_error(std::string("Could not find group class ") +
                             classType);
  } else
    return *found;
}

} // namespace utilities
} // namespace NexusGeometry
} // namespace Mantid
