// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveDspacemap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/System.h"
#include <fstream>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveDspacemap)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveDspacemap::init() {
  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input OffsetsWorkspace to save.");

  declareProperty(std::make_unique<FileProperty>("DspacemapFile", "",
                                                 FileProperty::Save, ".dat"),
                  "The DspacemapFile on output contains the d-space mapping");

  declareProperty("PadDetID", 300000, "Pad Data to this number of pixels");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveDspacemap::exec() {
  Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS =
      getProperty("InputWorkspace");
  std::string filename = getPropertyValue("DspacemapFile");
  CalculateDspaceFromCal(offsetsWS, filename);
}

//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 *
 * @param DFileName name of dspacemap file
 * @param offsetsWS :: OffsetsWorkspace with instrument and offsets
 */
void SaveDspacemap::CalculateDspaceFromCal(
    Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS,
    std::string DFileName) {
  const char *filename = DFileName.c_str();
  // Get a pointer to the instrument contained in the workspace
  Instrument_const_sptr instrument = offsetsWS->getInstrument();
  const auto &detectorInfo = offsetsWS->detectorInfo();
  double l1;
  Kernel::V3D beamline, samplePos;
  double beamline_norm;
  instrument->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

  // To get all the detector ID's
  detid2det_map allDetectors;
  instrument->getDetectors(allDetectors);

  detid2det_map::const_iterator it;
  detid_t maxdetID = 0;
  for (it = allDetectors.begin(); it != allDetectors.end(); ++it) {
    detid_t detectorID = it->first;
    if (detectorID > maxdetID)
      maxdetID = detectorID;
  }

  detid_t paddetID = detid_t(getProperty("PadDetID"));
  if (maxdetID < paddetID)
    maxdetID = paddetID;

  // Now write the POWGEN-style Dspace mapping file
  std::ofstream fout(filename, std::ios_base::out | std::ios_base::binary);
  Progress prog(this, 0.0, 1.0, maxdetID);

  for (detid_t i = 0; i != maxdetID; i++) {
    // Compute the factor
    double factor;
    Geometry::IDetector_const_sptr det;
    // Find the detector with that detector id
    it = allDetectors.find(i);
    if (it != allDetectors.end()) {
      det = it->second;
      const auto detectorIndex = detectorInfo.indexOf(i);
      factor = Mantid::Geometry::Conversion::tofToDSpacingFactor(
          l1, detectorInfo.l2(detectorIndex),
          detectorInfo.twoTheta(detectorIndex), offsetsWS->getValue(i, 0.0));
      // Factor of 10 between ISAW and Mantid
      factor *= 0.1;
      if (factor < 0)
        factor = 0.0;
      fout.write(reinterpret_cast<char *>(&factor), sizeof(double));
    } else {
      factor = 0;
      fout.write(reinterpret_cast<char *>(&factor), sizeof(double));
    }
    // Report progress
    prog.report();
  }
  fout.close();
}

} // namespace DataHandling
} // namespace Mantid
