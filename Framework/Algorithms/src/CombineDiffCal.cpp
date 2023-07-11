// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CombineDiffCal.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid::Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CombineDiffCal)

//----------------------------------------------------------------------------------------------

namespace { // anonymous namespace
namespace ColNames {
const std::string DETID("detid");
const std::string DIFC("difc");
const std::string DIFA("difa");
const std::string TZERO("tzero");
} // namespace ColNames

namespace PropertyNames {
const std::string PIXEL_CALIB("PixelCalibration");
const std::string GROUP_CALIB("GroupedCalibration");
const std::string ARB_CALIB("CalibrationWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string MASK_WKSP("MaskWorkspace");
} // namespace PropertyNames
} // anonymous namespace

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CombineDiffCal::name() const { return "CombineDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int CombineDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CombineDiffCal::category() const { return "Diffraction\\Utility"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CombineDiffCal::summary() const {
  return "Combine a per-pixel calibration with a grouped spectrum calibration";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CombineDiffCal::init() {
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>(PropertyNames::PIXEL_CALIB, "",
                                                                                   Direction::Input),
                  "OffsetsWorkspace generated from cross-correlation. This is the source of DIFCpixel.");
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>(PropertyNames::GROUP_CALIB, "",
                                                                                   Direction::Input),
                  "DiffCal table generated from calibrating grouped spectra. This is the source of DIFCgroup.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::ARB_CALIB, "", Direction::Input),
      "Workspace where conversion from d-spacing to time-of-flight for each spectrum is determined from. This is the "
      "source of DIFCarb.");
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>(PropertyNames::OUTPUT_WKSP, "",
                                                                                   Direction::Output),
                  "DiffCal table generated from calibrating grouped spectra");
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
                      PropertyNames::MASK_WKSP, "", Direction::Input, API::PropertyMode::Optional),
                  "MaskedWorkspace for PixelCalibration");
}

/// sort the calibration table according increasing values in column "detid"
API::ITableWorkspace_sptr CombineDiffCal::sortTableWorkspace(DataObjects::TableWorkspace_sptr &table) {
  auto alg = createChildAlgorithm("SortTableWorkspace");
  alg->setLoggingOffset(1);
  alg->setProperty("InputWorkspace", table);
  alg->setProperty("OutputWorkspace", table);
  alg->setProperty("Columns", ColNames::DETID);
  alg->executeAsChildAlg();

  return alg->getProperty("OutputWorkspace");
}

bool findColumn(const std::vector<std::string> &columnNames, const std::string &name) {
  return std::find(columnNames.begin(), columnNames.end(), name) != columnNames.end();
}

std::string generateErrorString(const DataObjects::TableWorkspace_sptr &ws) {
  const std::vector<std::string> columnNames = ws->getColumnNames();

  std::stringstream error;
  if (!findColumn(columnNames, ColNames::DETID))
    error << ColNames::DETID << " ";
  if (!findColumn(columnNames, ColNames::DIFC))
    error << ColNames::DIFC << " ";
  if (!findColumn(columnNames, ColNames::DIFA))
    error << ColNames::DIFA << " ";
  if (!findColumn(columnNames, ColNames::TZERO))
    error << ColNames::TZERO << " ";

  return error.str();
}

std::map<std::string, std::string> CombineDiffCal::validateInputs() {
  std::map<std::string, std::string> results;

  const DataObjects::TableWorkspace_sptr groupedCalibrationWS = getProperty(PropertyNames::GROUP_CALIB);
  const DataObjects::TableWorkspace_sptr pixelCalibrationWS = getProperty(PropertyNames::PIXEL_CALIB);

  const auto groupedResult = generateErrorString(groupedCalibrationWS);
  if (!groupedResult.empty())
    results[PropertyNames::GROUP_CALIB] = "The GroupedCalibration Workspace is missing [ " + groupedResult + "]";

  const auto pixelResult = generateErrorString(pixelCalibrationWS);
  if (!pixelResult.empty())
    results[PropertyNames::PIXEL_CALIB] = "The PixelCalibration Workspace is missing [ " + pixelResult + "]";

  return results;
}

std::shared_ptr<Mantid::API::TableRow> binarySearchForRow(const API::ITableWorkspace_sptr &ws, int detid) {
  size_t start = 0;
  size_t end = ws->rowCount() - 1;
  while (end >= start) {
    size_t currentPosition = start + ((end - start) / 2);

    Mantid::API::TableRow currentRow = ws->getRow(currentPosition);
    if (currentRow.Int(0) > detid) {
      end = currentPosition - 1;
    } else if (currentRow.Int(0) < detid) {
      start = currentPosition + 1;
    } else {
      return std::make_shared<Mantid::API::TableRow>(currentRow);
    }
  }
  return nullptr;
}

void addRowFromGroupedCalibration(const DataObjects::TableWorkspace_sptr &ws, Mantid::API::TableRow row) {
  Mantid::API::TableRow newRow = ws->appendRow();
  newRow << row.Int(0) << row.Double(1) << row.Double(2) << row.Double(3);
}

// Per Pixel:
//
// DIFC{eff} = (DIFC{pd}/DIFC{arb}) * DIFC{prev}
//
// DIFC{eff} = Output of this Alg, the combined DIFC
// DIFC{pd} = The DIFC produced by PDCalibration, found in the "GroupedCalibration"
// DIFC{arb} = found in the "CalibrationWorkspace" param
// DIFC{prev} = The previous DIFCs, found in "PixelCalibration", as per description this was the set generated by CC

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CombineDiffCal::exec() {
  const API::MatrixWorkspace_sptr calibrationWS = getProperty(PropertyNames::ARB_CALIB);

  DataObjects::TableWorkspace_sptr presortedGroupedCalibrationWS = getProperty(PropertyNames::GROUP_CALIB);
  const API::ITableWorkspace_sptr groupedCalibrationWS = sortTableWorkspace(presortedGroupedCalibrationWS);

  DataObjects::TableWorkspace_sptr presortedPixelCalibrationWS = getProperty(PropertyNames::PIXEL_CALIB);
  const API::ITableWorkspace_sptr pixelCalibrationWS = sortTableWorkspace(presortedPixelCalibrationWS);

  const DataObjects::MaskWorkspace_sptr maskWorkspace = getProperty(PropertyNames::MASK_WKSP);

  DataObjects::TableWorkspace_sptr outputWorkspace = std::make_shared<DataObjects::TableWorkspace>();
  outputWorkspace->addColumn("int", ColNames::DETID);
  outputWorkspace->addColumn("double", ColNames::DIFC);
  outputWorkspace->addColumn("double", ColNames::DIFA);
  outputWorkspace->addColumn("double", ColNames::TZERO);

  Mantid::API::TableRow groupedCalibrationRow = groupedCalibrationWS->getFirstRow();
  do {
    int detid = groupedCalibrationRow.Int(0);
    bool prevDifValsExist = false;

    if (!(maskWorkspace && maskWorkspace->isMasked(detid))) {
      std::shared_ptr<Mantid::API::TableRow> pixelCalibrationRow = binarySearchForRow(pixelCalibrationWS, detid);
      if (pixelCalibrationRow) {
        const double difcPD = groupedCalibrationRow.Double(1);
        const double difcArb = calibrationWS->spectrumInfo().diffractometerConstants(
            calibrationWS->getIndicesFromDetectorIDs({detid})[0])[Kernel::UnitParams::difc];
        const double difcPrev = pixelCalibrationRow->Double(1);
        const double difaPrev = pixelCalibrationRow->Double(2);

        const double difcNew = (difcPD / difcArb) * difcPrev;
        const double difaNew = ((difcPD / difcArb) * (difcPD / difcArb)) * difaPrev;

        const double tzeroNew = pixelCalibrationRow->Double(3);

        Mantid::API::TableRow newRow = outputWorkspace->appendRow();
        newRow << detid << difcNew << difaNew << tzeroNew;
        prevDifValsExist = true;
      }
    }

    if (!prevDifValsExist) {
      // copy from group
      addRowFromGroupedCalibration(outputWorkspace, groupedCalibrationRow);
    }
  } while (groupedCalibrationRow.next());

  setProperty(PropertyNames::OUTPUT_WKSP, outputWorkspace);
}

} // namespace Mantid::Algorithms
