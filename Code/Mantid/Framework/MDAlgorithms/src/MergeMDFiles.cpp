#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidMDAlgorithms/MergeMDFiles.h"
#include "MantidAPI/MemoryManager.h"

#include <boost/scoped_ptr.hpp>
#include <Poco/File.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MergeMDFiles)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MergeMDFiles::MergeMDFiles() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MergeMDFiles::~MergeMDFiles() { clearEventLoaders(); }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MergeMDFiles::init() {
  std::vector<std::string> exts(1, ".nxs");
  declareProperty(new MultipleFileProperty("Filenames", exts),
                  "Select several MDEventWorkspace NXS files to merge "
                  "together. Files must have common box structure.");

  declareProperty(
      new FileProperty("OutputFilename", "", FileProperty::OptionalSave, exts),
      "Choose a file to which to save the output workspace. \n"
      "Optional: if specified, the workspace created will be file-backed. \n"
      "If not, it will be created in memory.");

  declareProperty("Parallel", false,
                  "Run the loading tasks in parallel.\n"
                  "This can be faster but might use more memory.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output MDEventWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Loads all of the box data required (no events) for later use.
* Calculates total number events in each box
* Also opens the files and leaves them open */
void MergeMDFiles::loadBoxData() {
  this->progress(0.05, "Loading File Info");
  // Get plain box structure and box tree
  std::vector<API::IMDNode *> &Boxes = m_BoxStruct.getBoxes();
  std::vector<uint64_t> &targetEventIndexes = m_BoxStruct.getEventIndex();
  // clear the averages for target event indexes;
  targetEventIndexes.assign(targetEventIndexes.size(), 0);

  // Total number of events in ALL files.
  totalEvents = 0;

  m_fileComponentsStructure.resize(m_Filenames.size());
  m_EventLoader.assign(m_Filenames.size(), NULL);

  try {
    for (size_t i = 0; i < m_Filenames.size(); i++) {
      // load box structure and the experimental info from each target
      // workspace.
      m_fileComponentsStructure[i].loadBoxStructure(m_Filenames[i], m_nDims,
                                                    m_MDEventType, true, true);
      // export just loaded experiment info to the target workspace
      m_fileComponentsStructure[i].exportExperiment(m_OutIWS);

      // Check for consistency
      if (i > 0) {
        if (m_fileComponentsStructure[i].getEventIndex().size() !=
            targetEventIndexes.size())
          throw std::runtime_error(
              "Inconsistent number of boxes found in file " + m_Filenames[i] +
              ". Cannot merge these files. Did you generate them all with "
              "exactly the same box structure?");
      }

      // calculate total number of events per target cell, which will be
      size_t nBoxes = Boxes.size();
      for (size_t j = 0; j < nBoxes; j++) {
        size_t ID = Boxes[j]->getID();
        targetEventIndexes[2 * ID + 1] +=
            m_fileComponentsStructure[i].getEventIndex()[2 * ID + 1];
        totalEvents += m_fileComponentsStructure[i].getEventIndex()[2 * ID + 1];
      }

      // Open the event data, track the total number of events
      auto bc = boost::shared_ptr<API::BoxController>(
          new API::BoxController(static_cast<size_t>(m_nDims)));
      bc->fromXMLString(m_fileComponentsStructure[i].getBCXMLdescr());

      m_EventLoader[i] = new BoxControllerNeXusIO(bc.get());
      m_EventLoader[i]->setDataType(sizeof(coord_t), m_MDEventType);
      m_EventLoader[i]->openFile(m_Filenames[i], "r");
    }
  } catch (...) {
    // Close all open files in case of error
    clearEventLoaders();
    throw;
  }

  const std::vector<int> &boxType = m_BoxStruct.getBoxType();
  // calculate event positions in the target file.
  uint64_t eventsStart = 0;
  for (size_t i = 0; i < Boxes.size(); i++) {
    API::IMDNode *mdBox = Boxes[i];
    mdBox->clear();
    size_t ID = mdBox->getID();

    // avoid grid boxes;
    if (boxType[ID] == 2)
      continue;

    uint64_t nEvents = targetEventIndexes[2 * ID + 1];
    targetEventIndexes[ID * 2] = eventsStart;
    if (m_fileBasedTargetWS)
      mdBox->setFileBacked(eventsStart, nEvents, false);

    eventsStart += nEvents;
  }

  g_log.notice() << totalEvents << " events in " << m_Filenames.size()
                 << " files." << std::endl;
}

/** Task that loads all of the events from corresponded boxes of all files
  * that is being merged into a particular box in the output workspace.
*/

uint64_t MergeMDFiles::loadEventsFromSubBoxes(API::IMDNode *TargetBox) {
  /// get rid of the events and averages which are in the memory erroneously
  /// (from cloning)
  TargetBox->clear();

  uint64_t nBoxEvents(0);
  std::vector<size_t> numFileEvents(m_EventLoader.size());

  for (size_t iw = 0; iw < this->m_EventLoader.size(); iw++) {
    size_t ID = TargetBox->getID();
    numFileEvents[iw] = static_cast<size_t>(
        m_fileComponentsStructure[iw].getEventIndex()[2 * ID + 1]);
    nBoxEvents += numFileEvents[iw];
  }

  // At this point memory required is known, so it is reserved all in one go
  TargetBox->reserveMemoryForLoad(nBoxEvents);

  for (size_t iw = 0; iw < this->m_EventLoader.size(); iw++) {
    size_t ID = TargetBox->getID();
    uint64_t fileLocation =
        m_fileComponentsStructure[iw].getEventIndex()[2 * ID + 0];
    if (numFileEvents[iw] == 0)
      continue;
    TargetBox->loadAndAddFrom(m_EventLoader[iw], fileLocation,
                              numFileEvents[iw]);
  }

  return nBoxEvents;
}

//----------------------------------------------------------------------------------------------
/** Perform the merging, but clone the initial workspace and use the same
 *splitting
 * as its structure is equivalent to the partial box structures.
 *
 * @param ws :: first MDEventWorkspace in the list to merge to.
 * @param outputFile :: the name of the output file where file-based workspace
 *should be saved
 */
void MergeMDFiles::doExecByCloning(Mantid::API::IMDEventWorkspace_sptr ws,
                                   const std::string &outputFile) {
  m_OutIWS = ws;
  m_MDEventType = ws->getEventTypeName();

  // Run the tasks in parallel? TODO: enable
  // bool Parallel = this->getProperty("Parallel");

  // Fix the box controller settings in the output workspace so that it splits
  // normally
  BoxController_sptr bc = ws->getBoxController();
  // set up internal variables characterizing the workspace.
  m_nDims = static_cast<int>(bc->getNDims());

  // Fix the max depth to something bigger.
  bc->setMaxDepth(20);
  bc->setSplitThreshold(5000);
  auto saver = boost::shared_ptr<API::IBoxControllerIO>(
      new DataObjects::BoxControllerNeXusIO(bc.get()));
  saver->setDataType(sizeof(coord_t), m_MDEventType);
  if (m_fileBasedTargetWS) {
    bc->setFileBacked(saver, outputFile);
    // Complete the file-back-end creation.
    g_log.notice() << "Setting cache to 400 MB write." << std::endl;
    bc->getFileIO()->setWriteBufferSize(400000000 / m_OutIWS->sizeofEvent());
  }

  /*   else
     {
         saver->openFile(outputFile,"w");
     }*/
  // Init box structure used for memory/file space calculations
  m_BoxStruct.initFlatStructure(ws, outputFile);

  // First, load all the box data and experiment info and calculate file
  // positions of the target workspace
  this->loadBoxData();

  size_t numBoxes = m_BoxStruct.getNBoxes();
  // Progress report based on events processed.
  this->prog = new Progress(this, 0.1, 0.9, size_t(numBoxes));
  prog->setNotifyStep(0.1);

  // For tracking progress
  // uint64_t totalEventsInTasks = 0;

  // Prepare thread pool
  CPUTimer overallTime;

  ThreadSchedulerFIFO *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts);

  Kernel::DiskBuffer *DiskBuf(NULL);
  if (m_fileBasedTargetWS) {
    DiskBuf = bc->getFileIO();
  }

  this->totalLoaded = 0;
  std::vector<API::IMDNode *> &boxes = m_BoxStruct.getBoxes();

  for (size_t ib = 0; ib < numBoxes; ib++) {
    auto box = boxes[ib];
    if (!box->isBox())
      continue;
    // load all contributed events into current box;
    this->loadEventsFromSubBoxes(boxes[ib]);

    if (DiskBuf) {
      if (box->getDataInMemorySize() >
          0) { // data position has been already pre-calculated
        box->getISaveable()->save();
        box->clearDataFromMemory();
        // Kernel::ISaveable *Saver = box->getISaveable();
        // DiskBuf->toWrite(Saver);
      }
    }
    // else
    //{   size_t ID = box->getID();
    //    uint64_t filePosition = targetEventIndexes[2*ID];
    //    box->saveAt(saver.get(), filePosition);
    //}
    //
    // if (!Parallel)
    //{
    //  // Run the task serially only
    //  task->run();
    //  delete task;
    //}
    // else
    //{
    //  // Enqueue to run in parallel (at the joinAll() call below).
    //  ts->push(task);
    //}

    prog->reportIncrement(ib, "Loading and merging box data");
  }
  if (DiskBuf) {
    DiskBuf->flushCache();
    bc->getFileIO()->flushData();
  }
  //// Run any final tasks
  // tp.joinAll();
  g_log.information() << overallTime << " to do all the adding." << std::endl;

  // Close any open file handle
  clearEventLoaders();

  // Finish things up
  this->finalizeOutput(outputFile);
}

//----------------------------------------------------------------------------------------------
/** Now re-save the MDEventWorkspace to update the file back end */
void MergeMDFiles::finalizeOutput(const std::string &outputFile) {
  CPUTimer overallTime;

  this->progress(0.90, "Refreshing Cache");
  m_OutIWS->refreshCache();

  g_log.information() << overallTime << " to run refreshCache()." << std::endl;

  if (!outputFile.empty()) {
    g_log.notice() << "Starting SaveMD to update the file back-end."
                   << std::endl;
    // create or open WS group and put there additional information about WS and
    // its dimensions
    bool old_data_there;
    boost::scoped_ptr< ::NeXus::File> file(MDBoxFlatTree::createOrOpenMDWSgroup(
        outputFile, m_nDims, m_MDEventType, false, old_data_there));
    this->progress(0.94, "Saving ws history and dimensions");
    MDBoxFlatTree::saveWSGenericInfo(file.get(), m_OutIWS);
    // Save each ExperimentInfo to a spot in the file
    this->progress(0.98, "Saving experiment infos");
    MDBoxFlatTree::saveExperimentInfos(file.get(), m_OutIWS);

    file->closeGroup();
    file->close();
    // -------------- Save Box Structure  -------------------------------------
    // OK, we've filled these big arrays of data representing flat box
    // structure. Save them.
    progress(0.91, "Writing Box Data");
    prog->resetNumSteps(8, 0.92, 1.00);

    // Save box structure;
    m_BoxStruct.saveBoxStructure(outputFile);

    g_log.information() << overallTime << " to run SaveMD structure"
                        << std::endl;
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MergeMDFiles::exec() {
  // clear disk buffer which can remain from previous runs
  // the existence/ usage of the buffer indicates if the algorithm works with
  // file based or memory based target workspaces;
  // pDiskBuffer = NULL;
  MultipleFileProperty *multiFileProp =
      dynamic_cast<MultipleFileProperty *>(getPointerToProperty("Filenames"));
  m_Filenames =
      MultipleFileProperty::flattenFileNames(multiFileProp->operator()());
  if (m_Filenames.size() == 0)
    throw std::invalid_argument("Must specify at least one filename.");
  std::string firstFile = m_Filenames[0];

  std::string outputFile = getProperty("OutputFilename");
  m_fileBasedTargetWS = false;
  if (!outputFile.empty()) {
    m_fileBasedTargetWS = true;
    if (Poco::File(outputFile).exists())
      throw std::invalid_argument(
          " File " + outputFile + " already exists. Can not use existing file "
                                  "as the target to MergeMD files.\n" +
          " Use it as one of source files if you want to add MD data to it");
  }

  // Start by loading the first file but just the box structure, no events, and
  // not file-backed
  // m_BoxStruct.loadBoxStructure(firstFile,
  IAlgorithm_sptr loader = createChildAlgorithm("LoadMD", 0.0, 0.05, false);
  loader->setPropertyValue("Filename", firstFile);
  loader->setProperty("MetadataOnly", false);
  loader->setProperty("BoxStructureOnly", true);
  loader->setProperty("FileBackEnd", false);
  loader->executeAsChildAlg();
  IMDWorkspace_sptr result = (loader->getProperty("OutputWorkspace"));

  auto firstWS = boost::dynamic_pointer_cast<API::IMDEventWorkspace>(result);
  if (!firstWS)
    throw std::runtime_error(
        "Can not load MDEventWorkspace from initial file " + firstFile);

  // do the job
  this->doExecByCloning(firstWS, outputFile);

  m_OutIWS->setFileNeedsUpdating(false);

  setProperty("OutputWorkspace", m_OutIWS);
}
/**Delete all event loaders */
void MergeMDFiles::clearEventLoaders() {
  for (size_t i = 0; i < m_EventLoader.size(); i++) {
    if (m_EventLoader[i]) {
      delete m_EventLoader[i];
      m_EventLoader[i] = NULL;
    }
  }
}

} // namespace Mantid
} // namespace MDAlgorithms
