//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConjoinWorkspaces)

/// Sets documentation strings for this algorithm
void ConjoinWorkspaces::initDocs()
{
  this->setWikiSummary("Joins two partial, non-overlapping 2D workspaces into one. ");
  this->setOptionalMessage("Joins two partial, non-overlapping 2D workspaces into one.");
}


//----------------------------------------------------------------------------------------------
/// Default constructor
ConjoinWorkspaces::ConjoinWorkspaces() : PairedGroupAlgorithm(), m_progress(NULL) {}

//----------------------------------------------------------------------------------------------
/// Destructor
ConjoinWorkspaces::~ConjoinWorkspaces() 
{
  if( m_progress )
  {
    delete m_progress;
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the properties */
void ConjoinWorkspaces::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace1",
    "", Direction::InOut, new CommonBinsValidator<>),
    "The name of the first input workspace");
  declareProperty(new WorkspaceProperty<>("InputWorkspace2",
    "", Direction::Input, new CommonBinsValidator<>),
    "The name of the second input workspace");
  declareProperty(new PropertyWithValue<bool>("CheckOverlapping", true, Direction::Input),
                  "Verify that the supplied data do not overlap");
}

//----------------------------------------------------------------------------------------------
/** Executes the algorithm
 *  @throw std::invalid_argument If the input workspaces do not meet the requirements of this algorithm
 */
void ConjoinWorkspaces::exec()
{
  // Retrieve the input workspaces
  MatrixWorkspace_const_sptr ws1 = getProperty("InputWorkspace1");
  MatrixWorkspace_const_sptr ws2 = getProperty("InputWorkspace2");
  event_ws1 = boost::dynamic_pointer_cast<const EventWorkspace>(ws1);
  event_ws2 = boost::dynamic_pointer_cast<const EventWorkspace>(ws2);

  //Make sure that we are not mis-matching EventWorkspaces and other types of workspaces
  if (((event_ws1) && (!event_ws2)) || ((!event_ws1) && (event_ws2)))
  {
    const std::string message("Only one of the input workspaces are of type EventWorkspace; please use matching workspace types (both EventWorkspace's or both Workspace2D's).");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if (event_ws1 && event_ws2)
  {
    //Both are event workspaces. Use the special method
    this->execEvent();
    return;
  }

  // Check that the input workspaces meet the requirements for this algorithm
  this->validateInputs(ws1,ws2);

  // Create the output workspace
  const size_t totalHists = ws1->getNumberHistograms() + ws2->getNumberHistograms();
  MatrixWorkspace_sptr output = WorkspaceFactory::Instance().create("Workspace2D",totalHists,ws1->readX(0).size(),
                                                                             ws1->readY(0).size());
  // Copy over stuff from first input workspace
  WorkspaceFactory::Instance().initializeFromParent(ws1,output,true);

  // Create the X values inside a cow pointer - they will be shared in the output workspace
  cow_ptr<MantidVec> XValues;
  XValues.access() = ws1->readX(0);

  // Initialize the progress reporting object
  m_progress = new API::Progress(this, 0.0, 1.0, totalHists);

  // Loop over the input workspaces in turn copying the data into the output one
  Axis* outAxis = output->getAxis(1);
  const int64_t& nhist1 = ws1->getNumberHistograms();
  const Axis* axis1 = ws1->getAxis(1);
  PARALLEL_FOR2(ws1, output)
  for (int64_t i = 0; i < nhist1; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    output->setX(i,XValues);
    output->dataY(i) = ws1->readY(i);
    output->dataE(i) = ws1->readE(i);
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
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  //For second loop we use the offset from the first
  const int64_t& nhist2 = ws2->getNumberHistograms();
  const Axis* axis2 = ws2->getAxis(1);

  PARALLEL_FOR2(ws2, output)
  for (int64_t j = 0; j < nhist2; ++j)
  {
    PARALLEL_START_INTERUPT_REGION
    output->setX(nhist1 + j,XValues);
    output->dataY(nhist1 + j) = ws2->readY(j);
    output->dataE(nhist1 + j) = ws2->readE(j);
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
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Delete the second input workspace from the ADS
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));
  // Set the result workspace to the first input
  setProperty("InputWorkspace1",output);
}


//----------------------------------------------------------------------------------------------
/** Executes the algorithm
 *  @throw std::invalid_argument If the input workspaces do not meet the requirements of this algorithm
 */
void ConjoinWorkspaces::execEvent()
{
  //We do not need to check that binning is compatible, just that there is no overlap
  this->checkForOverlap(event_ws1, event_ws2, false);

  // Create the output workspace
  const size_t totalHists = event_ws1->getNumberHistograms() + event_ws2->getNumberHistograms();
  // Have the minimum # of histograms in the output.
  EventWorkspace_sptr output = boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace",
          1, event_ws1->readX(0).size(), event_ws1->readY(0).size())
      );
  // Copy over geometry (but not data) from first input workspace
  WorkspaceFactory::Instance().initializeFromParent(event_ws1,output,true);

  // Create the X values inside a cow pointer - they will be shared in the output workspace
  cow_ptr<MantidVec> XValues;
  XValues.access() = event_ws1->readX(0);

  // Initialize the progress reporting object
  m_progress = new API::Progress(this, 0.0, 1.0, totalHists);

  const int64_t& nhist1 = event_ws1->getNumberHistograms();
  for (int64_t i = 0; i < nhist1; ++i)
  {
    //Copy the events over
    output->getOrAddEventList(i) = event_ws1->getEventList(i); //Should fire the copy constructor
    m_progress->report();
  }

  //For second loop we use the offset from the first
  const int64_t& nhist2 = event_ws2->getNumberHistograms();
  for (int64_t j = 0; j < nhist2; ++j)
  {
    //This is the workspace index at which we assign in the output
    int output_wi = j + nhist1;
    //Copy the events over
    output->getOrAddEventList(output_wi) = event_ws2->getEventList(j); //Should fire the copy constructor
    m_progress->report();
  }

  //This will make the spectramap axis.
  output->doneAddingEventLists();

  //Set the same bins for all output pixels
  output->setAllX(XValues);

  // Delete the input workspaces from the ADS
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace1"));
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));

  // Create & assign an output workspace property with the workspace name the same as the first input
  declareProperty(new WorkspaceProperty<>("Output",getPropertyValue("InputWorkspace1"),Direction::Output));
  setProperty("Output", boost::dynamic_pointer_cast<MatrixWorkspace>(output) );
}


//----------------------------------------------------------------------------------------------
/** Checks that the two input workspace have common binning & size, the same instrument & unit.
 *  Also calls the checkForOverlap method.
 *  @param ws1 :: The first input workspace
 *  @param ws2 :: The second input workspace
 *  @throw std::invalid_argument If the workspaces are not compatible
 */
void ConjoinWorkspaces::validateInputs(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2) const
{
  // This is the full check for common binning
  if ( !WorkspaceHelpers::commonBoundaries(ws1) || !WorkspaceHelpers::commonBoundaries(ws2) )
  {
    g_log.error("Both input workspaces must have common binning for all their spectra");
    throw std::invalid_argument("Both input workspaces must have common binning for all their spectra");
  }

  if ( ws1->getInstrument()->getName() != ws2->getInstrument()->getName() )
  {
    const std::string message("The input workspaces are not compatible because they come from different instruments");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  Unit_const_sptr ws1_unit = ws1->getAxis(0)->unit();
  Unit_const_sptr ws2_unit = ws2->getAxis(0)->unit();
  const std::string ws1_unitID = ( ws1_unit ? ws1_unit->unitID() : "" );
  const std::string ws2_unitID = ( ws2_unit ? ws2_unit->unitID() : "" );

  if ( ws1_unitID != ws2_unitID )
  {
    const std::string message("The input workspaces are not compatible because they have different units on the X axis");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if ( ws1->isDistribution()   != ws2->isDistribution() )
  {
    const std::string message("The input workspaces have inconsistent distribution flags");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if ( !WorkspaceHelpers::matchingBins(ws1,ws2,true) )
  {
    const std::string message("The input workspaces are not compatible because they have different binning");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  this->checkForOverlap(ws1,ws2, true);
}

//----------------------------------------------------------------------------------------------
/** Checks that the two input workspaces have non-overlapping spectra numbers and contributing detectors
 *  @param ws1 :: The first input workspace
 *  @param ws2 :: The second input workspace
 *  @param checkSpectra :: set to true to check for overlapping spectra numbers (non-sensical for event workspaces)
 *  @throw std::invalid_argument If there is some overlap
 */
void ConjoinWorkspaces::checkForOverlap(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2, bool checkSpectra) const
{
  // make sure we should bother checking
  if (!this->getProperty("CheckOverlapping"))
    return;
  // Loop through the first workspace adding all the spectrum numbers & UDETS to a set
  const Axis* axis1 = ws1->getAxis(1);
  const SpectraDetectorMap& specmap1 = ws1->spectraMap();
  std::set<specid_t> spectra;
  std::set<detid_t> detectors;
  const size_t& nhist1 = ws1->getNumberHistograms();
  for (size_t i = 0; i < nhist1; ++i)
  {
    const specid_t spectrum = axis1->spectraNo(i);
    spectra.insert(spectrum);
    const std::vector<detid_t> dets = specmap1.getDetectors(spectrum);
    std::vector<detid_t>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it)
    {
      detectors.insert(*it);
    }
  }

  // Now go throught the spectrum numbers & UDETS in the 2nd workspace, making sure that there's no overlap
  const Axis* axis2 = ws2->getAxis(1);
  const SpectraDetectorMap& specmap2 = ws2->spectraMap();
  const size_t& nhist2 = ws2->getNumberHistograms();
  for (size_t j = 0; j < nhist2; ++j)
  {
    const specid_t spectrum = axis2->spectraNo(j);
    if (checkSpectra)
    {
      if ( spectrum > 0 && spectra.find(spectrum) != spectra.end() )
      {
        g_log.error("The input workspaces have overlapping spectrum numbers");
        throw std::invalid_argument("The input workspaces have overlapping spectrum numbers");
      }
    }
    std::vector<detid_t> dets = specmap2.getDetectors(spectrum);
    std::vector<detid_t>::const_iterator it;
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

/// Appends the removal of the empty group after execution to the PairedGroupAlgorithm::processGroups method
bool ConjoinWorkspaces::processGroups(API::WorkspaceGroup_sptr wsPt, const std::vector<Kernel::Property*>& prop)
{
  // Call the base class method for most of the functionality
  const bool retval = PairedGroupAlgorithm::processGroups(wsPt,prop);

  // If that was successful, remove the now empty group in the second input workspace property
  if (retval) AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));

  return retval;
}

} // namespace Algorithm
} // namespace Mantid
