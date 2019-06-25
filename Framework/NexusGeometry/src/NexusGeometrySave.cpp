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
  

auto compName = compInfo.name(0);
auto allComps = compInfo.componentsInSubtree(compInfo.root());


//hsize_t dims[2]; //for dataSpace
//hsize_t cdims[2];//for dataSpace

H5::H5File file = H5::H5File(fullPath, H5F_ACC_TRUNC); //create h5 file
H5::Group group = H5::Group(file.createGroup("/raw_data_1")); //create group in file
const std::string nxInstrument = "NXinstrument";

H5::DataType dataType = H5::DataType(H5T_class_t::H5T_STRING, nxInstrument.length()); //create dataype for attribute
std::vector<size_t> dims = {nxInstrument.length()};

H5::DataSpace dataSpace{};
//H5::DataSpace dataSpace(1, dims.data());
H5::Attribute attribute;                 // create attribute for group
//attribute.write(dataType, nxInstrument); //write datatype to attribute.

auto attr = group.createAttribute("NX_class", dataType, dataSpace);
attr.write(dataType, nxInstrument);





}; // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
