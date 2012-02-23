/*WIKI*

This algorithm appends the spectra of two workspaces together.

The output workspace from this algorithm will be a copy of the first
input workspace, to which the data from the second input workspace
will be appended.

Workspace data members other than the data (e.g. instrument etc.) will be copied
from the first input workspace (but if they're not identical anyway,
then you probably shouldn't be using this algorithm!).

==== Restrictions on the input workspace ====

For [[EventWorkspace]]s, there are no restrictions on the input workspaces if ValidateInputs=false.

For [[Workspace2D]]s, the number of bins must be the same in both inputs.

If ValidateInputs is selected, then the input workspaces must also:
* Come from the same instrument
* Have common units
* Have common bin boundaries

==== Spectrum Numbers ====

If there is an overlap in the spectrum numbers of both inputs, then the output
workspace will have its spectrum numbers reset starting at 0 and increasing by
1 for each spectrum.

==== See Also ====

* [[ConjoinWorkspaces]] for joining parts of the same workspace.

*WIKI*/

#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/SingletonHolder.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(AppendSpectra)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  AppendSpectra::AppendSpectra()
  : Algorithm(), m_progress(NULL), m_overlapChecked(false)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  AppendSpectra::~AppendSpectra()
  {
    if( m_progress )
    {
      delete m_progress;
    }
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string AppendSpectra::name() const { return "AppendSpectra";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int AppendSpectra::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string AppendSpectra::category() const { return "Transforms\\Merging";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void AppendSpectra::initDocs()
  {
    this->setWikiSummary("Join two workspaces together by appending their spectra.");
    this->setOptionalMessage("Join two workspaces together by appending their spectra.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void AppendSpectra::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace1",
      "", Direction::Input, new CommonBinsValidator<>),
      "The name of the first input workspace");
    declareProperty(new WorkspaceProperty<>("InputWorkspace2",
      "", Direction::Input, new CommonBinsValidator<>),
      "The name of the second input workspace");

    declareProperty("ValidateInputs", true,
      "Perform a set of checks that the two input workspaces are compatible.");

    declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the output workspace");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void AppendSpectra::exec()
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

    bool ValidateInputs = this->getProperty("ValidateInputs");
    if (ValidateInputs)
    {
      // Check that the input workspaces meet the requirements for this algorithm
      this->validateInputs(ws1,ws2);
    }

    if (event_ws1 && event_ws2)
    {
      //Both are event workspaces. Use the special method
      this->execEvent();
      return;
    }
    // So it is a workspace 2D.

    // The only restriction, even with ValidateInputs=false
    if (ws1->blocksize() != ws2->blocksize())
      throw std::runtime_error("Workspace2D's must have the same number of bins.");

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

    // Set the output workspace
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(output) );
  }


  //----------------------------------------------------------------------------------------------
  /** Executes the algorithm
   *  @throw std::invalid_argument If the input workspaces do not meet the requirements of this algorithm
   */
  void AppendSpectra::execEvent()
  {
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
      {  }

      if( ws2Det && ws2Det->isMasked() )
        output->maskWorkspaceIndex(output_wi);

      m_progress->report();
    }

    //This will make the spectramap axis.
    output->doneAddingEventLists();

    //Set the same bins for all output pixels
    output->setAllX(XValues);

    // Fix spectrum numbers if needed
    this->fixSpectrumNumbers(event_ws1, event_ws2, output);

    // Set the output workspace
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(output) );
  }


  //----------------------------------------------------------------------------------------------
  /** Checks that the two input workspace have common binning & size, the same instrument & unit.
   *  Also calls the checkForOverlap method.
   *  @param ws1 :: The first input workspace
   *  @param ws2 :: The second input workspace
   *  @throw std::invalid_argument If the workspaces are not compatible
   */
  void AppendSpectra::validateInputs(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
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

  }

  //--------------------------------------------------------------------------------------------
  /**
   * Determine the minimum and maximum spectra ids.
   *
   * @param ws the workspace to search
   * @param min The minimum id (output).
   * @param max The maximum id (output).
   */
  void AppendSpectra::getMinMax(MatrixWorkspace_const_sptr ws, specid_t& min, specid_t& max)
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

  //--------------------------------------------------------------------------------------------
  /** If there is an overlap in spectrum numbers between ws1 and ws2,
   * then the spectrum numbers are reset as a simple 1-1 correspondence
   * with the workspace index.
   *
   * @param ws1 The first workspace supplied to the algorithm.
   * @param ws2 The second workspace supplied to the algorithm.
   * @param output The workspace that is going to be returned by the algorithm.
   */
  void AppendSpectra::fixSpectrumNumbers(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2,
                                             API::MatrixWorkspace_sptr output)
  {
    specid_t ws1min;
    specid_t ws1max;
    getMinMax(ws1, ws1min, ws1max);

    specid_t ws2min;
    specid_t ws2max;
    getMinMax(ws2, ws2min, ws2max);

    // is everything possibly ok?
    if (ws2min > ws1max)
      return;

    // change the axis by adding the maximum existing spectrum number to the current value
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      output->getSpectrum(i)->setSpectrumNo( specid_t(i) );

    // To be deprecated:
    output->generateSpectraMap();
  }


} // namespace Mantid
} // namespace Algorithms
