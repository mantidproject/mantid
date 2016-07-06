#include "MantidAlgorithms/CreateUserDefinedBackground.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateUserDefinedBackground)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CreateUserDefinedBackground::name() const { return "CreateUserDefinedBackground"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateUserDefinedBackground::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateUserDefinedBackground::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateUserDefinedBackground::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateUserDefinedBackground::init() {
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "",
                                                             Direction::Input),
      "An input workspace.");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "",
                                                             Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateUserDefinedBackground::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
