//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertFromDistribution.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertFromDistribution)

using namespace API;

void ConvertFromDistribution::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new HistogramValidator<>);
  wsValidator->add(new RawCountValidator<>(false));
  declareProperty(new WorkspaceProperty<>("Workspace", "",
    Kernel::Direction::InOut, wsValidator),
    "The name of the workspace to convert");
}

void ConvertFromDistribution::exec()
{
  MatrixWorkspace_sptr workspace = getProperty("Workspace");

  WorkspaceHelpers::makeDistribution(workspace,false);
}

} // namespace Algorithms
} // namespace Mantid
