/*WIKI* 

This algorithm is meant to merge a large number of large MDEventWorkspaces together into one file-backed MDEventWorkspace, without exceeding available memory.

First, you will need to generate a MDEventWorkspaces NXS file for each run with a fixed box structure:
* You can call [[CreateMDWorkspace]] with MinRecursionDepth = MaxRecursionDepth.
** This will make the box structure identical. The number of boxes will be equal to SplitInto ^ (NumDims * MaxRecursionDepth).
** Aim for the boxes to be small enough for all events contained to fit in memory; without there being so many boxes as to slow down too much.
* This can be done immediately after acquiring each run so that less processing has to be done at once.

Then, enter the path to all of the files created previously. The algorithm avoids excessive memory use by only
keeping the events from ONE box from ALL the files in memory at once to further process and refine it.
This is why it requires a common box structure.

See also: [[MergeMD]], for merging any MDWorkspaces in system memory (faster, but needs more memory).

*WIKI*/

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDAlgorithms/MergeMDFiles.h"
#include "MantidNexusCPP/NeXusFile.hpp"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MergeMDFiles)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MergeMDFiles::MergeMDFiles()
  : clonedFirst(false)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MergeMDFiles::~MergeMDFiles()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MergeMDFiles::initDocs()
  {
    this->setWikiSummary("Merge multiple MDEventWorkspaces from files that obey a common box format.");
    this->setOptionalMessage("Merge multiple MDEventWorkspaces from files that obey a common box format.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MergeMDFiles::init()
  {
    std::vector<std::string> exts(1, ".nxs");
    declareProperty(new MultipleFileProperty("Filenames", exts),
        "Select several MDEventWorkspace NXS files to merge together. Files must have common box structure.");

    declareProperty(new FileProperty("OutputFilename", "", FileProperty::OptionalSave, exts),
        "Choose a file to which to save the output workspace. \n"
        "Optional: if specified, the workspace created will be file-backed. \n"
        "If not, it will be created in memory.");

    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "An output MDEventWorkspace.");
  }



//
//  //======================================================================================
//  /** Task that loads all of the events from a particular block from a file
//   * that is being merged and then adds them onto the output workspace.
//   */
//  TMDE_CLASS
//  class MergeMDLoadTask : public Mantid::Kernel::Task
//  {
//  public:
//    /** Constructor
//     *
//     * @param alg :: MergeMDFiles Algorithm - used to pass parameters etc. around
//     * @param blockNum :: Which block to load?
//     * @param outWS :: Output workspace
//     */
//    MergeMDLoadTask(MergeMDFiles * alg, size_t blockNum, typename MDEventWorkspace<MDE, nd>::sptr outWS)
//    : m_alg(alg), m_blockNum(blockNum), outWS(outWS)
//    {
//    }
//
//    //---------------------------------------------------------------------------------------------
//    /** Main method that performs the work for the task. */
//    void run()
//    {
//      // Vector of events accumulated from ALL files to merge.
//      std::vector<MDE> events;
//
//      // Go through each file
//      this->m_alg->fileMutex.lock();
//      for (size_t iw=0; iw<m_alg->files.size(); iw++)
//      {
//        // The file and the indexes into that file
//        ::NeXus::File * file = this->m_alg->files[iw];
//        std::vector<uint64_t> & box_event_index = this->m_alg->box_indexes[iw];
//
//        uint64_t indexStart = box_event_index[this->m_blockNum*2+0];
//        uint64_t numEvents = box_event_index[this->m_blockNum*2+1];
//        // This will APPEND the events to the one vector
//        MDE::loadVectorFromNexusSlab(events, file, indexStart, numEvents);
//      } // For each file
//      this->m_alg->fileMutex.unlock();
//
//      if (!events.empty())
//      {
//        // Add all the events from the same box
//        outWS->addEvents( events );
//
//        // Track the total number of added events
//        m_alg->statsMutex.lock();
//        m_alg->totalLoaded += uint64_t(events.size());
//        m_alg->getLogger().debug() << "Box " << m_blockNum << ". Total events " << m_alg->totalLoaded << ". This one added " << events.size() << ". "<< std::endl;
//        // Report the progress
//        m_alg->prog->reportIncrement(events.size(), "Loading Box");
//        m_alg->statsMutex.unlock();
//      } // there was something loaded
//
//    }
//
//
//  protected:
//    /// MergeMDFiles Algorithm - used to pass parameters etc. around
//    MergeMDFiles * m_alg;
//    /// Which block to load?
//    size_t m_blockNum;
//    /// Output workspace
//    typename MDEventWorkspace<MDE, nd>::sptr outWS;
//  };
//
//
//
//
//
//  //----------------------------------------------------------------------------------------------
//  /** Create the output workspace using the input as a guide
//   *
//   * @param ws :: first workspace from the inputs
//   * @return the MDEventWorkspace sptr.
//   */
//  template<typename MDE, size_t nd>
//  typename MDEventWorkspace<MDE, nd>::sptr MergeMDFiles::createOutputWS(typename MDEventWorkspace<MDE, nd>::sptr ws)
//  {
//    // Use the copy constructor to get the same dimensions etc.
//    typename MDEventWorkspace<MDE, nd>::sptr outWS(new MDEventWorkspace<MDE, nd>(*ws));
//    this->outIWS = outWS;
//
//    std::string outputFile = getProperty("OutputFilename");
//
//    // Fix the box controller settings in the output workspace so that it splits normally
//    BoxController_sptr bc = outWS->getBoxController();
//    // TODO: Specify these split parameters some smarter way?
//    bc->setMaxDepth(20);
//    bc->setSplitInto(4);
//    bc->setSplitThreshold(10000);
//
//    // Perform the initial box splitting.
//    IMDBox<MDE,nd> * box = outWS->getBox();
//    for (size_t d=0; d<nd; d++)
//      box->setExtents(d, outWS->getDimension(d)->getMinimum(), outWS->getDimension(d)->getMaximum());
//    box->setBoxController(bc);
//    outWS->splitBox();
//
//    // Save the empty WS and turn it into a file-backed MDEventWorkspace
//    if (!outputFile.empty())
//    {
//      IAlgorithm_sptr saver = this->createSubAlgorithm("SaveMD" ,0.01, 0.05);
//      saver->setProperty("InputWorkspace", outIWS);
//      saver->setPropertyValue("Filename", outputFile);
//      saver->setProperty("MakeFileBacked", true);
//      saver->executeAsSubAlg();
//    }
//
//    // Complete the file-back-end creation.
//    DiskBuffer & dbuf = bc->getDiskBuffer(); UNUSED_ARG(dbuf);
//    g_log.notice() << "Setting cache to 400 MB write." << std::endl;
//    bc->setCacheParameters(sizeof(MDE), 400000000/sizeof(MDE));
//
//
//    return outWS;
//  }
//
//
//  //----------------------------------------------------------------------------------------------
//  /** Perform the merging, with generalized output workspace
//   *
//   * @param ws :: first MDEventWorkspace in the list to merge
//   */
//  template<typename MDE, size_t nd>
//  void MergeMDFiles::doExec(typename MDEventWorkspace<MDE, nd>::sptr ws)
//  {
//    // First, load all the box data
//    this->loadBoxData<MDE,nd>();
//
//    // Now create the output workspace
//    typename MDEventWorkspace<MDE, nd>::sptr outWS = this->createOutputWS<MDE,nd>(ws);
//
//    // Progress report based on events processed.
//    this->prog = new Progress(this, 0.1, 0.9, size_t(totalEvents));
//    this->prog->setNotifyStep(0.1);
//
//    // For tracking progress
//    uint64_t totalEventsInTasks = 0;
//    this->totalLoaded = 0;
//
//    // Prepare thread pool
//    CPUTimer overallTime;
//    ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
//    ThreadPool tp(ts);
//
//    for (size_t ib=0; ib<numBoxes; ib++)
//    {
//      // Add a task for each box that actually has some events
//      if (this->eventsPerBox[ib] > 0)
//      {
//        totalEventsInTasks += eventsPerBox[ib];
//        MergeMDLoadTask<MDE,nd> * task = new MergeMDLoadTask<MDE,nd>(this, ib, outWS);
//        ts->push(task);
//      }
//
//      // You've added enough tasks that will fill up some memory.
//      if (totalEventsInTasks > 10000000)
//      {
//        // Run all the tasks
//        tp.joinAll();
//
//        // Occasionally release free memory (has an effect on Linux only).
//        MemoryManager::Instance().releaseFreeMemory();
//
//        // Now do all the splitting tasks
//        g_log.information() << "Splitting boxes since we have added " << totalEventsInTasks << " events." << std::endl;
//        outWS->splitAllIfNeeded(ts);
//        if (ts->size() > 0)
//          prog->doReport("Splitting Boxes");
//        tp.joinAll();
//
//        totalEventsInTasks = 0;
//      }
//    } // for each box
//
//    // Run any final tasks
//    tp.joinAll();
//
//    // Final splitting
//    g_log.debug() << "Final splitting of boxes. " << totalEventsInTasks << " events." << std::endl;
//    outWS->splitAllIfNeeded(ts);
//    tp.joinAll();
//    g_log.information() << overallTime << " to do all the adding." << std::endl;
//
//    // Finish things up
//    this->finalizeOutput<MDE,nd>(outWS);
//  }
//











  //----------------------------------------------------------------------------------------------
  /** Loads all of the box data required (no events) for later use.
   * Also opens the files and leaves them open */
  template<typename MDE, size_t nd>
  void MergeMDFiles::loadBoxData()
  {
    this->progress(0.05, "Loading File Info");

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
            throw std::runtime_error("Inconsistent number of boxes found in file " + m_filenames[i] + ". Cannot merge these files. Did you generate them all with exactly the same box structure?");
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
  /** Create the output workspace by cloning the first one
   *
   * @param ws :: first workspace from the inputs
   * @return the MDEventWorkspace sptr.
   */
  template<typename MDE, size_t nd>
  typename MDEventWorkspace<MDE, nd>::sptr MergeMDFiles::createOutputWSbyCloning(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    this->clonedFirst = true;
    std::string outputFile = getProperty("OutputFilename");

    // Convert the output workspace to file-backed
    if (!outputFile.empty())
    {
      IAlgorithm_sptr saver = this->createSubAlgorithm("SaveMD" ,0.05, 0.10, true);
      saver->setProperty("InputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(ws) );
      saver->setPropertyValue("Filename", outputFile);
      saver->setProperty("MakeFileBacked", true);
      saver->executeAsSubAlg();
    }
    // For the output
    outIWS = boost::dynamic_pointer_cast<IMDEventWorkspace>(ws);

    // Fix the box controller settings in the output workspace so that it splits normally
    BoxController_sptr bc = ws->getBoxController();
    // Fix the max depth to something bigger.
    bc->setMaxDepth(20);
    bc->setSplitThreshold(5000);

    // Complete the file-back-end creation.
    DiskBuffer & dbuf = bc->getDiskBuffer(); UNUSED_ARG(dbuf);
    g_log.notice() << "Setting cache to 400 MB write." << std::endl;
    bc->setCacheParameters(sizeof(MDE), 400000000/sizeof(MDE));

    return ws;
  }


  //======================================================================================
  /** Task that loads all of the events from a particular block from a file
   * that is being merged and then adds them to
   * a particular box in the output workspace.
   */
  TMDE_CLASS
  class MergeMDLoadToBoxTask : public Mantid::Kernel::Task
  {
  public:
    /// MergeMDFiles Algorithm - used to pass parameters etc. around
    MergeMDFiles * m_alg;
    /// Which block to load?
    size_t m_blockNum;
    /// Output workspace
    typename MDEventWorkspace<MDE, nd>::sptr outWS;
    /// List of boxes where index = box ID, value = the box pointer.
    typename std::vector<IMDBox<MDE,nd> *> & m_boxesById;
    /// True to split in parallel
    bool m_parallelSplit;

    /** Constructor
     *
     * @param alg :: MergeMDFiles Algorithm - used to pass parameters etc. around
     * @param blockNum :: Which block to load?
     * @param outWS :: Output workspace
     * @param boxesById :: list of boxes with IDs
     * @param parallelSplit :: if true, split the boxes via parallel mechanism
     */
    MergeMDLoadToBoxTask(MergeMDFiles * alg, size_t blockNum, typename MDEventWorkspace<MDE, nd>::sptr outWS,
        typename std::vector<IMDBox<MDE,nd> *> & boxesById, bool parallelSplit)
      : m_alg(alg), m_blockNum(blockNum), outWS(outWS),
        m_boxesById(boxesById), m_parallelSplit(parallelSplit)
    {
      this->m_cost = double(this->m_alg->eventsPerBox[this->m_blockNum]);
    }

    //---------------------------------------------------------------------------------------------
    /** Main method that performs the work for the task. */
    void run()
    {
      uint64_t numEvents = this->m_alg->eventsPerBox[this->m_blockNum];
      if (numEvents == 0) return;

      // Find the box in the output.
      IMDBox<MDE,nd> * outBox = this->m_boxesById[this->m_blockNum];
      if (!outBox)
        throw std::runtime_error("Could not find box at ID " + Strings::toString(this->m_blockNum) );
      BoxController_sptr bc = outBox->getBoxController();

      // Should we pre-emptively split the box
      MDBox<MDE,nd> * outMDBox = dynamic_cast<MDBox<MDE,nd> *>(outBox);
      if (outMDBox && (numEvents > bc->getSplitThreshold()))
      {
        // Yes, let's split it
        MDGridBox<MDE,nd> * parent = dynamic_cast<MDGridBox<MDE,nd> *>(outMDBox->getParent());
        if (parent)
        {
          size_t index = parent->getChildIndexFromID( outBox->getId() );
          if (index < parent->getNumChildren())
          {
            parent->splitContents(index);
            // Have to update our pointer - old one was deleted!
            outBox = parent->getChild(index);
          }
        }
      }
      // Is the output a grid box?
      MDGridBox<MDE,nd> * outGridBox = dynamic_cast<MDGridBox<MDE,nd> *>(outBox);


      // Vector of events accumulated from ALL files to merge.
      std::vector<MDE> events;

      // Occasionally release free memory (has an effect on Linux only).
      if (numEvents > 1000000)
        MemoryManager::Instance().releaseFreeMemory();

      // Reserve ALL the space you will need for this vector. Should speed up a lot.
      events.reserve(numEvents);

      // Go through each file
      this->m_alg->fileMutex.lock();
      //bc->fileMutex.lock();
      for (size_t iw=0; iw<this->m_alg->files.size(); iw++)
      {
        // The file and the indexes into that file
        ::NeXus::File * file = this->m_alg->files[iw];
        std::vector<uint64_t> & box_event_index = this->m_alg->box_indexes[iw];

        uint64_t indexStart = box_event_index[this->m_blockNum*2+0];
        uint64_t numEvents = box_event_index[this->m_blockNum*2+1];
        // This will APPEND the events to the one vector
        MDE::loadVectorFromNexusSlab(events, file, indexStart, numEvents);
      } // For each file
      //bc->fileMutex.unlock();
      this->m_alg->fileMutex.unlock();

      if (!events.empty())
      {
        // Add all the events from the same box
        outBox->addEventsUnsafe( events );
        events.clear();
        std::vector<MDE>().swap(events); // really free the data

        if (outGridBox)
        {
          // Occasionally release free memory (has an effect on Linux only).
          MemoryManager::Instance().releaseFreeMemory();

          // Now do a split on only this box.

          // On option, do the split in parallel
          ThreadSchedulerFIFO * ts = NULL;
          if (m_parallelSplit)
            ts = new ThreadSchedulerFIFO();
          ThreadPool tp(ts);

          outGridBox->splitAllIfNeeded(ts);

          if (m_parallelSplit)
            tp.joinAll();

          // Flush out any items to write.
          bc->getDiskBuffer().flushCache();
        }
      } // there was something loaded

      // Track the total number of added events
      this->m_alg->statsMutex.lock();
      this->m_alg->totalLoaded += numEvents;
      this->m_alg->getLogger().debug() << "Box " << this->m_blockNum << ". Total events " << this->m_alg->totalLoaded << ". This one added " << numEvents << ". "<< std::endl;
      // Report the progress
      this->m_alg->prog->reportIncrement(int(numEvents), "Loading Box");
      this->m_alg->statsMutex.unlock();

    } // (end run)

  };


  //----------------------------------------------------------------------------------------------
  /** Perform the merging, but clone the initial workspace and use the same splitting
   * as it
   *
   * @param ws :: first MDEventWorkspace in the list to merge
   */
  template<typename MDE, size_t nd>
  void MergeMDFiles::doExecByCloning(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // First, load all the box data
    this->loadBoxData<MDE,nd>();

    // Now create the output workspace
    typename MDEventWorkspace<MDE, nd>::sptr outWS = this->createOutputWSbyCloning<MDE,nd>(ws);

    // --------  Make a vector where index = box ID, value = box. ------------
    std::vector<IMDBox<MDE,nd> *> boxes;
    std::vector<IMDBox<MDE,nd> *> boxesById(outWS->getBoxController()->getMaxId()+1, NULL);
    //std::cout << boxesById.size() << " vector" << std::endl;
    outWS->getBox()->getBoxes(boxes, 1000, false);
    for (size_t i=0; i < boxes.size(); i++)
    {
      IMDBox<MDE,nd> * box = boxes[i];
      //std::cout << "Found box " << box->getId() << std::endl;
      boxesById[box->getId()] = box;
    }

    // Progress report based on events processed.
    this->prog = new Progress(this, 0.1, 0.9, size_t(totalEvents));
    prog->setNotifyStep(0.1);

    // For tracking progress
    uint64_t totalEventsInTasks = 0;
    this->totalLoaded = 0;
    // Prepare thread pool
    CPUTimer overallTime;
    //ThreadSchedulerLargestCost * ts = new ThreadSchedulerLargestCost();
    ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts);

    for (size_t ib=0; ib<numBoxes; ib++)
    {
      // Add a task for each box that actually has some events
      if (this->eventsPerBox[ib] > 0)
      {
        totalEventsInTasks += eventsPerBox[ib];
        MergeMDLoadToBoxTask<MDE,nd> * task = new MergeMDLoadToBoxTask<MDE,nd>(this, ib, outWS, boxesById, true);
        task->run();
        delete task;
        //ts->push(task);
      }
    } // for each box

    // Run any final tasks
    tp.joinAll();
    g_log.information() << overallTime << " to do all the adding." << std::endl;

    // Close any open file handle
    for (size_t iw=0; iw<this->files.size(); iw++)
    {
      ::NeXus::File * file = this->files[iw];
      if (file) file->close();
    }

    // Finish things up
    this->finalizeOutput<MDE,nd>(outWS);
  }



  //----------------------------------------------------------------------------------------------
  /** Now re-save the MDEventWorkspace to update the file back end */
  template<typename MDE, size_t nd>
  void MergeMDFiles::finalizeOutput(typename MDEventWorkspace<MDE, nd>::sptr outWS)
  {
    CPUTimer overallTime;

    this->progress(0.90, "Refreshing Cache");
    outWS->refreshCache();
    g_log.information() << overallTime << " to run refreshCache()." << std::endl;

    std::string outputFile = getProperty("OutputFilename");
    if (!outputFile.empty())
    {
      g_log.notice() << "Starting SaveMD to update the file back-end." << std::endl;
      IAlgorithm_sptr saver = this->createSubAlgorithm("SaveMD" ,0.9, 1.00);
      saver->setProperty("InputWorkspace", outIWS);
      saver->setProperty("UpdateFileBackEnd", true);
      saver->executeAsSubAlg();
    }

    g_log.information() << overallTime << " to run SaveMD." << std::endl;
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MergeMDFiles::exec()
  {
    m_filenames = getProperty("Filenames");
    if (m_filenames.size() == 0)
      throw std::invalid_argument("Must specify at least one filename.");
    std::string firstFile = m_filenames[0];

    // Start by loading the first file but just the box structure, no events, and not file-backed
    IAlgorithm_sptr loader = createSubAlgorithm("LoadMD", 0.0, 0.05, false);
    loader->setPropertyValue("Filename", firstFile);
    loader->setProperty("MetadataOnly", false);
    loader->setProperty("BoxStructureOnly", true);
    loader->setProperty("FileBackEnd", false);
    loader->setPropertyValue("OutputWorkspace", this->getPropertyValue("OutputWorkspace") );
    loader->executeAsSubAlg();
    IMDEventWorkspace_sptr firstWS = loader->getProperty("OutputWorkspace");

    // Call the templated method
    CALL_MDEVENT_FUNCTION( this->doExecByCloning, firstWS);

    setProperty("OutputWorkspace", outIWS);
  }



} // namespace Mantid
} // namespace MDAlgorithms

