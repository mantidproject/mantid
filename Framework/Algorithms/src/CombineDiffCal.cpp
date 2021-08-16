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

namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CombineDiffCal)

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
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("PixelCalibration", "", Direction::Input),
      "OffsetsWorkspace generated from cross-correlation. This is the source of DIFCpixel.");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("GroupedCalibration", "", Direction::Input),
      "DiffCal table generated from calibrating grouped spectra. This is the source of DIFCgroup.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("CalibrationWorkspace", "", Direction::Input),
      "Workspace where conversion from d-spacing to time-of-flight for each spectrum is determined from. This is the "
      "source of DIFCarb.");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("OutputWorkspace", "", Direction::Output),
      "DiffCal table generated from calibrating grouped spectra");
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>("MaskWorkspace", "", Direction::Input,
                                                                                  API::PropertyMode::Optional),
                  "MaskedWorkspace for PixelCalibration");
}

/// sort the calibration table according increasing values in column "detid"
API::ITableWorkspace_sptr CombineDiffCal::sortTableWorkspace(DataObjects::TableWorkspace_sptr &table) {
  auto alg = createChildAlgorithm("SortTableWorkspace");
  alg->setLoggingOffset(1);
  alg->setProperty("InputWorkspace", table);
  alg->setProperty("OutputWorkspace", table);
  alg->setProperty("Columns", "detid");
  alg->executeAsChildAlg();

  return alg->getProperty("OutputWorkspace");
}

bool findColumn(const std::vector<std::string> &columnNames, const std::string &name) {
  return std::find(columnNames.begin(), columnNames.end(), name) != columnNames.end();
}

std::string generateErrorString(const DataObjects::TableWorkspace_sptr ws) {
  const std::vector<std::string> columnNames = ws->getColumnNames();

  std::stringstream error;
  if (!findColumn(columnNames, "detid"))
    error << "detid ";
  if (!findColumn(columnNames, "difc"))
    error << "difc ";
  if (!findColumn(columnNames, "difa"))
    error << "difa ";
  if (!findColumn(columnNames, "tzero"))
    error << "tzero ";

  return error.str();
}

std::map<std::string, std::string> CombineDiffCal::validateInputs() {
  std::map<std::string, std::string> results;

  const DataObjects::TableWorkspace_sptr groupedCalibrationWS = getProperty("GroupedCalibration");
  const DataObjects::TableWorkspace_sptr pixelCalibrationWS = getProperty("PixelCalibration");

  const auto groupedResult = generateErrorString(groupedCalibrationWS);
  if (!groupedResult.empty())
    results["GroupedCalibration"] = "The GroupedCalibration Workspace is missing [ " + groupedResult + "]";

  const auto pixelResult = generateErrorString(pixelCalibrationWS);
  if (!pixelResult.empty())
    results["PixelCalibration"] = "The PixelCalibration Workspace is missing [ " + pixelResult + "]";

  return results;
}

std::shared_ptr<Mantid::API::TableRow> binarySearchForRow(API::ITableWorkspace_sptr ws, int detid) {
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

void addRowFromGroupedCalibration(DataObjects::TableWorkspace_sptr ws, Mantid::API::TableRow row) {
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
  const API::MatrixWorkspace_sptr calibrationWS = getProperty("CalibrationWorkspace");

  DataObjects::TableWorkspace_sptr presortedGroupedCalibrationWS = getProperty("GroupedCalibration");
  const API::ITableWorkspace_sptr groupedCalibrationWS = sortTableWorkspace(presortedGroupedCalibrationWS);

  DataObjects::TableWorkspace_sptr presortedPixelCalibrationWS = getProperty("PixelCalibration");
  const API::ITableWorkspace_sptr pixelCalibrationWS = sortTableWorkspace(presortedPixelCalibrationWS);

  const DataObjects::MaskWorkspace_sptr maskWorkspace = getProperty("MaskWorkspace");

  DataObjects::TableWorkspace_sptr outputWorkspace = std::make_shared<DataObjects::TableWorkspace>();
  outputWorkspace->addColumn("int", "detid");
  outputWorkspace->addColumn("double", "difc");
  outputWorkspace->addColumn("double", "difa");
  outputWorkspace->addColumn("double", "tzero");

  Mantid::API::TableRow groupedCalibrationRow = groupedCalibrationWS->getFirstRow();
  do {
    int detid = groupedCalibrationRow.Int(0);
    bool prevDifValsExist = false;

    if (!(maskWorkspace && maskWorkspace->isMasked(detid))) {
      std::shared_ptr<Mantid::API::TableRow> pixelCalibrationRow = binarySearchForRow(pixelCalibrationWS, detid);
      if (pixelCalibrationRow) {
        double difcPD = groupedCalibrationRow.Double(1);
        double difcArb = calibrationWS->spectrumInfo().diffractometerConstants(
            calibrationWS->getIndicesFromDetectorIDs({detid})[0])[Kernel::UnitParams::difc];
        double difcPrev = pixelCalibrationRow->Double(1);
        double difaPrev = pixelCalibrationRow->Double(2);

        double difcNew = (difcPD / difcArb) * difcPrev;
        double difaNew = ((difcPD / difcArb) * (difcPD / difcArb)) * difaPrev;

        double tzeroNew = pixelCalibrationRow->Double(3);

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

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
