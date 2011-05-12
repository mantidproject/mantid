//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"
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

CalculateTransmission::CalculateTransmission() : API::Algorithm(), m_done(0.0)
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

  declareProperty(new ArrayProperty<double>("RebinParams"),
    "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
    "this can be followed by a comma and more widths and last boundary pairs.\n"
    "Negative width values indicate logarithmic binning.");

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
  std::vector<int> udets,indices;
  // For LOQ at least, the incident beam monitor's UDET is 2 and the transmission monitor is 3
  udets.push_back(getProperty("IncidentBeamMonitor"));
  udets.push_back(getProperty("TransmissionMonitor"));
  // Convert UDETs to workspace indices via spectrum numbers
  const std::vector<int> sampleSpectra = sampleWS->spectraMap().getSpectra(udets);
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
  const std::vector<int> directSpectra = directWS->spectraMap().getSpectra(udets);
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
  
  Progress progress(this, m_done, m_done += 0.2, 2);
  progress.report("CalculateTransmission: Dividing transmission by incident");
  // The main calculation
  MatrixWorkspace_sptr transmission = (M3_sample/M3_direct)*(M2_direct/M2_sample);
  // This workspace is now a distribution
  progress.report("CalculateTransmission: Dividing transmission by incident");

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
  if ( transmission->dataY(0).size() > 1 )
  {
    transmission = fit(transmission, getProperty("RebinParams"), getProperty("FitMethod"));
  }
  setProperty("OutputWorkspace", transmission);
}

/** Extracts a single spectrum from a Workspace2D into a new workspaces. Uses CropWorkspace to do this.
 *  @param WS ::    The workspace containing the spectrum to extract
 *  @param index :: The workspace index of the spectrum to extract
 *  @return A Workspace2D containing the extracted spectrum
 *  @throw runtime_error if the ExtractSingleSpectrum algorithm fails during execution
 */
API::MatrixWorkspace_sptr CalculateTransmission::extractSpectrum(API::MatrixWorkspace_sptr WS, const int index)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("ExtractSingleSpectrum", m_done, m_done += 0.1);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("WorkspaceIndex", index);
  childAlg->executeAsSubAlg();

  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}
/** Calculate a workspace that contains the result of the fit to the transmission fraction that was calculated
*  @param[in] fitMethod string can either be Log or Linear
*  @param[in] WS the workspace with the unfitted transmission ratio data
*  @return a workspace that contains the evaluation of the fit
*  @throw runtime_error if the Linear or ExtractSpectrum algorithm fails during execution
*/
API::MatrixWorkspace_sptr CalculateTransmission::fit(API::MatrixWorkspace_sptr raw, std::vector<double> rebinParams, const std::string fitMethod)
{
  MatrixWorkspace_sptr output = this->extractSpectrum(raw,0);

  Progress progress(this, m_done, 1.0, 4);
  progress.report("CalculateTransmission: Performing fit");

  //these are calculated by the call to fit below
  double grad(0.0), offset(0.0);

  const bool logFit = ( fitMethod == "Log" );
  if (logFit)
  {
    g_log.debug("Fitting to the logarithm of the transmission");

    MantidVec & Y = output->dataY(0);
    MantidVec & E = output->dataE(0);
    Progress prog2(this, m_done, m_done+=0.1 ,Y.size());
    for (size_t i=0; i < Y.size(); ++i)
    {
      // Take the log of each datapoint for fitting. Recalculate errors remembering that d(log(a))/da  = 1/a
      E[i] = std::abs(E[i]/Y[i]);
      Y[i] = std::log10(Y[i]);
      progress.report("Fitting to the logarithm of the transmission");
    }
  
    // Now fit this to a straight line
    output = fitData(output, grad, offset);
  } // logFit true
  else
  { // Linear fit
    g_log.debug("Fitting directly to the data (i.e. linearly)");
    output = fitData(output, grad, offset);
  }

  progress.report("CalculateTransmission: Performing fit");

  //if no rebin parameters were set the output workspace will have the same binning as the input ones, otherwise rebin
  if ( ! rebinParams.empty() )
  {
    output = rebin(rebinParams, output);
  }
  progress.report("CalculateTransmission: Performing fit");
  // if there was rebinnning or log fitting we need to recalculate the Ys, otherwise we can just use the workspace kicked out by the fitData()'s call to Linear
  if ( ( ! rebinParams.empty() ) || logFit)
  {
    const MantidVec & X = output->readX(0);
    MantidVec & Y = output->dataY(0);
    if (logFit)
    {
      // Need to transform back to 'unlogged'
      const double m(std::pow(10,grad));
      const double factor(std::pow(10,offset));

      MantidVec & E = output->dataE(0);
      for (size_t i = 0; i < Y.size(); ++i)
      {
        //the relationship between the grad and interspt of the log fit and the un-logged value of Y contain this dependence on the X (bin center values)
        Y[i] = factor*(std::pow(m,0.5*(X[i]+X[i+1])));
        E[i] = std::abs(E[i]*Y[i]);
        progress.report();
      }
    }// end logFit
    else
    {
      //the simplar linear situation
      for (size_t i = 0; i < Y.size(); ++i)
      {
        Y[i] = (grad*0.5*(X[i]+X[i+1]))+offset;
      }
    }
  }
  progress.report("CalculateTransmission: Performing fit");

  return output;
}
/** Uses 'Linear' as a subalgorithm to fit the log of the exponential curve expected for the transmission.
 *  @param[in] WS The single-spectrum workspace to fit
 *  @param[out] grad The single-spectrum workspace to fit
 *  @param[out] offset The single-spectrum workspace to fit
 *  @return A workspace containing the fit
 *  @throw runtime_error if the Linear algorithm fails during execution
 */
API::MatrixWorkspace_sptr CalculateTransmission::fitData(API::MatrixWorkspace_sptr WS, double & grad, double & offset)
{
  g_log.information("Fitting the experimental transmission curve");
  IAlgorithm_sptr childAlg = createSubAlgorithm("Linear", m_done, m_done=0.9);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->executeAsSubAlg();

  std::string fitStatus = childAlg->getProperty("FitStatus");
  if ( fitStatus != "success" )
  {
    g_log.error("Unable to successfully fit the data: " + fitStatus);
    throw std::runtime_error("Unable to successfully fit the data");
  }
 
  // Only get to here if successful
  offset = childAlg->getProperty("FitIntercept");
  grad = childAlg->getProperty("FitSlope");
  
  return childAlg->getProperty("OutputWorkspace");
}
/** Calls rebin as sub-algorithm
*  @param rebinParams this string is passed to rebin as the "Params" property
*  @param ws the workspace to rebin
*  @return the resultant rebinned workspace
*  @throw runtime_error if the rebin algorithm fails during execution
*/
API::MatrixWorkspace_sptr CalculateTransmission::rebin(std::vector<double> & binParams, API::MatrixWorkspace_sptr ws)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("Rebin", m_done, m_done += 0.05);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
  childAlg->setProperty< std::vector<double> >("Params", binParams);
  childAlg->executeAsSubAlg();
  
  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}

} // namespace Algorithm
} // namespace Mantid
