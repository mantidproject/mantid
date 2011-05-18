#include "MantidAlgorithms/ChangePulsetime.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ChangePulsetime)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using std::size_t;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ChangePulsetime::ChangePulsetime()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ChangePulsetime::~ChangePulsetime()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ChangePulsetime::initDocs()
  {
    this->setWikiSummary("Adds a constant time value, in seconds, to the pulse time of events in an [[EventWorkspace]]. ");
    this->setOptionalMessage("Adds a constant time value, in seconds, to the pulse time of events in an EventWorkspace. ");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ChangePulsetime::init()
  {
    declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::Input), "An input event workspace.");
    declareProperty(new PropertyWithValue<double>("TimeOffset",Direction::Input),
          "Number of seconds (a float) to add to each event's pulse time. Required.");
    declareProperty(new ArrayProperty<int>("WorkspaceIndexList",""), "An optional list of workspace indices to change. If blank, all spectra in the workspace are modified.");
    declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Direction::Output), "An output event workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ChangePulsetime::exec()
  {
    EventWorkspace_const_sptr in_ws = getProperty("InputWorkspace");
    EventWorkspace_sptr out_ws = getProperty("OutputWorkspace");
    if (!out_ws)
    {
      //Make a brand new EventWorkspace
      out_ws = boost::dynamic_pointer_cast<EventWorkspace>(
          API::WorkspaceFactory::Instance().create("EventWorkspace", in_ws->getNumberHistograms(), 2, 1));
      //Copy geometry over.
      API::WorkspaceFactory::Instance().initializeFromParent(in_ws, out_ws, false);
      //You need to copy over the data as well.
      out_ws->copyDataFrom( (*in_ws) );
    }

    // Either use the given list or use all spectra
    std::vector<int> workspaceIndices = getProperty("WorkspaceIndexList");
    int64_t num_to_do = static_cast<int64_t>(workspaceIndices.size());
    bool doAll = false;
    if (workspaceIndices.size() == 0)
    {
      doAll = true;
      num_to_do = in_ws->getNumberHistograms();
    }

    double timeOffset = getProperty("TimeOffset");

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i=0; i < num_to_do; i++)
    {
      // What worksapce index?
      int64_t wi;
      if (doAll)
        wi = i;
      else
        wi = workspaceIndices[i];

      // Call the method on the event list
      out_ws->getEventList(wi).addPulsetime(timeOffset);
    }

    setProperty("OutputWorkspace", out_ws);
  }



} // namespace Mantid
} // namespace Algorithms

