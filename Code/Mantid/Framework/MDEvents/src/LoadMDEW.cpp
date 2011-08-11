#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/LoadMDEW.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <vector>
#include "MantidKernel/Memory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadMDEW)
  

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadMDEW::LoadMDEW()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadMDEW::~LoadMDEW()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadMDEW::initDocs()
  {
    this->setWikiSummary("Load a .nxs file into a MDEventWorkspace.");
    this->setOptionalMessage("Load a .nxs file into a MDEventWorkspace.");
    this->setWikiDescription("Load a .nxs file into a MDEventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadMDEW::init()
  {

    std::vector<std::string> exts;
    exts.push_back(".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the Nexus file to load, as a full or relative path");

    declareProperty(new PropertyWithValue<bool>("FileBackEnd", false),
        "Set to true to load the data only on demand.");

    declareProperty(new PropertyWithValue<int>("Memory", -1),
        "For FileBackEnd only: the amount of memory (in MB) to allocate to the in-memory cache.\n"
        "If not specified, a default of 40% of free physical memory is used.");

    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output MDEventWorkspace.");
  }




  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadMDEW::exec()
  {
    m_filename = getPropertyValue("Filename");

    // Start loading
    file = new ::NeXus::File(m_filename);

    // The main entry
    file->openGroup("MDEventWorkspace", "NXentry");

    std::vector<int32_t> vecDims;
    file->readData("dimensions", vecDims);
    if (vecDims.empty())
      throw std::runtime_error("LoadMDEW:: Error loading number of dimensions.");
    size_t numDims = vecDims[0];
    if (numDims <= 0)
      throw std::runtime_error("LoadMDEW:: number of dimensions <= 0.");

    //The type of event
    std::string eventType;
    file->getAttr("event_type", eventType);

    // Use the factory to make the workspace of the right type
    IMDEventWorkspace_sptr ws = MDEventFactory::CreateMDEventWorkspace(numDims, eventType);

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
  void LoadMDEW::doLoad(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // Are we using the file back end?
    bool FileBackEnd = getProperty("FileBackEnd");

    CPUTimer tim;
    bool verbose=true;
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

    // Load the box controller
    std::string bcXML;
    file->getAttr("box_controller_xml", bcXML);
    ws->getBoxController()->fromXMLString(bcXML);

    if (verbose) std::cout << tim << " to load the dimensions, etc." << std::endl;

    file->openGroup("data", "NXdata");

    prog->report("Creating Vectors");

    // Box type (0=None, 1=MDBox, 2=MDGridBox
    std::vector<int> boxType;
    // Recursion depth
    std::vector<int> depth;
    // Start/end indices into the list of events
    std::vector<uint64_t> box_event_index;
    // Min/Max extents in each dimension
    std::vector<double> extents;
    // Inverse of the volume of the cell
    std::vector<double> inverse_volume;
    // Box cached signal/error squared
    std::vector<double> box_signal_errorsquared;
    // Start/end children IDs
    std::vector<int> box_children;

    if (verbose) std::cout << tim << " to initialize the box data vectors." << std::endl;
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

    if (verbose) std::cout << tim << " to read all the box data vectors. There are " << numBoxes << " boxes." << std::endl;

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
    uint64_t cacheMemory = 0;
    uint64_t minDiskCacheSize = 0;
    if (FileBackEnd)
    {
      int mb = getProperty("Memory");
      if (mb < 0)
      {
        // Use 40% of available memory.
        Kernel::MemoryStats stats;
        stats.update();
        mb = int(double(stats.availMem()) * 0.4 / 1024.0);
      }
      if (mb <= 0) mb = 0;
      // Express the cache memory in units of number of events.
      cacheMemory = (uint64_t(mb) * 1024 * 1024) / sizeof(MDE);
      g_log.information() << "Setting a memory cache size of " << mb << " MB, or " << cacheMemory << " events." << std::endl;

      // Find a minimum size to bother to cache to disk.
      // We say that no more than 25% of the overall cache memory should be used on tiny boxes.
      minDiskCacheSize = uint64_t(double(cacheMemory) * 0.25) / numBoxes;
      g_log.information() << "Boxes with fewer than " << minDiskCacheSize << " events will not be cached to disk." << std::endl;

      uint64_t writeBuffer = uint64_t(1000000 / sizeof(MDE));
      g_log.information() << "Write buffer set to 1 MB (" << writeBuffer << " events)." << std::endl;

      // Set these values in the diskMRU
      bc->setCacheParameters(cacheMemory, writeBuffer, sizeof(MDE));
    }


    // ---------------------------------------- READ IN THE BOXES ------------------------------------
    prog->setNumSteps(numBoxes);

    // Get ready to read the slabs
    uint64_t totalNumEvents = MDE::openNexusData(file);

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
          extentsVector[d].min = extents[i*nd*2 + d*2];
          extentsVector[d].max = extents[i*nd*2 + d*2 + 1];
        }

        if (box_type == 1)
        {
          // --- Make a MDBox -----
          MDBox<MDE,nd> * box = new MDBox<MDE,nd>(bc, depth[i], extentsVector);
          ibox = box;

          // Load the events now
          uint64_t indexStart = box_event_index[i*2];
          uint64_t numEvents = box_event_index[i*2+1];
          if (numEvents > 0)
          {
            //std::cout << "box " << i << " from " << indexStart << " to " << indexEnd << std::endl;
            box->setFileIndex(uint64_t(indexStart), uint64_t(numEvents));
            if (!FileBackEnd || (numEvents < minDiskCacheSize))
            {
              // Load if NOT using the file as the back-end,
              // or if the box is small enough to keep in memory always
              box->loadNexus(file);
              box->setOnDisk(false);
            }
            else
              box->setOnDisk(true);
          }
          else
          {
            // Nothing on disk, since there are no events.
            box->setFileIndex(0, 0);
            box->setOnDisk(false);
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

    if (verbose) std::cout << tim << " to create all the boxes and fill them with events." << std::endl;

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

    if (verbose) std::cout << tim << " to give all the children to the boxes." << std::endl;

    // Box of ID 0 is the head box.
    ws->setBox( boxes[0] );
    // Make sure the max ID is ok for later ID generation
    bc->setMaxId(numBoxes);

    // Refresh cache
    ws->refreshCache();
    if (verbose) std::cout << tim << " to refreshCache(). " << ws->getNPoints() << " points after refresh." << std::endl;

    if (verbose) std::cout << tim << " to finish up." << std::endl;
    delete prog;
  }


} // namespace Mantid
} // namespace MDEvents

