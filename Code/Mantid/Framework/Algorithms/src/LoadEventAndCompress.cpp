#include "MantidAlgorithms/LoadEventAndCompress.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEventAndCompress)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadEventAndCompress::LoadEventAndCompress() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadEventAndCompress::~LoadEventAndCompress() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadEventAndCompress::name() const { return "LoadEventAndCompress"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadEventAndCompress::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadEventAndCompress::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadEventAndCompress::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadEventAndCompress::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadEventAndCompress::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid