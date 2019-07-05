// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

// test your new groups and data sets have their NXclasses.

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include <H5Cpp.h>
#include <boost/filesystem/operations.hpp>

namespace Mantid {
namespace NexusGeometry {

namespace {

// NEXUS COMPLIANT ATTRIBUTE NAMES
const std::string SHORT_NAME = "short_name";
const std::string NX_CLASS = "NX_class";
const std::string NX_SAMPLE = "NXsample";
const std::string NX_DETECTOR = "NXdetector";
const std::string NX_OFF_GEOMETRY = "NXoff_geometry";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_SOURCE = "NXsource";
const std::string SHAPE = "shape";
const std::string DEPENDS_ON = "depends_on";
const std::string UNITS = "UNITS";
const std::string PIXEL_SHAPE = "pixel_shape";
const std::string METRES = "m";

// NEXUS COMPLIANT ATTRIBUTE VALUES
const std::string NX_TRANSFORMATION = "NXtransformation";
const std::string NX_CHAR = "NX_CHAR";

// COMPONENT TYPES
auto DETECTOR = Mantid::Beamline::ComponentType(6);

const H5::StrType H5VARIABLE(0, H5T_VARIABLE);
// const H5::StrType H5ASCII(0, H5T_CSET_ASCII);
const H5::DataSpace H5SCALAR(H5S_SCALAR);

} // namespace

/*
==============================================================================================================

    Helper functions

==============================================================================================================
*/

void writeStrAttributeToGroupHelper(H5::Group &grp, const std::string &attrName,
                                    const std::string &attrVal,
                                    H5::StrType dataType = H5VARIABLE,
                                    H5::DataSpace dataSpace = H5SCALAR) {
  H5::Attribute attribute = grp.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

void writeStrAttributeToDataSetHelper(H5::DataSet &dSet,
                                      const std::string &attrName,
                                      const std::string &attrVal,
                                      H5::StrType dataType = H5VARIABLE,
                                      H5::DataSpace dataSpace = H5SCALAR) {
  auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

// saves the instrument to file.
void saveInstrument(const Geometry::ComponentInfo &compInfo,
                    const std::string &fullPath,
                    Kernel::ProgressBase *reporter) {

  boost::filesystem::path tmp(fullPath);

  /*
==============================================================================================================

   Exception handling

==============================================================================================================
*/

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

  // does reporting if optional reporter exists.
  if (reporter != nullptr) {
    reporter->report();
  }

  if (!compInfo.hasDetectorInfo()) {
    throw std::invalid_argument("The component has no detector info.\n");
  }

  if (!compInfo.hasSample()) {

    throw std::invalid_argument("The component has no sample.\n");
  }

  if (Mantid::Kernel::V3D{0, 0, 0} != compInfo.position(compInfo.sample())) {
    throw std::invalid_argument("The sample is not at the origin.\n");
  }

  if (!compInfo.hasSource()) {
    throw std::invalid_argument("The component has no source.");
  }

  { // so i dont forget they exist.
    // compInfo.samplePosition();
    // compInfo.sourcePosition();
    // compinfo.sourcePosition();
    // compInfo.hasSample();
    // compInfo.hasSource();
  }

  /*
==============================================================================================================

     Begin writing NX format tree structure to file

==============================================================================================================

*/

  H5::H5File file(fullPath, H5F_ACC_TRUNC);

  // create root group @NXentry
  H5::Group rootGroup = file.createGroup("/raw_data_1");
  writeStrAttributeToGroupHelper(rootGroup, NX_CLASS, NX_ENTRY);

  const size_t ROOT_INDEX = compInfo.root();
  // const size_t SAMPLE_INDEX = compInfo.sample();
  // const int SOURCE_INDEX = compInfo.source();

  /*
  ==============================================================================================================
         Root group in tree: @NXentry

         => Instrument
         => sample
  ==============================================================================================================
  */

  // subgroups
  H5::Group instrumentGroup = rootGroup.createGroup("instrument");
  H5::Group sampleGroup = rootGroup.createGroup("sample");

  // datasets

  // attributes
  writeStrAttributeToGroupHelper(instrumentGroup, NX_CLASS, NX_INSTRUMENT);
  writeStrAttributeToGroupHelper(sampleGroup, NX_CLASS, NX_SAMPLE);

  /*
 ==============================================================================================================
        Instrument group in tree: @NXinstrument

        => detector bank(s)
                => source
 ==============================================================================================================
 */
  // subgroups
  H5::Group sourceGroup = instrumentGroup.createGroup("source");
  H5::Group detectorGroup = instrumentGroup.createGroup("detector_1");

  // datasets

  auto valueInDataset = (compInfo.name(ROOT_INDEX)).c_str();
  H5::DSetCreatPropList plist;
  plist.setFillValue(H5VARIABLE, &valueInDataset);
  auto instrNameSize = compInfo.name(ROOT_INDEX).size();
  H5::DataType instrName(H5VARIABLE);
  H5::DataSet instrumentName =
      instrumentGroup.createDataSet("name", H5VARIABLE, H5SCALAR, plist);

  // add dataset to child group 'instrument'

  // attributes
  writeStrAttributeToGroupHelper(sourceGroup, NX_CLASS, NX_SOURCE);
  writeStrAttributeToGroupHelper(detectorGroup, NX_CLASS, NX_DETECTOR);
  writeStrAttributeToDataSetHelper(instrumentName, SHORT_NAME,
                                   compInfo.name(ROOT_INDEX));

  /*
==============================================================================================================
    Detector group in tree : @NXinstrument

        => pixel geometry
==============================================================================================================
*/

  // subgroups
  H5::Group pixelGroup = detectorGroup.createGroup(PIXEL_SHAPE);

  // datasets
  H5::DataSet bankLocationInNXformat =
      detectorGroup.createDataSet("location", H5VARIABLE, H5SCALAR);
  H5::DataSet bankOrientationInNXformat =
      detectorGroup.createDataSet("orientation", H5VARIABLE, H5SCALAR);
  H5::DataSet xPixelOffset =
      detectorGroup.createDataSet("x_pixel_offset", H5VARIABLE, H5SCALAR);
  H5::DataSet yPixelOffset =
      detectorGroup.createDataSet("y_pixel_offset", H5VARIABLE, H5SCALAR);
  H5::DataSet detectorName =
      detectorGroup.createDataSet("local_name", H5VARIABLE, H5SCALAR);

  // attributes
  writeStrAttributeToGroupHelper(pixelGroup, NX_CLASS, NX_OFF_GEOMETRY);
  writeStrAttributeToDataSetHelper(bankLocationInNXformat, NX_CLASS,
                                   NX_TRANSFORMATION);
  writeStrAttributeToDataSetHelper(bankOrientationInNXformat, NX_CLASS,
                                   NX_TRANSFORMATION);
  writeStrAttributeToDataSetHelper(xPixelOffset, UNITS, METRES);
  writeStrAttributeToDataSetHelper(yPixelOffset, UNITS, METRES);

  /*
  =============================================================================================================

  parse components in tree structure

  =============================================================================================================
  */

  // detector array and pixel datasets

  /*for (size_t index = ROOT_INDEX; index >= 0; --index) {

    std::vector<std::pair<size_t, size_t>> xyOffsets;
    if (compInfo.isDetector(index)) {
      Mantid::Kernel::V3D offset = compInfo.position(index);
      xyOffsets.push_back(std::make_pair(offset[0], offset[1]));

      // write offset to x and y dataset.
      //
      // xPixelOffset.write(offset)

      // write  of detector as dataset to NXdetector group

      // write any additional attributes

      // get its location and writed it as a dataset with nxclass
      // NXtransformation. write its attributes including Nxclass
      // NXtransformation do the same with orientation
    }
  }*/

  file.close();

} // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
