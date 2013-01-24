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
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDAlgorithms/MergeMDFiles.h"
#include "MantidAPI/MemoryManager.h"

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
  MergeMDFiles::MergeMDFiles() :
  m_BoxStruct("")
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

    declareProperty("Parallel", false, "Run the loading tasks in parallel.\n"
        "This can be faster but might use more memory.");

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
//    MDBoxBase<MDE,nd> * box = outWS->getBox();
//    for (size_t d=0; d<nd; d++)
//      box->setExtents(d, outWS->getDimension(d)->getMinimum(), outWS->getDimension(d)->getMaximum());
//    box->setBoxController(bc);
//    outWS->splitBox();
//
//    // Save the empty WS and turn it into a file-backed MDEventWorkspace
//    if (!outputFile.empty())
//    {
//      IAlgorithm_sptr saver = this->createChildAlgorithm("SaveMD" ,0.01, 0.05);
//      saver->setProperty("InputWorkspace", outIWS);
//      saver->setPropertyValue("Filename", outputFile);
//      saver->setProperty("MakeFileBacked", true);
//      saver->executeAsChildAlg();
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
    * Calculates total number events in each box
   * Also opens the files and leaves them open */
  template<typename MDE, size_t nd>
  void MergeMDFiles::loadBoxData()
  {
    this->progress(0.05, "Loading File Info");
    std::vector<double> &sigErr      = m_BoxStruct.getSigErrData();
    std::vector<uint64_t>& eventPlaces = m_BoxStruct.getEventIndex();
    // prepare target workspace arrays of data/indexes
    for(size_t i=0;i<sigErr.size();i++)
    {
      sigErr[i]=0;
      eventPlaces[i]=0;
    }
    // Get plain box structure and box tree
    std::vector<Kernel::ISaveable *> &pBoxes = m_BoxStruct.getBoxes();

    // Total number of events in ALL files.
    totalEvents = 0;
    m_EachBoxIndexes.reserve(m_Filenames.size());
    m_pFiles.reserve(m_Filenames.size());
    std::vector<double> fileN_SigErr;
    try
    {
      for (size_t i=0; i<m_Filenames.size(); i++)
      {
        // Open the file to read
        ::NeXus::File * file = new ::NeXus::File(m_Filenames[i], NXACC_READ);
        m_pFiles.push_back(file);

        file->openGroup("MDEventWorkspace", "NXentry");
        file->openGroup("box_structure", "NXdata");

        // Start index/length into the list of events
        auto spBoxEventsInd = boost::shared_ptr<std::vector<uint64_t> >(new std::vector<uint64_t>());//box_event_index;
        file->readData("box_event_index", *spBoxEventsInd);
        m_EachBoxIndexes.push_back(spBoxEventsInd);

        file->readData("box_signal_errorsquared",fileN_SigErr);

        // Check for consistency
        if (i>0)
        {
          if (spBoxEventsInd->size() != m_EachBoxIndexes[0]->size())
            throw std::runtime_error("Inconsistent number of boxes found in file " + m_Filenames[i] + ". Cannot merge these files. Did you generate them all with exactly the same box structure?");
        }
        file->closeGroup();
        // calculate total number of events per cell and total signal/error
        size_t nBoxes = spBoxEventsInd->size()/2;
        for(size_t j=0;j<nBoxes;j++)
        {
          size_t ID = pBoxes[j]->getId();
          eventPlaces[2*ID+1]+= spBoxEventsInd->operator[](2*ID+1);
          sigErr[2*ID  ]+= fileN_SigErr[2*ID ];
          sigErr[2*ID+1]+= fileN_SigErr[2*ID+1];
        }

        // Navigate to the event_data block and leave it open
        file->openGroup("event_data", "NXdata");
        // Open the event data, track the total number of events
        totalEvents += API::BoxController::openEventNexusData(file);
      }
    }
    catch (...)
    {
      // Close all open files in case of error
      for (size_t i=0; i<m_pFiles.size(); i++)
        m_pFiles[i]->close();
      throw;
    }

    // This is how many boxes are in all the files.
    size_t numBoxes = m_EachBoxIndexes[0]->size() / 2;
    //Kernel::ISaveable::sortObjByFilePos(pBoxes);
    if(pDiskBuffer)
    {
      // synchronize plain box structure and box tree
        uint64_t filePos=0;
        for(size_t i=0;i<numBoxes;i++)
        {
          size_t ID = pBoxes[i]->getId();
          uint64_t nEvents = eventPlaces[2*ID+1];
          pBoxes[i]->setFilePosition(filePos,nEvents,false);

          filePos +=nEvents;

          MDBoxBase<MDE,nd> * box = dynamic_cast<MDBoxBase<MDE,nd> *>(pBoxes[i]);
        // clear event data from memory if something left there from cloning -- it is rubbish anyway;
          box->clear();
        // set up integral signal and error
          box->setSignal(sigErr[2*ID  ]);
          box->setErrorSquared(sigErr[2*ID+1]);
        }
      if(filePos!=totalEvents)throw std::runtime_error("Number of total events is not equal to number of events in all files, logic");  
    }
    else
    {  // just clear boxes to be on a safe side
       for(size_t i=0;i<numBoxes;i++)
        {
          MDBoxBase<MDE,nd> * box = dynamic_cast<MDBoxBase<MDE,nd> *>(pBoxes[i]);
        // clear event data from memory if something left there from cloning -- it is rubbish anyway;
          box->clear();
        }


    }

    g_log.notice() << totalEvents << " events in " << m_pFiles.size() << " files." << std::endl;
  }



//  //======================================================================================
//  /** Task that loads all of the events from a particular block from a file
//   * that is being merged and then adds them to
//   * a particular box in the output workspace.
//   */
//  TMDE_CLASS
//  class MergeMDLoadToBoxTask : public Mantid::Kernel::Task
//  {
//  public:
//    /// MergeMDFiles Algorithm - used to pass parameters etc. around
//    MergeMDFiles * m_alg;
//     /// Output workspace
//    typename MDEventWorkspace<MDE, nd>::sptr outWS;
//    /// List of boxes where index = box ID, value = the box pointer.
//    typename std::vector<MDBoxBase<MDE,nd> *> & m_boxesById;
//    /// True to split in parallel
//    bool m_parallelSplit;
//
//    /** Constructor
//     *
//     * @param alg :: MergeMDFiles Algorithm - used to pass parameters etc. around
//     * @param TargetBox :: the box where data are loaded to.
//     * @param outWS :: Output workspace
//     
//     * @param parallelSplit :: if true, split the boxes via parallel mechanism
//     */
//    MergeMDLoadToBoxTask(MergeMDFiles * alg, MDBox<MDE,nd> *TargetBox,
//        typename MDEventWorkspace<MDE, nd>::sptr outWS)
//      : m_alg(alg), 
//        outWS(outWS)
//
//    {
//      //this->m_cost = double(this->m_alg->m_EventsPerBox[this->m_blockNumStart]);
//      this->m_cost=1;
//    }
//
//    //---------------------------------------------------------------------------------------------
//    /** Main method that performs the work for the task. */
//    void run()
//    {
//      // Vector of each vector of events in each inBox
//      std::vector<std::vector<MDE> > eventsPerEachBox(m_blockNumEnd-m_blockNumStart);
//      // Each of the outputted boxes
//      std::vector<MDBoxBase<MDE,nd> *> outputBoxes(m_blockNumEnd-m_blockNumStart);
//      // Running total of processed events
//      size_t totalEvents = 0;
//      // Box controller of the workspace
//      BoxController_sptr bc = outWS->getBoxController();
//
//      // ----------------- Prepare the boxes for each box -----------------------------------------
//      for (size_t blockNum = this->m_blockNumStart; blockNum < this->m_blockNumEnd; blockNum++)
//      {
//        uint64_t numEvents(0) ;
////        uint64_t numEvents = this->m_alg->m_EventsPerBox[blockNum];
//        if (numEvents == 0) continue;
//
//        // Find the box in the output.
//        MDBoxBase<MDE,nd> * outBox = this->m_boxesById[blockNum];
//        if (!outBox)
//          throw std::runtime_error("Could not find box at ID " + Strings::toString(blockNum) );
//
//        ///// its optimization, let's avoid it for the time being. 
//        //// Should we pre-emptively split the box
//        //MDBox<MDE,nd> * outMDBox = dynamic_cast<MDBox<MDE,nd> *>(outBox);
//        //if (outMDBox && (numEvents > bc->getSplitThreshold()))
//        //{
//        //  // Yes, let's split it
//        //  MDGridBox<MDE,nd> * parent = dynamic_cast<MDGridBox<MDE,nd> *>(outMDBox->getParent());
//        //  if (parent)
//        //  {
//        //    size_t index = parent->getChildIndexFromID( outBox->getId() );
//        //    if (index < parent->getNumChildren())
//        //    {
//        //      parent->splitContents(index);
//        //      // Have to update our pointer - old one was deleted!
//        //      outBox = parent->getChild(index);
//        //    }
//        //  }
//        //}
//        // Save the output box for later
//        outputBoxes[blockNum-this->m_blockNumStart] = outBox;
//
//        // Vector of events accumulated from ALL files to merge.
//        std::vector<MDE> & events = eventsPerEachBox[blockNum-this->m_blockNumStart];
//
//        // Reserve ALL the space you will need for this vector. Should speed up a lot.
//        events.reserve(numEvents);
//      } // (for each blockNum)
//
//      // --------------------- Go through each file -------------------------------------------
//      this->m_alg->fileMutex.lock();
//      //bc->fileMutex.lock();
//      for (size_t iw=0; iw<this->m_alg->m_pFiles.size(); iw++)
//      {
//
//        for (size_t blockNum = this->m_blockNumStart; blockNum < this->m_blockNumEnd; blockNum++)
//        {
//          // This is the events vector for this particular block we are adding to
//          std::vector<MDE> & events = eventsPerEachBox[blockNum-this->m_blockNumStart];
//
//          // The file and the indexes into that file
//          ::NeXus::File * file = this->m_alg->m_pFiles[iw];
//          auto spBoxEventInd = this->m_alg->m_EachBoxIndexes[iw];
//
//          uint64_t indexStart = spBoxEventInd->operator[](blockNum*2+0);
//          uint64_t numEvents  = spBoxEventInd->operator[](blockNum*2+1);
//          if (numEvents == 0) continue;
//          totalEvents += numEvents;
//
//          // Occasionally release free memory (has an effect on Linux only).
//          if (numEvents > 1000000)
//            MemoryManager::Instance().releaseFreeMemory();
//
//          // This will APPEND the events to the one vector
//          MDE::loadVectorFromNexusSlab(events, file, indexStart, numEvents);
//        } // For each block
//
//      } // For each file
//      //bc->fileMutex.unlock();
//      this->m_alg->fileMutex.unlock();
//
//
//      // -------------- Now we actually do the adding for each block --------------------------------------
//      for (size_t blockNum = this->m_blockNumStart; blockNum < this->m_blockNumEnd; blockNum++)
//      {
//        // This is the events vector for this particular block we are adding to
//        std::vector<MDE> & events = eventsPerEachBox[blockNum-this->m_blockNumStart];
//
//        if (!events.empty())
//        {
//          // Box we are adding to.
//          MDBoxBase<MDE,nd> * outBox = outputBoxes[blockNum-this->m_blockNumStart];
//
//          // Is the output a grid box?
//          MDGridBox<MDE,nd> * outGridBox = dynamic_cast<MDGridBox<MDE,nd> *>(outBox);
//
//          // Add all the events from the same box
//          outBox->addEventsUnsafe( events );
//          events.clear();
//          std::vector<MDE>().swap(events); // really free the data
//
//          if (outGridBox)
//          {
//            // Occasionally release free memory (has an effect on Linux only).
//            MemoryManager::Instance().releaseFreeMemory();
//
//            // Now do a split on only this box.
//
//            // On option, do the split in parallel
//            if (m_parallelSplit)
//            {
//              ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
//              ThreadPool tp(ts);
//              outGridBox->splitAllIfNeeded(ts);
//              tp.joinAll();
//            }
//            else
//              outGridBox->splitAllIfNeeded(NULL);
//
//          }
//        } // there was something loaded
//      } // (for each block)
//
//      // HDF5 is NOT thread safe (by default) even for accessing DIFFERENT files from different threads.
//      // Hence we need this mutex here :( ! BUT ALL WAS MUTEXED INSIDE FLUSH. WHY to lock here?
//  //    this->m_alg->fileMutex.lock();
//      // Flush out any items to write.
//      bc->getDiskBuffer().flushCache();
//  //    this->m_alg->fileMutex.unlock();
//
//      // Track the total number of added events
//      this->m_alg->statsMutex.lock();
//      this->m_alg->totalLoaded += totalEvents;
//      this->m_alg->getLogger().debug() << "Boxes " << this->m_blockNumStart << " to " << this->m_blockNumEnd << ". Total events " << this->m_alg->totalLoaded << ". These added " << totalEvents << ". "<< std::endl;
//      // Report the progress
//      this->m_alg->prog->reportIncrement(int(totalEvents), "Loading Box");
//      this->m_alg->statsMutex.unlock();
//
//    } // (end run)
//
//  };
   /** Task that loads all of the events from correspondend boxes of all files
    * that is being merged into a particular box in the output workspace. */

  template<typename MDE, size_t nd>
  uint64_t MergeMDFiles::loadEventsFromSubBoxes(MDBox<MDE, nd> *TargetBox)
  {
    /// the events which are in the 
    std::vector<MDE> AllBoxEvents;
    AllBoxEvents.reserve(TargetBox->getFileSize());

    uint64_t nBoxEvents(0);
    for (size_t iw=0; iw<this->m_pFiles.size(); iw++)
    {
      size_t ID = TargetBox->getId();
   // The file and the indexes into that file
      ::NeXus::File * file = this->m_pFiles[iw];
       auto spBoxEventInd = this->m_EachBoxIndexes[iw];

       uint64_t fileLocation   = spBoxEventInd->operator[](ID*2+0);
       uint64_t numFileEvents  = spBoxEventInd->operator[](ID*2+1);

      if (numFileEvents == 0) continue;
      nBoxEvents += numFileEvents;

          //// Occasionally release free memory (has an effect on Linux only).
          //if (numEvents > 1000000)
          //  MemoryManager::Instance().releaseFreeMemory();

      // This will APPEND the events to the one vector
      MDE::loadVectorFromNexusSlab(AllBoxEvents, file, fileLocation, numFileEvents);
    }
    std::vector<MDE> &data = TargetBox->getEvents();
    data.swap(AllBoxEvents);
    if(pDiskBuffer) // file based workspaces have to have correct file size and position already set
    {
      if(nBoxEvents!=TargetBox->getFileSize())
        throw std::runtime_error("Initially estimated and downloaded number of events are not consitant");
    }
    // tell everybody that these events will not be needed any more and can be saved on HDD if necessary
    TargetBox->releaseEvents();
    // set box attribute telling that the data were loaded from HDD to not trying to load them again (from the target file which is wrong)
    TargetBox->setLoaded();
    return nBoxEvents;
  }
  //----------------------------------------------------------------------------------------------
  /** Perform the merging, but clone the initial workspace and use the same splitting
   * as it
   *
   * @param ws :: first MDEventWorkspace in the list to merge
   */
  template<typename MDE, size_t nd>
  void MergeMDFiles::doExecByCloning(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::string outputFile = getProperty("OutputFilename");
    bool fileBasedWS(false);
    if (!outputFile.empty())
        fileBasedWS = true;

   // Run the tasks in parallel? TODO: enable
    //bool Parallel = this->getProperty("Parallel");

    // Fix the box controller settings in the output workspace so that it splits normally
    BoxController_sptr bc = ws->getBoxController();
    // Fix the max depth to something bigger.
    bc->setMaxDepth(20);
    bc->setSplitThreshold(5000);
    if(fileBasedWS)
    {
    // Complete the file-back-end creation.
      pDiskBuffer = &(bc->getDiskBuffer());
      g_log.notice() << "Setting cache to 400 MB write." << std::endl;
      bc->setCacheParameters(sizeof(MDE), 400000000/sizeof(MDE));
    }
    // Init box structure used for memory/file space calculations
    m_BoxStruct.initFlatStructure<MDE,nd>(ws,outputFile);
    // First, load all the box data and calculate file positions of the target workspace
    this->loadBoxData<MDE,nd>();


    if(fileBasedWS)
    {
      m_BoxStruct.saveBoxStructure(outputFile);
      m_BoxStruct.initEventFileStorage(outputFile,bc,fileBasedWS,MDE::getTypeName());
    }



    size_t numBoxes = m_BoxStruct.getNBoxes();
      // Progress report based on events processed.
    this->prog = new Progress(this, 0.1, 0.9, size_t(numBoxes));
    prog->setNotifyStep(0.1);

    // For tracking progress
    //uint64_t totalEventsInTasks = 0;
   
    // Prepare thread pool
    CPUTimer overallTime;

    ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts);

    this->totalLoaded = 0;
    std::vector<Kernel::ISaveable *> &boxes = m_BoxStruct.getBoxes();
    for(size_t ib=0;ib<numBoxes;ib++)
    {
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[ib]);
      if(!box)continue;
      // load all contributed events into current box;
      this->loadEventsFromSubBoxes<MDE,nd>(box);

      if(pDiskBuffer)
      {
        if(box->getDataMemorySize()>0)
          pDiskBuffer->toWrite(box);
      }
      //
      //if (!Parallel)
      //{
      //  // Run the task serially only
      //  task->run();
      //  delete task;
      //}
      //else
      //{
      //  // Enqueue to run in parallel (at the joinAll() call below).
      //  ts->push(task);
      //}

      prog->reportIncrement(ib,"Loading and merging box data");
    }
    if(pDiskBuffer)
      pDiskBuffer->flushCache();
    //// Run any final tasks
    //tp.joinAll();
    g_log.information() << overallTime << " to do all the adding." << std::endl;

    // Close any open file handle
    for (size_t iw=0; iw<this->m_pFiles.size(); iw++)
    {
      ::NeXus::File * file = this->m_pFiles[iw];
      if (file)
      { // subfile was left open on MDEventsDataGroup
        file->closeGroup(); // close MDEvents group
        file->closeGroup(); // close MDWorkspace group
        file->close();
      }
    }

    // Finish things up
    this->finalizeOutput<MDE,nd>(ws);
  }



  //----------------------------------------------------------------------------------------------
  /** Now re-save the MDEventWorkspace to update the file back end */
  template<typename MDE, size_t nd>
  void MergeMDFiles::finalizeOutput(typename MDEventWorkspace<MDE, nd>::sptr outWS)
  {
    CPUTimer overallTime;

    this->progress(0.90, "Refreshing Cache");
    outWS->refreshCache();
    m_OutIWS = outWS;
    g_log.information() << overallTime << " to run refreshCache()." << std::endl;

    std::string outputFile = getProperty("OutputFilename");
    if (!outputFile.empty())
    {
      g_log.notice() << "Starting SaveMD to update the file back-end." << std::endl;
      IAlgorithm_sptr saver = this->createChildAlgorithm("SaveMD" ,0.9, 1.00);
      saver->setProperty("InputWorkspace", m_OutIWS);
      saver->setProperty("UpdateFileBackEnd", true);
      saver->executeAsChildAlg();
    }

 
    g_log.information() << overallTime << " to run SaveMD." << std::endl;
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MergeMDFiles::exec()
  {
    // clear disk buffer which can remain from previous runs 
    // the existance/ usage of the buffer idicates if the algorithm works with file based or memory based target workspaces;
    pDiskBuffer = NULL;
    MultipleFileProperty * multiFileProp = dynamic_cast<MultipleFileProperty*>(getPointerToProperty("Filenames"));
    m_Filenames = MultipleFileProperty::flattenFileNames(multiFileProp->operator()());
    if (m_Filenames.size() == 0)
      throw std::invalid_argument("Must specify at least one filename.");
    std::string firstFile = m_Filenames[0];

    // Start by loading the first file but just the box structure, no events, and not file-backed
    IAlgorithm_sptr loader = createChildAlgorithm("LoadMD", 0.0, 0.05, false);
    loader->setPropertyValue("Filename", firstFile);
    loader->setProperty("MetadataOnly", false);
    loader->setProperty("BoxStructureOnly", true);
    loader->setProperty("FileBackEnd", false);
    loader->setPropertyValue("OutputWorkspace", this->getPropertyValue("OutputWorkspace") );
    loader->executeAsChildAlg();
    IMDWorkspace_sptr firstWS = loader->getProperty("OutputWorkspace");


    // Call the templated method
    CALL_MDEVENT_FUNCTION( this->doExecByCloning, firstWS);

    setProperty("OutputWorkspace", m_OutIWS);
  }



} // namespace Mantid
} // namespace MDAlgorithms

