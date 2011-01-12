//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/VectorHelper.h"
#include <cfloat>
#include <iomanip>

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseToMonitor)

using namespace Kernel;
using namespace DataObjects;
using namespace API;

/// Default constructor
NormaliseToMonitor::NormaliseToMonitor() :
  Algorithm(), m_monitor(), m_commonBins(false),
  m_integrationMin( EMPTY_DBL() ),//EMPTY_DBL() is a tag to say that the value hasn't been set
  m_integrationMax( EMPTY_DBL() )
{}

/// Destructor
NormaliseToMonitor::~NormaliseToMonitor() {}

void NormaliseToMonitor::init()
{
  CompositeValidator<> *val = new CompositeValidator<>;
  val->add(new HistogramValidator<>);
  val->add(new RawCountValidator<>);
  // It's been said that we should restrict the unit to being wavelength, but I'm not sure about that...
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,val),
    "Name of the input workspace2D");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "Name to use for the output workspace");

  // Can either set a spectrum within the workspace to be the monitor spectrum.....
  declareProperty("MonitorSpectrum",-1,
    "The spectrum number of the monitor spectrum within the InputWorkspace");
  // ...or provide it in a separate workspace
  std::set<std::string> allowedValues = AnalysisDataService::Instance().getObjectNames();
  // Empty is allowed because this is an optional property
  allowedValues.insert("");
  // Note that this is a text property to enable it to be optional
  declareProperty("MonitorWorkspace","",new ListValidator(allowedValues),
    "A single-spectrum workspace containing the monitor spectrum");

  //If users set either of these optional properties two things happen
  //1) normalisation is by an integrated count instead of bin-by-bin
  //2) if the value is within the range of X's in the spectrum it crops the spectrum
  declareProperty( "IntegrationRangeMin", EMPTY_DBL(),
    "Entering a value here will cause normalisation by integrated count\n"
    "mode to be used and crop the spectrum removing any of the spectrum at\n"
    "lower X values" );
  declareProperty( "IntegrationRangeMax", EMPTY_DBL(),
    "Entering a value here will cause normalisation by integrated count\n"
    "mode to be used and crop the spectrum removing any of the spectrum at\n"
    "higher X values");
  declareProperty("IncludePartialBins", false, 
    "If true and an integration range is set then partial bins at either \n"
    "end of the integration range are also included");
}

void NormaliseToMonitor::exec()
{
  // Get the input workspace
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  // First check the inputs, throws std::runtime_error if a property is invalid
  this->checkProperties(inputWS);
  // See if the normalisation with integration properties are set,
  // throws std::runtime_error if a property is invalid
  const bool integrate = this->setIntegrationProps(inputWS);

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  if ( integrate )
  {
    this->normaliseByIntegratedCount(inputWS,outputWS);
  }
  else
  {
    this->normaliseBinByBin(inputWS,outputWS);
  }

  setProperty("OutputWorkspace",outputWS);
}

/** Makes sure that the input properties are set correctly
 *  @param inputWorkspace the input workspace
 *  @throw std::runtime_error If a property is invalid
 */
void NormaliseToMonitor::checkProperties(API::MatrixWorkspace_sptr inputWorkspace)
{
  // Check where the monitor spectrum should come from
  Property* monSpec = getProperty("MonitorSpectrum");
  Property* monWS = getProperty("MonitorWorkspace");
  // Is the monitor spectrum within the main input workspace
  const bool inWS = !monSpec->isDefault();
  // Or is it in a separate workspace
  const bool sepWS = !monWS->isDefault();
  // One and only one of these properties should have been set
  if ( !inWS && !sepWS )
  {
    const std::string mess("Neither the MonitorSpectrum nor the MonitorWorkspace property has been set");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }
  if ( inWS && sepWS )
  {
    const std::string mess("Only one of the MonitorSpectrum and MonitorWorkspace properties should be set");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }

  // Do a check for common binning
  m_commonBins = API::WorkspaceHelpers::commonBoundaries(inputWorkspace);

  if (inWS)
  {
    // Get hold of the monitor spectrum
    const int monitorSpec = getProperty("MonitorSpectrum");
    if (monitorSpec < 1)
    {
      g_log.error("MonitorSpectrum must be at least 1");
    }
    SpectraAxis::spec2index_map specs;
    const SpectraAxis* axis = dynamic_cast<const SpectraAxis*>(inputWorkspace->getAxis(1));
    if (axis)
    {
      axis->getSpectraIndexMap(specs);
      const int monitorIndex = specs[monitorSpec];
      m_monitor = this->extractMonitorSpectrum(inputWorkspace,monitorIndex);
    }
  }
  else
  { 
    // Get the workspace from the ADS. Will throw if it's not there.
    MatrixWorkspace_sptr monitorWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve(getPropertyValue("MonitorWorkspace")));
    // Check it's the right type
    if (!monitorWS)
    {
      g_log.error("MonitorWorkspace must be a MatrixWorkspace");
      throw std::runtime_error("MonitorWorkspace must be a MatrixWorkspace");
    }
    // Check that it's a single spectrum workspace
    if ( monitorWS->getNumberHistograms() != 1 )
    {
      g_log.error("The MonitorWorkspace must contain only 1 spectrum");
      throw std::runtime_error("The MonitorWorkspace must contain only 1 spectrum");
    }
    // Check it's not a distribution
    if ( monitorWS->isDistribution() )
    {
      g_log.error("The MonitorWorkspace must not be a distribution");
      throw std::runtime_error("The MonitorWorkspace must not be a distribution");
    }
    // Check that it contains histogram data
    if ( !monitorWS->isHistogramData() )
    {
      g_log.error("The MonitorWorkspace must contain histogram data");
      throw std::runtime_error("The MonitorWorkspace must contain histogram data");
    }
    // Check that the two workspace come from the same instrument
    if ( monitorWS->getBaseInstrument() != inputWorkspace->getBaseInstrument() )
    {
      g_log.error("The Input and Monitor workspaces must come from the same instrument");
      throw std::runtime_error("The Input and Monitor workspaces must come from the same instrument");
    }
    // Check that they're in the same units
    if ( monitorWS->getAxis(0)->unit() != inputWorkspace->getAxis(0)->unit() )
    {
      g_log.error("The Input and Monitor workspaces must have the same unit");
      throw std::runtime_error("The Input and Monitor workspaces must have the same unit");
    }

    // In this case we need to test whether the bins in the monitor workspace match
    m_commonBins = (m_commonBins && 
                    API::WorkspaceHelpers::matchingBins(inputWorkspace,monitorWS,true) );

    // If the workspace passes all these tests, then make a local copy because this will
    // get changed
    m_monitor = this->extractMonitorSpectrum(monitorWS,0);
  }

  // Check that the 'monitor' spectrum actually relates to a monitor - warn if not
  try {
    Geometry::IDetector_const_sptr mon = m_monitor->getDetector(0);
    if ( !mon->isMonitor() )
    {
      g_log.warning("The spectrum in MonitorWorkspace does not refer to a monitor.\n"
                    "Continuing with normalisation regardless.");
    }
  } catch (Kernel::Exception::NotFoundError) {
    g_log.warning("Unable to check if the spectrum provided relates to a monitor - "
                  "the instrument is not fully specified.\n"
                  "Continuing with normalisation regardless.");
  }

}

/** Pulls the monitor spectrum out of a larger workspace
 *  @param WS The workspace containing the spectrum to extract
 *  @param index The index of the spectrum to extract
 *  @returns A workspace containing the single spectrum requested
 */
API::MatrixWorkspace_sptr NormaliseToMonitor::extractMonitorSpectrum(API::MatrixWorkspace_sptr WS, const int index)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("ExtractSingleSpectrum");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
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

/** Sets the maximum and minimum X values of the monitor spectrum to use for integration
 *  @param inputWorkspace A constant shared pointer to the input workspace
 *  @return True if the maximum or minimum values are set
 *  @throw std::runtime_error If the minimum was set higher than the maximum
 */
bool NormaliseToMonitor::setIntegrationProps(API::MatrixWorkspace_const_sptr inputWorkspace)
{
  (void) inputWorkspace; //Avoid compiler warning
  //get the values if set the default values of the m_inter*s and the properties are EMPTY_DBL()
  m_integrationMin = getProperty("IntegrationRangeMin");
  m_integrationMax = getProperty("IntegrationRangeMax");

  //check if neither of these have been changed using the function in the MandatoryValidator
  if ( isEmpty(m_integrationMin) && isEmpty(m_integrationMax) )
  {
    //nothing has been set so the user doesn't want to use integration so let's move on
    return false;
  }
  //yes integration is going to be used

  //There is only one set of values that is unacceptable let's check for that
  if ( !isEmpty(m_integrationMin) && !isEmpty(m_integrationMax) )
  {
    if ( m_integrationMin > m_integrationMax )
    {
      g_log.error("Integration minimum set to larger value than maximum!");
      throw std::runtime_error("Integration minimum set to larger value than maximum!");
    }
  }

  //now check the end X values are within the X value range of the workspace
  if ( isEmpty(m_integrationMin) || m_integrationMin < m_monitor->readX(0).front() )
  {
    g_log.warning() << "Integration range minimum set to workspace min: " << m_integrationMin << std::endl;
    m_integrationMin = m_monitor->readX(0).front();
  }
  if ( isEmpty(m_integrationMax) || m_integrationMax > m_monitor->readX(0).back() )
  {
    g_log.warning() << "Integration range maximum set to workspace max: " << m_integrationMax << std::endl;
    m_integrationMax = m_monitor->readX(0).back();
  }
  
  //Return indicating that these properties should be used
  return true;
}

/** Carries out a normalisation based on the integrated count of the monitor over a range
 *  @param inputWorkspace A pointer to the input workspace
 *  @param outputWorkspace A pointer to the result workspace
 */
void NormaliseToMonitor::normaliseByIntegratedCount(API::MatrixWorkspace_sptr inputWorkspace,
                                                    API::MatrixWorkspace_sptr& outputWorkspace)
{
  // Add up all the bins so it's just effectively a single value with an error
  IAlgorithm_sptr integrate = createSubAlgorithm("Integration");
  integrate->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_monitor);
  integrate->setProperty("RangeLower",m_integrationMin);
  integrate->setProperty("RangeUpper",m_integrationMax);
  const bool incPartBins = getProperty("IncludePartialBins");
  integrate->setProperty("IncludePartialBins",incPartBins);
  try {
    integrate->execute();
  } catch (std::runtime_error&) {
    g_log.error("Unable to successfully run Integration sub-algorithm");
    throw;
  }
  // Get back the result
  m_monitor = integrate->getProperty("OutputWorkspace");

  // Run the divide algorithm explicitly to enable progress reporting
  IAlgorithm_sptr divide = createSubAlgorithm("Divide",0.0,1.0);
  divide->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", inputWorkspace);
  divide->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", m_monitor);
  divide->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWorkspace);
  try {
    divide->execute();
  } catch (std::runtime_error&) {
    g_log.error("Unable to successfully run Divide sub-algorithm");
    throw;
  }
  // Get back the result
  outputWorkspace = divide->getProperty("OutputWorkspace");
}

/** Carries out the bin-by-bin normalisation
 *  @param inputWorkspace The input workspace
 *  @param outputWorkspace The output workspace
 */
void NormaliseToMonitor::normaliseBinByBin(API::MatrixWorkspace_sptr inputWorkspace,
                                           API::MatrixWorkspace_sptr& outputWorkspace)
{ 
  // Only create output workspace if different to input one
  if (outputWorkspace != inputWorkspace ) 
    outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);

//  // ---- For event workspaces, the Divide operator will do the work. -----
//  if (boost::dynamic_pointer_cast<const EventWorkspace>(inputWorkspace))
//  {
//    if (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace"))
//    {
//      outputWorkspace /= m_monitor;
//    }
//    else
//    {
//      //outputWorkspace = inputWorkspace / boost::dynamic_pointer_cast<const MatrixWorkspace>(m_monitor);
//      outputWorkspace = inputWorkspace / m_monitor;
//    }
//    return;
//  }

  // Get hold of the monitor spectrum
  const MantidVec& monX = m_monitor->readX(0);
  MantidVec& monY = m_monitor->dataY(0);
  MantidVec& monE = m_monitor->dataE(0);
  // Calculate the overall normalisation just the once if bins are all matching
  if (m_commonBins) this->normalisationFactor(m_monitor->readX(0),&monY,&monE);

  const int numHists = inputWorkspace->getNumberHistograms();
  MantidVec::size_type specLength = inputWorkspace->blocksize();
  Progress prog(this,0.0,1.0,numHists);
  // Loop over spectra
  PARALLEL_FOR3(inputWorkspace,outputWorkspace,m_monitor)
  for (int i = 0; i < numHists; ++i)
  {
		PARALLEL_START_INTERUPT_REGION
    prog.report();

    const MantidVec& X = inputWorkspace->readX(i);
    // If not rebinning, just point to our monitor spectra, otherwise create new vectors
    MantidVec* Y = ( m_commonBins ? &monY : new MantidVec(specLength) );
    MantidVec* E = ( m_commonBins ? &monE : new MantidVec(specLength) );

    if (!m_commonBins)
    {
      // ConvertUnits can give X vectors of all zeroes - skip these, they cause problems
      if (X.back() == 0.0 && X.front() == 0.0) continue;
      // Rebin the monitor spectrum to match the binning of the current data spectrum
      VectorHelper::rebinHistogram(monX,monY,monE,X,*Y,*E,false);
      // Recalculate the overall normalisation factor
      this->normalisationFactor(X,Y,E);
    }

    const MantidVec& inY = inputWorkspace->readY(i);
    const MantidVec& inE = inputWorkspace->readE(i);
    MantidVec& YOut = outputWorkspace->dataY(i);
    MantidVec& EOut = outputWorkspace->dataE(i);
    outputWorkspace->dataX(i) = inputWorkspace->readX(i);
    // The code below comes more or less straight out of Divide.cpp
    for (MantidVec::size_type k = 0; k < specLength; ++k)
    {
      // Get references to the input Y's
      const double& leftY = inY[k];
      const double& rightY = (*Y)[k];

      // Calculate result and store in local variable to avoid overwriting original data if
      // output workspace is same as one of the input ones
      const double newY = leftY/rightY;

      if (fabs(rightY)>1.0e-12 && fabs(newY)>1.0e-12)
      {
        const double lhsFactor = (inE[k]<1.0e-12|| fabs(leftY)<1.0e-12) ? 0.0 : pow((inE[k]/leftY),2);
        const double rhsFactor = (*E)[k]<1.0e-12 ? 0.0 : pow(((*E)[k]/rightY),2);
        EOut[k] = std::abs(newY) * sqrt(lhsFactor+rhsFactor);
      }

      // Now store the result
      YOut[k] = newY;
    } // end loop over current spectrum

    if (!m_commonBins) { delete Y; delete E; }
		PARALLEL_END_INTERUPT_REGION
  } // end loop over spectra
	PARALLEL_CHECK_INTERUPT_REGION
}

/** Calculates the overall normalisation factor.
 *  This multiplies result by (bin width * sum of monitor counts) / total frame width.
 *  @param X The X vector
 *  @param Y The data vector
 *  @param E The error vector
 */
void NormaliseToMonitor::normalisationFactor(const MantidVec& X, MantidVec* Y, MantidVec* E)
{
  const double monitorSum = std::accumulate(Y->begin(), Y->end(), 0.0);
  const double range = X.back() - X.front();
  MantidVec::size_type specLength = Y->size();
  for (MantidVec::size_type j = 0; j < specLength; ++j)
  {
    const double factor = range / ( (X[j+1]-X[j]) * monitorSum );
    (*Y)[j] *= factor;
    (*E)[j] *= factor;
  }
}

} // namespace Algorithm
} // namespace Mantid
