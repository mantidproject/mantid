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

/// Sets documentation strings for this algorithm
void CalculateTransmission::initDocs()
{
  this->setWikiSummary("Calculates the transmission correction, as a function of wavelength, for a SANS instrument. ");
  this->setOptionalMessage("Calculates the transmission correction, as a function of wavelength, for a SANS instrument.");
}


using namespace Kernel;
using namespace API;
using std::size_t;

CalculateTransmission::CalculateTransmission() : API::Algorithm(), logFit(false)
{}

CalculateTransmission::~CalculateTransmission()
{}

void CalculateTransmission::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new CommonBinsValidator<>);
  wsValidator->add(new HistogramValidator<>);
  
  declareProperty(new WorkspaceProperty<>("SampleRunWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("DirectRunWorkspace","",Direction::Input,wsValidator->clone()));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  BoundedValidator<int> *zeroOrMore = new BoundedValidator<int>();
  zeroOrMore->setLower(0);
  // The defaults here are the correct detector numbers for LOQ
  declareProperty("IncidentBeamMonitor",2,zeroOrMore,"The UDET of the incident beam monitor");
  declareProperty("TransmissionMonitor",3,zeroOrMore->clone(),"The UDET of the transmission monitor");

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);  
  declareProperty("MinWavelength",2.2,mustBePositive,"The minimum wavelength for the fit");
  declareProperty("MaxWavelength",10.0,mustBePositive->clone(),"The maximum wavelength for the fit");

  std::vector<std::string> options(2);
  options[0] = "Linear";
  options[1] = "Log";
  declareProperty("FitMethod","Log",new ListValidator(options),
    "Whether to fit directly to the transmission curve (Linear) or to the log of it (Log)");

  declareProperty("OutputUnfittedData",false);
}

void CalculateTransmission::exec()
{
  MatrixWorkspace_sptr sampleWS = getProperty("SampleRunWorkspace");
  MatrixWorkspace_sptr directWS = getProperty("DirectRunWorkspace");

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
  std::vector<int64_t> udets,indices;
  // For LOQ at least, the incident beam monitor's UDET is 2 and the transmission monitor is 3
  udets.push_back(getProperty("IncidentBeamMonitor"));
  udets.push_back(getProperty("TransmissionMonitor"));
  // Convert UDETs to workspace indices via spectrum numbers
  const std::vector<int64_t> sampleSpectra = sampleWS->spectraMap().getSpectra(udets);
  sampleWS->getIndicesFromSpectra(sampleSpectra,indices);
  if (indices.size() < 2)
  {
    if (indices.size() == 1)
    {
      g_log.error() << "Incident and transmitted spectra must be set to different spectra that exist in the workspaces. Only found one valid index " << indices.front() << std::endl;
    }
    else
    {
      g_log.debug() << "sampleWS->getIndicesFromSpectra() returned empty\n";
    }
    throw std::invalid_argument("Could not find the incident and transmission monitor spectra\n");
  }
  // Check that given spectra are monitors
  if ( !sampleWS->getDetector(indices.front())->isMonitor() )
  {
    g_log.information("The Incident Beam Monitor UDET provided is not marked as a monitor");
  }
  if ( !sampleWS->getDetector(indices.back())->isMonitor() )
  {
    g_log.information("The Transmission Monitor UDET provided is not marked as a monitor");
  }
  MatrixWorkspace_sptr M2_sample = this->extractSpectrum(sampleWS,indices[0]);
  MatrixWorkspace_sptr M3_sample = this->extractSpectrum(sampleWS,indices[1]);
  const std::vector<int64_t> directSpectra = directWS->spectraMap().getSpectra(udets);
  sampleWS->getIndicesFromSpectra(directSpectra,indices);
  // Check that given spectra are monitors
  if ( !directWS->getDetector(indices.front())->isMonitor() )
  {
    g_log.information("The Incident Beam Monitor UDET provided is not marked as a monitor");
  }
  if ( !directWS->getDetector(indices.back())->isMonitor() )
  {
    g_log.information("The Transmission Monitor UDET provided is not marked as a monitor");
  }
  MatrixWorkspace_sptr M2_direct = this->extractSpectrum(directWS,indices[0]);
  MatrixWorkspace_sptr M3_direct = this->extractSpectrum(directWS,indices[1]);
  
  // The main calculation
  MatrixWorkspace_sptr transmission = (M3_sample/M3_direct)*(M2_direct/M2_sample);
  // This workspace is now a distribution
  //transmission->isDistribution(true);

  // Output this data if requested
  const bool outputRaw = getProperty("OutputUnfittedData");
  if ( outputRaw )
  {
    std::string outputWSName = getPropertyValue("OutputWorkspace");
    outputWSName += "_unfitted";
    declareProperty(new WorkspaceProperty<>("UnfittedData",outputWSName,Direction::Output));
    setProperty("UnfittedData",transmission);
  }
  
  // Check that there are more than a single bin in the transmission
  // workspace. Skip the fit it there isn't.
  if (transmission->dataY(0).size()==1)
  {
    setProperty("OutputWorkspace",transmission);
  }
  else
  {
    MatrixWorkspace_sptr fit;
    const std::string fitMethod = getProperty("FitMethod");
    logFit = ( fitMethod == "Log" );
    if (logFit)
    {
      g_log.debug("Fitting to the logarithm of the transmission");
      // Take a copy of this workspace for the fitting
      MatrixWorkspace_sptr logTransmission = this->extractSpectrum(transmission,0);

      // Take the log of each datapoint for fitting. Preserve errors percentage-wise.
      MantidVec & Y = logTransmission->dataY(0);
      MantidVec & E = logTransmission->dataE(0);
      Progress progress(this,0.4,0.6,Y.size());
      for (size_t i=0; i < Y.size(); ++i)
      {
        E[i] = std::abs(E[i]/Y[i]);
        Y[i] = std::log10(Y[i]);
        progress.report();
      }
  
      // Now fit this to a straight line
      fit = this->fitToData(logTransmission);
    } // logFit true
    else
    {
      g_log.debug("Fitting directly to the data (i.e. linearly)");
      fit = this->fitToData(transmission);
    }

    setProperty("OutputWorkspace",fit);
  }
}

/** Extracts a single spectrum from a Workspace2D into a new workspaces. Uses CropWorkspace to do this.
 *  @param WS ::    The workspace containing the spectrum to extract
 *  @param index :: The workspace index of the spectrum to extract
 *  @return A Workspace2D containing the extracted spectrum
 */
API::MatrixWorkspace_sptr CalculateTransmission::extractSpectrum(API::MatrixWorkspace_sptr WS, const int64_t index)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("ExtractSingleSpectrum",0.0,0.4);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int64_t>("WorkspaceIndex", index);
  childAlg->executeAsSubAlg();
  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}

/** Uses 'Linear' as a subalgorithm to fit the log of the exponential curve expected for the transmission.
 *  @param WS :: The single-spectrum workspace to fit
 *  @return A workspace containing the fit
 */
API::MatrixWorkspace_sptr CalculateTransmission::fitToData(API::MatrixWorkspace_sptr WS)
{
  g_log.information("Fitting the experimental transmission curve");
  IAlgorithm_sptr childAlg = createSubAlgorithm("Linear",0.6,1.0);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  const double lambdaMin = getProperty("MinWavelength");
  const double lambdaMax = getProperty("MaxWavelength");
  childAlg->setProperty<double>("StartX",lambdaMin);
  childAlg->setProperty<double>("EndX",lambdaMax);
  childAlg->executeAsSubAlg();

  std::string fitStatus = childAlg->getProperty("FitStatus");
  if ( fitStatus != "success" )
  {
    g_log.error("Unable to successfully fit the data: " + fitStatus);
    throw std::runtime_error("Unable to successfully fit the data");
  }
 
  // Only get to here if successful
  MatrixWorkspace_sptr result = childAlg->getProperty("OutputWorkspace");

  if (logFit)
  {
    // Need to transform back to 'unlogged'
    double b = childAlg->getProperty("FitIntercept");
    double m = childAlg->getProperty("FitSlope");
    b = std::pow(10,b);
    m = std::pow(10,m);

    const MantidVec & X = result->readX(0);
    MantidVec & Y = result->dataY(0);
    MantidVec & E = result->dataE(0);
    for (size_t i = 0; i < Y.size(); ++i)
    {
      Y[i] = b*(std::pow(m,0.5*(X[i]+X[i+1])));
      E[i] = std::abs(E[i]*Y[i]);
    }
  }

  return result;
}

} // namespace Algorithm
} // namespace Mantid
