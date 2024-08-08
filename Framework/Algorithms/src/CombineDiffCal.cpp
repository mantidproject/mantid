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

int getDetId(const std::shared_ptr<const API::Column> &detIdColumn, const std::size_t rowNum) {
  return static_cast<int>((*detIdColumn)[rowNum]);
}
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
                  "DiffCal TableWorkspace that will be updated. This is often generated from cross-correlation. "
                  "These are the \"prev\" values in the documentation.");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>(PropertyNames::GROUP_CALIB, "",
                                                                       Direction::Input),
      "DiffCal table generated from calibrating grouped spectra. This is the \"DIFCpd\" value in the documentation.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::ARB_CALIB, "", Direction::Input),
      "Workspace where conversion from d-spacing to time-of-flight for each spectrum is determined from. "
      "This is the \"DIFCarb\" value in the documentation.");
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>(PropertyNames::OUTPUT_WKSP, "",
                                                                                   Direction::Output),
                  "DiffCal table generated from calibrating grouped spectra");
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
                      PropertyNames::MASK_WKSP, "", Direction::Input, API::PropertyMode::Optional),
                  "MaskedWorkspace for PixelCalibration");
}

/// sort the calibration table according increasing values in column "detid"
DataObjects::TableWorkspace_sptr CombineDiffCal::sortTableWorkspace(DataObjects::TableWorkspace_sptr &table) {
  auto alg = createChildAlgorithm("SortTableWorkspace");
  alg->setLoggingOffset(1);
  alg->setProperty("InputWorkspace", table);
  alg->setProperty("OutputWorkspace", table);
  alg->setProperty("Columns", ColNames::DETID);
  alg->executeAsChildAlg();

  API::ITableWorkspace_sptr output = alg->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<DataObjects::TableWorkspace>(output);
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
  Mantid::API::MatrixWorkspace_sptr calibrationWS = getProperty(PropertyNames::ARB_CALIB);

  const auto groupedResult = generateErrorString(groupedCalibrationWS);
  if (!groupedResult.empty()) {
    results[PropertyNames::GROUP_CALIB] = "The GroupedCalibration Workspace is missing [ " + groupedResult + "]";
  } else {
    // the equations haven't been derived for difa != tzero != 0
    std::stringstream msg("");
    const auto difa = groupedCalibrationWS->getColumn(ColNames::DIFA);
    const auto tzero = groupedCalibrationWS->getColumn(ColNames::TZERO);
    const auto numRows = groupedCalibrationWS->rowCount();
    for (std::size_t i = 0; i < numRows; ++i) {
      if (difa->toDouble(i) != 0.) {
        msg << "found nonzero difa in row " << i;
        break;
      } else if (tzero->toDouble(i) != 0.) {
        msg << "found nonzero tzero in row " << i;
        break;
      }
    }
    if (!msg.str().empty()) {
      results[PropertyNames::GROUP_CALIB] = msg.str();
    }
  }

  const auto pixelResult = generateErrorString(pixelCalibrationWS);
  if (!pixelResult.empty())
    results[PropertyNames::PIXEL_CALIB] = "The PixelCalibration Workspace is missing [ " + pixelResult + "]";

  // Ensure compatible detector IDs in tables
  // Ensure all detids in GroupedCalibration(detid_pd) and PixelCalibration(detid_prev)
  // can be found in CalibrationWorkspace (detid_arb)

  // put all detector IDs into sets, for easier searching
  auto prev_col = pixelCalibrationWS->getColVector<detid_t>(ColNames::DETID);
  auto pd_col = groupedCalibrationWS->getColVector<detid_t>(ColNames::DETID);
  auto arb_col = calibrationWS->detectorInfo().detectorIDs();
  std::set<detid_t> detid_pd(std::make_move_iterator(pd_col.begin()), std::make_move_iterator(pd_col.end()));
  std::set<detid_t> detid_prev(std::make_move_iterator(prev_col.begin()), std::make_move_iterator(prev_col.end()));
  std::set<detid_t> detid_arb(std::make_move_iterator(arb_col.begin()), std::make_move_iterator(arb_col.end()));

  std::set<std::string> unmatched;
  // check that all detector IDs from PixelCalibration are present in CalibrationWorkspace
  if (!std::includes(detid_arb.begin(), detid_arb.end(), detid_prev.begin(), detid_prev.end())) {
    unmatched.insert(PropertyNames::PIXEL_CALIB);
    unmatched.insert(PropertyNames::ARB_CALIB);
  }
  // check that all detector IDs from GroupedCalibration are present in CalibrationWorkspace
  if (!std::includes(detid_arb.begin(), detid_arb.end(), detid_pd.begin(), detid_pd.end())) {
    unmatched.insert(PropertyNames::GROUP_CALIB);
    unmatched.insert(PropertyNames::ARB_CALIB);
  }
  // if any workspaces do not match, throw an error
  if (!unmatched.empty()) {
    std::stringstream msg("");
    msg << "Inconsistent detector IDs between";
    for (std::string const &x : unmatched) {
      msg << " " << x;
    }
    msg << ".";
    for (std::string const &x : unmatched) {
      results[x] = msg.str();
    }
  }

  return results;
}

std::shared_ptr<Mantid::API::TableRow> binarySearchForRow(const API::ITableWorkspace_sptr &ws, const int detid,
                                                          const size_t lastStart) {
  size_t start = lastStart;
  size_t end = ws->rowCount() - 1;

  // looking at the detid column is faster
  std::shared_ptr<const API::Column> detIdColumn = ws->getColumn(ColNames::DETID);

  // since both tables are already sorted, linear search the first two rows
  if (getDetId(detIdColumn, start) == detid) {
    return std::make_shared<Mantid::API::TableRow>(ws->getRow(start));
  } else {
    start += 1; // advance to the second position
    if ((end >= start) && (getDetId(detIdColumn, start) == detid)) {
      return std::make_shared<Mantid::API::TableRow>(ws->getRow(start));
    } else {
      start += 1; // don't need to include this index in the bisect
    }
  }

  // do bisecting search
  while (end >= start) {
    const size_t currentPosition = start + ((end - start) / 2);

    const auto detIdInRow = getDetId(detIdColumn, currentPosition);
    if (detIdInRow > detid) {
      end = currentPosition - 1;
    } else if (detIdInRow < detid) {
      start = currentPosition + 1;
    } else {
      return std::make_shared<Mantid::API::TableRow>(ws->getRow(currentPosition));
    }
  }

  // did not find the row
  return nullptr;
}

void addRowFromPreviousCalibration(DataObjects::TableWorkspace_sptr &ws,
                                   const DataObjects::TableWorkspace_sptr &previousTable, const std::size_t rowNum) {
  // get the old values
  const int detid = previousTable->cell_cast<int>(rowNum, 0);
  const double difc = previousTable->cell_cast<double>(rowNum, 1);
  const double difa = previousTable->cell_cast<double>(rowNum, 2);
  const double tzero = previousTable->cell_cast<double>(rowNum, 3);

  // copy to new table workspace
  Mantid::API::TableRow newRow = ws->appendRow();
  newRow << detid << difc << difa << tzero;
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
  // read in properties and sort copies of the tables
  const API::MatrixWorkspace_sptr calibrationWS = getProperty(PropertyNames::ARB_CALIB);

  DataObjects::TableWorkspace_sptr presortedGroupedCalibrationWS = getProperty(PropertyNames::GROUP_CALIB);
  const auto groupedCalibrationWS = sortTableWorkspace(presortedGroupedCalibrationWS);

  DataObjects::TableWorkspace_sptr presortedPixelCalibrationWS = getProperty(PropertyNames::PIXEL_CALIB);
  const auto pixelCalibrationWS = sortTableWorkspace(presortedPixelCalibrationWS);

  const DataObjects::MaskWorkspace_sptr maskWorkspace = getProperty(PropertyNames::MASK_WKSP);

  // make the output workspace
  DataObjects::TableWorkspace_sptr outputWorkspace = std::make_shared<DataObjects::TableWorkspace>();
  outputWorkspace->addColumn("int", ColNames::DETID);
  outputWorkspace->addColumn("double", ColNames::DIFC);
  outputWorkspace->addColumn("double", ColNames::DIFA);
  outputWorkspace->addColumn("double", ColNames::TZERO);

  // keep track of which ids were already added to the output
  std::set<int> detidsInGrpCalib;

  API::Progress prog(this, 0.0, 1.0, groupedCalibrationWS->rowCount());

  // cache values from calibrationWS since getting DIFC can be expensive
  std::map<std::size_t, double> difcArbMap;

  // searching for the row in the pixel calibration table can be expensive
  // this variable is used to keep track of how far has already been processed
  // in order to reduce the search space
  std::size_t lastStart = 0;

  // get a map from detector ID to workspace index in the calibration WS
  // there is no guarantee detectorIDs in GroupedWorkspace will be contiguous,
  // and therefore a map and not a vector is necessary.
  auto wkspIndices = calibrationWS->getDetectorIDToWorkspaceIndexMap();

  // access the spectrum info outside of the loop
  // this prevents constantly re-calling the method,
  // which can have some O(N) behavior
  auto spectrumInfo = calibrationWS->spectrumInfo();

  // loop through all rows in the grouped calibration table
  // this will calculate an updated row or copy the row if it is missing from the pixel calibration
  Mantid::API::TableRow groupedCalibrationRow = groupedCalibrationWS->getFirstRow();
  do {
    const int detid = groupedCalibrationWS->cell_cast<int>(groupedCalibrationRow.row(), 0);
    detidsInGrpCalib.insert(detid);
    bool prevDifValsExist = false;

    if (!(maskWorkspace && maskWorkspace->isMasked(detid))) {
      std::shared_ptr<Mantid::API::TableRow> pixelCalibrationRow =
          binarySearchForRow(pixelCalibrationWS, detid, lastStart);
      if (pixelCalibrationRow) {
        // value from groupedCalibrationRow
        const double difcPD = groupedCalibrationWS->cell_cast<double>(groupedCalibrationRow.row(), 1);

        // get workspace index from the map
        const auto wkspIndex = wkspIndices[detid];

        double difcArb;
        const auto difcArbIter = difcArbMap.find(wkspIndex);
        if (difcArbIter == difcArbMap.end()) {
          difcArb = spectrumInfo.diffractometerConstants(wkspIndex)[Kernel::UnitParams::difc];
          difcArbMap[wkspIndex] = difcArb;
        } else {
          difcArb = difcArbIter->second;
        }

        // difc and difa values from pixelCalibrationWS
        const double difcPrev = pixelCalibrationWS->cell_cast<double>(pixelCalibrationRow->row(), 1);
        const double difaPrev = pixelCalibrationWS->cell_cast<double>(pixelCalibrationRow->row(), 2);

        // calculate new difc and difa
        const double difcNew = (difcPD / difcArb) * difcPrev;
        const double difaNew = ((difcPD / difcArb) * (difcPD / difcArb)) * difaPrev;

        // copy tzero from pixelCalibrationWS
        const double tzeroNew = pixelCalibrationWS->cell_cast<double>(pixelCalibrationRow->row(), 3);

        Mantid::API::TableRow newRow = outputWorkspace->appendRow();
        newRow << detid << difcNew << difaNew << tzeroNew;
        prevDifValsExist = true;

        // update where to start next search from
        lastStart = pixelCalibrationRow->row() + 1;
      }
    }

    if (!prevDifValsExist) {
      // copy from group calibration
      addRowFromPreviousCalibration(outputWorkspace, groupedCalibrationWS, groupedCalibrationRow.row());
    }

    prog.report();
  } while (groupedCalibrationRow.next());

  // loop through rows in the pixel calibration table
  // this will add rows that are not already represented
  bool shouldSortOutputTable{false};
  const std::size_t NUM_PIXEL_ROW = pixelCalibrationWS->rowCount();
  for (std::size_t i = 0; i < NUM_PIXEL_ROW; ++i) {
    const int detid = pixelCalibrationWS->cell_cast<int>(i, 0);
    if (!(maskWorkspace && maskWorkspace->isMasked(detid))) {
      if (detidsInGrpCalib.count(detid) == 0) {
        // copy from pixel calibration
        addRowFromPreviousCalibration(outputWorkspace, pixelCalibrationWS, i);
        shouldSortOutputTable = true;
      }
    }
  }

  // if rows were copied from the pixel calibration, the output should be sorted
  if (shouldSortOutputTable)
    outputWorkspace = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(sortTableWorkspace(outputWorkspace));

  setProperty(PropertyNames::OUTPUT_WKSP, outputWorkspace);
}

} // namespace Mantid::Algorithms
