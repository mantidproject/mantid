// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/ProgressBase.h"

#include <H5Cpp.h>
#include <boost/filesystem/operations.hpp>

namespace Mantid {
namespace NexusGeometry {

namespace {

// NEXUS COMPLIANT ATTRIBUTE NAMES
const std::string SHORT_NAME = "short_name";
const std::string NX_CLASS = "NX_class";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_SOURCE = "NXsource";
const std::string SHAPE = "shape";

// NEXUS COMPLIANT ATTRIBUTE VALUES
const std::string NX_TRANSFORMATION = "NXtransformation";
const std::string NX_CHAR = "NX_CHAR";

} // namespace

void writeStrAttributeToGroup(H5::Group &grp, const std::string &NX_class,
                              const std::string &attrVal) {
  H5::StrType attrType(0, H5T_VARIABLE);
  H5::DataSpace attrSpace(H5S_SCALAR);
  auto attribute = grp.createAttribute(NX_class, attrType, attrSpace);
  attribute.write(attrType, attrVal);
}

void writeStrAttributeToDataSet(H5::DataSet &dSet, const std::string &NX_class,
                                const std::string &attrVal) {
  H5::StrType attrType(0, H5T_VARIABLE);
  H5::DataSpace attrSpace(H5S_SCALAR);
  auto attribute = dSet.createAttribute(NX_class, attrType, attrSpace);
  attribute.write(attrType, attrVal);
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

  H5::H5File file(fullPath, H5F_ACC_TRUNC); // create h5 file

  H5::Group parentGroup =
      file.createGroup("/raw_data_1"); // create parent group in file.

  H5::Group instrumentGroup =
      parentGroup.createGroup("instrument"); // create child group in parent.

  writeStrAttributeToGroup(parentGroup, NX_CLASS,
                           NX_ENTRY); // write attributes to parent.

  writeStrAttributeToGroup(instrumentGroup, NX_CLASS,
                           NX_INSTRUMENT); // write attributes to child.

  // create DataSet 'data' in instrument.

  H5::StrType dataType(0, H5T_VARIABLE);
  H5::DataSpace dataSpace(H5S_SCALAR);

  std::string dataSetData = "test_data_for_instrument"; // value for dataset.
  auto dataSetDataAsCStr = dataSetData.c_str();         // for plist..

  H5::DSetCreatPropList plist;
  plist.setFillValue(dataType, &dataSetDataAsCStr); // for dataSet.

  // add dataset to child group 'instrument'
  H5::DataSet dataSet = instrumentGroup.createDataSet(
      "name", dataType, dataSpace, // <= compInfo.name(compInfo.root())
      plist); // name of dataset initialised with fill value.

  writeStrAttributeToDataSet(
      dataSet, SHORT_NAME,
      "name"); // compInfo.name(compInfo.root())); // add  atribute to dataset.

  writeStrAttributeToDataSet(dataSet, NX_CLASS,
                             NX_CHAR); // add NX_class attribute to dataset

  file.close();

}; // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
