/*WIKI* 

This algorithm loads a [[MDEventWorkspace]] that was previously
saved using the [[SaveMD]] algorithm to a .nxs file format.

If the workspace is too large to fit into memory,
You can load the workspace as a [[MDWorkspace#File-Backed MDWorkspaces|file-backed MDWorkspace]]
by checking the FileBackEnd option. This will load the box structure
(allowing for some visualization with no speed penalty) but leave the
events on disk until requested. Processing file-backed MDWorkspaces
is signficantly slower than in-memory workspaces due to frequeny file access!

For file-backed workspaces, the Memory option allows you to specify a cache
size, in MB, to keep events in memory before caching to disk.

Finally, the BoxStructureOnly and MetadataOnly options are for special situations
and used by other algorithms, they should not be needed in daily use.

*WIKI*/

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/LoadMD.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidNexusCPP/NeXusException.hpp"
#include <boost/algorithm/string.hpp>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
  namespace MDEvents
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(LoadMD);
    DECLARE_LOADALGORITHM(LoadMD);


    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    LoadMD::LoadMD()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    LoadMD::~LoadMD()
    {
    }


    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void LoadMD::initDocs()
    {
      this->setWikiSummary("Load a MDEventWorkspace in .nxs format.");
      this->setOptionalMessage("Load a MDEventWorkspace in .nxs format.");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
    */
    void LoadMD::init()
    {

      std::vector<std::string> exts;
      exts.push_back(".nxs");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the Nexus file to load, as a full or relative path");

      declareProperty(new Kernel::PropertyWithValue<bool>("MetadataOnly", false),
        "Load Metadata without events.");

      declareProperty(new Kernel::PropertyWithValue<bool>("BoxStructureOnly", false),
        "Load the structure of boxes but do not fill them with events. The loaded workspace will be empty and not file-backed.");

      declareProperty(new PropertyWithValue<bool>("FileBackEnd", false),
        "Set to true to load the data only on demand.");
      setPropertySettings("FileBackEnd", new EnabledWhenProperty("MetadataOnly", IS_EQUAL_TO, "0") );

      declareProperty(new PropertyWithValue<double>("Memory", -1),
        "For FileBackEnd only: the amount of memory (in MB) to allocate to the in-memory cache.\n"
        "If not specified, a default of 40% of free physical memory is used.");
      setPropertySettings("Memory", new EnabledWhenProperty("FileBackEnd", IS_EQUAL_TO, "1") );

      declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output MDEventWorkspace.");
    }



    //-------------------------------------------------------------------------------------------------
    /** Do a quick file type check by looking at the first 100 bytes of the file
     *  @param filePath :: path of the file including name.
     *  @param nread :: no.of bytes read
     *  @param header :: The first 100 bytes of the file as a union
     *  @return true if the given file is of type which can be loaded by this algorithm
     */
    bool LoadMD::quickFileCheck(const std::string& filePath,size_t nread, const file_header& header)
    {
      std::string ext = this->extension(filePath);
      // If the extension is nxs then give it a go
      if( ext.compare("nxs") == 0 ) return true;

      // If not then let's see if it is a HDF file by checking for the magic cookie
      if ( nread >= sizeof(int32_t) && (ntohl(header.four_bytes) == g_hdf_cookie) ) return true;
      return false;
    }

    //-------------------------------------------------------------------------------------------------
    /** Checks the file by opening it and reading few lines
     *  @param filePath :: name of the file inluding its path
     *  @return an integer value how much this algorithm can load the file
     */
    int LoadMD::fileCheck(const std::string& filePath)
    {
      int confidence(0);
      typedef std::map<std::string,std::string> string_map_t;
      try
      {
        string_map_t::const_iterator it;
        ::NeXus::File file = ::NeXus::File(filePath);
        string_map_t entries = file.getEntries();
        for(string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it)
        {
          if ( (it->first == "MDEventWorkspace") && (it->second == "NXentry") )
            confidence = 95;

        }
        file.close();
      }
      catch(::NeXus::Exception&)
      {
      }
      return confidence;
    }





    /** Load the ExperimentInfo blocks, if any, in the NXS file
     *
     * @param ws :: MDEventWorkspace to load
     */
    void LoadMD::loadExperimentInfos(IMDEventWorkspace_sptr ws)
    {
      // First, find how many experimentX blocks there are
      std::map<std::string,std::string> entries;
      file->getEntries(entries);
      std::map<std::string,std::string>::iterator it = entries.begin();
      std::vector<bool> hasExperimentBlock;
      uint16_t numExperimentInfo = 0;
      for (; it != entries.end(); ++it)
      {
        std::string name = it->first;
        if (boost::starts_with(name, "experiment"))
        {
          try
          {
            uint16_t num = boost::lexical_cast<uint16_t>(name.substr(10, name.size()-10));
            if (num+1 > numExperimentInfo)
            {
              numExperimentInfo = uint16_t(num+uint16_t(1));
              hasExperimentBlock.resize(numExperimentInfo, false);
              hasExperimentBlock[num] = true;
            }
          }
          catch (boost::bad_lexical_cast &)
          { /* ignore */ }
        }
      }

      // Now go through in order, loading and adding
      for (uint16_t i=0; i < numExperimentInfo; i++)
      {
        std::string groupName = "experiment" + Strings::toString(i);
        if (!numExperimentInfo)
        {
          g_log.warning() << "NXS file is missing a ExperimentInfo block " << groupName << ". Workspace will be missing ExperimentInfo." << std::endl;
          break;
        }
        file->openGroup(groupName, "NXgroup");
        ExperimentInfo_sptr ei(new ExperimentInfo);
        std::string parameterStr;
        try
        {
          // Get the sample, logs, instrument
          ei->loadExperimentInfoNexus(file, parameterStr);
          // Now do the parameter map
          ei->readParameterMap(parameterStr);
          // And set it in the workspace.
          ws->addExperimentInfo(ei);
        }
        catch (std::exception & e)
        {
          g_log.information("Error loading section '" + groupName + "' of nxs file.");
          g_log.information(e.what());
        }
        file->closeGroup();
      }

    }


    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void LoadMD::exec()
    {
      m_filename = getPropertyValue("Filename");

      // Start loading
      bool fileBacked = this->getProperty("FileBackEnd");
      if (fileBacked)
        file = new ::NeXus::File(m_filename, NXACC_RDWR);
      else
        file = new ::NeXus::File(m_filename, NXACC_READ);

      // The main entry
      file->openGroup("MDEventWorkspace", "NXentry");

      std::vector<int32_t> vecDims;
      file->readData("dimensions", vecDims);
      if (vecDims.empty())
        throw std::runtime_error("LoadMD:: Error loading number of dimensions.");
      size_t numDims = vecDims[0];
      if (numDims <= 0)
        throw std::runtime_error("LoadMD:: number of dimensions <= 0.");

      //The type of event
      std::string eventType;
      file->getAttr("event_type", eventType);

      // Use the factory to make the workspace of the right type
      IMDEventWorkspace_sptr ws = MDEventFactory::CreateMDWorkspace(numDims, eventType);

      // Now the ExperimentInfo
      loadExperimentInfos(ws);

      // Wrapper to cast to MDEventWorkspace then call the function
      CALL_MDEVENT_FUNCTION(this->doLoad, ws);

      // Save to output
      setProperty("OutputWorkspace", ws);
    }


    //----------------------------------------------------------------------------------------------
    /** Do the loading.
    *
    * The file should be open at the entry level at this point.
    *
    * @param ws :: MDEventWorkspace of the given type
    */
    template<typename MDE, size_t nd>
    void LoadMD::doLoad(typename MDEventWorkspace<MDE, nd>::sptr ws)
    {
      // Are we using the file back end?
      bool FileBackEnd = getProperty("FileBackEnd");
      bool BoxStructureOnly = getProperty("BoxStructureOnly");

      if (FileBackEnd && BoxStructureOnly)
        throw std::invalid_argument("Both BoxStructureOnly and FileBackEnd were set to TRUE: this is not possible.");

      CPUTimer tim;
      Progress * prog = new Progress(this, 0.0, 1.0, 100);

      prog->report("Opening file.");

      std::string title;
      file->getAttr("title", title);
      ws->setTitle("title");

      // TODO: notes, sample, logs, instrument, process, run_start

      // The workspace-specific data
      // Load each dimension, as their XML representation
      for (size_t d=0; d<nd; d++)
      {
        std::ostringstream mess;
        mess << "dimension" << d;
        std::string dimXML;
        file->getAttr(mess.str(), dimXML);
        // Use the dimension factory to read the XML
        IMDDimensionFactory factory = IMDDimensionFactory::createDimensionFactory(dimXML);
        IMDDimension_sptr dim(factory.create());
        ws->addDimension(dim);
      }

      bool bMetadataOnly = getProperty("MetadataOnly");
      if(!bMetadataOnly)
      {
        g_log.debug() << tim << " to load the dimensions, etc." << std::endl;

        // ----------------------------------------- Box Structure ------------------------------
        file->openGroup("box_structure", "NXdata");

        // Load the box controller
        std::string bcXML;
        file->getAttr("box_controller_xml", bcXML);
        ws->getBoxController()->fromXMLString(bcXML);

        prog->report("Creating Vectors");

        // Box type (0=None, 1=MDBox, 2=MDGridBox
        std::vector<int> boxType;
        // Recursion depth
        std::vector<int> depth;
        // Start index/length into the list of events
        std::vector<uint64_t> box_event_index;
        // Min/Max extents in each dimension
        std::vector<double> extents;
        // Inverse of the volume of the cell
        std::vector<double> inverse_volume;
        // Box cached signal/error squared
        std::vector<double> box_signal_errorsquared;
        // Start/end children IDs
        std::vector<int> box_children;

        g_log.debug() << tim << " to initialize the box data vectors." << std::endl;
        prog->report("Reading Box Data");

        // Read all the data blocks
        file->readData("box_type", boxType);
        file->readData("depth", depth);
        file->readData("inverse_volume", inverse_volume);
        file->readData("extents", extents);
        file->readData("box_children", box_children);
        file->readData("box_signal_errorsquared", box_signal_errorsquared);
        file->readData("box_event_index", box_event_index);

        size_t numBoxes = boxType.size();
        if (numBoxes == 0) throw std::runtime_error("Zero boxes found. There must have been an error reading or writing the file.");

        g_log.debug() << tim << " to read all the box data vectors. There are " << numBoxes << " boxes." << std::endl;

        // Check all vector lengths match
        if (depth.size() != numBoxes) throw std::runtime_error("Incompatible size for data: depth.");
        if (inverse_volume.size() != numBoxes) throw std::runtime_error("Incompatible size for data: inverse_volume.");
        if (boxType.size() != numBoxes) throw std::runtime_error("Incompatible size for data: boxType.");
        if (extents.size() != numBoxes*nd*2) throw std::runtime_error("Incompatible size for data: extents.");
        if (box_children.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_children.");
        if (box_event_index.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_event_index.");
        if (box_signal_errorsquared.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_signal_errorsquared.");

        std::vector<IMDBox<MDE,nd> *> boxes(numBoxes, NULL);
        BoxController_sptr bc = ws->getBoxController();

        // ---------------------------------------- MEMORY FOR CACHE ------------------------------------
        // How much memory for the cache?
        if (FileBackEnd)
        {
          // TODO: Clean up, only a write buffer now
          double mb = getProperty("Memory");
          if (mb < 0)
          {
            // Use 40% of available memory.
            Kernel::MemoryStats stats;
            stats.update();
            mb = double(stats.availMem()) * 0.4 / 1024.0;
          }
          if (mb <= 0) mb = 0;

          // Express the cache memory in units of number of events.
          uint64_t cacheMemory = (uint64_t(mb) * 1024 * 1024) / sizeof(MDE);

          double writeBufferMB = mb;
          uint64_t writeBufferMemory = (uint64_t(writeBufferMB) * 1024 * 1024) / sizeof(MDE);

          // Set these values in the diskMRU
          bc->setCacheParameters(sizeof(MDE), writeBufferMemory);

          g_log.information() << "Setting a DiskBuffer cache size of " << mb << " MB, or " << cacheMemory << " events." << std::endl;
        }


        // ---------------------------------------- READ IN THE BOXES ------------------------------------
        prog->setNumSteps(numBoxes);

        // Get ready to read the slabs
        file->closeGroup();
        file->openGroup("event_data", "NXdata");
        uint64_t totalNumEvents = MDE::openNexusData(file);

#ifdef COORDT_IS_FLOAT
        if (FileBackEnd && file->getInfo().type == ::NeXus::FLOAT64)
        {
          g_log.warning() << "You have loaded, in file-backed mode, an older NXS file where event_data is in doubles." << std::endl;
          g_log.warning() << "It is recommended that you save to a new file under the new format, where event_data is in floats." << std::endl;
        }
#endif

        for (size_t i=0; i<numBoxes; i++)
        {
          prog->report();
          size_t box_type = boxType[i];
          if (box_type > 0)
          {
            IMDBox<MDE,nd> * ibox = NULL;

            // Extents of the box, as a vector
            std::vector<Mantid::Geometry::MDDimensionExtents> extentsVector(nd);
            for (size_t d=0; d<nd; d++)
            {
              extentsVector[d].min = static_cast<coord_t>(extents[i*nd*2 + d*2]);
              extentsVector[d].max = static_cast<coord_t>(extents[i*nd*2 + d*2 + 1]);
            }

            if (box_type == 1)
            {
              // --- Make a MDBox -----
              MDBox<MDE,nd> * box = new MDBox<MDE,nd>(bc, depth[i], extentsVector);
              ibox = box;

              if (!BoxStructureOnly)
              {
                // Load the events now
                uint64_t indexStart = box_event_index[i*2];
                uint64_t numEvents = box_event_index[i*2+1];
                // Save the index in the file in the box data
                box->setFileIndex(uint64_t(indexStart), uint64_t(numEvents));

                if (!FileBackEnd)
                {
                  // Load if NOT using the file as the back-end,
                  box->loadNexus(file);
                  box->setOnDisk(false);
                  box->setInMemory(true);
                }
                else
                {
                  // Box is on disk and NOT in memory
                  box->setOnDisk(true);
                  box->setInMemory(false);
                }
              }
              else
              {
                // Only the box structure is being loaded
                box->setOnDisk(false);
                box->setInMemory(true);
                box->setFileIndex(0,0);
              }
            }
            else if (box_type == 2)
            {
              // --- Make a MDGridBox -----
              ibox = new MDGridBox<MDE,nd>(bc, depth[i], extentsVector);
            }
            else
              continue;

            // Force correct ID
            ibox->setId(i);
            ibox->calcVolume();

            // Set the cached values
            ibox->setSignal(box_signal_errorsquared[i*2]);
            ibox->setErrorSquared(box_signal_errorsquared[i*2+1]);

            // Save the box at its index in the vector.
            boxes[i] = ibox;
          }
        }


        if (FileBackEnd)
        {
          // Leave the file open in the box controller
          bc->setFile(file, m_filename, totalNumEvents);
        }
        else
        {
          // Done reading in all the events.
          MDE::closeNexusData(file);
          file->closeGroup();
          file->close();
          // Make sure no back-end is used
          bc->setFile(NULL, "", 0);
        }

        g_log.debug() << tim << " to create all the boxes and fill them with events." << std::endl;

        // Go again, giving the children to the parents
        for (size_t i=0; i<numBoxes; i++)
        {
          if (boxType[i] == 2)
          {
            size_t indexStart = box_children[i*2];
            size_t indexEnd = box_children[i*2+1] + 1;
            boxes[i]->setChildren( boxes, indexStart, indexEnd);
          }
        }

        g_log.debug() << tim << " to give all the children to the boxes." << std::endl;

        // Box of ID 0 is the head box.
        ws->setBox( boxes[0] );
        // Make sure the max ID is ok for later ID generation
        bc->setMaxId(numBoxes);

      } //end-of bMetaDataOnly
      // Refresh cache
      ws->refreshCache();
      g_log.debug() << tim << " to refreshCache(). " << ws->getNPoints() << " points after refresh." << std::endl;

      g_log.debug() << tim << " to finish up." << std::endl;
      delete prog;
    }


  } // namespace Mantid
} // namespace MDEvents

