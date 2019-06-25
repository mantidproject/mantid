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

void saveInstrument(const Geometry::ComponentInfo &compInfo,
                    const std::string &fullPath,
                    Kernel::ProgressBase *reporter) {

  boost::filesystem::path tmp(fullPath);
  if (!boost::filesystem::is_directory(tmp.root_directory())) {
    throw std::invalid_argument(
        "The path provided for saving the file is invalid: " + fullPath + "\n");
  }

  if (!compInfo.hasDetectorInfo()) {
    throw std::invalid_argument("The component has no detector info.\n");
  }

  if (reporter != nullptr) {
    reporter->report();
  }

  //auto compName = compInfo.name(0);	//potentially dead code
  //auto allComps = compInfo.componentsInSubtree(compInfo.root()); //potentially dead code

  const std::string nexusClassName = "NX_class";
  const std::string nexusClassType = "NXinstrument"; // class type

  H5::H5File file(fullPath, H5F_ACC_TRUNC); // create h5 file
  H5::Group group = file.createGroup("/raw_data_1");  // create group in file


  writeStrAttribute(group, nexusClassName, nexusClassType);

  file.close();


}; // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid


/* possibly dead code
  H5::DataType(H5T_class_t::H5T_STRING,attributeType.length()); H5::DataSpace
  dataSpace{}; //create dataSpace for attr

   group.createDataSet("sample", dataType, dataSpace);

  auto attr = group.createAttribute(attributeName, dataType, dataSpace);
  attr.write(dataType, attributeType);
  */