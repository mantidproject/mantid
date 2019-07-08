// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/*
==============================================================================================================

    saves the instrument to file.

==============================================================================================================
*/

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

// NEXUS NAMES
const std::string NX_CLASS = "NX_class";
const std::string NX_SAMPLE = "NXsample";
const std::string NX_DETECTOR = "NXdetector";
const std::string NX_OFF_GEOMETRY = "NXoff_geometry";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_CHAR = "NX_CHAR";
const std::string NX_SOURCE = "NXsource";
const std::string NX_TRANSFORMATIONS = "NXtransformations";

// group/attribute/dataset names
const std::string SHORT_NAME = "short_name";
const std::string TRANSFORMATIONS = "transformations";
const std::string LOCAL_NAME = "local_name";
const std::string LOCATION = "location";
const std::string ORIENTATION = "orientation";
const std::string DEPENDS_ON = "depends_on";
const std::string X_PIXEL_OFFSET = "x_pixel_offset";
const std::string Y_PIXEL_OFFSET = "y_pixel_offset";
const std::string PIXEL_SHAPE = "pixel_shape";
const std::string SOURCE = "source";
const std::string DETECTOR_NUMBER = "depends_on";

// metadata
const std::string METRES = "m";
const std::string NAME = "name";
const std::string UNITS = "UNITS";
const std::string SHAPE = "shape";

// NEXUS COMPLIANT ATTRIBUTE VALUES

//
const H5::StrType H5VARIABLE(0, H5T_VARIABLE); // this may be inefficient
const H5::DataSpace H5SCALAR(H5S_SCALAR);

} // namespace

/*
==============================================================================================================

    Helper functions

==============================================================================================================
*/

inline void writeStrAttributeToGroupHelper(H5::Group &grp,
                                           const std::string &attrName,
                                           const std::string &attrVal,
                                           H5::StrType dataType = H5VARIABLE,
                                           H5::DataSpace dataSpace = H5SCALAR) {
  H5::Attribute attribute = grp.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

inline void writeStrAttributeToDataSetHelper(
    H5::DataSet &dSet, const std::string &attrName, const std::string &attrVal,
    H5::StrType dataType = H5VARIABLE, H5::DataSpace dataSpace = H5SCALAR) {
  auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

inline void writeStrValueToDataSetHelper(H5::DataSet &dSet,
                                         std::string dSetValue) {

  dSet.write(dSetValue, dSet.getDataType(), H5SCALAR);
}

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

  if (Mantid::Kernel::V3D{0, 0, 0} != compInfo.samplePosition()) {
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
    // auto DETECTOR = Mantid::Beamline::ComponentType(6);
  }

  /*
==============================================================================================================

 Parse tree structure in component

==============================================================================================================
*/

  H5::H5File file(fullPath, H5F_ACC_TRUNC);

  // create root group @NXentry
  H5::Group rootGroup = file.createGroup("/raw_data_1");
  writeStrAttributeToGroupHelper(rootGroup, NX_CLASS, NX_ENTRY);

  const size_t ROOT_INDEX = compInfo.root();

  std::vector<int> detectorBanks;
  for (int i = ROOT_INDEX; i > 0; --i) {
    if (compInfo.isDetector(i)) {
      detectorBanks.push_back(i);
    }
  }

  // get number of detector banks/arrays

  // get pixels for each detector

  //

  /*
==============================================================================================================

     Begin writing NX format tree structure to file

==============================================================================================================

*/

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
                Parent: NXentry

        Children:
        => detector arrays
        => source
 ==============================================================================================================
 */
  // subgroups
  H5::Group sourceGroup = instrumentGroup.createGroup("source");
  H5::Group detectorGroup = instrumentGroup.createGroup("detector_1");

  // datasets
  H5::DataSet instrumentName =
      instrumentGroup.createDataSet("name", H5VARIABLE, H5SCALAR);

  // attributes
  writeStrAttributeToGroupHelper(sourceGroup, NX_CLASS, NX_SOURCE);
  writeStrAttributeToGroupHelper(detectorGroup, NX_CLASS, NX_DETECTOR);
  writeStrAttributeToDataSetHelper(instrumentName, SHORT_NAME,
                                   compInfo.name(ROOT_INDEX));
  // dataset values
  writeStrValueToDataSetHelper(instrumentName,
                               compInfo.name(ROOT_INDEX).c_str());

  /*
==============================================================================================================
                Detector group in tree : @NXinstrument
        Parent: NXinstrument

        Children:
        => pixel geometry
==============================================================================================================
*/

  // get num of detector banks and do for each:

  // subgroups
  H5::Group pixelGroup = detectorGroup.createGroup(PIXEL_SHAPE);

  // datasets
  H5::DataSet bankLocationInNXformat =
      detectorGroup.createDataSet(LOCATION, H5VARIABLE, H5SCALAR);
  H5::DataSet bankOrientationInNXformat =
      detectorGroup.createDataSet(ORIENTATION, H5VARIABLE, H5SCALAR);
  H5::DataSet xPixelOffset =
      detectorGroup.createDataSet(X_PIXEL_OFFSET, H5VARIABLE, H5SCALAR);
  H5::DataSet yPixelOffset =
      detectorGroup.createDataSet(Y_PIXEL_OFFSET, H5VARIABLE, H5SCALAR);
  H5::DataSet detectorName =
      detectorGroup.createDataSet(LOCAL_NAME, H5VARIABLE, H5SCALAR);
  H5::DataSet detectorDependency =
      detectorGroup.createDataSet(DEPENDS_ON, H5VARIABLE, H5SCALAR);

  // attributes
  writeStrAttributeToGroupHelper(pixelGroup, NX_CLASS, NX_OFF_GEOMETRY);
  writeStrAttributeToDataSetHelper(bankLocationInNXformat, NX_CLASS,
                                   NX_TRANSFORMATIONS);
  writeStrAttributeToDataSetHelper(bankOrientationInNXformat, NX_CLASS,
                                   NX_TRANSFORMATIONS);
  writeStrAttributeToDataSetHelper(xPixelOffset, UNITS, METRES);
  writeStrAttributeToDataSetHelper(yPixelOffset, UNITS, METRES);

  // dataset values
  writeStrValueToDataSetHelper(detectorDependency,
                               detectorGroup.getObjName() + "/" + LOCATION);

  /*
 =============================================================================================================

                Source group in tree : @NXsource
                Parent: NXentry

                children:
         => transformations
 =============================================================================================================
 */

  // subgroups
  H5::Group transformationsGroup = sourceGroup.createGroup(TRANSFORMATIONS);

  // datatsets
  H5::DataSet sourceName =
      sourceGroup.createDataSet(NAME, H5VARIABLE, H5SCALAR);
  H5::DataSet sourceDependency =
      sourceGroup.createDataSet(DEPENDS_ON, H5VARIABLE, H5SCALAR);

  // atributes
  writeStrAttributeToGroupHelper(transformationsGroup, NX_CLASS,
                                 NX_TRANSFORMATIONS);

  // dataset Values
  writeStrValueToDataSetHelper(sourceName, SOURCE);
  writeStrValueToDataSetHelper(sourceDependency,
                               sourceGroup.getObjName() + "/" + LOCATION);

  /*
  =============================================================================================================

                Transformations group in tree: @NXtransformations
                Parent: NXsource

                Children:
                => location
  =============================================================================================================
  */

  // datasets
  H5::DataSet sourceLocation =
      transformationsGroup.createDataSet(LOCATION, H5VARIABLE, H5SCALAR);

  // attributes
  writeStrAttributeToDataSetHelper(sourceLocation, NX_CLASS, LOCATION);

  /*
  =============================================================================================================

         parse components in tree structure

  =============================================================================================================
  */

  //  pixel datasets to detector group

  for (int index = ROOT_INDEX; index >= 0; index--) {

    if (compInfo.isDetector(index)) {
      Mantid::Kernel::V3D offset = compInfo.position(index);

      // write  of detector as dataset to NXdetector group

      // write any additional attributes

      // get its location and writed it as a dataset with nxclass
      // NXtransformation. write its attributes including Nxclass
      // NXtransformation do the same with orientation
    }
  }

  file.close();

  // detector number
  hsize_t dims[1] = {13};
  H5::DataSpace space = H5Screate_simple(1, dims, NULL);
  H5::DataSet dset = sampleGroup.createDataSet("test", H5T_NATIVE_INT, space);

  int wdata[13];

  for (int i = 0; i < 13; i++) {
    wdata[i] = i + 1;
  }

 
  dset.write(wdata, H5T_NATIVE_INT, space);


} // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
