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
  CompositeValidator<> *wsVal = new CompositeValidator<>;
  wsVal->add(new WorkspaceUnitValidator<>("TOF"));
  wsVal->add(new HistogramValidator<>);
  wsVal->add(new SpectraAxisValidator<>);
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
  const int nHist = inputWS->getNumberHistograms();
  const int nBins = inputWS->blocksize();
  const double maxX = inputWS->readX(0)[nBins];
  std::map<int, double> intMap;
  int prelow = -1;
  std::vector<std::string> workspaces;
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
		  Mantid::API::IAlgorithm_sptr integ = Mantid::API::Algorithm::createSubAlgorithm("Integration");
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

    int indexLow, indexHigh;
    
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
    
    int nbins = indexHigh - indexLow;
    
    MatrixWorkspace_sptr workspace = 
      Mantid::API::WorkspaceFactory::Instance().create(inputWS, nHist, 
      nbins+1, nbins);

    // Copy over X, Y and E data
    PARALLEL_FOR2(inputWS, workspace)
    for ( int j = 0 ; j < nHist ; j++ )
    {
      PARALLEL_START_INTERUPT_REGION
      ;
      for ( int k = 0 ; k < nbins ; k++ )
      {
        int oldbin = indexLow + k;
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
    Mantid::API::AnalysisDataService::Instance().add(wsname, workspace);

    declareProperty(new WorkspaceProperty<>(wsname, wsname, Direction::Output));
    setProperty(wsname, workspace);

    workspaces.push_back(wsname);
  }

  // Create workspace group that holds output workspaces
  WorkspaceGroup_sptr wsgroup = WorkspaceGroup_sptr(new WorkspaceGroup());

  for ( std::vector<std::string>::iterator it = workspaces.begin(); it != workspaces.end(); ++it )
  {
    wsgroup->add(*it);
  }
  // set the output property
  setProperty("OutputWorkspace", wsgroup);

}
} // namespace Algorithms
} // namespace Mantid