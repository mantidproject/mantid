// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/ProgressBase.h"
#include <boost/filesystem/operations.hpp>
#include <iostream>

namespace Mantid {
namespace NexusGeometry {

namespace {

const H5G_obj_t GROUP_TYPE = static_cast<H5G_obj_t>(0);
const H5G_obj_t DATASET_TYPE = static_cast<H5G_obj_t>(1);
const std::string NX_CLASS = "NX_class";

} // namespace

void saveInstrument(const Geometry::ComponentInfo &compInfo,
                    const std::string &fullPath,
                    Kernel::ProgressBase *reporter) {

  boost::filesystem::path tmp(fullPath);
  if (!boost::filesystem::is_directory(tmp)) {
    //throw std::invalid_argument( //exception unhandled
      //  "The path provided for the file saving is invalid: " + fullPath + "\n");
	
	std::cout << "\ntest\n";
  }

  if (!compInfo.hasDetectorInfo()) {
    throw std::invalid_argument("The component has no detector info.\n");
  }

  if (reporter != nullptr) {
    reporter->report();
  }

  /*do checks on instrument attributes and classes.*/

  // save file to destination 'fullPath' WIP

  std::string instrumentData;
  std::string filename;
  std::string pathToFile = fullPath + "\\" + filename;
  std::ofstream file(pathToFile); // open file.

  file << instrumentData; // write data to file

}; // saveInstrument

// define HDF5FileTestUtility class here
HDF5FileTestUtility::HDF5FileTestUtility(const std::string &fullPath)
    : m_file(fullPath, H5F_ACC_RDONLY) {}

bool HDF5FileTestUtility::hasNxClass(std::string className,
                                     std::string HDF5Path) const {

  H5::Group parentGroup = m_file.openGroup(HDF5Path);

  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      H5::Group childGroup = parentGroup.openGroup(childPath);
      // Iterate through attributes to find NX_class
      for (uint32_t attribute_index = 0;
           attribute_index < static_cast<uint32_t>(childGroup.getNumAttrs());
           ++attribute_index) {
        // Test attribute at current index for NX_class
        H5::Attribute attribute = childGroup.openAttribute(attribute_index);
        if (attribute.getName() == NX_CLASS) {
          // Get attribute data type
          H5::DataType dataType = attribute.getDataType();
          // Get the NX_class type
          H5std_string classType;
          attribute.read(dataType, classType);
          // If group of correct type, return true

          return classType == className ? true : false;
        }
      }
    }

    /*
      // std::string classTree = HDF5Path + "\\" + className;
      for (hsize_t i = 0; i < m_file.getNumObjs(); ++i) {
        H5::Attribute attribute = m_file.openAttribute(1);
        std::string classType;

        H5::DataType dataType = attribute.getDataType();
        attribute.read(dataType, classType);

        return classType == className ? true : false;
      };
      */
  } // hdf5testfileutility

} // namespace NexusGeometry
} // namespace NexusGeometry
} // namespace Mantid
