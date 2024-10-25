// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateDIFC.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string CALIB_WKSP("CalibrationWorkspace");
const std::string OFFSTS_WKSP("OffsetsWorkspace");
const std::string OFFSET_MODE("OffsetMode");
const std::string BINWIDTH("BinWidth");
} // namespace PropertyNames
namespace {

enum class OffsetMode { RELATIVE_OFFSET, ABSOLUTE_OFFSET, SIGNED_OFFSET, enum_count };
const std::vector<std::string> offsetModeNames{"Relative", "Absolute", "Signed"};
typedef Mantid::Kernel::EnumeratedString<OffsetMode, &offsetModeNames> OFFSETMODE;

/** Calculate the DIFC values and write them to OutputWorkspace
 *
 * @param progress :: progress indicator
 * @param outputWs :: OutputWorkspace for DIFC Values
 * @param offsetsWS :: Offset Workspace used to calculate DIFC
 * @param detectorInfo :: Detector Info we are using
 * @param binWidth :: binWidth used for logarithmically binned data
 * @param offsetMode :: indicates which offset mode to use ('Relative', 'Absolute', 'Signed')
 */
void calculateFromOffset(API::Progress &progress, DataObjects::SpecialWorkspace2D &outputWs,
                         const DataObjects::OffsetsWorkspace *const offsetsWS,
                         const Geometry::DetectorInfo &detectorInfo, double binWidth, OFFSETMODE offsetMode) {
  const auto &detectorIDs = detectorInfo.detectorIDs();
  const bool haveOffset = (offsetsWS != nullptr);
  const double l1 = detectorInfo.l1();

  std::function<double(size_t const &, double const &)> difc_for_offset_mode;
  if (offsetMode == OffsetMode::SIGNED_OFFSET) {
    difc_for_offset_mode = [l1, detectorInfo, binWidth](size_t const &i, double const &offset) {
      return Geometry::Conversion::calculateDIFCCorrection(l1, detectorInfo.l2(i), detectorInfo.twoTheta(i), offset,
                                                           binWidth);
    };
  } else {
    difc_for_offset_mode = [l1, detectorInfo](size_t const &i, double const &offset) {
      return 1. / Geometry::Conversion::tofToDSpacingFactor(l1, detectorInfo.l2(i), detectorInfo.twoTheta(i), offset);
    };
  }

  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if ((!detectorInfo.isMasked(i)) && (!detectorInfo.isMonitor(i))) {
      // offset=0 means that geometry is correct
      const double offset = (haveOffset ? offsetsWS->getValue(detectorIDs[i], 0.) : 0.);
      outputWs.setValue(detectorIDs[i], difc_for_offset_mode(i, offset));
    }

    progress.report("Calculate DIFC");
  }
}

// look through the columns of detid and difc and copy them into the
// SpecialWorkspace2D
void calculateFromTable(API::Progress &progress, DataObjects::SpecialWorkspace2D &outputWs,
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
const std::string CalculateDIFC::category() const { return "Diffraction\\Utility"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateDIFC::summary() const { return "Calculate the DIFC for every pixel"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateDIFC::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
                  "Name of the workspace to have DIFC calculated from");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "Workspace containing DIFC for each pixel");
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      PropertyNames::CALIB_WKSP, "", Direction::Input, Mantid::API::PropertyMode::Optional),
                  "Optional: A TableWorkspace containing the DIFC values, "
                  "which will be copied. This property cannot be set in "
                  "conjunction with property OffsetsWorkspace.");

  declareProperty(std::make_unique<Mantid::Kernel::EnumeratedStringProperty<OffsetMode, &offsetModeNames>>(
                      PropertyNames::OFFSET_MODE),
                  "Optional: Whether to calculate a relative, absolute, or signed offset.  Default relative");

  declareProperty(PropertyNames::BINWIDTH, EMPTY_DBL(),
                  "Optional: The bin width of the X axis.  If using 'Signed' OffsetMode, this value is mandatory");
  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                      PropertyNames::OFFSTS_WKSP, "", Direction::Input, Mantid::API::PropertyMode::Optional),
                  "Optional: A OffsetsWorkspace containing the calibration "
                  "offsets. This property cannot be set in conjunction with "
                  "property CalibrationWorkspace.");
}

std::map<std::string, std::string> CalculateDIFC::validateInputs() {
  std::map<std::string, std::string> result;

  OffsetsWorkspace_const_sptr offsetsWS = getProperty(PropertyNames::OFFSTS_WKSP);
  API::ITableWorkspace_const_sptr calibrationWS = getProperty(PropertyNames::CALIB_WKSP);

  if ((bool(offsetsWS)) && (bool(calibrationWS))) {
    std::string msg = "Only specify calibration one way";
    result[PropertyNames::OFFSTS_WKSP] = msg;
    result[PropertyNames::CALIB_WKSP] = msg;
  }

  OFFSETMODE offsetMode = std::string(getProperty(PropertyNames::OFFSET_MODE));
  if (isDefault(PropertyNames::BINWIDTH) && (offsetMode == OffsetMode::SIGNED_OFFSET)) {
    std::string msg = "Signed offset mode requires bin width to be specified.";
    result[PropertyNames::BINWIDTH] = msg;
    result[PropertyNames::OFFSET_MODE] = msg;
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateDIFC::exec() {

  DataObjects::OffsetsWorkspace_const_sptr offsetsWs = getProperty(PropertyNames::OFFSTS_WKSP);
  API::ITableWorkspace_const_sptr calibWs = getProperty(PropertyNames::CALIB_WKSP);
  API::MatrixWorkspace_sptr inputWs = getProperty(PropertyNames::INPUT_WKSP);
  API::MatrixWorkspace_sptr outputWs = getProperty(PropertyNames::OUTPUT_WKSP);
  double binWidth = getProperty(PropertyNames::BINWIDTH);

  if ((!bool(inputWs == outputWs)) ||
      // SpecialWorkspace2D is a Workspace2D where each spectrum
      // has one detector pixel, one X-value, and one Y-value.
      (!bool(std::dynamic_pointer_cast<SpecialWorkspace2D>(outputWs)))) {
    outputWs =
        std::dynamic_pointer_cast<MatrixWorkspace>(std::make_shared<SpecialWorkspace2D>(inputWs->getInstrument()));
    outputWs->setTitle("DIFC workspace");
  }

  // convert to actual type being used
  DataObjects::SpecialWorkspace2D_sptr outputSpecialWs =
      std::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(outputWs);

  API::Progress progress(this, 0.0, 1.0, inputWs->getNumberHistograms());
  if (bool(calibWs)) {
    calculateFromTable(progress, *outputSpecialWs, *calibWs);
  } else {
    // this method handles calculating from instrument geometry as well,
    // and even when OffsetsWorkspace hasn't been set
    const auto &detectorInfo = inputWs->detectorInfo();
    OFFSETMODE offsetMode = std::string(getProperty(PropertyNames::OFFSET_MODE));
    calculateFromOffset(progress, *outputSpecialWs, offsetsWs.get(), detectorInfo, binWidth, offsetMode);
  }

  setProperty(PropertyNames::OUTPUT_WKSP, outputWs);
}

} // namespace Algorithms
} // namespace Mantid
