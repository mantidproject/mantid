// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CombineDiffCal.h"
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

std::map<std::string, std::string> CombineDiffCal::validateInputs() {
  std::map<std::string, std::string> results;

  return results;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CombineDiffCal::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
