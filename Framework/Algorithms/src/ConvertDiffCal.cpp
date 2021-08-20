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
 * @param offsetsWS
 * @param index
 * @param spectrumInfo
 * @return The offset adjusted value of DIFC
 */
double calculateDIFC(const OffsetsWorkspace_const_sptr &offsetsWS, const size_t index,
                     const Mantid::API::SpectrumInfo &spectrumInfo) {
  const detid_t detid = getDetID(offsetsWS, index);
  const double offset = getOffset(offsetsWS, detid);
  double twotheta;
  try {
    twotheta = spectrumInfo.twoTheta(index);
  } catch (std::runtime_error &) {
    // Choose an arbitrary angle if detector 2theta determination fails.
    twotheta = 0.;
  }
  // the factor returned is what is needed to convert TOF->d-spacing
  // the table is supposed to be filled with DIFC which goes the other way
  const double factor =
      Mantid::Geometry::Conversion::tofToDSpacingFactor(spectrumInfo.l1(), spectrumInfo.l2(index), twotheta, offset);
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

  // create values in the table
  const size_t numberOfSpectra = offsetsWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  const Mantid::API::SpectrumInfo &spectrumInfo = offsetsWS->spectrumInfo();
  Mantid::Geometry::DetectorInfo const &d_info = offsetsWS->detectorInfo();
  for (size_t i = 0; i < numberOfSpectra; ++i) {

    /* obtain detector id for this spectra */
    detid_t detector_id = getDetID(offsetsWS, i);
    size_t internal_index = d_info.indexOf(detector_id);
    /* obtain the mask */
    if (!d_info.isMasked(internal_index)) {
      /* find the detector id's offset value in the offset workspace */
      double new_offset_value = offsetsWS->getValue(detector_id);

      /* search for it in the table */
      auto iter = id_to_row.find(detector_id);

      /* if it is found, update the correct row in the output table */
      if (iter != id_to_row.end()) {
        /* Get the row and update the difc value in the first column */
        int row_to_update = iter->second;

        double &difc_value_to_update = configWksp->cell<double>(row_to_update, 1);

        difc_value_to_update = difc_value_to_update / (1 + new_offset_value);
      }

      /* value was not found in PreviousCalibration - calculate from experiment's geometry */
      else {
        API::TableRow newrow = configWksp->appendRow();
        newrow << static_cast<int>(detector_id);
        newrow << calculateDIFC(offsetsWS, i, spectrumInfo);
        newrow << 0.; // difa
        newrow << 0.; // tzero
      }
    }

    progress.report();
  }

  // sort the results
  auto sortTable = createChildAlgorithm("SortTableWorkspace");
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
