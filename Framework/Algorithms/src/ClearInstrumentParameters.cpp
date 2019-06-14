// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ClearInstrumentParameters.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearInstrumentParameters)

using namespace Kernel;
using namespace API;

/// Algorithm's name for identification. @see Algorithm::name
const std::string ClearInstrumentParameters::name() const {
  return "ClearInstrumentParameters";
}

/// Summary of the algorithm's purpose. @see Algorithm::summary
const std::string ClearInstrumentParameters::summary() const {
  return "Clears all the parameters associated with a workspace's instrument.";
}

/// Algorithm's version for identification. @see Algorithm::version
int ClearInstrumentParameters::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearInstrumentParameters::category() const {
  return "DataHandling\\Instrument";
}

/** Initialize the algorithm's properties.
 */
void ClearInstrumentParameters::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "Workspace", "", Direction::InOut,
                      boost::make_shared<InstrumentValidator>()),
                  "Workspace whose instrument parameters are to be cleared.");
}

/** Execute the algorithm.
 */
void ClearInstrumentParameters::exec() {
  const MatrixWorkspace_sptr ws = getProperty("Workspace");
  // Clear old parameters. We also want to clear fields stored in DetectorInfo
  // (masking, positions, rotations). By setting the base instrument (which
  // does not include a ParameterMap or DetectorInfo) we make use of the
  // mechanism in ExperimentInfo that builds a clean DetectorInfo from the
  // instrument being set.
  const auto instrument = ws->getInstrument();
  ws->setInstrument(instrument->baseInstrument());
}

} // namespace Algorithms
} // namespace Mantid
