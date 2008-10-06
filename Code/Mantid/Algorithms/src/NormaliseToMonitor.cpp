//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"

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
  CompositeValidator<Workspace2D_sptr> *val = new CompositeValidator<Workspace2D_sptr>;
  val->add(new HistogramValidator<Workspace2D_sptr>);
  val->add(new CommonBinsValidator<Workspace2D_sptr>);
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input,val));
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

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
  Workspace_sptr outputWS;
  // Get the workspace index relating to the spectrum number given
  this->findMonitorIndex(inputWS);

  if ( integrate )
  {
    outputWS = this->normaliseByIntegratedCount(inputWS);
  }
  else
  {
    // Create a Workspace1D with the monitor spectrum in it.
    Workspace_sptr monitor = WorkspaceFactory::Instance().create("Workspace1D",1,inputWS->blocksize()+1,inputWS->blocksize());
    monitor->dataX(0) = inputWS->readX(m_monitorIndex);
    monitor->dataY(0) = inputWS->readY(m_monitorIndex);
    monitor->dataE(0) = inputWS->readE(m_monitorIndex);

    outputWS = inputWS / monitor;
    // Divide the data by bin width (will have been lost in division above, if previously present).
    outputWS->isDistribution(false);
    this->doUndoDistribution(outputWS);
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
  if (! inputWS->getSpectraMap()->getDetector(monitorSpec)->isMonitor() )
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
void NormaliseToMonitor::findMonitorIndex(API::Workspace_const_sptr inputWorkspace)
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
API::Workspace_sptr NormaliseToMonitor::normaliseByIntegratedCount(API::Workspace_sptr inputWorkspace)
{
  bool fixInput = false;
  // Don't run regroup algorithm if integrating over full workspace range
  if ( m_integrationMin != inputWorkspace->readX(0).front() || m_integrationMax != inputWorkspace->readX(0).back() )
  {
    // (This just lops bins off the beginning and end - should write an algorithm that just does this)
    Algorithm_sptr regroup = createSubAlgorithm("Regroup");
    regroup->setProperty<Workspace_sptr>("InputWorkspace", inputWorkspace);
    std::vector<double> p(3);
    p[0] = m_integrationMin;
    // Need a more efficient way than this of ensuring we get the same bins out as went in
    p[1] = 1e-7;
    p[2] = m_integrationMax;
    regroup->setProperty<std::vector<double> >("params",p);
    try {
      regroup->execute();
    } catch (std::runtime_error& err) {
      g_log.error("Unable to successfully run Rebunch sub-algorithm");
      throw;
    }
    // Get back the result
    inputWorkspace = regroup->getProperty("OutputWorkspace");
  }
  else
  {
    // Indicates that I'll have to later take out the bin width division that I'm about to put in
    if ( !inputWorkspace->isDistribution() ) fixInput = true;
  }
  // Need to divide by bin with if it isn't already done
  if ( !inputWorkspace->isDistribution() ) this->doUndoDistribution(inputWorkspace);

  // Now create a Workspace1D with the monitor spectrum
  Workspace_sptr monitor = WorkspaceFactory::Instance().create("Workspace1D",1,inputWorkspace->blocksize()+1,inputWorkspace->blocksize());
  monitor->dataX(0) = inputWorkspace->readX(m_monitorIndex);
  monitor->dataY(0) = inputWorkspace->readY(m_monitorIndex);
  monitor->dataE(0) = inputWorkspace->readE(m_monitorIndex);
  monitor->isDistribution(true);

  // Add up all the bins so it's just effectively a single value with an error
  Algorithm_sptr rebunch = createSubAlgorithm("Rebunch");
  rebunch->setProperty<Workspace_sptr>("InputWorkspace", monitor);
  rebunch->setProperty<int>("n_bunch",monitor->blocksize());
  try {
    rebunch->execute();
  } catch (std::runtime_error& err) {
    g_log.error("Unable to successfully run Rebunch sub-algorithm");
    throw;
  }
  // Get back the result
  monitor = rebunch->getProperty("OutputWorkspace");

  Workspace_sptr outputWS = inputWorkspace / monitor;
  this->doUndoDistribution(outputWS);
  if (fixInput) this->doUndoDistribution(inputWorkspace,false);
  return outputWS;
}

/// Divides a workspace's data (and errors) by the bin width
void NormaliseToMonitor::doUndoDistribution(API::Workspace_sptr workspace, const bool forwards)
{
  // Check workspace isn't already in the correct state - do nothing if it is
  if ( workspace->isDistribution() == forwards ) return;

  const int numberOfSpectra = workspace->getNumberHistograms();
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    const int size = workspace->blocksize();
    for (int j = 0; j < size; ++j)
    {
      double width = std::abs( workspace->readX(i)[j+1] - workspace->readX(i)[j] );
      if (!forwards) width = 1.0/width;
      workspace->dataY(i)[j] /= width;
      workspace->dataE(i)[j] /= width;
    }
  }
  workspace->isDistribution(forwards);
}

} // namespace Algorithm
} // namespace Mantid
