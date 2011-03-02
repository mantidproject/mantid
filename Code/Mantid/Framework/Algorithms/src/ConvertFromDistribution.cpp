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

/// Sets documentation strings for this algorithm
void ConvertFromDistribution::initDocs()
{
  this->setWikiSummary("Converts a histogram workspace from a distribution i.e. multiplies by the bin width. ");
  this->setOptionalMessage("Converts a histogram workspace from a distribution i.e. multiplies by the bin width.");
}


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
