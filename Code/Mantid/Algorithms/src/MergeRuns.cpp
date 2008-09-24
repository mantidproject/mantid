//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MergeRuns.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(MergeRuns)

using namespace Kernel;
using namespace API;

// Get a reference to the logger
Logger& MergeRuns::g_log = Logger::get("MergeRuns");

/// Default constructor
MergeRuns::MergeRuns() : Algorithm() {}

/// Destructor
MergeRuns::~MergeRuns() {}

/// Initialisation method
void MergeRuns::init()
{
  // declare arbitrary number of input workspaces as a list of strings at the moment
  declareProperty(new ArrayProperty<std::string>("InputWorkspaces", new MandatoryValidator<std::vector<std::string> >));
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));
}

/** Executes the algorithm
 *  @throw Exception::NotFoundError If an input workspace doesn't exist
 *  @throw std::invalid_argument If the input workspaces are not compatible
 */
void MergeRuns::exec()
{
  // Check that all input workspaces exist and match in certain important ways
  const std::vector<std::string> inputs = getProperty("InputWorkspaces");
  if ( inputs.size() == 1 )
  {
    g_log.error("Only one input workspace specified");
    throw std::invalid_argument("Only one input workspace specified");
  }
  const std::vector<Workspace_sptr> inWS = this->validateInputs(inputs);

  // Now add them all together
  Workspace_sptr outWS = inWS[0] + inWS[1];
  for (unsigned int i=2; i < inputs.size(); ++i)
  {
    outWS = outWS + inWS[i];
  }
  setProperty("OutputWorkspace",outWS);
}

/** Checks that the input workspace all exist, that they are the same size, have the same units
 *  and the same instrument name. Will throw if they don't.
 *  @param inputWorkspaces The names of the input workspaces
 *  @return A vector of pointers to the input workspace
 *  @throw Exception::NotFoundError If an input workspace doesn't exist
 *  @throw std::invalid_argument If the input workspaces are not compatible
 */
std::vector<API::Workspace_sptr> MergeRuns::validateInputs(const std::vector<std::string>& inputWorkspaces) const
{
  std::vector<Workspace_sptr> inWS;
  int numSpec;
  boost::shared_ptr<Unit> unit;
  bool dist;
  // Going to check that name of instrument matches - think that's the best possible at the moment
  //   because if instrument is created from raw file it'll be a different object
  std::string instrument;

  for ( unsigned int i = 0; i < inputWorkspaces.size(); ++i )
  {
    try {
      inWS.push_back(AnalysisDataService::Instance().retrieve(inputWorkspaces[i]));
    }
    catch (Exception::NotFoundError) {
      g_log.error() << "Input workspace " << inputWorkspaces[i] << " not found." << std::endl;
      throw;
    }
    if ( i == 0 )
    {
      numSpec = inWS[0]->getNumberHistograms();
      unit = inWS[0]->getAxis(0)->unit();
      dist = inWS[0]->isDistribution();
      instrument = inWS[0]->getInstrument()->getName();
    }
    else
    {
      if ( inWS[i]->getNumberHistograms() != numSpec
           || inWS[i]->getAxis(0)->unit() != unit
           || inWS[i]->isDistribution()   != dist
           || inWS[i]->getInstrument()->getName() != instrument )
      {
        g_log.error("Input workspaces are not compatible");
        throw std::invalid_argument("Input workspaces are not compatible");
      }
    }
  }

  // Now check that the binning matches
  for ( unsigned int i = 1; i < inWS.size(); ++i )
  {
    if ( ! WorkspaceHelpers::matchingBins(inWS[0],inWS[i]) )
    {
      g_log.error("Input workspaces are not compatible");
      throw std::invalid_argument("Input workspaces are not compatible");
    }
  }

  return inWS;
}

} // namespace Algorithm
} // namespace Mantid
