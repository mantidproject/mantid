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
#include "MantidMDEvents/SaveMD.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidMDEvents/MDBox.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <Poco/File.h>
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
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
      MDE::closeNexusData(file);
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
    if (!update)
      file->makeGroup("MDEventWorkspace", "NXentry", 0);
    file->openGroup("MDEventWorkspace", "NXentry");

    // General information
    if (!update)
    {
      // Write out some general information like # of dimensions
      file->writeData("dimensions", int32_t(nd));
      file->putAttr("event_type", MDE::getTypeName());

      // Save the algorithm history under "process"
      ws->getHistory().saveNexus(file);
    }

    // Save each NEW ExperimentInfo to a spot in the file
    std::map<std::string,std::string> entries;
    file->getEntries(entries);
    for (uint16_t i=0; i < ws->getNumExperimentInfo(); i++)
    {
      ExperimentInfo_sptr ei = ws->getExperimentInfo(i);
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


    // Start the event Data group
    if (!update)
      file->makeGroup("event_data", "NXdata");
    file->openGroup("event_data", "NXdata");
    file->putAttr("version", "1.0");

    // Prepare the data chunk storage.
    size_t chunkSize = 10000; // TODO: Determine a smart chunk size!
    if (!update)
    {
      MDE::prepareNexusData(file, chunkSize);
      // Initialize the file-backing
      if (MakeFileBacked)
        bc->setFile(file, filename, 0);
    }
    else
    {
      uint64_t totalNumEvents = MDE::openNexusData(file);
      // Set it back to the new file handle
      bc->setFile(file, filename, totalNumEvents);
    }

    size_t maxBoxes = bc->getMaxId();

    // Prepare the vectors we will fill with data.

    // Box type (0=None, 1=MDBox, 2=MDGridBox
    std::vector<int> box_type(maxBoxes, 0);
    // Recursion depth
    std::vector<int> depth(maxBoxes, -1);
    // Start/end indices into the list of events
    std::vector<uint64_t> box_event_index(maxBoxes*2, 0);
    // Min/Max extents in each dimension
    std::vector<double> extents(maxBoxes*nd*2, 0);
    // Inverse of the volume of the cell
    std::vector<double> inverse_volume(maxBoxes, 0);
    // Box cached signal/error squared
    std::vector<double> box_signal_errorsquared(maxBoxes*2, 0);
    // Start/end children IDs
    std::vector<int> box_children(maxBoxes*2, 0);

    // The slab start for events, start at 0
    uint64_t start = 0;

    // Get a starting iterator
    MDBoxIterator<MDE,nd> it(ws->getBox(), 1000, false);

    Progress * prog = new Progress(this, 0.05, 0.9, maxBoxes);

    MDBoxBase<MDE,nd> * box;
    while (true)
    {
      box = it.getBox();
      size_t id = box->getId();
      if (id < maxBoxes)
      {
        // The start/end children IDs
        size_t numChildren = box->getNumChildren();
        if (numChildren > 0)
        {
          // Make sure that all children are ordered. TODO: This might not be needed if the IDs are rigorously done
          size_t lastId = box->getChild(0)->getId();
          for (size_t i = 1; i < numChildren; i++)
          {
            if (box->getChild(i)->getId() != lastId+1)
              throw std::runtime_error("Non-sequential child ID encountered!");
            lastId = box->getChild(i)->getId();
          }

          box_children[id*2] = int(box->getChild(0)->getId());
          box_children[id*2+1] = int(box->getChild(numChildren-1)->getId());
          box_type[id] = 2;
        }
        else
          box_type[id] = 1;


        MDBox<MDE,nd> * mdbox = dynamic_cast<MDBox<MDE,nd> *>(box);
        if (mdbox)
        {
          if (update)
          {
            // File-backed: update where on the file it is
            // This will relocate and save the box if it has any events
            mdbox->save();
            // We've now forced it to go on disk
            mdbox->setOnDisk(true);
            // Save the index
            box_event_index[id*2] = mdbox->getFileIndexStart();
            box_event_index[id*2+1] = mdbox->getFileNumEvents();
          }
          else
          {
            // Save for the first time
            const std::vector<MDE> & events = mdbox->getConstEvents();
            uint64_t numEvents = uint64_t(events.size());
            if (numEvents > 0)
            {
              mdbox->setFileIndex(uint64_t(start), numEvents);
              // Just save but don't clear the events or anything
              mdbox->saveNexus(file);
              if (MakeFileBacked)
              {
                // Save, set that it is on disk and clear the actual events to free up memory
                mdbox->setOnDisk(true);
                mdbox->clearDataOnly();
              }
              // Save the index
              box_event_index[id*2] = start;
              box_event_index[id*2+1] = numEvents;
              // Move forward in the file.
              start += numEvents;
            }
            // Like releaseEvents(), but without saving to disk.
            mdbox->setDataBusy(false);
          }
        }

        // Various bits of data about the box
        depth[id] = int(box->getDepth());
        box_signal_errorsquared[id*2] = double(box->getSignal());
        box_signal_errorsquared[id*2+1] = double(box->getErrorSquared());
        inverse_volume[id] = box->getInverseVolume();
        for (size_t d=0; d<nd; d++)
        {
          size_t newIndex = id*(nd*2) + d*2;
          extents[newIndex] = box->getExtents(d).min;
          extents[newIndex+1] = box->getExtents(d).max;
        }

        // Move on to the next box
        prog->report("Saving Box");
        if (!it.next()) break;
      }
      else
      {
        // Some sort of problem
        g_log.warning() << "Unexpected box ID found (" << id << ") which is > than maxBoxes (" << maxBoxes << ")" << std::endl;
        break;
      }
    }

    // Done writing the event data.
    MDE::closeNexusData(file);

    // ------------------------- Save Free Blocks --------------------------------------------------
    // Get a vector of the free space blocks to save to the file
    std::vector<uint64_t> freeSpaceBlocks;
    bc->getDiskBuffer().getFreeSpaceVector(freeSpaceBlocks);
    if (freeSpaceBlocks.size() == 0)
      freeSpaceBlocks.resize(2, 0); // Needs a minimum size
    std::vector<int> free_dims(2,2); free_dims[0] = int(freeSpaceBlocks.size()/2);
    std::vector<int> free_chunk(2,2); free_chunk[0] = 1000;

    // Now the free space blocks under event_data
    if (!update)
      file->writeExtendibleData("free_space_blocks", freeSpaceBlocks, free_dims, free_chunk);
    else
      file->writeUpdatedData("free_space_blocks", freeSpaceBlocks, free_dims);
    file->closeGroup();


    // -------------- Save Box Structure  -------------------------------------
    // OK, we've filled these big arrays of data. Save them.
    progress(0.91, "Writing Box Data");
    prog->resetNumSteps(8, 0.92, 1.00);

    // Start the box data group
    if (!update)
      file->makeGroup("box_structure", "NXdata");
    file->openGroup("box_structure", "NXdata");
    file->putAttr("version", "1.0");

    // Add box controller info to this group
    file->putAttr("box_controller_xml", bc->toXMLString());

    std::vector<int> exents_dims(2,0);
    exents_dims[0] = (int(maxBoxes));
    exents_dims[1] = (nd*2);
    std::vector<int> exents_chunk(2,0);
    exents_chunk[0] = int(16384);
    exents_chunk[1] = (nd*2);

    std::vector<int> box_2_dims(2,0);
    box_2_dims[0] = int(maxBoxes);
    box_2_dims[1] = (2);
    std::vector<int> box_2_chunk(2,0);
    box_2_chunk[0] = int(16384);
    box_2_chunk[1] = (2);

    if (!update)
    {
      // Write it for the first time
      file->writeExtendibleData("box_type", box_type);
      file->writeExtendibleData("depth", depth);
      file->writeExtendibleData("inverse_volume", inverse_volume);
      file->writeExtendibleData("extents", extents, exents_dims, exents_chunk);
      file->writeExtendibleData("box_children", box_children, box_2_dims, box_2_chunk);
      file->writeExtendibleData("box_signal_errorsquared", box_signal_errorsquared, box_2_dims, box_2_chunk);
      file->writeExtendibleData("box_event_index", box_event_index, box_2_dims, box_2_chunk);
    }
    else
    {
      // Update the extendible data sets
      file->writeUpdatedData("box_type", box_type);
      file->writeUpdatedData("depth", depth);
      file->writeUpdatedData("inverse_volume", inverse_volume);
      file->writeUpdatedData("extents", extents, exents_dims);
      file->writeUpdatedData("box_children", box_children, box_2_dims);
      file->writeUpdatedData("box_signal_errorsquared", box_signal_errorsquared, box_2_dims);
      file->writeUpdatedData("box_event_index", box_event_index, box_2_dims);
    }

    // Finished - close the file. This ensures everything gets written out even when updating.
    file->close();

    if (update || MakeFileBacked)
    {
      // Need to keep the file open since it is still used as a back end.
      // Reopen the file
      filename = bc->getFilename();
      file = new ::NeXus::File(filename, NXACC_RDWR);
      // Re-open the data for events.
      file->openGroup("MDEventWorkspace", "NXentry");
      file->openGroup("event_data", "NXdata");
      uint64_t totalNumEvents = MDE::openNexusData(file);
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

