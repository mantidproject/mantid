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

#include "H5cpp.h"
#include <H5DataSet.h>
#include <H5FPublic.h>
#include <H5File.h>
#include <H5Group.h>
#include <H5Location.h>
#include <H5Object.h>

namespace Mantid {
namespace NexusGeometry {

void writeStrAttribute(H5::Group &grp, const std::string &nexusClass,
                       const std::string &nexusType) {
  H5::StrType attrType(0, H5T_VARIABLE);
  H5::DataSpace attrSpace(H5S_SCALAR);
  auto attribute = grp.createAttribute(nexusClass, attrType, attrSpace);
  attribute.write(attrType, nexusType);
}

// saves the instrument to file.
void saveInstrument(const Geometry::ComponentInfo &compInfo,
                    const std::string &fullPath,
                    Kernel::ProgressBase *reporter) {

  boost::filesystem::path tmp(fullPath);

  // check the directory for the file is valid.
  if (!boost::filesystem::is_directory(tmp.root_directory())) {
    throw std::invalid_argument(
        "The path provided for saving the file is invalid: " + fullPath + "\n");
  }

  // check the file itself has valid extensions.
  const auto ext = boost::filesystem::path(tmp).extension();
  if ((ext != ".nxs") && (ext != ".hdf5")) {

    throw std::invalid_argument("invalid extension for file: " +
                                ext.generic_string());
  }

  // check if the component has detector info.
  if (!compInfo.hasDetectorInfo()) {
    throw std::invalid_argument("The component has no detector info.\n");
  }

  // does reporting if optional reporter exists.
  if (reporter != nullptr) {
    reporter->report();
  }

  // create parent 'raw_data_1' and child 'instrument' group with NX_class
  // attributes of type NXentry and NXinstrument, then write to file.

  const std::string parentNexusClassName = "NX_class"; // class name
  const std::string parentNexusClassType = "NXentry";  // class type

  const std::string childNexusClassName = "NX_class";     // class name
  const std::string childNexusClassType = "NXinstrument"; // class type

  H5::H5File file(fullPath, H5F_ACC_TRUNC); // create h5 file

  H5::Group parentGroup = file.createGroup("/raw_data_1");  //create parent group in file.
  H5::Group childGroup = parentGroup.createGroup("instrument"); //create child group in parent.

  writeStrAttribute(parentGroup, parentNexusClassName, parentNexusClassType); //write attributes to parent.
  writeStrAttribute(childGroup, childNexusClassName, childNexusClassType); //write attributes to child.

  file.close();

}; // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
