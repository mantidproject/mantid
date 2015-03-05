#include "MantidAlgorithms/CalculateDIFC.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateDIFC)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CalculateDIFC::CalculateDIFC() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CalculateDIFC::~CalculateDIFC() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateDIFC::name() const { return "CalculateDIFC"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculateDIFC::version() const {
  return 1;
};

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateDIFC::category() const {
  return TODO : FILL IN A CATEGORY;
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateDIFC::summary() const {
  return TODO : FILL IN A SUMMARY;
};

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateDIFC::init() {
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
void CalculateDIFC::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid