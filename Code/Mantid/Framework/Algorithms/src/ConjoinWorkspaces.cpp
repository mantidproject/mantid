/*WIKI* 


This algorithm can be useful when working with large datasets. It enables the raw file to be loaded in two parts (not necessarily of equal size), the data processed in turn and the results joined back together into a single dataset. This can help avoid memory problems either because intermediate workspaces will be smaller and/or because the data will be much reduced after processing.

The output of the algorithm, in which the data from the second input workspace will be appended to the first, will be stored under the name of the first input workspace. Workspace data members other than the data (e.g. instrument etc.) will be copied from the first input workspace (but if they're not identical anyway, then you probably shouldn't be using this algorithm!). Both input workspaces will be deleted.

==== Restrictions on the input workspace ====

The input workspaces must come from the same instrument, have common units and bins and no detectors that contribute to spectra should overlap.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/ISpectraDetectorMap.h"

namespace Mantid
{
namespace Algorithms
{

using std::size_t;
using Geometry::ISpectraDetectorMap;
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
ConjoinWorkspaces::ConjoinWorkspaces() : Algorithm(), m_progress(NULL), m_overlapChecked(false)
{
}

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
  // Copy over stuff from first input workspace. This will include the spectrum masking
  WorkspaceFactory::Instance().initializeFromParent(ws1,output,true);

  // Create the X values inside a cow pointer - they will be shared in the output workspace
  cow_ptr<MantidVec> XValues;
  XValues.access() = ws1->readX(0);

  // Initialize the progress reporting object
  m_progress = new API::Progress(this, 0.0, 1.0, totalHists);

  // Loop over the input workspaces in turn copying the data into the output one
  const int64_t& nhist1 = ws1->getNumberHistograms();
  PARALLEL_FOR2(ws1, output)
  for (int64_t i = 0; i < nhist1; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    ISpectrum * outSpec = output->getSpectrum(i);
    const ISpectrum * inSpec = ws1->getSpectrum(i);

    // Copy X,Y,E
    outSpec->setX(XValues);
    outSpec->setData(inSpec->dataY(), inSpec->dataE());
    // Copy the spectrum number/detector IDs
    outSpec->copyInfoFrom(*inSpec);

    // Propagate binmasking, if needed
    if ( ws1->hasMaskedBins(i) )
    {
      const MatrixWorkspace::MaskList& inputMasks = ws1->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it)
      {
        output->flagMasked(i,(*it).first,(*it).second);
      }
    }    

    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION


  //For second loop we use the offset from the first
  const int64_t& nhist2 = ws2->getNumberHistograms();

  PARALLEL_FOR2(ws2, output)
  for (int64_t j = 0; j < nhist2; ++j)
  {
    PARALLEL_START_INTERUPT_REGION
    // The spectrum in the output workspace
    ISpectrum * outSpec = output->getSpectrum(nhist1 + j);
    // Spectrum in the second workspace
    const ISpectrum * inSpec = ws2->getSpectrum(j);

    // Copy X,Y,E
    outSpec->setX(XValues);
    outSpec->setData(inSpec->dataY(), inSpec->dataE());
    // Copy the spectrum number/detector IDs
    outSpec->copyInfoFrom(*inSpec);

    // Propagate masking, if needed
    if ( ws2->hasMaskedBins(j) )
    {
      const MatrixWorkspace::MaskList& inputMasks = ws2->maskedBins(j);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it)
      {
        output->flagMasked(nhist1 + j,(*it).first,(*it).second);
      }
    }
    // Propagate spectrum masking
    Geometry::IDetector_const_sptr ws2Det;
    try
    {
      ws2Det = ws2->getDetector(j);
    }
    catch(Exception::NotFoundError &)
    {
    }
    if( ws2Det && ws2Det->isMasked() )
    {
      output->maskWorkspaceIndex(nhist1 + j);
    }

    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  this->fixSpectrumNumbers(ws1,ws2, output);

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
  // make sure we should bother checking
  if (this->getProperty("CheckOverlapping"))
  {
    this->checkForOverlap(event_ws1, event_ws2, false);
    m_overlapChecked = true;
  }

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
    ISpectrum * outSpec = output->getSpectrum(i);
    const ISpectrum * inSpec = event_ws1->getSpectrum(i);
    outSpec->copyInfoFrom(*inSpec);

    m_progress->report();
  }

  //For second loop we use the offset from the first
  const int64_t& nhist2 = event_ws2->getNumberHistograms();
  for (int64_t j = 0; j < nhist2; ++j)
  {
    //This is the workspace index at which we assign in the output
    int64_t output_wi = j + nhist1;
    //Copy the events over
    output->getOrAddEventList(output_wi) = event_ws2->getEventList(j); //Should fire the copy constructor
    ISpectrum * outSpec = output->getSpectrum(output_wi);
    const ISpectrum * inSpec = event_ws2->getSpectrum(j);
    outSpec->copyInfoFrom(*inSpec);

    // Propagate spectrum masking. First workspace will have been done by the factory
    Geometry::IDetector_const_sptr ws2Det;
    try
    {
      ws2Det = event_ws2->getDetector(j);
    }
    catch(Exception::NotFoundError &)
    {
    }
    if( ws2Det && ws2Det->isMasked() )
    {
      output->maskWorkspaceIndex(output_wi);
    }

    m_progress->report();
  }

  //This will make the spectramap axis.
  output->doneAddingEventLists();

  //Set the same bins for all output pixels
  output->setAllX(XValues);

  this->fixSpectrumNumbers(event_ws1, event_ws2, output);

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
void ConjoinWorkspaces::validateInputs(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
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

  if (this->getProperty("CheckOverlapping") )
  {
    this->checkForOverlap(ws1,ws2, true);
    m_overlapChecked = true;
  }
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
  // Loop through the first workspace adding all the spectrum numbers & UDETS to a set
  std::set<specid_t> spectra;
  std::set<detid_t> detectors;
  const size_t& nhist1 = ws1->getNumberHistograms();
  for (size_t i = 0; i < nhist1; ++i)
  {
    const ISpectrum * spec = ws1->getSpectrum(i);
    const specid_t spectrum = spec->getSpectrumNo();
    spectra.insert(spectrum);
    const std::set<detid_t> & dets = spec->getDetectorIDs();
    std::set<detid_t>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it)
    {
      detectors.insert(*it);
    }
  }

  // Now go throught the spectrum numbers & UDETS in the 2nd workspace, making sure that there's no overlap
  const size_t& nhist2 = ws2->getNumberHistograms();
  for (size_t j = 0; j < nhist2; ++j)
  {
    const ISpectrum * spec = ws2->getSpectrum(j);
    const specid_t spectrum = spec->getSpectrumNo();
    if (checkSpectra)
    {
      if ( spectrum > 0 && spectra.find(spectrum) != spectra.end() )
      {
        g_log.error() << "The input workspaces have overlapping spectrum numbers " << spectrum << "\n";
        throw std::invalid_argument("The input workspaces have overlapping spectrum numbers");
      }
    }
    const std::set<detid_t> & dets = spec->getDetectorIDs();
    std::set<detid_t>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it)
    {
      if ( detectors.find(*it) != detectors.end() )
      {
        g_log.error() << "The input workspaces have common detectors: " << (*it) << "\n";
        throw std::invalid_argument("The input workspaces have common detectors");
      }
    }
  }
}

/**
 * Determine the minimum and maximum spectra ids.
 *
 * @param ws the workspace to search
 * @param min The minimum id (output).
 * @param max The maximum id (output).
 */
void getMinMax(MatrixWorkspace_const_sptr ws, specid_t& min, specid_t& max)
{
  specid_t temp;
  size_t length = ws->getNumberHistograms();
  // initial values
  min = max = ws->getSpectrum(0)->getSpectrumNo();
  for (size_t i = 0; i < length; i++)
  {
    temp = ws->getSpectrum(i)->getSpectrumNo();
    // Adjust min/max
    if (temp < min)
      min = temp;
    if (temp > max)
      max = temp;
  }
}

/***
 * This will ensure the spectrum numbers do not overlap by starting the second on at the first + 1
 *
 * @param ws1 The first workspace supplied to the algorithm.
 * @param ws2 The second workspace supplied to the algorithm.
 * @param output The workspace that is going to be returned by the algorithm.
 */
void ConjoinWorkspaces::fixSpectrumNumbers(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2,
                                           API::MatrixWorkspace_sptr output)
{
  // If check throws then we need to fix the output
  bool needsFix(false);
  try
  {
    if( !m_overlapChecked ) checkForOverlap(ws1, ws2, true);
  }
  catch(std::invalid_argument&)
  {
    needsFix = true;
  }

  if( !needsFix ) return;
  // is everything possibly ok?
  specid_t min;
  specid_t max;
  getMinMax(output, min, max);
  if (max - min >= static_cast<specid_t>(output->getNumberHistograms())) // nothing to do then
    return;

  // information for remapping the spectra numbers
  specid_t ws1min;
  specid_t ws1max;
  getMinMax(ws1, ws1min, ws1max);

  // change the axis by adding the maximum existing spectrum number to the current value
  for (size_t i = ws1->getNumberHistograms(); i < output->getNumberHistograms(); i++)
  {
    specid_t origid;
    origid = output->getSpectrum(i)->getSpectrumNo();
    output->getSpectrum(i)->setSpectrumNo(origid + ws1max);
  }
  // To be deprecated:
  output->generateSpectraMap();
}

/// Appends the removal of the empty group after execution to the Algorithm::processGroups() method
bool ConjoinWorkspaces::processGroups()
{
  // Call the base class method for most of the functionality
  const bool retval = Algorithm::processGroups();

  // If that was successful, remove the now empty group in the second input workspace property
  if (retval) AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));

  return retval;
}

} // namespace Algorithm
} // namespace Mantid
