//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertFromDistribution.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertFromDistribution)

using namespace API;

void ConvertFromDistribution::init() {
  auto wsValidator = boost::make_shared<Kernel::CompositeValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<RawCountValidator>(false);
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "Workspace", "", Kernel::Direction::InOut, wsValidator),
                  "The name of the workspace to convert.");
}

void ConvertFromDistribution::exec() {
  MatrixWorkspace_sptr workspace = getProperty("Workspace");

  WorkspaceHelpers::makeDistribution(workspace, false);
}

} // namespace Algorithms
} // namespace Mantid
