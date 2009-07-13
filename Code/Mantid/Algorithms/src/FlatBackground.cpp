//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FlatBackground.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FlatBackground)

using namespace Kernel;
using namespace API;

void FlatBackground::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input ),
    "Name of the input workspace.");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output ),
    "Name to use for the output workspace.");
  declareProperty(new ArrayProperty<int>("SpectrumIndexList", new MandatoryValidator<std::vector<int> > ),
    "Indices of the spectra that will have their background removed");
  MandatoryValidator<double> *mustHaveValue = new MandatoryValidator<double>;
  declareProperty("StartX", Mantid::EMPTY_DBL(), mustHaveValue,
    "The X value at which to start the background fit");
  declareProperty("EndX", Mantid::EMPTY_DBL(), mustHaveValue->clone(),
    "The X value at which to end the background fit");
}

void FlatBackground::exec()
{
  // Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Get the required X range
  double startX,endX;
  this->checkRange(startX,endX);

  // Create the output workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS);
  // Copy over all the data
  const int numHists = inputWS->getNumberHistograms();
  const int blocksize = inputWS->blocksize();
 // Get the spectra under consideration
  const std::vector<int> toFit = getProperty("SpectrumIndexList");
 // Initialise the progress reporting object
  m_progress = new Progress(this,0.0,0.3,numHists); 
  PARALLEL_FOR2(inputWS,outputWS)
  for (int i=0; i < numHists; ++i)
  {
    outputWS->dataX(i) = inputWS->readX(i);
    outputWS->dataY(i) = inputWS->readY(i);
    outputWS->dataE(i) = inputWS->readE(i);
	m_progress->report();
  }
 
 // m_progress = new Progress(this,0.3,1.0,toFit.size()); 
  
  int toFitsize=toFit.size();
  double prg=0.3;
  std::vector<int>::const_iterator specIt;
  // Now loop over the required spectra
  for (specIt = toFit.begin(); specIt != toFit.end(); ++specIt)
  {
    const int currentSpec = *specIt;
    // Get references to the current spectrum
    std::vector<double> &Y = outputWS->dataY(currentSpec);
    //std::vector<double> &E = outputWS->dataE(currentSpec);

    // Now call Linear as a sub-algorithm to fit the data
    const double background = this->doFit(outputWS,currentSpec,startX,endX);
    if (background < 0)
    {
      g_log.warning() << "Background fit failed for spectrum with index " << currentSpec << std::endl;
      continue;
    }
    g_log.information() << "The background to be subtracted from spectrum " << currentSpec
                        << " is " << background << std::endl;

    // Now subtract the background from the data. Make sure it doesn't lead to negative values.
    for (int j=0; j < blocksize; ++j)
    {
      Y[j] -= background;
      if (Y[j] < 0.0) Y[j]=0;
      // Will do errors later...
    }
	prg+=(0.7/toFitsize);
	progress(double(prg));
    // m_progress->report();
  } // Loop over spectra to be fitted

  // Assign the output workspace to its property
  setProperty("OutputWorkspace",outputWS);
}

/** Checks that the range parameters have been set correctly
 *  @param startX The starting point
 *  @param endX   The ending point
 *  @throw std::invalid_argument If XMin or XMax are not set, or XMax is less than XMin
 */
void FlatBackground::checkRange(double& startX, double& endX)
{
  //use the overloaded operator =() to get the X value stored in each property
  startX = getProperty("StartX");
  endX = getProperty("EndX");
      
  if (startX > endX)
  {
    const std::string failure("XMax must be greater than XMin.");
    g_log.error(failure);
    throw std::invalid_argument(failure);
  }
}

/// Calls Linear as a sub-algorithm to do the fitting
double FlatBackground::doFit(API::MatrixWorkspace_sptr WS, int spectrum, double startX, double endX)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("Linear");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("SpectrumIndex",spectrum);
  childAlg->setProperty<double>("StartX",startX);
  childAlg->setProperty<double>("EndX",endX);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run Linear fit sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("Unable to successfully run Linear fit sub-algorithm");
    throw std::runtime_error("Unable to successfully run Linear fit sub-algorithm");
  }

  std::string fitStatus = childAlg->getProperty("FitStatus");
  if ( fitStatus != "success" )
  {
    g_log.warning("Unable to successfully fit the data");
    return -1.0;
  }

  // Calculate the value of the flat background by taking the value at the centre point of the fit
  const double c = childAlg->getProperty("FitIntercept");
  const double m = childAlg->getProperty("FitSlope");
  const double centre = (startX+endX)/2.0;
  const double background = m*centre + c;

  return background;
}

} // namespace Algorithms
} // namespace Mantid
