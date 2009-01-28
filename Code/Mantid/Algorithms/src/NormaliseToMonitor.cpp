//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include <cfloat>

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseToMonitor)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;
using DataObjects::Workspace2D_const_sptr;

// Get a reference to the logger
Logger& NormaliseToMonitor::g_log = Logger::get("NormaliseToMonitor");

/// Default constructor
NormaliseToMonitor::NormaliseToMonitor() :
  Algorithm(), m_monitorIndex(-1), m_integrationMin(0.0), m_integrationMax(0.0)
{}

// Destructor
NormaliseToMonitor::~NormaliseToMonitor() {}

void NormaliseToMonitor::init()
{
  CompositeValidator<Workspace2D> *val = new CompositeValidator<Workspace2D>;
  val->add(new HistogramValidator<Workspace2D>);
  val->add(new CommonBinsValidator<Workspace2D>);
  val->add(new RawCountValidator<Workspace2D>);
  // It's been said that we should restrict the unit to being wavelength, but I'm not sure about that...
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input,val));
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("MonitorSpectrum",-1,mustBePositive);

  // Now the optional properties to normalise by an integrated count instead of bin-by-bin
  declareProperty("IntegrationRangeMin",0.0);
  declareProperty("IntegrationRangeMax",0.0);
}

void NormaliseToMonitor::exec()
{
  // First check the inputs
  const bool integrate = this->checkProperties();

  // Get the input workspace
  const Workspace2D_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS;
  // Get the workspace index relating to the spectrum number given
  this->findMonitorIndex(inputWS);

  if ( integrate )
  {
    outputWS = this->normaliseByIntegratedCount(inputWS);
  }
  else
  {
    // Create a Workspace1D with the monitor spectrum in it.
    MatrixWorkspace_sptr monitor = WorkspaceFactory::Instance().create("Workspace1D",1,inputWS->blocksize()+1,inputWS->blocksize());
    const std::vector<double> &monX = monitor->dataX(0) = inputWS->readX(m_monitorIndex);
    const std::vector<double> &monY = monitor->dataY(0) = inputWS->readY(m_monitorIndex);
    monitor->dataE(0) = inputWS->readE(m_monitorIndex);
    monitor->getAxis(0)->unit() = inputWS->getAxis(0)->unit();

    const double monitorSum = std::accumulate(monY.begin(), monY.end(), 0.0);
    const double range = monX.back() - monX.front();
    for(MatrixWorkspace::iterator wi(*monitor); wi != wi.end(); ++wi)
    {
        LocatedDataRef tr = *wi;
        const double factor = (tr.X2()-tr.X()) * monitorSum / range;
        tr.Y() = tr.Y() / factor;
        tr.E() = tr.E() / factor;
        // If monitor count is zero, set to a very large number so we get zero out of normalisation
        // division below, instead of infinities or nan
        if (tr.Y() == 0) tr.Y() = DBL_MAX;
        if (tr.E() == 0) tr.E() = DBL_MAX;
    }

    outputWS = inputWS / monitor;
    outputWS->isDistribution(false);
  }

  setProperty("OutputWorkspace",outputWS);
}

/** Makes sure that the input properties are set correctly
 *  @return True if the optional properties have been set
 *  @throw std::runtime_error If a property is invalid
 */
const bool NormaliseToMonitor::checkProperties()
{
  // Get the input workspace
  const Workspace2D_const_sptr inputWS = getProperty("InputWorkspace");
  // Do the full check for common bin boundaries
  if ( !API::WorkspaceHelpers::commonBoundaries(inputWS) )
  {
    g_log.error("Can only normalise if the histograms have common bin boundaries");
    throw std::runtime_error("Can only normalise if the histograms have common bin boundaries");
  }

  // Check that the spectrum given is related to a monitor
  const int monitorSpec = getProperty("MonitorSpectrum");
  Axis::spec2index_map specs;
  inputWS->getAxis(1)->getSpectraIndexMap(specs);
  const int workspaceIndex = specs[monitorSpec];
  if (! inputWS->getDetector(workspaceIndex)->isMonitor() )
  {
    g_log.error("MonitorSpectrum does not refer to a monitor spectrum");
    throw std::runtime_error("MonitorSpectrum does not refer to a monitor spectrum");
  }

  // Check the optional 'integration' properties
  Property* min = getProperty("IntegrationRangeMin");
  Property* max = getProperty("IntegrationRangeMax");
  if ( !min->isDefault() || !max->isDefault() )
  {
    m_integrationMin = getProperty("IntegrationRangeMin");
    m_integrationMax = getProperty("IntegrationRangeMax");
    if ( !min->isDefault() && !max->isDefault() && m_integrationMin > m_integrationMax )
    {
      g_log.error("Integration minimum set to larger value than maximum!");
      throw std::runtime_error("Integration minimum set to larger value than maximum!");
    }
    if ( min->isDefault() || m_integrationMin < inputWS->dataX(0).front() )
    {
      g_log.warning() << "Integration range minimum set to workspace min: " << m_integrationMin << std::endl;
      m_integrationMin = inputWS->dataX(0).front();
    }
    if ( min->isDefault() || m_integrationMax > inputWS->dataX(0).back() )
    {
      g_log.warning() << "Integration range maximum set to workspace max: " << m_integrationMax << std::endl;
      m_integrationMax = inputWS->dataX(0).back();
    }
    // Return indicating that these properties should be used
    return true;
  }

  return false;
}

/// Finds the workspace index of the monitor from the spectrum number
void NormaliseToMonitor::findMonitorIndex(API::MatrixWorkspace_const_sptr inputWorkspace)
{
  const int monitorIndex = getProperty("MonitorSpectrum");
  Axis* spectraAxis = inputWorkspace->getAxis(1);
  const int numberOfSpectra = inputWorkspace->getNumberHistograms();
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    if ( spectraAxis->spectraNo(i) == monitorIndex )
    {
      m_monitorIndex = i;
      return;
    }
  }
  // Only get to here if spectrum number isn't found
  g_log.error("Monitor spectrum not found in input workspace");
  throw std::runtime_error("Monitor spectrum not found in input workspace");
}

/// Carries out a normalisation based on the integrated count of the monitor over a range
API::MatrixWorkspace_sptr NormaliseToMonitor::normaliseByIntegratedCount(API::MatrixWorkspace_sptr inputWorkspace)
{
  // Don't run CropWorkspace algorithm if integrating over full workspace range
  if ( m_integrationMin != inputWorkspace->readX(0).front() || m_integrationMax != inputWorkspace->readX(0).back() )
  {
    Algorithm_sptr crop = createSubAlgorithm("CropWorkspace");
    DataObjects::Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(inputWorkspace);
    crop->setProperty("InputWorkspace", input2D);
    crop->setProperty("XMin",m_integrationMin);
    crop->setProperty("Xmax",m_integrationMax);
    try {
      crop->execute();
    } catch (std::runtime_error) {
      g_log.error("Unable to successfully run CropWorkspace sub-algorithm");
      throw;
    }
    // Get back the result
    inputWorkspace = crop->getProperty("OutputWorkspace");
  }

  // Now create a Workspace1D with the monitor spectrum
  MatrixWorkspace_sptr monitor = WorkspaceFactory::Instance().create("Workspace1D",1,inputWorkspace->blocksize()+1,inputWorkspace->blocksize());
  monitor->dataX(0) = inputWorkspace->readX(m_monitorIndex);
  monitor->dataY(0) = inputWorkspace->readY(m_monitorIndex);
  monitor->dataE(0) = inputWorkspace->readE(m_monitorIndex);
  monitor->getAxis(0)->unit() = inputWorkspace->getAxis(0)->unit();

  // Add up all the bins so it's just effectively a single value with an error
  Algorithm_sptr integrate = createSubAlgorithm("Integration");
  integrate->setProperty<MatrixWorkspace_sptr>("InputWorkspace", monitor);
  try {
    integrate->execute();
  } catch (std::runtime_error) {
    g_log.error("Unable to successfully run Integration sub-algorithm");
    throw;
  }
  // Get back the result
  monitor = integrate->getProperty("OutputWorkspace");

  MatrixWorkspace_sptr outputWS = inputWorkspace / monitor;
//  WorkspaceHelpers::makeDistribution(outputWS);
  return outputWS;
}

} // namespace Algorithm
} // namespace Mantid
