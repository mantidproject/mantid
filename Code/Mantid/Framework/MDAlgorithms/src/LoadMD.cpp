/*WIKI* 

This algorithm loads a [[MDEventWorkspace]] that was previously
saved using the [[SaveMD]] algorithm to a .nxs file format.

If the workspace is too large to fit into memory,
You can load the workspace as a [[MDWorkspace#File-Backed MDWorkspaces|file-backed MDWorkspace]]
by checking the FileBackEnd option. This will load the box structure
(allowing for some visualization with no speed penalty) but leave the
events on disk until requested. Processing file-backed MDWorkspaces
is significantly slower than in-memory workspaces due to frequency file access!

For file-backed workspaces, the Memory option allows you to specify a cache
size, in MB, to keep events in memory before caching to disk.

Finally, the BoxStructureOnly and MetadataOnly options are for special situations
and used by other algorithms, they should not be needed in daily use.

*WIKI*/

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/LoadMD.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDBoxFlatTree.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/BoxControllerNeXusIO.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include <nexus/NeXusException.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>

#if defined(__GLIBCXX__) && __GLIBCXX__ >= 20100121 // libstdc++-4.4.3
 typedef std::unique_ptr< Mantid::API::IBoxControllerIO>  file_holder_type;
#else
 typedef std::auto_ptr< Mantid::API::IBoxControllerIO>  file_holder_type;
#endif

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;

namespace Mantid
{
  namespace MDAlgorithms
  {

    DECLARE_HDF_FILELOADER_ALGORITHM(LoadMD);

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


    /**
     * Return the confidence with with this algorithm can load the file
     * @param descriptor A descriptor for the file
     * @returns An integer specifying the confidence level. 0 indicates it will not be used
     */
    int LoadMD::confidence(const Kernel::HDFDescriptor & descriptor) const
    {
      int confidence(0);
      const auto & rootPathNameType = descriptor.firstEntryNameType();
      if(rootPathNameType.second != "NXentry") return 0;
      if(descriptor.pathExists("/" + rootPathNameType.first + "MDEventWorkspace") ||
         descriptor.pathExists("/" + rootPathNameType.first + "MDHistoWorkspace"))
      {
        return 95;
      }
      else return 0;
      return confidence;
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

      declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output MDEventWorkspace.");
    }

    //----------------------------------------------------------------------------------------------
    /** Load the ExperimentInfo blocks, if any, in the NXS file
    *
    * @param ws :: MDEventWorkspace/MDHisto to load
    */
    void LoadMD::loadExperimentInfos(boost::shared_ptr<Mantid::API::MultipleExperimentInfos> ws)
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
      std::map<std::string, std::string> entries;
      file->getEntries(entries);

      std::string entryName;
      if (entries.find("MDEventWorkspace") != entries.end())
        entryName = "MDEventWorkspace";
      else if (entries.find("MDHistoWorkspace") != entries.end())
        entryName = "MDHistoWorkspace";
      else
        throw std::runtime_error("Unexpected NXentry name. Expected 'MDEventWorkspace' or 'MDHistoWorkspace'.");

      // Open the entry
      file->openGroup(entryName, "NXentry");

      // How many dimensions?
      std::vector<int32_t> vecDims;
      file->readData("dimensions", vecDims);
      if (vecDims.empty())
        throw std::runtime_error("LoadMD:: Error loading number of dimensions.");
      m_numDims = vecDims[0];
      if (m_numDims <= 0)
        throw std::runtime_error("LoadMD:: number of dimensions <= 0.");

      // Now load all the dimension xml
      this->loadDimensions();

      if (entryName == "MDEventWorkspace")
      {
        //The type of event
        std::string eventType;
        file->getAttr("event_type", eventType);

        // Use the factory to make the workspace of the right type
        IMDEventWorkspace_sptr ws = MDEventFactory::CreateMDWorkspace(m_numDims, eventType);

        // Now the ExperimentInfo
        MDBoxFlatTree::loadExperimentInfos(file,ws);

        // Wrapper to cast to MDEventWorkspace then call the function
        CALL_MDEVENT_FUNCTION(this->doLoad, ws);

        // Save to output
        setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDWorkspace>(ws));
      }
      else
      {
        // MDHistoWorkspace case.
        this->loadHisto();
      }

      delete file;

    }

    /**
     * Load a slab of double data into a bare array.
     * Checks that the size is correct.
     * @param name
     * @param data bare pointer to doublel array
     * @param ws
     * @param dataType
     */
    void LoadMD::loadSlab(std::string name, void * data, MDHistoWorkspace_sptr ws, NeXus::NXnumtype dataType)
    {
      file->openData(name);
      if (file->getInfo().type != dataType)
        throw std::runtime_error("Unexpected data type for '" + name + "' data set.'");
      if (file->getInfo().dims[0] != static_cast<int>(ws->getNPoints()))
        throw std::runtime_error("Inconsistency between the number of points in '" + name + "' and the number of bins defined by the dimensions.");
      std::vector<int> start(1,0);
      std::vector<int> size(1, static_cast<int>(ws->getNPoints()));
      file->getSlab(data, start, size);
      file->closeData();
    }

    //----------------------------------------------------------------------------------------------
    /** Perform loading for a MDHistoWorkspace.
    * The entry should be open already.
    */
    void LoadMD::loadHisto()
    {
      // Create the initial MDHisto.
      MDHistoWorkspace_sptr ws(new MDHistoWorkspace(m_dims));

      // Now the ExperimentInfo
      MDBoxFlatTree::loadExperimentInfos(file,ws);

      // Load the WorkspaceHistory "process"
      ws->history().loadNexus(file);

      this->loadAffineMatricies(boost::dynamic_pointer_cast<IMDWorkspace>(ws));

      // Load each data slab
      this->loadSlab("signal", ws->getSignalArray(), ws, ::NeXus::FLOAT64);
      this->loadSlab("errors_squared", ws->getErrorSquaredArray(), ws, ::NeXus::FLOAT64);
      this->loadSlab("num_events", ws->getNumEventsArray(), ws, ::NeXus::FLOAT64);
      this->loadSlab("mask", ws->getMaskArray(), ws, ::NeXus::INT8);

      file->close();

      // Save to output
      setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDWorkspace>(ws));
    }


    //----------------------------------------------------------------------------------------------
    /** Load all the dimensions into this->m_dims */
    void LoadMD::loadDimensions()
    {
      m_dims.clear();

      // Load each dimension, as their XML representation
      for (size_t d=0; d<m_numDims; d++)
      {
        std::ostringstream mess;
        mess << "dimension" << d;
        std::string dimXML;
        file->getAttr(mess.str(), dimXML);
        // Use the dimension factory to read the XML
        IMDDimensionFactory factory = IMDDimensionFactory::createDimensionFactory(dimXML);
        IMDDimension_sptr dim(factory.create());
        m_dims.push_back(dim);
      }

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

      // Load the WorkspaceHistory "process"
      ws->history().loadNexus(file);

      this->loadAffineMatricies(boost::dynamic_pointer_cast<IMDWorkspace>(ws));

      file->closeGroup();
      file->close();
      // Add each of the dimension
      for (size_t d=0; d<nd; d++)
        ws->addDimension(m_dims[d]);

      bool bMetadataOnly = getProperty("MetadataOnly");

      // ----------------------------------------- Box Structure ------------------------------
      MDBoxFlatTree FlatBoxTree;
      FlatBoxTree.loadBoxStructure(m_filename,nd,MDE::getTypeName());

      BoxController_sptr bc = ws->getBoxController();
      bc->fromXMLString(FlatBoxTree.getBCXMLdescr());

      std::vector<API::IMDNode *> boxTree;
   //   uint64_t totalNumEvents = FlatBoxTree.restoreBoxTree<MDE,nd>(boxTree,bc,FileBackEnd,bMetadataOnly);
      FlatBoxTree.restoreBoxTree(boxTree,bc,FileBackEnd,bMetadataOnly);
      size_t numBoxes = boxTree.size();

    // ---------------------------------------- DEAL WITH BOXES  ------------------------------------
      if (FileBackEnd)
      { // TODO:: call to the file format factory
          auto loader = boost::shared_ptr<API::IBoxControllerIO>(new MDEvents::BoxControllerNeXusIO(bc.get()));
          loader->setDataType(sizeof(coord_t),MDE::getTypeName());
          bc->setFileBacked(loader,m_filename);
          // boxes have been already made file-backed when restoring the boxTree;
      // How much memory for the cache?
        {
        // TODO: Clean up, only a write buffer now
          double mb = getProperty("Memory");
       
          // Defaults have changed, defauld disk buffer size should be 10 data chunks TODO: find optimal, 100 may be better. 
          if (mb <= 0) mb = double(10*loader->getDataChunk()* sizeof(MDE))/double(1024*1024);

          // Express the cache memory in units of number of events.
          uint64_t cacheMemory = static_cast<uint64_t>((mb * 1024. * 1024.) / sizeof(MDE))+1;
              
          // Set these values in the diskMRU
          bc->getFileIO()->setWriteBufferSize(cacheMemory);

          g_log.information() << "Setting a DiskBuffer cache size of " << mb << " MB, or " << cacheMemory << " events." << std::endl;
        }   
      } // Not file back end
      else
      {
        // ---------------------------------------- READ IN THE BOXES ------------------------------------
       // TODO:: call to the file format factory
        auto loader = file_holder_type(new MDEvents::BoxControllerNeXusIO(bc.get()));
        loader->setDataType(sizeof(coord_t),MDE::getTypeName());

        loader->openFile(m_filename,"r");

        const std::vector<uint64_t> &BoxEventIndex = FlatBoxTree.getEventIndex();
        prog->setNumSteps(numBoxes);

        for (size_t i=0; i<numBoxes; i++)
        {
          prog->report();
          MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxTree[i]);
          if(!box)continue;

          if(BoxEventIndex[2*i+1]>0) // Load in memory NOT using the file as the back-end,
              boxTree[i]->loadAndAddFrom(loader.get(),BoxEventIndex[2*i],static_cast<size_t>(BoxEventIndex[2*i+1]));

        }
        loader->closeFile();
      }

      g_log.debug() << tim << " to create all the boxes and fill them with events." << std::endl;


      // Box of ID 0 is the head box.
      ws->setBox(boxTree[0] );
      // Make sure the max ID is ok for later ID generation
      bc->setMaxId(numBoxes);

      //end-of bMetaDataOnly
      // Refresh cache
      //TODO:if(!FileBackEnd)ws->refreshCache();
      ws->refreshCache();
      g_log.debug() << tim << " to refreshCache(). " << ws->getNPoints() << " points after refresh." << std::endl;

      g_log.debug() << tim << " to finish up." << std::endl;
      delete prog;
    }

  /**
   * Load all of the affine matricies from the file, create the
   * appropriate coordinate transform and set those on the workspace.
   * @param ws : workspace to set the coordinate transforms on
   */
  void LoadMD::loadAffineMatricies(IMDWorkspace_sptr ws)
  {
    std::map<std::string, std::string> entries;
    file->getEntries(entries);

    if (entries.find("transform_to_orig") != entries.end())
    {
      CoordTransform *transform = this->loadAffineMatrix("transform_to_orig");
      ws->setTransformToOriginal(transform);
    }
    if (entries.find("transform_from_orig") != entries.end())
    {
      CoordTransform *transform = this->loadAffineMatrix("transform_from_orig");
      ws->setTransformFromOriginal(transform);
    }
  }

  /**
   * Do that actual loading and manipulating of the read data to create
   * the affine matrix and then the appropriate transformation. This is
   * currently limited to CoordTransformAffine transforms.
   * @param entry_name : the entry point in the NeXus file to read
   * @return the coordinate transform object
   */
  CoordTransform *LoadMD::loadAffineMatrix(std::string entry_name)
  {
    file->openData(entry_name);
    std::vector<coord_t> vec;
    file->getData<coord_t>(vec);
    std::string type;
    int inD(0);
    int outD(0);
    file->getAttr("type", type);
    file->getAttr<int>("rows", outD);
    file->getAttr<int>("columns", inD);
    file->closeData();
    // Adjust dimensions
    inD--;
    outD--;
    Matrix<coord_t> mat(vec);
    CoordTransform *transform = NULL;
    if ("CoordTransformAffine" == type)
    {
      CoordTransformAffine *affine = new CoordTransformAffine(inD, outD);
      affine->setMatrix(mat);
      transform = affine;
    }
    else
    {
      g_log.information("Do not know how to process coordinate transform " + type);
    }
    return transform;
  }

  } // namespace Mantid
} // namespace MDEvents

