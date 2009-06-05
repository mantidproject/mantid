//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(ConjoinWorkspaces)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;
using DataObjects::Workspace2D_const_sptr;

// Get a reference to the logger
Logger& ConjoinWorkspaces::g_log = Logger::get("ConjoinWorkspaces");

/// Default constructor
  ConjoinWorkspaces::ConjoinWorkspaces() : Algorithm(), m_progress(NULL) {}

/// Destructor
ConjoinWorkspaces::~ConjoinWorkspaces() 
{
  if( m_progress )
  {
    delete m_progress;
  }
}

void ConjoinWorkspaces::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace1",
    "", Direction::Input, new CommonBinsValidator<Workspace2D>),
    "The name of the first input workspace");
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace2",
    "", Direction::Input, new CommonBinsValidator<Workspace2D>),
    "The name of the second input workspace");
}

/** Executes the algorithm
 *  @throw std::invalid_argument If the input workspaces do not meet the requirements of this algorithm
 */
void ConjoinWorkspaces::exec()
{
  // Retrieve the input workspaces
  Workspace2D_const_sptr ws1 = getProperty("InputWorkspace1");
  Workspace2D_const_sptr ws2 = getProperty("InputWorkspace2");

  // Check that the input workspaces meet the requirements for this algorithm
  this->validateInputs(ws1,ws2);

  // Create the output workspace
  const int totalHists = ws1->getNumberHistograms() + ws2->getNumberHistograms();
  MatrixWorkspace_sptr output = WorkspaceFactory::Instance().create(ws1,totalHists,ws1->readX(0).size(),
                                                                             ws1->readY(0).size());
  Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

  // Create the X values inside a cow pointer - they will be shared in the output workspace
  DataObjects::Histogram1D::RCtype XValues;
  XValues.access() = ws1->readX(0);

  // Initialize the progress reporting object
  m_progress = new API::Progress(this, 0.0, 1.0, totalHists);

  // Loop over the input workspaces in turn copying the data into the output one
    Axis* outAxis = output2D->getAxis(1);
  const int& nhist1 = ws1->getNumberHistograms();
  const Axis* axis1 = ws1->getAxis(1);
  PARALLEL_FOR2(ws1, output2D)
  for (int i = 0; i < nhist1; ++i)
  {
    output2D->setX(i,XValues);
    output2D->dataY(i) = ws1->readY(i);
    output2D->dataE(i) = ws1->readE(i);
    // Copy the spectrum number
    outAxis->spectraNo(i) = axis1->spectraNo(i);
    // Propagate masking, if needed
    if ( ws1->hasMaskedBins(i) )
    {
      const MatrixWorkspace::MaskList& inputMasks = ws1->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it)
      {
        output->maskBin(i,(*it).first,(*it).second);
      }
    }    
    m_progress->report();
  }
  //For second loop we use the offset from the first
  const int& nhist2 = ws2->getNumberHistograms();
  const Axis* axis2 = ws2->getAxis(1);

  PARALLEL_FOR2(ws2, output2D)
  for (int j = 0; j < nhist2; ++j)
  {
    output2D->setX(nhist1 + j,XValues);
    output2D->dataY(nhist1 + j) = ws2->readY(j);
    output2D->dataE(nhist1 + j) = ws2->readE(j);
    // Copy the spectrum number
    outAxis->spectraNo(nhist1 + j) = axis2->spectraNo(j);
    // Propagate masking, if needed
    if ( ws2->hasMaskedBins(j) )
    {
      const MatrixWorkspace::MaskList& inputMasks = ws2->maskedBins(j);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it)
      {
        output->maskBin(nhist1 + j,(*it).first,(*it).second);
      }
    }
    m_progress->report();
  }

  // Delete the input workspaces from the ADS
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace1"));
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));
  // Create & assign an output workspace property with the workspace name the same as the first input
  declareProperty(new WorkspaceProperty<Workspace2D>("Output",getPropertyValue("InputWorkspace1"),Direction::Output));
  setProperty("Output",output2D);
}

/** Checks that the two input workspace have common binning & size, the same instrument & unit.
 *  Also calls the checkForOverlap method.
 *  @param ws1 The first input workspace
 *  @param ws2 The second input workspace
 *  @throw std::invalid_argument If the workspaces are not compatible
 */
void ConjoinWorkspaces::validateInputs(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2) const
{
  if ( !WorkspaceHelpers::commonBoundaries(ws1) || !WorkspaceHelpers::commonBoundaries(ws2) )
  {
    g_log.error("Both input workspaces must have common binning for all their spectra");
    throw std::invalid_argument("Both input workspaces must have common binning for all their spectra");
  }

  if ( ws1->getInstrument()->getName() != ws2->getInstrument()->getName()
       || ws1->getAxis(0)->unit() != ws2->getAxis(0)->unit()
       || ws1->isDistribution()   != ws2->isDistribution()
       || !WorkspaceHelpers::matchingBins(ws1,ws2,true)  )
  // Would be good to also check the spectra-detector maps are the same
  {
    g_log.error("The input workspaces are not compatible");
    throw std::invalid_argument("The input workspaces are not compatible");
  }

  this->checkForOverlap(ws1,ws2);
}

/** Checks that the two input workspaces have non-overlapping spectra numbers and contributing detectors
 *  @param ws1 The first input workspace
 *  @param ws2 The second input workspace
 *  @throw std::invalid_argument If there is some overlap
 */
void ConjoinWorkspaces::checkForOverlap(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2) const
{
  // Loop through the first workspace adding all the spectrum numbers & UDETS to a set
  const Axis* axis1 = ws1->getAxis(1);
  const SpectraDetectorMap& specmap1 = ws1->spectraMap();
  std::set<int> spectra, detectors;
  const int& nhist1 = ws1->getNumberHistograms();
  for (int i = 0; i < nhist1; ++i)
  {
    const int spectrum = axis1->spectraNo(i);
    spectra.insert(spectrum);
    const std::vector<int> dets = specmap1.getDetectors(spectrum);
    std::vector<int>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it)
    {
      detectors.insert(*it);
    }
  }

  // Now go throught the spectrum numbers & UDETS in the 2nd workspace, making sure that there's no overlap
  const Axis* axis2 = ws2->getAxis(1);
  const SpectraDetectorMap& specmap2 = ws2->spectraMap();
  const int& nhist2 = ws2->getNumberHistograms();
  for (int j = 0; j < nhist2; ++j)
  {
    const int spectrum = axis2->spectraNo(j);
    if ( spectrum > 0 && spectra.find(spectrum) != spectra.end() )
    {
      g_log.error("The input workspaces have overlapping spectrum numbers");
      throw std::invalid_argument("The input workspaces have overlapping spectrum numbers");
    }
    std::vector<int> dets = specmap2.getDetectors(spectrum);
    std::vector<int>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it)
    {
      if ( detectors.find(*it) != detectors.end() )
      {
        g_log.error("The input workspaces have common detectors");
        throw std::invalid_argument("The input workspaces have common detectors");
      }
    }
  }
}

} // namespace Algorithm
} // namespace Mantid
