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
ConjoinWorkspaces::ConjoinWorkspaces() : Algorithm() {}

/// Destructor
ConjoinWorkspaces::~ConjoinWorkspaces() {}

void ConjoinWorkspaces::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace1","",Direction::Input,
                                                       new CommonBinsValidator<Workspace2D>));
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace2","",Direction::Input,
                                                       new CommonBinsValidator<Workspace2D>));
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
  Workspace_sptr output = WorkspaceFactory::Instance().create(ws1,totalHists,ws1->readX(0).size(),
                                                                             ws1->readY(0).size());
  Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

  // Create the X values inside a cow pointer - they will be shared in the output workspace
  DataObjects::Histogram1D::RCtype XValues;
  XValues.access() = ws1->readX(0);

  // Loop over the input workspaces in turn copying the data into the output one
  int outIndex = 0;
  Axis* outAxis = output2D->getAxis(1);
  const int& nhist1 = ws1->getNumberHistograms();
  const Axis* axis1 = ws1->getAxis(1);
  for (int i = 0; i < nhist1; ++i,++outIndex)
  {
    output2D->setX(outIndex,XValues);
    output2D->dataY(outIndex) = ws1->readY(i);
    output2D->dataE(outIndex) = ws1->readE(i);
    outAxis->spectraNo(outIndex) = axis1->spectraNo(i);
    output2D->setErrorHelper(outIndex,ws1->errorHelper(i));
  }
  const int& nhist2 = ws2->getNumberHistograms();
  const Axis* axis2 = ws2->getAxis(1);
  for (int j = 0; j < nhist2; ++j,++outIndex)
  {
    output2D->setX(outIndex,XValues);
    output2D->dataY(outIndex) = ws2->readY(j);
    output2D->dataE(outIndex) = ws2->readE(j);
    outAxis->spectraNo(outIndex) = axis2->spectraNo(j);
    output2D->setErrorHelper(outIndex,ws2->errorHelper(j));
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
void ConjoinWorkspaces::validateInputs(API::Workspace_const_sptr ws1, API::Workspace_const_sptr ws2) const
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
void ConjoinWorkspaces::checkForOverlap(API::Workspace_const_sptr ws1, API::Workspace_const_sptr ws2) const
{
  // Loop through the first workspace adding all the spectrum numbers & UDETS to a set
  const Axis* axis1 = ws1->getAxis(1);
  const SpectraMap_const_sptr specmap1 = ws1->getSpectraMap();
  std::set<int> spectra, detectors;
  const int& nhist1 = ws1->getNumberHistograms();
  for (int i = 0; i < nhist1; ++i)
  {
    const int spectrum = axis1->spectraNo(i);
    spectra.insert(spectrum);
    const std::vector<Geometry::IDetector_sptr> dets = specmap1->getDetectors(spectrum);
    std::vector<Geometry::IDetector_sptr>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it)
    {
      detectors.insert((*it)->getID());
    }
  }

  // Now go throught the spectrum numbers & UDETS in the 2nd workspace, making sure that there's no overlap
  const Axis* axis2 = ws2->getAxis(1);
  const SpectraMap_const_sptr specmap2 = ws2->getSpectraMap();
  const int& nhist2 = ws2->getNumberHistograms();
  for (int j = 0; j < nhist2; ++j)
  {
    const int spectrum = axis2->spectraNo(j);
    if ( spectra.find(spectrum) != spectra.end() )
    {
      g_log.error("The input workspaces overlap");
      throw std::invalid_argument("The input workspaces overlap");
    }
    std::vector<Geometry::IDetector_sptr> dets = specmap2->getDetectors(spectrum);
    std::vector<Geometry::IDetector_sptr>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it)
    {
      if ( detectors.find((*it)->getID()) != detectors.end() )
      {
        g_log.error("The input workspaces have common detectors");
        throw std::invalid_argument("The input workspaces have common detectors");
      }
    }
  }
}

} // namespace Algorithm
} // namespace Mantid
