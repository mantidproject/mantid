#include "MantidWorkflowAlgorithms/SendUsage.h"

namespace Mantid {
namespace WorkflowAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SendUsage)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SendUsage::SendUsage() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SendUsage::~SendUsage() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SendUsage::name() const { return "SendUsage"; }

/// Algorithm's version for identification. @see Algorithm::version
int SendUsage::version() const {
  return 1;
};

/// Algorithm's category for identification. @see Algorithm::category
const std::string SendUsage::category() const {
  return TODO : FILL IN A CATEGORY;
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SendUsage::summary() const {
  return TODO : FILL IN A SUMMARY;
};

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SendUsage::init() {
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
void SendUsage::exec() {
  // TODO Auto-generated execute stub
}

} // namespace WorkflowAlgorithms
} // namespace Mantid