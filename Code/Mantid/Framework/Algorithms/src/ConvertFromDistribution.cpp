/*WIKI* 

Converts a histogram workspace from a distribution i.e. multiplies by the bin width to take out the bin width dependency.

==== Restrictions on the input workspace ====
The workspace to convert must contain histogram data which is flagged as being a distribution.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertFromDistribution.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

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
  auto wsValidator = boost::make_shared<Kernel::CompositeValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<RawCountValidator>(false);
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
