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

// Get a reference to the logger. It is used to print out information, warning and error messages
Logger& FlatBackground::g_log = Logger::get("FlatBackground");

void FlatBackground::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty(new ArrayProperty<int>("SpectrumIndexList", new MandatoryValidator<std::vector<int> >));
  declareProperty("StartX",0.0);
  declareProperty("EndX",0.0);
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
  for (int i=0; i < numHists; ++i)
  {
    outputWS->dataX(i) = inputWS->readX(i);
    outputWS->dataY(i) = inputWS->readY(i);
    outputWS->dataE(i) = inputWS->readE(i);
  }

  // Get the spectra under consideration
  const std::vector<int> toFit = getProperty("SpectrumIndexList");
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

  } // Loop over spectra to be fitted

  // Assign the output workspace to its property
  setProperty("OutputWorkspace",outputWS);
}

/// Checks that the range parameters have been set correctly
void FlatBackground::checkRange(double& startX, double& endX)
{
  // Both XMin and XMax are mandatory
  Property* XMin = getProperty("StartX");
  Property* XMax = getProperty("EndX");
  if ( XMin->isDefault() || XMax->isDefault() )
  {
    g_log.error("This algorithm requires that both XMin and XMax are set");
    throw std::invalid_argument("This algorithm requires that both XMin and XMax are set");
  }

  // If that was OK, then we can get their values
  startX = getProperty("StartX");
  endX = getProperty("EndX");
      
  if (startX > endX)
  {
    g_log.warning("StartX greater than EndX: the two have been swapped.");
    const double temp = startX;
    startX = endX;
    endX = temp;
  }
}

/// Calls Linear as a sub-algorithm to do the fitting
double FlatBackground::doFit(API::MatrixWorkspace_sptr WS, int spectrum, double startX, double endX)
{
  Algorithm_sptr childAlg = createSubAlgorithm("Linear");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("SpectrumIndex",spectrum);
  childAlg->setProperty<double>("StartX",startX);
  childAlg->setProperty<double>("EndX",endX);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error& err)
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
