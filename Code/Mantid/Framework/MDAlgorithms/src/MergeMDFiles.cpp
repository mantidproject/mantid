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
  MergeMDFiles::MergeMDFiles()
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

  //----------------------------------------------------------------------------------------------
  /** Loads all of the box data required (no events) for later use.
    * Calculates total number events in each box
   * Also opens the files and leaves them open */
  //template<typename MDE, size_t nd>
  //void MergeMDFiles::loadBoxData()
  //{
  //  this->progress(0.05, "Loading File Info");
  //  std::vector<double> &sigErr      = m_BoxStruct.getSigErrData();
  //  std::vector<uint64_t>& eventPlaces = m_BoxStruct.getEventIndex();
  //  // prepare target workspace arrays of data/indexes
  //  for(size_t i=0;i<sigErr.size();i++)
  //  {
  //    sigErr[i]=0;
  //    eventPlaces[i]=0;
  //  }
  //  // Get plain box structure and box tree
  //  std::vector<Kernel::ISaveable *> &pBoxes = m_BoxStruct.getBoxes();

  //  // Total number of events in ALL files.
  //  totalEvents = 0;
  //  m_EachBoxIndexes.reserve(m_Filenames.size());
  //  m_pFiles.reserve(m_Filenames.size());
  //  std::vector<double> fileN_SigErr;
  //  try
  //  {
  //    for (size_t i=0; i<m_Filenames.size(); i++)
  //    {
  //      // Open the file to read
  //      ::NeXus::File * file = new ::NeXus::File(m_Filenames[i], NXACC_READ);
  //      m_pFiles.push_back(file);

  //      file->openGroup("MDEventWorkspace", "NXentry");
  //      file->openGroup("box_structure", "NXdata");

  //      // Start index/length into the list of events
  //      auto spBoxEventsInd = boost::shared_ptr<std::vector<uint64_t> >(new std::vector<uint64_t>());//box_event_index;
  //      file->readData("box_event_index", *spBoxEventsInd);
  //      m_EachBoxIndexes.push_back(spBoxEventsInd);

  //      file->readData("box_signal_errorsquared",fileN_SigErr);

  //      // Check for consistency
  //      if (i>0)
  //      {
  //        if (spBoxEventsInd->size() != m_EachBoxIndexes[0]->size())
  //          throw std::runtime_error("Inconsistent number of boxes found in file " + m_Filenames[i] + ". Cannot merge these files. Did you generate them all with exactly the same box structure?");
  //      }
  //      file->closeGroup();
  //      // calculate total number of events per cell and total signal/error
  //      size_t nBoxes = spBoxEventsInd->size()/2;
  //      for(size_t j=0;j<nBoxes;j++)
  //      {
  //        size_t ID = pBoxes[j]->getId();
  //        eventPlaces[2*ID+1]+= spBoxEventsInd->operator[](2*ID+1);
  //        sigErr[2*ID  ]+= fileN_SigErr[2*ID ];
  //        sigErr[2*ID+1]+= fileN_SigErr[2*ID+1];
  //      }

  //      // Navigate to the event_data block and leave it open
  //      file->openGroup("event_data", "NXdata");
  //      // Open the event data, track the total number of events
  //      totalEvents += API::BoxController::openEventNexusData(file);
  //    }
  //  }
  //  catch (...)
  //  {
  //    // Close all open files in case of error
  //    for (size_t i=0; i<m_pFiles.size(); i++)
  //      m_pFiles[i]->close();
  //    throw;
  //  }

  //  // This is how many boxes are in all the files.
  //  size_t numBoxes = m_EachBoxIndexes[0]->size() / 2;
  //  //Kernel::ISaveable::sortObjByFilePos(pBoxes);
  //  if(pDiskBuffer)
  //  {
  //    // synchronize plain box structure and box tree
  //      uint64_t filePos=0;
  //      for(size_t i=0;i<numBoxes;i++)
  //      {
  //        size_t ID = pBoxes[i]->getId();
  //        uint64_t nEvents = eventPlaces[2*ID+1];
  //        pBoxes[i]->setFilePosition(filePos,nEvents,false);

  //        filePos +=nEvents;

  //        MDBoxBase<MDE,nd> * box = dynamic_cast<MDBoxBase<MDE,nd> *>(pBoxes[i]);
  //      // clear event data from memory if something left there from cloning -- it is rubbish anyway;
  //        box->clear();
  //      // set up integral signal and error
  //        box->setSignal(sigErr[2*ID  ]);
  //        box->setErrorSquared(sigErr[2*ID+1]);
  //      }
  //    if(filePos!=totalEvents)throw std::runtime_error("Number of total events is not equal to number of events in all files, logic");  
  //  }
  //  else
  //  {  // just clear boxes to be on a safe side
  //     for(size_t i=0;i<numBoxes;i++)
  //      {
  //        MDBoxBase<MDE,nd> * box = dynamic_cast<MDBoxBase<MDE,nd> *>(pBoxes[i]);
  //      // clear event data from memory if something left there from cloning -- it is rubbish anyway;
  //        box->clear();
  //      }


  //  }

  //  g_log.notice() << totalEvents << " events in " << m_pFiles.size() << " files." << std::endl;
  //}

  ///** Task that loads all of the events from correspondend boxes of all files
  //  * that is being merged into a particular box in the output workspace.
  //*/
  //template<typename MDE, size_t nd>
  //uint64_t MergeMDFiles::loadEventsFromSubBoxes(MDBox<MDE, nd> *TargetBox)
  //{
  //  /// the events which are in the 
  //  std::vector<MDE> AllBoxEvents;
  //  AllBoxEvents.reserve(TargetBox->getFileSize());

  //  uint64_t nBoxEvents(0);
  //  for (size_t iw=0; iw<this->m_pFiles.size(); iw++)
  //  {
  //    size_t ID = TargetBox->getId();
  // // The file and the indexes into that file
  //    ::NeXus::File * file = this->m_pFiles[iw];
  //     auto spBoxEventInd = this->m_EachBoxIndexes[iw];

  //     uint64_t fileLocation   = spBoxEventInd->operator[](ID*2+0);
  //     uint64_t numFileEvents  = spBoxEventInd->operator[](ID*2+1);

  //    if (numFileEvents == 0) continue;
  //    nBoxEvents += numFileEvents;

  //        //// Occasionally release free memory (has an effect on Linux only).
  //        //if (numEvents > 1000000)
  //        //  MemoryManager::Instance().releaseFreeMemory();

  //    // This will APPEND the events to the one vector
  //    MDE::loadVectorFromNexusSlab(AllBoxEvents, file, fileLocation, numFileEvents);
  //  }
  //  std::vector<MDE> &data = TargetBox->getEvents();
  //  data.swap(AllBoxEvents);
  //  if(pDiskBuffer) // file based workspaces have to have correct file size and position already set
  //  {
  //    if(nBoxEvents!=TargetBox->getFileSize())
  //      throw std::runtime_error("Initially estimated and downloaded number of events are not consitant");
  //  }
  //  // tell everybody that these events will not be needed any more and can be saved on HDD if necessary
  //  TargetBox->releaseEvents();
  //  // set box attribute telling that the data were loaded from HDD to not trying to load them again (from the target file which is wrong)
  //  TargetBox->setLoaded();
  //  return nBoxEvents;
  //}
  ////----------------------------------------------------------------------------------------------
  ///** Perform the merging, but clone the initial workspace and use the same splitting
  // * as it
  // *
  // * @param ws :: first MDEventWorkspace in the list to merge
  // */
  //template<typename MDE, size_t nd>
  //void MergeMDFiles::doExecByCloning(typename MDEventWorkspace<MDE, nd>::sptr ws)
  //{
  //  std::string outputFile = getProperty("OutputFilename");
  //  bool fileBasedWS(false);
  //  if (!outputFile.empty())
  //      fileBasedWS = true;

  // // Run the tasks in parallel? TODO: enable
  //  //bool Parallel = this->getProperty("Parallel");

  //  // Fix the box controller settings in the output workspace so that it splits normally
  //  BoxController_sptr bc = ws->getBoxController();
  //  // Fix the max depth to something bigger.
  //  bc->setMaxDepth(20);
  //  bc->setSplitThreshold(5000);
  //  if(fileBasedWS)
  //  {
  //  // Complete the file-back-end creation.
  //    pDiskBuffer = &(bc->getDiskBuffer());
  //    g_log.notice() << "Setting cache to 400 MB write." << std::endl;
  //    bc->setCacheParameters(sizeof(MDE), 400000000/sizeof(MDE));
  //  }
  //  // Init box structure used for memory/file space calculations
  //  m_BoxStruct.initFlatStructure<MDE,nd>(ws,outputFile);
  //  // First, load all the box data and calculate file positions of the target workspace
  //  this->loadBoxData<MDE,nd>();


  //  if(fileBasedWS)
  //  {
  //    m_BoxStruct.saveBoxStructure(outputFile);
  //    m_BoxStruct.initEventFileStorage(outputFile,bc,fileBasedWS,MDE::getTypeName());
  //  }



  //  size_t numBoxes = m_BoxStruct.getNBoxes();
  //    // Progress report based on events processed.
  //  this->prog = new Progress(this, 0.1, 0.9, size_t(numBoxes));
  //  prog->setNotifyStep(0.1);

  //  // For tracking progress
  //  //uint64_t totalEventsInTasks = 0;
  // 
  //  // Prepare thread pool
  //  CPUTimer overallTime;

  //  ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
  //  ThreadPool tp(ts);

  //  this->totalLoaded = 0;
  //  std::vector<API::IMDNode *> &boxes = m_BoxStruct.getBoxes();
  //  for(size_t ib=0;ib<numBoxes;ib++)
  //  {
  //    MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[ib]);
  //    if(!box)continue;
  //    // load all contributed events into current box;
  //    this->loadEventsFromSubBoxes<MDE,nd>(box);

  //    if(pDiskBuffer)
  //    {
  //      if(box->getDataInMemorySize()>0)
  //        pDiskBuffer->toWrite(box);
  //    }
  //    //
  //    //if (!Parallel)
  //    //{
  //    //  // Run the task serially only
  //    //  task->run();
  //    //  delete task;
  //    //}
  //    //else
  //    //{
  //    //  // Enqueue to run in parallel (at the joinAll() call below).
  //    //  ts->push(task);
  //    //}

  //    prog->reportIncrement(ib,"Loading and merging box data");
  //  }
  //  if(pDiskBuffer)
  //    pDiskBuffer->flushCache();
  //  //// Run any final tasks
  //  //tp.joinAll();
  //  g_log.information() << overallTime << " to do all the adding." << std::endl;

  //  // Close any open file handle
  //  for (size_t iw=0; iw<this->m_pFiles.size(); iw++)
  //  {
  //    ::NeXus::File * file = this->m_pFiles[iw];
  //    if (file)
  //    { // subfile was left open on MDEventsDataGroup
  //      file->closeGroup(); // close MDEvents group
  //      file->closeGroup(); // close MDWorkspace group
  //      file->close();
  //    }
  //  }

  //  // Finish things up
  //  this->finalizeOutput<MDE,nd>(ws);
  //}



  ////----------------------------------------------------------------------------------------------
  ///** Now re-save the MDEventWorkspace to update the file back end */
  //template<typename MDE, size_t nd>
  //void MergeMDFiles::finalizeOutput(typename MDEventWorkspace<MDE, nd>::sptr outWS)
  //{
  //  CPUTimer overallTime;

  //  this->progress(0.90, "Refreshing Cache");
  //  outWS->refreshCache();
  //  m_OutIWS = outWS;
  //  g_log.information() << overallTime << " to run refreshCache()." << std::endl;

  //  std::string outputFile = getProperty("OutputFilename");
  //  if (!outputFile.empty())
  //  {
  //    g_log.notice() << "Starting SaveMD to update the file back-end." << std::endl;
  //    IAlgorithm_sptr saver = this->createChildAlgorithm("SaveMD" ,0.9, 1.00);
  //    saver->setProperty("InputWorkspace", m_OutIWS);
  //    saver->setProperty("UpdateFileBackEnd", true);
  //    saver->executeAsChildAlg();
  //  }

 
  //  g_log.information() << overallTime << " to run SaveMD." << std::endl;
  //}


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MergeMDFiles::exec()
  {
    //// clear disk buffer which can remain from previous runs 
    //// the existance/ usage of the buffer idicates if the algorithm works with file based or memory based target workspaces;
    //pDiskBuffer = NULL;
    //MultipleFileProperty * multiFileProp = dynamic_cast<MultipleFileProperty*>(getPointerToProperty("Filenames"));
    //m_Filenames = MultipleFileProperty::flattenFileNames(multiFileProp->operator()());
    //if (m_Filenames.size() == 0)
    //  throw std::invalid_argument("Must specify at least one filename.");
    //std::string firstFile = m_Filenames[0];

    //// Start by loading the first file but just the box structure, no events, and not file-backed
    //IAlgorithm_sptr loader = createChildAlgorithm("LoadMD", 0.0, 0.05, false);
    //loader->setPropertyValue("Filename", firstFile);
    //loader->setProperty("MetadataOnly", false);
    //loader->setProperty("BoxStructureOnly", true);
    //loader->setProperty("FileBackEnd", false);
    //loader->setPropertyValue("OutputWorkspace", this->getPropertyValue("OutputWorkspace") );
    //loader->executeAsChildAlg();
    //IMDWorkspace_sptr firstWS = loader->getProperty("OutputWorkspace");


    //// Call the templated method
    //CALL_MDEVENT_FUNCTION( this->doExecByCloning, firstWS);

    //setProperty("OutputWorkspace", m_OutIWS);
  }



} // namespace Mantid
} // namespace MDAlgorithms

