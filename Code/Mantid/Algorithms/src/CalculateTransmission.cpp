//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <cmath>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateTransmission)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Get a reference to the logger. It is used to print out information, warning and error messages
Logger& CalculateTransmission::g_log = Logger::get("CalculateTransmission");

static const double dblLog(double in) { return std::log10(in); }

void CalculateTransmission::init()
{
  CompositeValidator<Workspace2D> *wsValidator = new CompositeValidator<Workspace2D>;
  wsValidator->add(new WorkspaceUnitValidator<Workspace2D>("Wavelength"));
  wsValidator->add(new CommonBinsValidator<Workspace2D>);
  
  declareProperty(new WorkspaceProperty<Workspace2D>("SampleRunWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<Workspace2D>("DirectRunWorkspace","",Direction::Input,wsValidator->clone()));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("MinWavelength",2.2);
  declareProperty("MaxWavelength",10.0);

  declareProperty("OutputUnfittedData",false);
}

void CalculateTransmission::exec()
{
  Workspace2D_sptr sampleWS = getProperty("SampleRunWorkspace");
  Workspace2D_sptr directWS = getProperty("DirectRunWorkspace");

  // Check that the two inputs have matching binning
  if ( ! WorkspaceHelpers::matchingBins(sampleWS,directWS) )
  {
    g_log.error("Input workspaces do not have matching binning");
    throw std::invalid_argument("Input workspaces do not have matching binning");
  }
  
  // Extract the required spectra into separate workspaces
  // Hard coding the spectrum indices is risky - what if someone doesn't load whole workspace?
  // Better solution would be to retrieve the indices from the linked UDET
  MatrixWorkspace_sptr M2_sample = this->extractSpectrum(sampleWS,1);
  MatrixWorkspace_sptr M3_sample = this->extractSpectrum(sampleWS,2);
  MatrixWorkspace_sptr M2_direct = this->extractSpectrum(directWS,1);
  MatrixWorkspace_sptr M3_direct = this->extractSpectrum(directWS,2);
  
  // Need to remove prompt spike and correct for flat background in each
  
  // The main calculation
  MatrixWorkspace_sptr transmission = (M3_sample/M3_direct)*(M2_direct/M2_sample);

  // Output this data if requested
  const bool outputRaw = getProperty("OutputUnfittedData");
  if ( outputRaw )
  {
    std::string outputWSName = getPropertyValue("OutputWorkspace");
    outputWSName += "_unfitted";
    declareProperty(new WorkspaceProperty<>("UnfittedData",outputWSName,Direction::Output));
    setProperty("UnfittedData",transmission);
  }

  // Take a copy of this workspace for the fitting
  MatrixWorkspace_sptr logTransmission = this->extractSpectrum(boost::dynamic_pointer_cast<DataObjects::Workspace2D>(transmission),0);
  // Take the log of each datapoint for fitting. Zero the errors until I work out what to do with them.
  std::vector<double> &Y = logTransmission->dataY(0);
  std::transform(Y.begin(),Y.end(),Y.begin(),dblLog);
  std::vector<double> &E = logTransmission->dataE(0);
  E.assign(E.size(),0.0);
 
  // Now fit this to a straight line
  MatrixWorkspace_sptr fit = this->fitToData(logTransmission);
  // What do I do with the errors? Zero for now
  std::vector<double> &Efit = fit->dataE(0);
  Efit.assign(Efit.size(),0.0);
  
  // TEMPORARY: set units on outputworkspace - this should be done in Linear
  fit->getAxis(0)->unit() = transmission->getAxis(0)->unit();

  setProperty("OutputWorkspace",fit);
}

API::MatrixWorkspace_sptr CalculateTransmission::extractSpectrum(DataObjects::Workspace2D_sptr WS, const int index)
{
  // Would be better to write an 'ExtractSingleSpectrum' algorithm for here, that returns a Workspace1D
  Algorithm_sptr childAlg = createSubAlgorithm("CropWorkspace");
  childAlg->setProperty<Workspace2D_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("StartSpectrum", index);
  childAlg->setProperty<int>("EndSpectrum", index);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error& err)
  {
    g_log.error("Unable to successfully run sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("Unable to successfully run sub-algorithm");
    throw std::runtime_error("Unable to successfully run sub-algorithm");
  }

  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr CalculateTransmission::fitToData(API::MatrixWorkspace_sptr WS)
{
  g_log.information("Fitting the experimental transmission curve");
  Algorithm_sptr childAlg = createSubAlgorithm("Linear");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  const double lambdaMin = getProperty("MinWavelength");
  const double lambdaMax = getProperty("MaxWavelength");
  childAlg->setProperty<double>("StartX",lambdaMin);
  childAlg->setProperty<double>("EndX",lambdaMax);

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
    g_log.error("Unable to successfully fit the data");
    throw std::runtime_error("Unable to successfully fit the data");
  }
  
  // Only get to here if successful
  MatrixWorkspace_sptr result = childAlg->getProperty("OutputWorkspace");
  // Need to transform back to 'unlogged'
  double b = childAlg->getProperty("FitIntercept");
  double m = childAlg->getProperty("FitSlope");
  b = std::pow(10,b);
  m = std::pow(10,m);

  const std::vector<double> &X = result->readX(0);
  std::vector<double> &Y = result->dataY(0);
  for (unsigned int i = 0; i < Y.size(); ++i)
  {
    Y[i] = b*(std::pow(m,0.5*(X[i]+X[i+1])));
  }

  return result;
}

} // namespace Algorithm
} // namespace Mantid
