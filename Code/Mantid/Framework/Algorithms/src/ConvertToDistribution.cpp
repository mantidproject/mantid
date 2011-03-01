//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToDistribution.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToDistribution)

using namespace API;

void ConvertToDistribution::init()
{
  this->setWikiSummary("Makes a histogram workspace a distribution i.e. divides by the bin width.");
  this->setOptionalMessage("Makes a histogram workspace a distribution i.e. divides by the bin width.");

  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new HistogramValidator<>);
  wsValidator->add(new RawCountValidator<>);
  declareProperty(
    new WorkspaceProperty<>("Workspace","",Kernel::Direction::InOut,wsValidator),
    "The name of the workspace to convert" );
}

void ConvertToDistribution::exec()
{
  MatrixWorkspace_sptr workspace = getProperty("Workspace");

  WorkspaceHelpers::makeDistribution(workspace);
}

} // namespace Algorithms
} // namespace Mantid
