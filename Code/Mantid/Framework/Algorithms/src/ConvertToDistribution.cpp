//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToDistribution.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToDistribution)

using namespace API;

void ConvertToDistribution::init() {
  auto wsValidator = boost::make_shared<Kernel::CompositeValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<RawCountValidator>();
  declareProperty(new WorkspaceProperty<>(
                      "Workspace", "", Kernel::Direction::InOut, wsValidator),
                  "The name of the workspace to convert.");
}

void ConvertToDistribution::exec() {
  MatrixWorkspace_sptr workspace = getProperty("Workspace");

  WorkspaceHelpers::makeDistribution(workspace);
}

} // namespace Algorithms
} // namespace Mantid
