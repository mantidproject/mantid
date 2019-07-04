// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

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
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_SOURCE = "NXsource";
const std::string SHAPE = "shape";
const std::string DEPENDS_ON = "depends_on";

// NEXUS COMPLIANT ATTRIBUTE VALUES
const std::string NX_TRANSFORMATION = "NXtransformation";
const std::string NX_CHAR = "NX_CHAR";

// COMPONENT TYPES
auto DETECTOR = Mantid::Beamline::ComponentType(6);

} // namespace

void writeStrAttributeToGroup(H5::Group &grp, const std::string &NX_class,
                              const std::string &attrVal) {
  H5::StrType attrType(0, H5T_VARIABLE);
  H5::DataSpace attrSpace(H5S_SCALAR);
  H5::Attribute attribute = grp.createAttribute(NX_class, attrType, attrSpace);
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

  H5::H5File file(fullPath, H5F_ACC_TRUNC);

  H5::Group parentGroup = file.createGroup("/raw_data_1");
  H5::Group instrumentGroup = parentGroup.createGroup("instrument");
  H5::Group sourceGroup = instrumentGroup.createGroup("source");
  H5::Group sampleGroup = parentGroup.createGroup("sample");

  writeStrAttributeToGroup(parentGroup, NX_CLASS, NX_ENTRY);
  writeStrAttributeToGroup(instrumentGroup, NX_CLASS, NX_INSTRUMENT);
  writeStrAttributeToGroup(sourceGroup, NX_CLASS, NX_SOURCE);
  writeStrAttributeToGroup(sampleGroup, NX_CLASS, NX_SAMPLE);

  const size_t ROOT_INDEX = compInfo.root();
  const size_t SAMPLE_INDEX = compInfo.sample();
  const int SOURCE_INDEX = compInfo.source();

  for (size_t index = 0; index <= ROOT_INDEX; ++index) {

    if (compInfo.isDetector(index)) {
      // write to file a group in instrument named detetctor_[index]
      H5::Group detectorGroup =
          instrumentGroup.createGroup("detector_" + std::to_string(index));

      writeStrAttributeToGroup(detectorGroup, NX_CLASS, NX_DETECTOR);

      // write location of detector as dataset to detector group
      H5::DataSet location;

      // write any additional attributes

      // get its location and writed it as a dataset with nxclass
      // NXtransformation. write its attributes including Nxclass
      // NXtransformation do the same with orientation
    }
  }

  // create DataSet 'data' in instrument.

  H5::StrType dataType(0, H5T_VARIABLE);
  H5::DataSpace dataSpace(H5S_SCALAR);

  std::string dataSetData = "test_data_for_instrument";
  auto dataSetDataAsCStr = dataSetData.c_str();

  H5::DSetCreatPropList plist;
  plist.setFillValue(dataType, &dataSetDataAsCStr);

  // add dataset to child group 'instrument'
  H5::DataSet dataSet = instrumentGroup.createDataSet(
      compInfo.name(ROOT_INDEX), dataType, dataSpace,
      plist); // name of dataset initialised with fill value.

  writeStrAttributeToDataSet(dataSet, SHORT_NAME,
                             compInfo.name(compInfo.root()));
  writeStrAttributeToDataSet(dataSet, NX_CLASS,
                             NX_CHAR); // add NX_class attribute to dataset

  file.close();

} // saveInstrument

// returns coumpound transform of rotation about one axis and translation.

} // namespace NexusGeometry
} // namespace Mantid
