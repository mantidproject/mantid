// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertDiffCal.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::IAlgorithm_sptr;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_const_sptr;
using Mantid::DataObjects::TableWorkspace;
using Mantid::DataObjects::TableWorkspace_sptr;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertDiffCal)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvertDiffCal::name() const { return "ConvertDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertDiffCal::category() const { return "Diffraction\\Utility"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertDiffCal::summary() const { return "Convert diffraction calibration from old to new style"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertDiffCal::init() {
  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>("OffsetsWorkspace", "", Direction::Input),
                  "OffsetsWorkspace containing the calibration offsets.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("PreviousCalibration", "", Direction::Input,
                                                                       API::PropertyMode::Optional),
                  "A calibration table used as a cache for creating the OutputWorkspace. "
                  "Effectively, this algorithm applies partial updates to this table and "
                  "returns it as the OutputWorkspace");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/**
 * @throws std::logic_error if there is more than one detector id
 * for the spectrum.
 * @param offsetsWS
 * @param index
 * @return The proper detector id.
 */
detid_t getDetID(const OffsetsWorkspace_const_sptr &offsetsWS, const size_t index) {
  auto detIDs = offsetsWS->getSpectrum(index).getDetectorIDs();
  if (detIDs.size() != 1) {
    std::stringstream msg;
    msg << "Encountered spectrum with multiple detector ids (size=" << detIDs.size() << ")";
    throw std::logic_error(msg.str());
  }
  return (*(detIDs.begin()));
}

/**
 * @throws std::logic_error if the offset found is non-physical.
 * @param offsetsWS
 * @param detid
 * @return The offset value or zero if not specified.
 */
double getOffset(const OffsetsWorkspace_const_sptr &offsetsWS, const detid_t detid) {
  const double offset = offsetsWS->getValue(detid, 0.0);
  if (offset <= -1.) { // non-physical
    std::stringstream msg;
    msg << "Encountered offset of " << offset << " which converts data to negative d-spacing for detectorID " << detid
        << "\n";
    throw std::logic_error(msg.str());
  }
  return offset;
}

/**
 * @param d_info - detector info
 * @param offset - d-spacing offset of the detector
 * @param index - index of the detector in the detector info object
 * @return The offset adjusted value of DIFC
 */
double calculateDIFC(Mantid::Geometry::DetectorInfo const &d_info, double offset, size_t index) {

  double twotheta;
  try {
    twotheta = d_info.twoTheta(index); // we can pass in the internal index too, and d_info
  } catch (std::runtime_error &) {
    // Choose an arbitrary angle if detector 2theta determination fails.
    twotheta = 0.;
  }
  // the factor returned is what is needed to convert TOF->d-spacing
  // the table is supposed to be filled with DIFC which goes the other way
  const double factor =
      Mantid::Geometry::Conversion::tofToDSpacingFactor(d_info.l1(), d_info.l2(index), twotheta, offset);
  return 1. / factor;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertDiffCal::exec() {
  OffsetsWorkspace_const_sptr offsetsWS = getProperty("OffsetsWorkspace");

  /* If a previous calibration is provided, initialize the output calibration table
   * with it */
  ITableWorkspace_sptr configWksp;

  ITableWorkspace_sptr previous_calibration = getProperty("PreviousCalibration");

  std::vector<std::string> correct_columns{"detid", "difc", "difa", "tzero"};
  if (previous_calibration) {
    /* validate correct format */
    std::vector<std::string> column_names = previous_calibration->getColumnNames();
    if (column_names != correct_columns) {
      throw std::runtime_error("PreviousCalibration table's column names do not match expected "
                               "format");
    }

    configWksp = previous_calibration->clone();
  }

  else {
    // initial setup of new style config
    configWksp = std::make_shared<TableWorkspace>();

    configWksp->addColumn("int", correct_columns[0]);
    configWksp->addColumn("double", correct_columns[1]);
    configWksp->addColumn("double", correct_columns[2]);
    configWksp->addColumn("double", correct_columns[3]);
  }

  /* obtain detector ids from the previous calibration workspace */
  std::unordered_map<int, int> id_to_row;
  if (previous_calibration) {
    std::vector<int> previous_calibration_ids = previous_calibration->getColumn(0)->numeric_fill<int>();

    int row = 0;

    for (auto id : previous_calibration_ids) {
      id_to_row[id] = row++;
    }
  }

  /* iterate through all detectors in the offsets workspace */
  Mantid::Geometry::DetectorInfo const &d_info = offsetsWS->detectorInfo();
  auto const &detector_ids = d_info.detectorIDs();
  Progress progress(this, 0.0, 1.0, detector_ids.size());
  for (auto id : detector_ids) {
    size_t internal_index = d_info.indexOf(id);

    /* only update non-masked, non zero-offset entries */
    double new_offset_value = offsetsWS->getValue(id);
    if (!d_info.isMasked(internal_index)) {
      /* check for the detector id in the calibration table */
      auto row_to_update = id_to_row.find(id);
      /* if it is found, update it according to a simple linear equation: */
      if (row_to_update != id_to_row.end()) {
        /* Get the row and update the difc value in the first column */
        double &value_to_update = configWksp->cell<double>(row_to_update->second, 1);

        value_to_update = value_to_update / (1 + new_offset_value);
      }

      /* It was not found: calculate the value as before with calculateDiffc */
      else {
        API::TableRow newrow = configWksp->appendRow();

        newrow << static_cast<int>(id);
        newrow << calculateDIFC(d_info, new_offset_value, internal_index);
        newrow << 0.0;
        newrow << 0.0;
      }
    }
    progress.report();
  }

  // sort the results
  IAlgorithm_sptr sortTable = createChildAlgorithm("SortTableWorkspace");
  sortTable->setProperty("InputWorkspace", configWksp);
  sortTable->setProperty("OutputWorkspace", configWksp);
  sortTable->setPropertyValue("Columns", "detid");
  sortTable->executeAsChildAlg();

  // copy over the results
  configWksp = sortTable->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", configWksp);
}

} // namespace Algorithms
} // namespace Mantid
