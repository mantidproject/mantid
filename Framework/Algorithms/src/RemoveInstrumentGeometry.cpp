// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/RemoveInstrumentGeometry.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemoveInstrumentGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string RemoveInstrumentGeometry::name() const { return "RemoveInstrumentGeometry"; }

/// Algorithm's version for identification. @see Algorithm::version
int RemoveInstrumentGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RemoveInstrumentGeometry::category() const { return "TODO: FILL IN A CATEGORY"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RemoveInstrumentGeometry::summary() const { return "TODO: FILL IN A SUMMARY"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RemoveInstrumentGeometry::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("MDExperimentInfoNumbers"),
                  "For MD workspaces, the ExperimentInfo indices to have the instrument removed."
                  "If empty, the instrument will be removed from all ExperimentInfo objects."
                  "The parameter is ignored for any other workspace type.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RemoveInstrumentGeometry::exec() {
  // TODO Auto-generated execute stub
  API::Workspace_const_sptr inputWS = this->getProperty("InputWorkspace");
  API::Workspace_sptr outputWS = this->getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
  }
  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
