//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidCurveFitting/Fit1D.h"
#include "MantidCurveFitting/GaussianLinearBG1D.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(GetDetectorOffsets)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;

// Get a reference to the logger
Logger& GetDetectorOffsets::g_log = Logger::get("GetDetectorOffsets");

/// Constructor
GetDetectorOffsets::GetDetectorOffsets() :
  API::Algorithm()
{}

/// Destructor
GetDetectorOffsets::~GetDetectorOffsets()
{}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void GetDetectorOffsets::init()
{
  declareProperty(
    new API::WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "A 2D workspace with X values of d-spacing" );
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output),"Workspace containing the offsets");
  declareProperty("Step",0.001);
  declareProperty("dReference",2.0);
  declareProperty("Xmin",0.0);
  declareProperty("Xmax",0.0);
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw std::runtime_error If the rebinning process fails
 */
void GetDetectorOffsets::exec()
{
	///
	retrieveProperties();
	/// Fit all the spectra with a gaussian
	for (int i=0;i<nspec;++i)
	{
		outputW->getAxis(1)->spectraNo(i)=inputW->getAxis(1)->spectraNo(i);
		fitSpectra(i);
	}
	setProperty("OutputWorkspace",outputW);
}

void GetDetectorOffsets::retrieveProperties()
{
	inputW=getProperty("InputWorkspace");
	Xmin=getProperty("Xmin");
	Xmax=getProperty("Xmax");
	dreference=getProperty("dReference");
	step=getProperty("Step");
	nspec=inputW->getNumberHistograms();
	outputW=API::WorkspaceFactory::Instance().create(inputW,nspec,2,1);
}

void GetDetectorOffsets::fitSpectra(const int s)
{
	IAlgorithm_sptr fit_alg;
	  try
	  {
	    fit_alg = createSubAlgorithm("Gaussian1D");
	  }
	  catch (Exception::NotFoundError)
	  {
	    g_log.error("Can't locate Gaussian1D");
	    throw;
	  }
	  fit_alg->setProperty("InputWorkspace",inputW);
	  fit_alg->setProperty("SpectrumIndex",s);
	  fit_alg->setProperty("StartX",Xmin);
	  fit_alg->setProperty("EndX",Xmax);
	  fit_alg->setProperty("bg0",0.0);
	  fit_alg->setProperty("bg1",0.0);
	  fit_alg->setProperty("height",1.0);
	  fit_alg->setProperty("peakCentre",0.0);
	  fit_alg->setProperty("sigma",10.0);
	  fit_alg->setProperty("MaxIterations",100);


	    try {
	      fit_alg->execute();
	    } catch (std::runtime_error) {
	      g_log.error("Unable to successfully run Gausssian1D sub-algorithm");
	      throw;
	    }

	    if ( ! fit_alg->isExecuted() )
	    {
	      g_log.error("Unable to successfully run Gaussian1D sub-algorithm");
	      throw std::runtime_error("Unable to successfully run Gaussian1D sub-algorithm");
	    }

	    std::ostringstream mess;
	    CurveFitting::Fit1D* test=dynamic_cast<CurveFitting::Fit1D*>(fit_alg.get());
	    if (test)
	    {
	    	mess << "Offset for spectrum " << s << " : " << test->getfittedParam(3) << std::endl;
	    	g_log.information(mess.str());
	    	mess.str();
	    }
	    outputW->dataY(s)[0]=-test->getfittedParam(3)*step/dreference;
	    return;
}



} // namespace Algorithm
} // namespace Mantid
