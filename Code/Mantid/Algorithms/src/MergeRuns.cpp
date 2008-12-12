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
  std::list<Workspace_sptr> inWS = this->validateInputs(inputs);

  // Iterate over the collection of input workspaces
  std::list<Workspace_sptr>::iterator it = inWS.begin();
  // Take the first input workspace as the first argument to the addition
  Workspace_sptr outWS = inWS.front();
  // Note that the iterator is incremented before first pass so that 1st workspace isn't added to itself
  for (++it; it != inWS.end(); ++it)
  {
    Workspace_sptr addee;
    // Only do a rebinning if the bins don't already match - otherwise can just add (see the 'else')
    if ( ! WorkspaceHelpers::matchingBins(outWS,*it,true) )
    {
      std::vector<double> rebinParams;
      this->calculateRebinParams(outWS,*it,rebinParams);

      // Rebin the two workspaces in turn to the same set of bins
      outWS = this->rebinInput(outWS,rebinParams);
      addee = this->rebinInput(*it,rebinParams);
    }
    else
    {
      addee = *it;
    }

    // Add the current workspace to the total
    outWS = outWS + addee;
  }

  // Set the final workspace to the output property
  setProperty("OutputWorkspace",outWS);
}

/// @cond
// Local function used within validateInputs() below in a call to std::list::sort(compare)
// to order the input workspaces by the start of their frame (i.e. the first X value).
static bool compare(Workspace_sptr first, Workspace_sptr second)
{
  return (first->readX(0).front() < second->readX(0).front() );
}
/// @endcond

/** Checks that the input workspace all exist, that they are the same size, have the same units
 *  and the same instrument name. Will throw if they don't.
 *  @param  inputWorkspaces The names of the input workspaces
 *  @return A list of pointers to the input workspace, ordered by increasing frame starting point
 *  @throw  Exception::NotFoundError If an input workspace doesn't exist
 *  @throw  std::invalid_argument    If the input workspaces are not compatible
 */
std::list<API::Workspace_sptr> MergeRuns::validateInputs(const std::vector<std::string>& inputWorkspaces) const
{
  std::list<Workspace_sptr> inWS;
  int numSpec;
  boost::shared_ptr<Unit> unit;
  bool dist;
  // Going to check that name of instrument matches - think that's the best possible at the moment
  //   because if instrument is created from raw file it'll be a different object
  std::string instrument;

  for ( unsigned int i = 0; i < inputWorkspaces.size(); ++i )
  {
    // Fetch the next input workspace - throw an error if it's not there
    try {
      inWS.push_back(AnalysisDataService::Instance().retrieve(inputWorkspaces[i]));
    }
    catch (Exception::NotFoundError) {
      g_log.error() << "Input workspace " << inputWorkspaces[i] << " not found." << std::endl;
      throw;
    }
    // Check that it has common binning
    if ( !WorkspaceHelpers::commonBoundaries(inWS.back()) )
    {
      g_log.error("Input workspaces must have common binning for all spectra");
      throw std::invalid_argument("Input workspaces must have common binning for all spectra");
    }
    // Check a few things are the same for all input workspaces
    if ( i == 0 )
    {
      numSpec = inWS.back()->getNumberHistograms();
      unit = inWS.back()->getAxis(0)->unit();
      dist = inWS.back()->isDistribution();
      instrument = inWS.back()->getInstrument()->getName();
    }
    else
    {
      if ( inWS.back()->getNumberHistograms() != numSpec
           || inWS.back()->getAxis(0)->unit() != unit
           || inWS.back()->isDistribution()   != dist
           || inWS.back()->getInstrument()->getName() != instrument )
      {
        g_log.error("Input workspaces are not compatible");
        throw std::invalid_argument("Input workspaces are not compatible");
      }
    }
  }

  // Order the workspaces by ascending frame (X) starting point
  inWS.sort(compare);

  return inWS;
}

/** Calculates the parameters to hand to the Rebin algorithm. Specifies the new binning, bin-by-bin,
 *  to cover the full range covered by the two input workspaces. In regions of overlap, the bins from
 *  the workspace having the wider bins are taken. Note that because the list of input workspaces
 *  is sorted, ws1 will always start before (or at the same point as) ws2.
 *  @param ws1    The first input workspace. Will start before ws2.
 *  @param ws2    The second input workspace.
 *  @param params A reference to the vector of rebinning parameters
 */
void MergeRuns::calculateRebinParams(const API::Workspace_const_sptr& ws1, const API::Workspace_const_sptr& ws2, std::vector<double>& params) const
{
  const std::vector<double> &X1 = ws1->readX(0);
  const std::vector<double> &X2 = ws2->readX(0);
  const double end1 = X1.back();
  const double start2 = X2.front();
  const double end2 = X2.back();

  if ( end1 <= start2 )
  {
    // First case is if there's no overlap between the workspaces
    this->noOverlapParams(X1,X2,params);
  }
  else
  {
    // Add the bins from the first workspace up to the start of the overlap
    params.push_back(X1[0]);
    int i;
    for (i = 1; X1[i] <= start2; ++i)
    {
      params.push_back(X1[i]-X1[i-1]);
      params.push_back(X1[i]);
    }
    // If the range of workspace2 is completely within that of workspace1, then call the
    // 'inclusion' routine. Otherwise call the standard 'intersection' one.
    if ( end1 < end2 )
    {
      this->intersectionParams(X1,i,X2,params);
    }
    else
    {
      this->inclusionParams(X1,i,X2,params);
    }
  }
}

/** Calculates the rebin paramters in the case where the two input workspaces do not overlap at all.
 *  @param X1     The bin boundaries from the first workspace
 *  @param X2     The bin boundaries from the second workspace
 *  @param params A reference to the vector of rebinning parameters
 */
void MergeRuns::noOverlapParams(const std::vector<double>& X1, const std::vector<double>& X2, std::vector<double>& params) const
{
  // Add all the bins from the first workspace
  for (unsigned int i = 1; i < X1.size(); ++i)
  {
    params.push_back(X1[i-1]);
    params.push_back(X1[i]-X1[i-1]);
  }
  // Put a single bin in the 'gap' (but check first the 'gap' isn't zero)
  if ( X1.back() < X2.front() )
  {
    params.push_back(X1.back());
    params.push_back(X2.front()-X1.back());
  }
  // Now add all the bins from the second workspace
  for (unsigned int j = 1; j < X2.size(); ++j)
  {
    params.push_back(X2[j-1]);
    params.push_back(X2[j]-X2[j-1]);
  }
  params.push_back(X2.back());
}

/** Calculates the rebin parameters in the case where the bins of the two workspaces intersect.
 *  'Intersect' is used in the sense of two intersecting sets.
 *  @param X1     The bin boundaries from the first workspace
 *  @param i      Indicates the index in X1 immediately before the overlap region starts
 *  @param X2     The bin boundaries from the second workspace
 *  @param params A reference to the vector of rebinning parameters
 */
void MergeRuns::intersectionParams(const std::vector<double>& X1, int& i, const std::vector<double>& X2, std::vector<double>& params) const
{
  // First calculate the number of bins in each workspace that are in the overlap region
  int overlapbins1, overlapbins2;
  overlapbins1 = X1.size()-i;
  for (overlapbins2 = 0; X2[overlapbins2] < X1.back(); ++overlapbins2) {}

  // We want to use whichever one has the larger bins (on average)
  if ( overlapbins1 < overlapbins2 )
  {
    // In this case we want the rest of the bins from the first workspace.....
    for (; i < static_cast<int>(X1.size()); ++i)
    {
      params.push_back(X1[i]-X1[i-1]);
      params.push_back(X1[i]);
    }
    // Now remove the last bin & boundary
    params.pop_back();
    params.pop_back();
    // ....and then the non-overlap ones from the second workspace
    for (unsigned int j = overlapbins2; j < X2.size(); ++j)
    {
      params.push_back(X2[j]-params.back());
      params.push_back(X2[j]);
    }
  }
  else
  {
    // In this case we just have to add all the bins from the second workspace
    for (unsigned int j = 1; j < X2.size(); ++j)
    {
      params.push_back(X2[j]-params.back());
      params.push_back(X2[j]);
    }
  }

}

/** Calculates the rebin parameters in the case where the range of the second workspace is
 *  entirely within that of the first workspace.
 *  'Inclusion' is used in the sense of a set being included in anothre.
 *  @param X1     The bin boundaries from the first workspace
 *  @param i      Indicates the index in X1 immediately before the overlap region starts
 *  @param X2     The bin boundaries from the second workspace
 *  @param params A reference to the vector of rebinning parameters
 */
void MergeRuns::inclusionParams(const std::vector<double>& X1, int& i, const std::vector<double>& X2, std::vector<double>& params) const
{
  // First calculate the number of bins in each workspace that are in the overlap region
  int overlapbins1, overlapbins2;
  for (overlapbins1 = 1; X1[i+overlapbins1] < X2.back(); ++overlapbins1 ) {}
  //++overlapbins1;
  overlapbins2 = X2.size()-1;

  // In the overlap region, we want to use whichever one has the larger bins (on average)
  if ( overlapbins1+1 <= overlapbins2 )
  {
    // In the case where the first workspace has larger bins it's easy
    // - just add the rest of X1's bins
    for (; i < static_cast<int>(X1.size()); ++i)
    {
      params.push_back(X1[i]-X1[i-1]);
      params.push_back(X1[i]);
    }
  }
  else
  {
    // In this case we want all of X2's bins first (without the first and last boundaries)
    for (unsigned int j = 1; j < X2.size()-1; ++j)
    {
      params.push_back(X2[j]-params.back());
      params.push_back(X2[j]);
    }
    // And now those from X1 that lie above the overlap region
    i += overlapbins1;
    for (; i < static_cast<int>(X1.size()); ++i)
    {
      params.push_back(X1[i]-params.back());
      params.push_back(X1[i]);
    }
  }

}

/** Calls the Rebin algorithm as a subalgorithm.
 *  @param  workspace The workspace to use as input to the Rebin algorithms
 *  @param  params    The rebin parameters
 *  @return A shared pointer to the output (rebinned) workspace
 *  @throw  std::runtime_error If the Rebin algorithm fails
 */
API::Workspace_sptr MergeRuns::rebinInput(const API::Workspace_sptr& workspace, const std::vector<double>& params)
{
  // Create a Rebin child algorithm
  Algorithm_sptr rebin = createSubAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", workspace);
  rebin->setProperty("params",params);

  // Now execute the sub-algorithm. Catch and log any error
  try { rebin->execute(); }
  catch (std::runtime_error)
  {
    g_log.error("Unable to successfully run Rebin sub-algorithm");
    throw;
  }

  if ( !rebin->isExecuted() )
  {
    g_log.error("Unable to successfully run Rebin sub-algorithm");
    throw std::runtime_error("Unable to successfully run Rebin sub-algorithm");
  }

  return rebin->getProperty("OutputWorkspace");
}

} // namespace Algorithm
} // namespace Mantid
