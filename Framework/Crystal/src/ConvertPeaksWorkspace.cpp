// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidCrystal/ConvertPeaksWorkspace.h"
#include "MantidDataObjects/LeanElasticPeak.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Crystal {

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

/// Config logger
namespace {
Logger logger("ConvertPeaksWorkspace");
}

DECLARE_ALGORITHM(ConvertPeaksWorkspace)

/**
 * @brief initialization of alg
 *
 */
void ConvertPeaksWorkspace::init() {
  // Input peakworkspace
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeakWorkspace", "", Kernel::Direction::Input),
                  "Workspace of Indexed Peaks");

  // donor workspace if going from lean to regular
  declareProperty(std::make_unique<WorkspaceProperty<>>("InstrumentWorkpace", "", Kernel::Direction::Input),
                  "Workspace of Indexed Peaks");

  // output
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The workspace containing the calibration table.");
}

/**
 * @brief Inputs validation
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> ConvertPeaksWorkspace::validateInputs() {}

/**
 * @brief Algorithm entrance func
 *
 */
void ConvertPeaksWorkspace::exec() {}

/**
 * @brief make a LeanElasticPeaksWorkspace using peaks from a regular PeaksWorkspace
 *
 * @param pws
 * @return IPeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr ConvertPeaksWorkspace::makeLeanElasticPeaksWorkspace(IPeaksWorkspace_sptr pws) {}

/**
 * @brief Build a regular PeaksWorkspace using peaks from a LeanElasticPeaksWorkspace and the provided instrument
 *
 * @param lpws
 * @param instrument
 * @return IPeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr ConvertPeaksWorkspace::makeLeanElasticPeaksWorkspace(IPeaksWorkspace_sptr lpws,
                                                                          std::shared_ptr<Instrument> &instrument) {}

} // namespace Crystal
} // namespace Mantid
