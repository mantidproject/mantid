// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/RecalculateTrajectoriesExtents.h"
#include "MantidAPI/IMDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {
using Mantid::Kernel::Direction;
using Mantid::Kernel::make_unique;
using Mantid::API::WorkspaceProperty;
using Mantid::API::IMDEventWorkspace;
using Mantid::API::IMDEventWorkspace_sptr;
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RecalculateTrajectoriesExtents)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string RecalculateTrajectoriesExtents::name() const { return "RecalculateTrajectoriesExtents"; }

/// Algorithm's version for identification. @see Algorithm::version
int RecalculateTrajectoriesExtents::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RecalculateTrajectoriesExtents::category() const {
  return "MDAlgorithms\\Normalisation";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RecalculateTrajectoriesExtents::summary() const {
  return "Recalculates trajectory limits set by CropWorkspaceForMDNorm";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RecalculateTrajectoriesExtents::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                        "InputWorkspace", "", Direction::Input),
                    "An input MDEventWorkspace.");
  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                        "OutputWorkspace", "", Direction::Output),
                    "Copy of the input MDEventWorkspace with the corrected trajectory extents.");
}

/// Validate the input workspace @see Algorithm::validateInputs
std::map<std::string, std::string> RecalculateTrajectoriesExtents::validateInputs() {
  std::map<std::string, std::string> errorMessage;
/*
  // Check for input workspace type
  Mantid::API::IMDWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  if (!boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(inputWS) &&
      !boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(inputWS)) {
    errorMessage.emplace("InputWorkspace",
                         "Only MDHisto or MDEvent workspaces can be saved.");
  }

  // Check for the dimensionality
  if (!saver->is3DWorkspace(*inputWS)) {
    errorMessage.emplace("InputWorkspace", "The MD workspace must be 3D.");
  }

  // Check for file location
  */
  return errorMessage;
}


//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RecalculateTrajectoriesExtents::exec() {
  IMDEventWorkspace_sptr inWS = getProperty("InputWorkspace");
  IMDEventWorkspace_sptr outWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (outWS != inWS) {
    outWS = inWS->clone();
  }
  setProperty("OutputWorkspace", outWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
