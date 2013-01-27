/*WIKI* 

Save a [[MDEventWorkspace]] to a .nxs file. The workspace's current box structure and entire list of events is preserved.
The resulting file can be loaded via [[LoadMD]].

If you specify MakeFileBacked, then this will turn an in-memory workspace to a file-backed one. Memory will be released as it is written to disk.

If you specify UpdateFileBackEnd, then any changes (e.g. events added using the PlusMD algorithm) will be saved to the file back-end.

*WIKI*/

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDAlgorithms/SaveMD.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <Poco/File.h>
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDBoxFlatTree.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveMD)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveMD::SaveMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveMD::~SaveMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveMD::initDocs()
  {
    this->setWikiSummary("Save a MDEventWorkspace or MDHistoWorkspace to a .nxs file.");
    this->setOptionalMessage("Save a MDEventWorkspace or MDHistoWorkspace to a .nxs file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveMD::init()
  {
    declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace or MDHistoWorkspace.");

    std::vector<std::string> exts;
    exts.push_back(".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::OptionalSave, exts),
        "The name of the Nexus file to write, as a full or relative path.\n"
        "Optional if UpdateFileBackEnd is checked.");
    // Filename is NOT used if UpdateFileBackEnd
    setPropertySettings("Filename", new EnabledWhenProperty("UpdateFileBackEnd", IS_EQUAL_TO, "0"));

    declareProperty("UpdateFileBackEnd", false,
        "Only for MDEventWorkspaces with a file back end: check this to update the NXS file on disk\n"
        "to reflect the current data structure. Filename parameter is ignored.");
    setPropertySettings("UpdateFileBackEnd", new EnabledWhenProperty("MakeFileBacked", IS_EQUAL_TO, "0"));

    declareProperty("MakeFileBacked", false,
        "For an MDEventWorkspace that was created in memory:\n"
        "This saves it to a file AND makes the workspace into a file-backed one.");
    setPropertySettings("MakeFileBacked", new EnabledWhenProperty("UpdateFileBackEnd", IS_EQUAL_TO, "0"));
  }

 /// Save each NEW ExperimentInfo to a spot in the file
 void SaveMD::saveExperimentInfos(::NeXus::File * const file, API::IMDEventWorkspace_const_sptr ws)
 {

    std::map<std::string,std::string> entries;
    file->getEntries(entries);
    for (uint16_t i=0; i < ws->getNumExperimentInfo(); i++)
    {
      ExperimentInfo_const_sptr ei = ws->getExperimentInfo(i);
      std::string groupName = "experiment" + Strings::toString(i);
      if (entries.find(groupName) == entries.end())
      {
        // Can't overwrite entries. Just add the new ones
        file->makeGroup(groupName, "NXgroup", true);
        file->putAttr("version", 1);
        ei->saveExperimentInfoNexus(file);
        file->closeGroup();

        // Warning for high detector IDs.
        // The routine in MDEvent::saveVectorToNexusSlab() converts detector IDs to single-precision floats
        // Floats only have 24 bits of int precision = 16777216 as the max, precise detector ID
        detid_t min = 0;
        detid_t max = 0;
        try
        {
          ei->getInstrument()->getMinMaxDetectorIDs(min, max);
        }
        catch (std::runtime_error &)
        { /* Ignore error. Min/max will be 0 */ }

        if (max > 16777216)
        {
          g_log.warning() << "This instrument (" << ei->getInstrument()->getName() <<
              ") has detector IDs that are higher than can be saved in the .NXS file as single-precision floats." << std::endl;
          g_log.warning() << "Detector IDs above 16777216 will not be precise. Please contact the developers." << std::endl;
        }
      }
    }



 }
  //----------------------------------------------------------------------------------------------
  /** Save the MDEventWorskpace to a file.
   * Based on the Intermediate Data Format Detailed Design Document, v.1.R3 found in SVN.
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void SaveMD::doSaveEvents(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::string filename = getPropertyValue("Filename");
    bool update = getProperty("UpdateFileBackEnd");
    bool MakeFileBacked = getProperty("MakeFileBacked");

    if (update && MakeFileBacked)
      throw std::invalid_argument("Please choose either UpdateFileBackEnd or MakeFileBacked, not both.");

    if (MakeFileBacked && ws->isFileBacked())
      throw std::invalid_argument("You picked MakeFileBacked but the workspace is already file-backed!");

    BoxController_sptr bc = ws->getBoxController();

    // Open/create the file
    ::NeXus::File * file;
    if (update)
    {
      progress(0.01, "Flushing Cache");
      // First, flush to disk. This writes all the event data to disk!
      bc->getDiskBuffer().flushCache();

      // Use the open file
      file = bc->getFile();
      if (!file)
        throw std::invalid_argument("MDEventWorkspace is not file-backed. Do not check UpdateFileBackEnd!");

      // Normally the file is left open with the event data open, but in READ only mode.
      // Needs to be closed and reopened for things to work
      file->closeData();
      file->close();
      // Reopen the file
      filename = bc->getFilename();
      file = new ::NeXus::File(filename, NXACC_RDWR);
    }
    else
    {
    // Erase the file if it exists
    Poco::File oldFile(filename);
    if (oldFile.exists())
      oldFile.remove();
      // Create a new file in HDF5 mode.
      file = new ::NeXus::File(filename, NXACC_CREATE5);
    }

    // The base entry. Named so as to distinguish from other workspace types.
    if (update) // open workspace group
      file->openGroup("MDEventWorkspace", "NXentry");
    else // create and open workspace group
      file->makeGroup("MDEventWorkspace", "NXentry", true);
    

    // General information
    if (!update)
    {
      // Write out some general information like # of dimensions
      file->writeData("dimensions", int32_t(nd));
      // Save the algorithm history under "process"
      ws->getHistory().saveNexus(file);
    }

    file->putAttr("event_type", MDE::getTypeName());
    // Save each NEW ExperimentInfo to a spot in the file
    this->saveExperimentInfos(file,ws);

    // Save some info as attributes. (Note: need to use attributes, not data sets because those cannot be resized).
    file->putAttr("definition",  ws->id());
    file->putAttr("title",  ws->getTitle() );
    // Save each dimension, as their XML representation
    for (size_t d=0; d<nd; d++)
    {
      std::ostringstream mess;
      mess << "dimension" << d;
      file->putAttr( mess.str(), ws->getDimension(d)->toXMLString() );
    }
    MDBoxFlatTree BoxFlatStruct;

  // flatten the box structure
    BoxFlatStruct.initFlatStructure<MDE,nd>(ws);

    // Start the event Data group and prepare the data chunk storage.
    BoxFlatStruct.initEventFileStorage(file,bc,MakeFileBacked||update,MDE::getTypeName());
//----------------------------------------------------------------------------------------------------------------
 
    // get boxes vector
    std::vector<Kernel::ISaveable *> &boxes = BoxFlatStruct.getBoxes();

    size_t maxBoxes = boxes.size();
    size_t chunkSize = bc->getDataChunk();

    Progress * prog = new Progress(this, 0.05, 0.9, maxBoxes);
    if(update)
    {
      // use write buffer to update file and allocate/reallocate all data chunk to their rightfull positions
      Kernel::DiskBuffer &db = bc->getDiskBuffer();
      // if write buffer size is smaller then chunk size it is usually not very efficietn
      if(db.getWriteBufferSize()<chunkSize)db.setWriteBufferSize(chunkSize);
      for(size_t i=0;i<maxBoxes;i++)
      {
        MDBox<MDE,nd> * mdBox = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
        if(!mdBox)continue;
        if(mdBox->getDataMemorySize()>0) // if part of the object is on HDD, this will load it into memory before saving
          db.toWrite(mdBox);
      }
      // clear all still remaining in the buffer. 
      db.flushCache();

    }
    else 
    {
      BoxFlatStruct.setBoxesFilePositions(MakeFileBacked);
      boxes = BoxFlatStruct.getBoxes();
      for(size_t i=0;i<maxBoxes;i++)
      {
        MDBox<MDE,nd> * mdBox = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
        if(!mdBox)continue;
        // avoid HDF/Nexus error on empty writes
        if(mdBox->getNPoints() != 0)
            mdBox->saveNexus(file);
          // set that it is on disk and clear the actual events to free up memory, saving occured earlier
        if (MakeFileBacked) mdBox->clearDataFromMemory();
        prog->report("Saving Box");
      }
    }

    // Done writing the event data.
    file->closeData();


    // ------------------------- Save Free Blocks --------------------------------------------------
    // Get a vector of the free space blocks to save to the file
    std::vector<uint64_t> freeSpaceBlocks;
    bc->getDiskBuffer().getFreeSpaceVector(freeSpaceBlocks);
    if (freeSpaceBlocks.empty())
      freeSpaceBlocks.resize(2, 0); // Needs a minimum size
    std::vector<int> free_dims(2,2); free_dims[0] = int(freeSpaceBlocks.size()/2);
    std::vector<int> free_chunk(2,2); free_chunk[0] =int(bc->getDataChunk());

    // Now the free space blocks under event_data -- should be done better
    try
    {
       file->writeUpdatedData("free_space_blocks", freeSpaceBlocks, free_dims);
    }catch(...)
    {
       file->writeExtendibleData("free_space_blocks", freeSpaceBlocks, free_dims, free_chunk);
    }
/*    if (!update)
      file->writeExtendibleData("free_space_blocks", freeSpaceBlocks, free_dims, free_chunk);
    else
      file->writeUpdatedData("free_space_blocks", freeSpaceBlocks, free_dims);
  */  // close event group
    file->closeGroup();


    // -------------- Save Box Structure  -------------------------------------
    // OK, we've filled these big arrays of data. Save them.
    progress(0.91, "Writing Box Data");
    prog->resetNumSteps(8, 0.92, 1.00);

    //Save box structure;
    BoxFlatStruct.saveBoxStructure(file);

    if (update || MakeFileBacked)
    {
      // Need to keep the file open since it is still used as a back end.
      // Reopen the file
      filename = bc->getFilename();
      file = new ::NeXus::File(filename, NXACC_RDWR);
      // Re-open the data for events.
      file->openGroup("MDEventWorkspace", "NXentry");
      file->openGroup("event_data", "NXdata");
      uint64_t totalNumEvents = API::BoxController::openEventNexusData(file);
      bc->setFile(file, filename, totalNumEvents);
      // Mark file is up-to-date
      ws->setFileNeedsUpdating(false);

    }

    delete prog;

  }


  //----------------------------------------------------------------------------------------------
  /** Save a MDHistoWorkspace to a .nxs file
   *
   * @param ws :: MDHistoWorkspace to save
   */
  void SaveMD::doSaveHisto(Mantid::MDEvents::MDHistoWorkspace_sptr ws)
  {
    std::string filename = getPropertyValue("Filename");

    // Erase the file if it exists
    Poco::File oldFile(filename);
    if (oldFile.exists())
      oldFile.remove();

    // Create a new file in HDF5 mode.
    ::NeXus::File * file;
    file = new ::NeXus::File(filename, NXACC_CREATE5);

    // The base entry. Named so as to distinguish from other workspace types.
    file->makeGroup("MDHistoWorkspace", "NXentry", true);

    // Save the algorithm history under "process"
    ws->getHistory().saveNexus(file);

    // Save all the ExperimentInfos
    for (uint16_t i=0; i < ws->getNumExperimentInfo(); i++)
    {
      ExperimentInfo_sptr ei = ws->getExperimentInfo(i);
      std::string groupName = "experiment" + Strings::toString(i);
      if (ei)
      {
        // Can't overwrite entries. Just add the new ones
        file->makeGroup(groupName, "NXgroup", true);
        file->putAttr("version", 1);
        ei->saveExperimentInfoNexus(file);
        file->closeGroup();
      }
    }

    // Write out some general information like # of dimensions
    file->writeData("dimensions", int32_t(ws->getNumDims()));

    // Save each dimension, as their XML representation
    for (size_t d=0; d<ws->getNumDims(); d++)
    {
      std::ostringstream mess;
      mess << "dimension" << d;
      file->putAttr( mess.str(), ws->getDimension(d)->toXMLString() );
    }

    // Check that the typedef has not been changed. The NeXus types would need changing if it does!
    assert(sizeof(signal_t) == sizeof(double));

    // Number of data points
    int nPoints = static_cast<int>(ws->getNPoints());

    file->makeData("signal", ::NeXus::FLOAT64, nPoints, true);
    file->putData(ws->getSignalArray());
    file->closeData();

    file->makeData("errors_squared", ::NeXus::FLOAT64, nPoints, true);
    file->putData(ws->getErrorSquaredArray());
    file->closeData();

    file->makeData("num_events", ::NeXus::FLOAT64, nPoints, true);
    file->putData(ws->getNumEventsArray());
    file->closeData();

    file->makeData("mask", ::NeXus::INT8, nPoints, true);
    file->putData(ws->getMaskArray());
    file->closeData();


    // TODO: Links to original workspace???

    file->closeGroup();
    file->close();

  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveMD::exec()
  {
    IMDWorkspace_sptr ws = getProperty("InputWorkspace");
    IMDEventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<IMDEventWorkspace>(ws);
    MDHistoWorkspace_sptr histoWS =  boost::dynamic_pointer_cast<MDHistoWorkspace>(ws);

    if (eventWS)
    {
      // Wrapper to cast to MDEventWorkspace then call the function
      CALL_MDEVENT_FUNCTION(this->doSaveEvents, eventWS);
    }
    else if (histoWS)
    {
      this->doSaveHisto(histoWS);
    }
    else
      throw std::runtime_error("SaveMD can only save MDEventWorkspaces and MDHistoWorkspaces.\nPlease use SaveNexus or another algorithm appropriate for this workspace type.");
  }



} // namespace Mantid
} // namespace MDEvents

