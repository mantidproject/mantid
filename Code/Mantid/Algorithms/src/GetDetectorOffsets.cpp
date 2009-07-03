//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <fstream>
#include <ostream>
#include <iomanip>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(GetDetectorOffsets)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;

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
  declareProperty("GroupingFileName","",
      new FileValidator(std::vector<std::string>(1,"cal"),false),
      "The name of the output CalFile" );
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 */
void GetDetectorOffsets::exec()
{
	retrieveProperties();
	// Fit all the spectra with a gaussian
	std::string filename=getProperty("GroupingFileName");
	const SpectraDetectorMap& specMap = inputW->spectraMap();
	std::fstream out;
	out.open(filename.c_str(),std::ios::out);
	if (!out.is_open())
	{
		std::runtime_error("Problem opening file"+filename);
	}
	else
	{
		int n=0;
		out << "# Offsets generated by Mantid" << std::endl;
		for (int i=0;i<nspec;++i)
		{
			outputW->getAxis(1)->spectraNo(i)=inputW->getAxis(1)->spectraNo(i);
			double offset=fitSpectra(i);
			// Assign the value of the offset
			outputW->dataY(i)[0]=offset;
			// Put it into file
			int specno=outputW->getAxis(1)->spectraNo(i);
			const std::vector<int> dets=specMap.getDetectors(specno);
			for (unsigned int j=0;j<dets.size();j++)
			{
				out << std::fixed << std::setw(9) << n++ <<
				     std::fixed << std::setw(15) << dets[j] <<
				     std::fixed << std::setprecision(7) << std::setw(15) << offset <<
				     std::fixed << std::setw(8) << "1" <<
				     std::fixed << std::setw(8) << "1"  << "\n";
			}
			progress((double)(i)/nspec);
		}
	out.close();
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

/** Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
 *  @param s The spectrum index to fit
 *  @return The calculated offset value
 */
double GetDetectorOffsets::fitSpectra(const int s)
{
  IAlgorithm_sptr fit_alg;
  try
  {
    fit_alg = createSubAlgorithm("Gaussian1D");
  } catch (Exception::NotFoundError)
  {
    g_log.error("Can't locate Gaussian1D");
    throw ;
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

  try
  {
    fit_alg->execute();
  }
  catch (std::runtime_error)
  {
    g_log.error("Unable to successfully run Gausssian1D sub-algorithm");
    throw;
  }

  if ( ! fit_alg->isExecuted() )
  {
    g_log.error("Unable to successfully run Gaussian1D sub-algorithm");
    throw std::runtime_error("Unable to successfully run Gaussian1D sub-algorithm");
  }

  const double offset = fit_alg->getProperty("peakCentre");  
  return (-offset*step/(dreference+offset*step));
}



} // namespace Algorithm
} // namespace Mantid
