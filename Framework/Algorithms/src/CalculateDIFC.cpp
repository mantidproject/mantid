// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateDIFC.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {

namespace {

// calculate DIFC using the geometry and
void calculateFromOffset(API::Progress &progress,
                         DataObjects::SpecialWorkspace2D &outputWs,
                         const DataObjects::OffsetsWorkspace *const offsetsWS,
                         const Geometry::DetectorInfo &detectorInfo) {
  const auto &detectorIDs = detectorInfo.detectorIDs();
  const bool haveOffset = (offsetsWS != nullptr);
  const double l1 = detectorInfo.l1();

  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if ((!detectorInfo.isMasked(i)) && (!detectorInfo.isMonitor(i))) {
      // offset=0 means that geometry is correct
      const double offset =
          (haveOffset) ? offsetsWS->getValue(detectorIDs[i], 0.) : 0.;

      // tofToDSpacingFactor gives 1/DIFC
      double difc =
          1. / Geometry::Conversion::tofToDSpacingFactor(
                   l1, detectorInfo.l2(i), detectorInfo.twoTheta(i), offset);
      outputWs.setValue(detectorIDs[i], difc);
    }

    progress.report("Calculate DIFC");
  }
}

// look through the columns of detid and difc and put it into the
// SpecialWorkspace2D
void calculateFromTable(API::Progress &progress,
                        DataObjects::SpecialWorkspace2D &outputWs,
                        const API::ITableWorkspace &calibWs) {
  API::ConstColumnVector<double> difcCol = calibWs.getVector("difc");
  API::ConstColumnVector<int> detIDs = calibWs.getVector("detid");

  const size_t numRows = detIDs.size();
  for (size_t i = 0; i < numRows; ++i) {
    outputWs.setValue(detIDs[i], difcCol[i]);
    progress.report("Calculate DIFC");
  }
}
} // namespace

namespace Algorithms {

using Mantid::API::MatrixWorkspace;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_const_sptr;
using Mantid::DataObjects::SpecialWorkspace2D;
using Mantid::DataObjects::SpecialWorkspace2D_sptr;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateDIFC)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateDIFC::name() const { return "CalculateDIFC"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculateDIFC::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateDIFC::category() const {
  return "Diffraction\\Utility";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateDIFC::summary() const {
  return "Calculate the DIFC for every pixel";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateDIFC::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the workspace to have DIFC calculated from");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Workspace containing DIFC for each pixel");
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "CalibrationWorkspace", "", Direction::Input,
                      Mantid::API::PropertyMode::Optional),
                  "Optional: A OffsetsWorkspace containing the calibration "
                  "offsets.  This cannot be specified with an "
                  "OffsetsWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                      "OffsetsWorkspace", "", Direction::Input,
                      Mantid::API::PropertyMode::Optional),
                  "Optional: A OffsetsWorkspace containing the calibration "
                  "offsets. This cannot be specified with a "
                  "CalibrationWorkspace.");
}

std::map<std::string, std::string> CalculateDIFC::validateInputs() {
  std::map<std::string, std::string> result;

  OffsetsWorkspace_const_sptr offsetsWS = getProperty("OffsetsWorkspace");
  API::ITableWorkspace_const_sptr calibrationWS =
      getProperty("CalibrationWorkspace");

  if ((bool(offsetsWS)) && (bool(calibrationWS))) {
    std::string msg = "Only specify calibration one way";
    result["OffsetsWorkspace"] = msg;
    result["CalibrationWorkspace"] = msg;
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateDIFC::exec() {

  DataObjects::OffsetsWorkspace_const_sptr offsetsWs =
      getProperty("OffsetsWorkspace");
  API::ITableWorkspace_const_sptr calibWs = getProperty("CalibrationWorkspace");
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  API::MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");

  if ((!bool(inputWs == outputWs)) ||
      (!bool(boost::dynamic_pointer_cast<SpecialWorkspace2D>(outputWs)))) {
    outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
        boost::make_shared<SpecialWorkspace2D>(inputWs->getInstrument()));
    outputWs->setTitle("DIFC workspace");
  }

  // convert to actual type being used
  DataObjects::SpecialWorkspace2D_sptr outputSpecialWs =
      boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(outputWs);

  API::Progress progress(this, 0.0, 1.0, inputWs->getNumberHistograms());
  if (bool(calibWs)) {
    calculateFromTable(progress, *outputSpecialWs, *calibWs);
  } else {
    // this method handles calculating from instrument geometry as well
    const auto &detectorInfo = inputWs->detectorInfo();
    calculateFromOffset(progress, *outputSpecialWs, offsetsWs.get(),
                        detectorInfo);
  }

  setProperty("OutputWorkspace", outputWs);
}

} // namespace Algorithms
} // namespace Mantid
