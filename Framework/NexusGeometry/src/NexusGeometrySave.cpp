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
#include <H5File.h>
#include <H5Group.h>
#include <H5Location.h>
#include <H5Object.h>
#include <H5FPublic.h>

namespace Mantid {
namespace NexusGeometry {

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

  /*do checks on instrument attributes and classes.*/

  // save file to destination 'fullPath' WIP
  //create attribute in group
  
 std::string attr_name = "NX_class";
   
  H5::H5File *file = new H5::H5File(fullPath, H5F_ACC_TRUNC); // delete later
  H5::Group *group = new H5::Group(file->createGroup("/raw_data_1/instrument"));

  const H5::DataType dataType(*group, "NXinstrument");
  H5::DataSpace dataSpace;
  file->createAttribute(attr_name, dataType, dataSpace);

  delete file;
  delete group;
  /*
  std::string instrumentData; //
  std::string filename; // 
  std::string pathToFile; // fullPath + filename
  std::ofstream file(pathToFile); // open file.
  */

}; // saveInstrument


} // namespace NexusGeometry
} // namespace Mantid
