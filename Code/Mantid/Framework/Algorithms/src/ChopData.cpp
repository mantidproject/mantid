/*WIKI* 

This algorithm will chop the input workspace into equally sized workspaces, and adjust the X-values given so that they all begin from the same point. This is useful if your raw files contain multiple frames.

=== Identifying Extended Frames ===
[[File:ChopDataIntegrationExplanation.png|frame|Figure 1: Example Monitor Spectrum with Extended Frames]]

If the parameters ''IntegrationRangeLower'', ''IntegrationRangeUpper'' and ''MonitorWorkspaceIndex'' are provided to the algorithm, then it will attempt to identify where in the workspace the frames have been extended.

For example: looking at Figure 1 which shows an input workspace covering 100000 microseconds, we can see that the first frame covers forty thousand, and the other three cover twenty thousand each.

In order for Mantid to determine this programatically, it integrates over a range (defined by IntegrationRangeLower and IntegrationRangeUpper) for each "chop" of the data. If the relative values for this integration fall within certain bounds, then the chop is deemed to be a continuation of the previous one rather than a separate frame. If this happens, then they will be placed in the same workspace within the result group.

The algorithm will only look at the workspace given in ''MonitorWorkspaceIndex'' property to determine this. Though it is expected and recommended that you use a monitor spectrum for this purpose, it is not enforced so you may use a regular detector if you have cause to do so.


*WIKI*/
#include "MantidAlgorithms/ChopData.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid
{
namespace Algorithms
{
using namespace Kernel;
using namespace API;
using namespace Geometry;

DECLARE_ALGORITHM(ChopData)

void ChopData::init()
{
  this->setWikiSummary("Splits an input workspace into a grouped workspace, where each spectra "
      "if 'chopped' at a certain point (given in 'Step' input value) "
      "and the X values adjusted to give all the workspace in the group the same binning.");
  auto wsVal = boost::make_shared<CompositeValidator>();
  wsVal->add<WorkspaceUnitValidator>("TOF");
  wsVal->add<HistogramValidator>();
  wsVal->add<SpectraAxisValidator>();
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,wsVal),
    "Name of the input workspace to be split.");
  declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspace", "", Direction::Output),
    "Name for the WorkspaceGroup that will be created.");

  declareProperty("Step", 20000.0);
  declareProperty("NChops", 5);
  declareProperty("IntegrationRangeLower", EMPTY_DBL());
  declareProperty("IntegrationRangeUpper", EMPTY_DBL());
  declareProperty("MonitorWorkspaceIndex", EMPTY_INT());
}

void ChopData::exec()
{
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const std::string output = getPropertyValue("OutputWorkspace");
  const double step = getProperty("Step");
  const int chops = getProperty("NChops");
  const double rLower = getProperty("IntegrationRangeLower");
  const double rUpper = getProperty("IntegrationRangeUpper");
  const int monitorWi = getProperty("MonitorWorkspaceIndex");
  const int nHist = static_cast<int>(inputWS->getNumberHistograms());
  const size_t nBins = inputWS->blocksize();
  const double maxX = inputWS->readX(0)[nBins];
  std::map<int, double> intMap;
  int prelow = -1;
  std::vector<MatrixWorkspace_sptr> workspaces;
  if ( maxX < step )
  {
    throw std::invalid_argument("Step value provided larger than size of workspace.");
  }

  if ( rLower != EMPTY_DBL() && rUpper != EMPTY_DBL() && monitorWi != EMPTY_INT() )
  {
	  // Select the spectrum that is to be used to compare the sections of the workspace
	  // This will generally be the monitor spectrum.
	  MatrixWorkspace_sptr monitorWS;
	  monitorWS = WorkspaceFactory::Instance().create(inputWS, 1);
	  monitorWS->dataX(0) = inputWS->dataX(monitorWi);
	  monitorWS->dataY(0) = inputWS->dataY(monitorWi);
	  monitorWS->dataE(0) = inputWS->dataE(monitorWi);

	  int lowest = 0;

	  // Get ranges
	  for ( int i = 0; i < chops; i++ )
	  {
		  Mantid::API::IAlgorithm_sptr integ = Mantid::API::Algorithm::createChildAlgorithm("Integration");
		  integ->initialize();
		  integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", monitorWS);
		  integ->setProperty<double>("RangeLower", i*step+rLower);
		  integ->setProperty<double>("RangeUpper", i*step+rUpper);
		  integ->execute();
		  MatrixWorkspace_sptr integR = integ->getProperty("OutputWorkspace");
		  intMap[i] = integR->dataY(0)[0];

		  if ( intMap[i] < intMap[lowest] ) { lowest = i; }
	  }

	  std::map<int,double>::iterator nlow = intMap.find(lowest-1);
	  if ( nlow != intMap.end() && intMap[lowest] < ( 0.1 * nlow->second ) )
	  {
		  prelow = nlow->first;
	  }
  }
  
  for ( int i = 0; i < chops; i++ )
  {
    const double stepDiff = ( i * step );

    size_t indexLow, indexHigh;
    
    try
    {
      indexLow = inputWS->binIndexOf(stepDiff);
      if ( indexLow < ( nBins + 1 ) ) { indexLow++; }
    }
    catch ( std::out_of_range & ) { indexLow = 0; }

    if ( i == prelow ) { i++; }

    try
    {
      indexHigh = inputWS->binIndexOf((i+1)*step);
    }
    catch ( std::out_of_range & ) { indexHigh = nBins; }
    
    size_t nbins = indexHigh - indexLow;
    
    MatrixWorkspace_sptr workspace = 
      Mantid::API::WorkspaceFactory::Instance().create(inputWS, nHist, 
      nbins+1, nbins);

    // Copy over X, Y and E data
    PARALLEL_FOR2(inputWS, workspace)
    for ( int j = 0 ; j < nHist ; j++ )
    {
      PARALLEL_START_INTERUPT_REGION
      ;
      for ( size_t k = 0 ; k < nbins ; k++ )
      {
        size_t oldbin = indexLow + k;
        workspace->dataY(j)[k] = inputWS->readY(j)[oldbin];
        workspace->dataE(j)[k] = inputWS->readE(j)[oldbin];
        workspace->dataX(j)[k] = inputWS->readX(j)[oldbin] - stepDiff;
      }
      workspace->dataX(j)[nbins] = inputWS->readX(j)[indexLow+nbins] - stepDiff;

      PARALLEL_END_INTERUPT_REGION
      ;
    }
    PARALLEL_CHECK_INTERUPT_REGION
    ;

    // add the workspace to the AnalysisDataService
    std::stringstream name;
    name << output << "-" << i+1;
    std::string wsname = name.str();

    declareProperty(new WorkspaceProperty<>(wsname, wsname, Direction::Output));
    setProperty(wsname, workspace);

    workspaces.push_back(workspace);
  }

  // Create workspace group that holds output workspaces
  WorkspaceGroup_sptr wsgroup = WorkspaceGroup_sptr(new WorkspaceGroup());

  for ( auto it = workspaces.begin(); it != workspaces.end(); ++it )
  {
    wsgroup->addWorkspace(*it);
  }
  // set the output property
  setProperty("OutputWorkspace", wsgroup);

}
} // namespace Algorithms
} // namespace Mantid
