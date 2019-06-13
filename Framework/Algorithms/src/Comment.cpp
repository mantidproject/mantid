// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Comment.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::Workspace;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::MandatoryValidator;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Comment)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string Comment::name() const { return "Comment"; }

/// Algorithm's version for identification. @see Algorithm::version
int Comment::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string Comment::category() const { return "Utility\\Workspaces"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string Comment::summary() const {
  return "Adds a comment into the history record of a workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Comment::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "Workspace", "", Direction::InOut),
                  "An InOut workspace that will store the new history comment");

  declareProperty("Text", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The text you want to store in the history of the workspace",
                  Direction::Input);

  // always record history for this algorithm
  enableHistoryRecordingForChild(true);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Comment::exec() {
  // do nothing
}

} // namespace Algorithms
} // namespace Mantid
