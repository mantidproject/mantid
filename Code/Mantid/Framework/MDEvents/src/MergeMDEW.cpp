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

  //----------------------------------------------------------------------------------------------
  /** Perform the merging
   *
   * @param ws :: first MDEventWorkspace in the list to merge
   */
  template<typename MDE, size_t nd>
  void MergeMDEW::doExec(typename MDEventWorkspace<MDE, nd>::sptr ws)
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
    bc->setSplitThreshold(2000);

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

    // Vector of file handles to each input file
    std::vector< ::NeXus::File *> files;
    // Vector of the box_index vector for each each input file
    std::vector< std::vector<uint64_t> > box_indexes;

    this->progress(0.01, "Loading File Info");

    // Total number of events in ALL files.
    uint64_t totalEvents = 0;

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

    g_log.notice() << totalEvents << " events in " << files.size() << " files." << std::endl;

    // This is how many boxes are in all the files.
    size_t numBoxes = box_indexes[0].size() / 2;
    Progress prog(this, 0.1, 0.8, totalEvents);

    // For tracking progress
    uint64_t totalLoaded = 0;
    uint64_t totalSinceLastSplit = 0;
    Kernel::Mutex fileMutex;
    Kernel::Mutex statsMutex;
    //DiskMRU & mru = bc->getDiskMRU();
    g_log.notice() << "Setting cache to 300 MB read, 30 MB write." << std::endl;
    bc->setCacheParameters(300000000/sizeof(MDE), 30000000/sizeof(MDE), sizeof(MDE));

    //PRAGMA_OMP( parallel for schedule(dynamic, numBoxes/100) )
    for (int iib=0; iib<int(numBoxes); iib++)
    {
      size_t ib = size_t(iib);
      // Vector of events accumulated for ALL files to merge.
      std::vector<MDE> events;

      // Go through each file
      fileMutex.lock();
      for (size_t iw=0; iw<files.size(); iw++)
      {
        // The file and the indexes into that file
        ::NeXus::File * file = files[iw];
        std::vector<uint64_t> & box_event_index = box_indexes[iw];

        uint64_t indexStart = box_event_index[ib*2+0];
        uint64_t numEvents = box_event_index[ib*2+1];
        // This will APPEND the events to the one vector
        MDE::loadVectorFromNexusSlab(events, file, indexStart, numEvents);
      } // For each file
      fileMutex.unlock();

      if (events.size() > 0)
      {
        // Add all the events from the same box
        outWS->addEvents( events );

        // Track the total number of added events
        statsMutex.lock();
        totalSinceLastSplit += uint64_t(events.size());
        totalLoaded += uint64_t(events.size());
        g_log.debug() << "Box " << ib << ". Total events " << totalLoaded << ". This one added " << events.size() << "." << std::endl;
        //g_log.debug() << mru.getMemoryStr() << std::endl;
        // Report the progress
        prog.reportIncrement(events.size(), "Loading Box");
        statsMutex.unlock();

        // Perform splitting in a single thread. All others block while this runs.
        //PRAGMA_OMP( single )
        {
          // Is now a good time to split?
          //if (totalSinceLastSplit > (bc->getSplitThreshold() * bc->getTotalNumMDBoxes()))
          if (totalSinceLastSplit > 1000000)
          {
            g_log.debug() << "Splitting because we collected " << totalSinceLastSplit << " events" << std::endl;
            CPUTimer splitTime;
            ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
            ThreadPool tp(ts);
            outWS->splitAllIfNeeded(ts);
            tp.joinAll();
            g_log.debug() << splitTime << " to split." << std::endl;
            totalSinceLastSplit = 0;
          }
        }

      }
    } // for each box


    this->progress(0.91, "Refreshing Cache");
    outWS->refreshCache();

    // Now re-save the MDEventWorkspace to update the file
    if (!outputFile.empty())
    {
      g_log.notice() << "Starting SaveMDEW to update the file back-end." << std::endl;
      IAlgorithm_sptr saver = this->createSubAlgorithm("SaveMDEW" ,0.92, 1.00);
      saver->setProperty("InputWorkspace", outIWS);
      saver->setProperty("UpdateFileBackEnd", true);
      saver->executeAsSubAlg();
    }
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

    CALL_MDEVENT_FUNCTION( this->doExec, firstWS);

    setProperty("OutputWorkspace", outIWS);
  }



} // namespace Mantid
} // namespace MDEvents

