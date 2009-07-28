//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
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

CalculateTransmission::CalculateTransmission() : API::Algorithm(),m_progress(NULL)
{}

CalculateTransmission::~CalculateTransmission()
{
  if (m_progress) delete m_progress;
}

void CalculateTransmission::init()
{
  CompositeValidator<Workspace2D> *wsValidator = new CompositeValidator<Workspace2D>;
  wsValidator->add(new WorkspaceUnitValidator<Workspace2D>("Wavelength"));
  wsValidator->add(new CommonBinsValidator<Workspace2D>);
  
  declareProperty(new WorkspaceProperty<Workspace2D>("SampleRunWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<Workspace2D>("DirectRunWorkspace","",Direction::Input,wsValidator->clone()));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  // The defaults here are the correct detector numbers for LOQ
  declareProperty("IncidentBeamMonitor",2,"The UDET of the incident beam monitor");
  declareProperty("TransmissionMonitor",3,"The UDET of the transmission monitor");

  declareProperty("MinWavelength",2.2,"The minimum wavelength for the fit");
  declareProperty("MaxWavelength",10.0,"The maximum wavelength for the fit");

  declareProperty("OutputUnfittedData",false);
}

void CalculateTransmission::exec()
{
  Workspace2D_sptr sampleWS = getProperty("SampleRunWorkspace");
  Workspace2D_sptr directWS = getProperty("DirectRunWorkspace");

  // Check that the two input workspaces are from the same instrument
  if ( sampleWS->getBaseInstrument() != directWS->getBaseInstrument() )
  {
    g_log.error("The input workspaces do not come from the same instrument");
    throw std::invalid_argument("The input workspaces do not come from the same instrument");
  }
  // Check that the two inputs have matching binning
  if ( ! WorkspaceHelpers::matchingBins(sampleWS,directWS) )
  {
    g_log.error("Input workspaces do not have matching binning");
    throw std::invalid_argument("Input workspaces do not have matching binning");
  }
  
  // Extract the required spectra into separate workspaces
  std::vector<int> udets,indices;
  // For LOQ at least, the incident beam monitor's UDET is 2 and the transmission monitor is 3
  udets.push_back(getProperty("IncidentBeamMonitor"));
  udets.push_back(getProperty("TransmissionMonitor"));
  // Convert UDETs to workspace indices via spectrum numbers
  const std::vector<int> sampleSpectra = sampleWS->spectraMap().getSpectra(udets);
  WorkspaceHelpers::getIndicesFromSpectra(sampleWS,sampleSpectra,indices);
  // Check that given spectra are monitors
  if ( !sampleWS->getDetector(indices.front())->isMonitor()
       || !sampleWS->getDetector(indices.front())->isMonitor() )
  {
    g_log.error("One of the UDETs provided is not marked as a monitor");
    throw std::invalid_argument("One of the UDETs provided is not marked as a monitor");
  }
  MatrixWorkspace_sptr M2_sample = this->extractSpectrum(sampleWS,indices[0]);
  MatrixWorkspace_sptr M3_sample = this->extractSpectrum(sampleWS,indices[1]);
  const std::vector<int> directSpectra = directWS->spectraMap().getSpectra(udets);
  WorkspaceHelpers::getIndicesFromSpectra(sampleWS,directSpectra,indices);
  // Check that given spectra are monitors
  if ( !directWS->getDetector(indices.front())->isMonitor()
       || !directWS->getDetector(indices.front())->isMonitor() )
  {
    g_log.error("One of the UDETs provided is not marked as a monitor");
    throw std::invalid_argument("One of the UDETs provided is not marked as a monitor");
  }
  MatrixWorkspace_sptr M2_direct = this->extractSpectrum(directWS,indices[0]);
  MatrixWorkspace_sptr M3_direct = this->extractSpectrum(directWS,indices[1]);
  
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
  
 
  // Take the log of each datapoint for fitting. Preserve errors percentage-wise.
  std::vector<double> &Y = logTransmission->dataY(0);
  std::vector<double> &E = logTransmission->dataE(0);
  m_progress = new Progress(this,0.1,0.7,Y.size());
  for (unsigned int i=0; i < Y.size(); ++i)
  {
    const double errorPerc = E[i]/Y[i];
    Y[i] = std::log10(Y[i]);
    E[i] = std::abs(errorPerc*Y[i]);
    m_progress->report();
  }
  
  // Now fit this to a straight line
  MatrixWorkspace_sptr fit = this->fitToData(logTransmission);
   
  setProperty("OutputWorkspace",fit);
}

/** Extracts a single spectrum from a Workspace2D into a new workspaces. Uses CropWorkspace to do this.
 *  @param WS    The workspace containing the spectrum to extract
 *  @param index The workspace index of the spectrum to extract
 *  @return A Workspace2D containing the extracted spectrum
 */
API::MatrixWorkspace_sptr CalculateTransmission::extractSpectrum(DataObjects::Workspace2D_sptr WS, const int index)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("ExtractSingleSpectrum",0.0,0.1);
  childAlg->setProperty<Workspace2D_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("WorkspaceIndex", index);
  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("Unable to successfully run ExtractSingleSpectrum sub-algorithm");
    throw std::runtime_error("Unable to successfully run ExtractSingleSpectrum sub-algorithm");
  }

  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}

/** Uses 'Linear' as a subalgorithm to fit the log of the exponential curve expected for the transmission.
 *  @param WS The single-spectrum workspace to fit
 *  @return A workspace containing the fit
 */
API::MatrixWorkspace_sptr CalculateTransmission::fitToData(API::MatrixWorkspace_sptr WS)
{
  g_log.information("Fitting the experimental transmission curve");
  IAlgorithm_sptr childAlg = createSubAlgorithm("Linear",0.7,1.0);
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
  std::vector<double> &E = result->dataE(0);
  for (unsigned int i = 0; i < Y.size(); ++i)
  {
    const double errorPerc = E[i]/Y[i];
    Y[i] = b*(std::pow(m,0.5*(X[i]+X[i+1])));
    E[i] = std::abs(errorPerc*Y[i]);
  }

  return result;
}

} // namespace Algorithm
} // namespace Mantid
