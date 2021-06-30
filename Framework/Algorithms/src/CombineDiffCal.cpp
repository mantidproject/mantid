// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CombineDiffCal.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
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
      std::make_unique<WorkspaceProperty<DataObjects::Workspace2D>>("CalibrationWorkspace", "", Direction::Input),
      "Workspace where conversion from d-spacing to time-of-flight for each spectrum is determined from. This is the "
      "source of DIFCarb.");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("OutputWorkspace", "", Direction::Output),
      "DiffCal table generated from calibrating grouped spectra");
}

int getTableWorspaceSortDirection(DataObjects::TableWorkspace_sptr ws) {
  Mantid::API::TableRow row = ws->getFirstRow();
  int sortDirection = 1;
  if (ws->rowCount() > 2) {
    Mantid::API::TableRow lastRow = ws->getRow(ws->rowCount() - 1);
    if (row.Int(0) > lastRow.Int(0)) {
      sortDirection = -1;
    }
  }
  return sortDirection;
}

bool isTableWorkspaceSortedById(DataObjects::TableWorkspace_sptr ws) {
  Mantid::API::TableRow row = ws->getFirstRow();
  int sortDirection = getTableWorspaceSortDirection(ws);
  if (ws->rowCount() > 2) {
    row.next();
    Mantid::API::TableRow nextRow = row;
    row = ws->getFirstRow();
    do {
      if (row.Int(0) * sortDirection > nextRow.Int(0) * sortDirection) {
        return false;
      }
    } while (row.next() && nextRow.next());
  }
  return true;
}

std::map<std::string, std::string> CombineDiffCal::validateInputs() {
  std::map<std::string, std::string> results;

  const DataObjects::TableWorkspace_sptr groupedCalibrationWS = getProperty("GroupedCalibration");
  const DataObjects::TableWorkspace_sptr pixelCalibrationWS = getProperty("PixelCalibration");

  if (!isTableWorkspaceSortedById(groupedCalibrationWS)) {
    results["GroupedCalibration"] = "Please run SortTableWorkspace on the table submited as 'GroupCalibration'. ";
  }

  if (!isTableWorkspaceSortedById(pixelCalibrationWS)) {
    results["PixelCalibration"] = "Please run SortTableWorkspace on the table submited as 'PixelCalibration'. ";
  }

  if (getTableWorspaceSortDirection(groupedCalibrationWS) != getTableWorspaceSortDirection(pixelCalibrationWS)) {
    results["GroupedCalibration"] = "'GroupedCalibration's sort direction does not match 'PixelCalibration's sort "
                                    "direction, flip one of their orders.";
    results["PixelCalibration"] = "'PixelCalibration's sort direction does not match 'GroupedCalibration's sort "
                                  "direction, flip one of their orders.";
  }

  return results;
}

std::shared_ptr<Mantid::API::TableRow> binarySearchForRow(DataObjects::TableWorkspace_sptr ws, int detid) {
  size_t start = 0;
  size_t end = ws->rowCount();
  size_t currentPosition = end / 2;
  int sortDirection = getTableWorspaceSortDirection(ws);
  while (end - start > 0) {
    Mantid::API::TableRow currentRow = ws->getRow(currentPosition);
    if (currentRow.Int(0) * sortDirection > detid * sortDirection) {
      end = currentPosition;
    } else if (currentRow.Int(0) * sortDirection < detid * sortDirection) {
      start = currentPosition;
    } else {
      return std::make_shared<Mantid::API::TableRow>(currentRow);
    }
    currentPosition = start + ((end - start) / 2);
  }
  return nullptr;
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
  const DataObjects::Workspace2D_sptr calibrationWS = getProperty("CalibrationWorkspace");
  const DataObjects::TableWorkspace_sptr groupedCalibrationWS = getProperty("GroupedCalibration");
  const DataObjects::TableWorkspace_sptr pixelCalibrationWS = getProperty("PixelCalibration");

  DataObjects::TableWorkspace_sptr outputWorkspace = std::make_shared<DataObjects::TableWorkspace>();
  outputWorkspace->addColumn("int", "detid");
  outputWorkspace->addColumn("double", "difc");
  outputWorkspace->addColumn("double", "difa");
  outputWorkspace->addColumn("double", "tzero");

  Mantid::API::TableRow groupedCalibrationRow = groupedCalibrationWS->getFirstRow();
  do {
    std::shared_ptr<Mantid::API::TableRow> pixelCalibrationRow =
        binarySearchForRow(pixelCalibrationWS, groupedCalibrationRow.Int(0));
    if (pixelCalibrationRow) {
      double value = (groupedCalibrationRow.Double(1) /
                      calibrationWS->spectrumInfo().diffractometerConstants(calibrationWS->getIndicesFromDetectorIDs(
                          {pixelCalibrationRow->Int(0)})[0])[Kernel::UnitParams::difc]) *
                     pixelCalibrationRow->Double(1);

      Mantid::API::TableRow newRow = outputWorkspace->appendRow();
      newRow << pixelCalibrationRow->Int(0) << value << 0.0 << 0.0;
    }
  } while (groupedCalibrationRow.next());

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
