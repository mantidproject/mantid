#include "MantidMDEvents/MergeMDEW.h"
#include "MantidKernel/System.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidNexus/NeXusFile.hpp"
#include "MantidMDEvents/IMDBox.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/CPUTimer.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MergeMDEW)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MergeMDEW::MergeMDEW()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MergeMDEW::~MergeMDEW()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MergeMDEW::initDocs()
  {
    this->setWikiSummary("Merge multiple MDEventWorkspaces from files that obey a common box format.");
    this->setOptionalMessage("Merge multiple MDEventWorkspaces from files that obey a common box format.");
    this->setWikiDescription(""
        "This algorithm is meant to merge a large number of large MDEventWorkspaces together into one "
        "file-backed MDEventWorkspace, without exceeding available memory."
        "\n\n"
        "First, you will need to generate a MDEventWorkspaces NXS file for each run with a fixed box structure:"
        "\n\n"
        "* This would be a MaxDepth=1 structure but with finer boxes, maybe 50x50x50.\n"
        "* This can be done immediately after acquiring each run so that less processing has to be done at once.\n"
        "\n\n"
        "Then, enter the path to all of the files created previously. The algorithm avoids excessive memory use by only keeping "
        "the events from ONE box from ALL the files in memory at once to further process and refine it.\n"
        "This is why it requires a common box structure.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MergeMDEW::init()
  {
    std::vector<std::string> exts(1, ".nxs");
    declareProperty(new MultipleFileProperty("Filenames", exts),
        "Select several MDEventWorkspace NXS files to merge together. Files must have common box structure.");

    declareProperty(new FileProperty("OutputFilename", "", FileProperty::OptionalSave, exts),
        "Choose a file to which to save the output workspace. Optional: if specified, the workspace created will be file-backed.");

    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "An output MDEventWorkspace.");
  }


  //======================================================================================
  /** Task that loads all of the events from a particular block from a file
   * that is being merged and then adds them onto another workspace.
   */
  TMDE_CLASS
  class MergeMDEWLoadTask : public Mantid::Kernel::Task
  {
  public:
    /** Constructor
     *
     * @param alg :: MergeMDEW Algorithm - used to pass parameters etc. around
     * @param blockNum :: Which block to load?
     * @param outWS :: Output workspace
     */
    MergeMDEWLoadTask(MergeMDEW * alg, size_t blockNum, typename MDEventWorkspace<MDE, nd>::sptr outWS)
        : m_alg(alg), m_blockNum(blockNum), outWS(outWS)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Main method that performs the work for the task. */
    void run()
    {
      // Vector of events accumulated from ALL files to merge.
      std::vector<MDE> events;

      // Go through each file
      m_alg->fileMutex.lock();
      for (size_t iw=0; iw<m_alg->files.size(); iw++)
      {
        // The file and the indexes into that file
        ::NeXus::File * file = m_alg->files[iw];
        std::vector<uint64_t> & box_event_index = m_alg->box_indexes[iw];

        uint64_t indexStart = box_event_index[m_blockNum*2+0];
        uint64_t numEvents = box_event_index[m_blockNum*2+1];
        // This will APPEND the events to the one vector
        MDE::loadVectorFromNexusSlab(events, file, indexStart, numEvents);
      } // For each file
      m_alg->fileMutex.unlock();

      if (events.size() > 0)
      {
        // Add all the events from the same box
        outWS->addEvents( events );

        // Track the total number of added events
        m_alg->statsMutex.lock();
        m_alg->totalLoaded += uint64_t(events.size());
        m_alg->getLogger().debug() << "Box " << m_blockNum << ". Total events " << m_alg->totalLoaded << ". This one added " << events.size() << ". "<< std::endl;
        // Report the progress
        m_alg->prog->reportIncrement(events.size(), "Loading Box");
        m_alg->statsMutex.unlock();
      } // there was something loaded

    }


  private:
    /// MergeMDEW Algorithm - used to pass parameters etc. around
    MergeMDEW * m_alg;
    /// Which block to load?
    size_t m_blockNum;
    /// Output workspace
    typename MDEventWorkspace<MDE, nd>::sptr outWS;
  };



  //----------------------------------------------------------------------------------------------
  /** Loads all of the box data required (no events) for later use.
   * Also opens the files and leaves them open */
  template<typename MDE, size_t nd>
  void MergeMDEW::loadBoxData()
  {
    this->progress(0.01, "Loading File Info");

    // Total number of events in ALL files.
    totalEvents = 0;

    try
    {
      for (size_t i=0; i<m_filenames.size(); i++)
      {
        // Open the file to read
        ::NeXus::File * file = new ::NeXus::File(m_filenames[i], NXACC_READ);
        files.push_back(file);

        file->openGroup("MDEventWorkspace", "NXentry");
        file->openGroup("box_structure", "NXdata");

        // Start index/length into the list of events
        std::vector<uint64_t> box_event_index;
        file->readData("box_event_index", box_event_index);
        box_indexes.push_back(box_event_index);

        // Check for consistency
        if (i>0)
        {
          if (box_event_index.size() != box_indexes[0].size())
            throw std::runtime_error("Inconsistent number of boxes found in file " + m_filenames[i] + ". Cannot merge these files.");
        }
        file->closeGroup();

        // Navigate to the event_data block and leave it open
        file->openGroup("event_data", "NXdata");
        // Open the event data, track the total number of events
        totalEvents += MDE::openNexusData(file);
      }
    }
    catch (...)
    {
      // Close all open files in case of error
      for (size_t i=0; i<files.size(); i++)
        files[i]->close();
      throw;
      return;
    }

    // This is how many boxes are in all the files.
    numBoxes = box_indexes[0].size() / 2;

    // Count the number of events in each box.
    eventsPerBox.resize(numBoxes, 0);
    for (size_t ib=0; ib<numBoxes; ib++)
    {
      uint64_t tot = 0;
      for (size_t j=0; j<files.size(); j++)
        tot += box_indexes[j][ib*2 + 1];
      eventsPerBox[ib] = tot;
    }

    g_log.notice() << totalEvents << " events in " << files.size() << " files." << std::endl;
  }



  //----------------------------------------------------------------------------------------------
  /** Create the output workspace using the input as a guide
   *
   * @param ws :: first workspace from the inputs
   * @return the MDEventWorkspace sptr.
   */
  template<typename MDE, size_t nd>
  typename MDEventWorkspace<MDE, nd>::sptr MergeMDEW::createOutputWS(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // Use the copy constructor to get the same dimensions etc.
    typename MDEventWorkspace<MDE, nd>::sptr outWS(new MDEventWorkspace<MDE, nd>(*ws));
    this->outIWS = outWS;

    std::string outputFile = getProperty("OutputFilename");

    // Fix the box controller settings in the output workspace so that it splits normally
    BoxController_sptr bc = outWS->getBoxController();
    // TODO: Specify these split parameters some smarter way?
    bc->setMaxDepth(20);
    bc->setSplitInto(4);
    bc->setSplitThreshold(10000);

    // Perform the initial box splitting.
    IMDBox<MDE,nd> * box = outWS->getBox();
    for (size_t d=0; d<nd; d++)
      box->setExtents(d, outWS->getDimension(d)->getMinimum(), outWS->getDimension(d)->getMaximum());
    box->setBoxController(bc);
    outWS->splitBox();

    // Save the empty WS and turn it into a file-backed MDEventWorkspace
    if (!outputFile.empty())
    {
      IAlgorithm_sptr saver = this->createSubAlgorithm("SaveMDEW" ,0.01, 0.05);
      saver->setProperty("InputWorkspace", outIWS);
      saver->setPropertyValue("Filename", outputFile);
      saver->setProperty("MakeFileBacked", true);
      saver->executeAsSubAlg();
    }

    // Complete the file-back-end creation.
    DiskMRU & mru = bc->getDiskMRU(); UNUSED_ARG(mru);
    g_log.notice() << "Setting cache to 0 MB read, 30 MB write, 2000 MB small objects." << std::endl;
    bc->setCacheParameters(sizeof(MDE), 0, 30000000/sizeof(MDE), 2000000000/sizeof(MDE));
    g_log.notice() << "Threshold for small boxes: " << bc->getDiskMRU().getSmallThreshold() << " events." << std::endl;


    return outWS;
  }


  //----------------------------------------------------------------------------------------------
  /** Perform the merging
   *
   * @param ws :: first MDEventWorkspace in the list to merge
   */
  template<typename MDE, size_t nd>
  void MergeMDEW::doExec(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // First, load all the box data
    this->loadBoxData<MDE,nd>();

    // Now create the output workspace
    typename MDEventWorkspace<MDE, nd>::sptr outWS = this->createOutputWS<MDE,nd>(ws);

    // Progress report based on events processed.
    this->prog = new Progress(this, 0.1, 0.8, size_t(totalEvents));

    // For tracking progress
    uint64_t totalEventsInTasks = 0;
    this->totalLoaded = 0;

    // Prepare thread pool
    CPUTimer overallTime;
    ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts);

    for (size_t ib=0; ib<numBoxes; ib++)
    {
      // Add a task for each box that actually has some events
      if (this->eventsPerBox[ib] > 0)
      {
        totalEventsInTasks += eventsPerBox[ib];
        MergeMDEWLoadTask<MDE,nd> * task = new MergeMDEWLoadTask<MDE,nd>(this, ib, outWS);
        ts->push(task);
      }

      // You've added enough tasks that will fill up some memory.
      if (totalEventsInTasks > 10000000)
      {
        // Run all the tasks
        tp.joinAll();

        // Now do all the splitting tasks
        g_log.information() << "Splitting boxes since we have added " << totalEventsInTasks << " events." << std::endl;
        outWS->splitAllIfNeeded(ts);
        if (ts->size() > 0)
          prog->doReport("Splitting Boxes");
        tp.joinAll();

        totalEventsInTasks = 0;
      }
    } // for each box

    // Run any final tasks
    tp.joinAll();

    // Final splitting
    g_log.debug() << "Final splitting of boxes. " << totalEventsInTasks << " events." << std::endl;
    outWS->splitAllIfNeeded(ts);
    tp.joinAll();

    g_log.debug() << overallTime << " to do all the adding." << std::endl;

    this->progress(0.91, "Refreshing Cache");
    outWS->refreshCache();
    g_log.debug() << overallTime << " to run refreshCache()." << std::endl;


    // Now re-save the MDEventWorkspace to update the filec
    std::string outputFile = getProperty("OutputFilename");
    if (!outputFile.empty())
    {
      g_log.notice() << "Starting SaveMDEW to update the file back-end." << std::endl;
      IAlgorithm_sptr saver = this->createSubAlgorithm("SaveMDEW" ,0.92, 1.00);
      saver->setProperty("InputWorkspace", outIWS);
      saver->setProperty("UpdateFileBackEnd", true);
      saver->executeAsSubAlg();
    }

    g_log.debug() << overallTime << " to run SaveMDEW." << std::endl;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MergeMDEW::exec()
  {
    m_filenames = getProperty("Filenames");
    if (m_filenames.size() == 0)
      throw std::invalid_argument("Must specify at least one filename.");
    std::string firstFile = m_filenames[0];

    // Start by loading the first file but just the meta data to get dimensions, etc.
    IAlgorithm_sptr loader = createSubAlgorithm("LoadMDEW", 0.0, 0.05, false);
    loader->setPropertyValue("Filename", firstFile);
    loader->setPropertyValue("MetadataOnly", "1");
    loader->setPropertyValue("OutputWorkspace", "anonymous");
    loader->executeAsSubAlg();
    IMDEventWorkspace_sptr firstWS = loader->getProperty("OutputWorkspace");

    // Call the templated method
    CALL_MDEVENT_FUNCTION( this->doExec, firstWS);

    setProperty("OutputWorkspace", outIWS);
  }



} // namespace Mantid
} // namespace MDEvents

