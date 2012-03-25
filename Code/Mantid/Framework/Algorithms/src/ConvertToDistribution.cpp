/*WIKI* 

Makes a histogram workspace a distribution i.e. divides by the bin width.

==== Restrictions on the input workspace ====
The workspace to convert must contain histogram data which is not already flagged as a distribution.


*WIKI*/
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

/// Sets documentation strings for this algorithm
void ConvertToDistribution::initDocs()
{
  this->setWikiSummary("Makes a histogram workspace a distribution i.e. divides by the bin width. ");
  this->setOptionalMessage("Makes a histogram workspace a distribution i.e. divides by the bin width.");
}


using namespace API;

void ConvertToDistribution::init()
{
  auto wsValidator = boost::make_shared<Kernel::CompositeValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<RawCountValidator>();
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
