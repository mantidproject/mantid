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
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include <nexus/NeXusException.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>

#if defined(__GLIBCXX__) && __GLIBCXX__ >= 20100121 // libstdc++-4.4.3
typedef std::unique_ptr<Mantid::API::IBoxControllerIO> file_holder_type;
#else
typedef std::auto_ptr<Mantid::API::IBoxControllerIO> file_holder_type;
#endif

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMD);

//----------------------------------------------------------------------------------------------
/** Constructor
*/
LoadMD::LoadMD()
    : m_numDims(0), // uninitialized incorrect value
      m_coordSystem(None),
      m_BoxStructureAndMethadata(true) // this is faster but rarely needed.
{}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
LoadMD::~LoadMD() {}

/**
* Return the confidence with which this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadMD::confidence(Kernel::NexusDescriptor &descriptor) const {
  int confidence(0);
  const auto &rootPathNameType = descriptor.firstEntryNameType();
  if (rootPathNameType.second != "NXentry")
    return 0;
  if (descriptor.pathExists("/MDEventWorkspace") ||
      descriptor.pathExists("/MDHistoWorkspace")) {
    return 95;
  } else
    return 0;
  return confidence;
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void LoadMD::init() {

  std::vector<std::string> exts;
  exts.push_back(".nxs");
  declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name of the Nexus file to load, as a full or relative path");

  declareProperty(new Kernel::PropertyWithValue<bool>("MetadataOnly", false),
                  "Load Box structure and other metadata without events. The "
                  "loaded workspace will be empty and not file-backed.");

  declareProperty(
      new Kernel::PropertyWithValue<bool>("BoxStructureOnly", false),
      "Load partial information about the boxes and events. Redundant property "
      "currently equivalent to  MetadataOnly");

  declareProperty(new PropertyWithValue<bool>("FileBackEnd", false),
                  "Set to true to load the data only on demand.");
  setPropertySettings(
      "FileBackEnd", new EnabledWhenProperty("MetadataOnly", IS_EQUAL_TO, "0"));

  declareProperty(
      new PropertyWithValue<double>("Memory", -1),
      "For FileBackEnd only: the amount of memory (in MB) to allocate to the "
      "in-memory cache.\n"
      "If not specified, a default of 40% of free physical memory is used.");
  setPropertySettings("Memory",
                      new EnabledWhenProperty("FileBackEnd", IS_EQUAL_TO, "1"));

  declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace", "",
                                                      Direction::Output),
                  "Name of the output MDEventWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void LoadMD::exec() {
  m_filename = getPropertyValue("Filename");

  // Start loading
  bool fileBacked = this->getProperty("FileBackEnd");

  m_BoxStructureAndMethadata = getProperty("MetadataOnly");

  bool BoxAndEventInfoOnly = this->getProperty("BoxStructureOnly");
  if (m_BoxStructureAndMethadata || BoxAndEventInfoOnly) {
    m_BoxStructureAndMethadata = true;
  }

  // Nexus constructor/destructor throw, so can not be used with scoped pointers
  // directly
  //(do they lock file because of this and this code is useless?)
  std::string for_access;
  if (fileBacked) {

    for_access = "for Read/Write access";
    m_file.reset(new ::NeXus::File(m_filename, NXACC_RDWR));
  } else {
    for_access = "for Read access";
    m_file.reset(new ::NeXus::File(m_filename, NXACC_READ));
  }

  if (!m_file)
    throw Kernel::Exception::FileError("Can not open file " + for_access,
                                       m_filename);

  // The main entry
  std::map<std::string, std::string> entries;
  m_file->getEntries(entries);

  std::string entryName;
  if (entries.find("MDEventWorkspace") != entries.end())
    entryName = "MDEventWorkspace";
  else if (entries.find("MDHistoWorkspace") != entries.end())
    entryName = "MDHistoWorkspace";
  else
    throw std::runtime_error("Unexpected NXentry name. Expected "
                             "'MDEventWorkspace' or 'MDHistoWorkspace'.");

  // Open the entry
  m_file->openGroup(entryName, "NXentry");

  // How many dimensions?
  std::vector<int32_t> vecDims;
  m_file->readData("dimensions", vecDims);
  if (vecDims.empty())
    throw std::runtime_error("LoadMD:: Error loading number of dimensions.");
  m_numDims = vecDims[0];
  if (m_numDims <= 0)
    throw std::runtime_error("LoadMD:: number of dimensions <= 0.");

  // Now load all the dimension xml
  this->loadDimensions();
  // Coordinate system
  this->loadCoordinateSystem();

  if (entryName == "MDEventWorkspace") {
    // The type of event
    std::string eventType;
    m_file->getAttr("event_type", eventType);

    // Use the factory to make the workspace of the right type
    IMDEventWorkspace_sptr ws =
        MDEventFactory::CreateMDWorkspace(m_numDims, eventType);

    // Now the ExperimentInfo
    bool lazyLoadExpt = fileBacked;
    MDBoxFlatTree::loadExperimentInfos(m_file.get(), m_filename, ws, lazyLoadExpt);

    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->doLoad, ws);

    // Save to output
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<IMDWorkspace>(ws));
  } else {
    // MDHistoWorkspace case.
    this->loadHisto();
  }
}

/**
* Load a slab of double data into a bare array.
* Checks that the size is correct.
* @param name
* @param data bare pointer to double array
* @param ws
* @param dataType
*/
void LoadMD::loadSlab(std::string name, void *data, MDHistoWorkspace_sptr ws,
                      NeXus::NXnumtype dataType) {
  m_file->openData(name);
  if (m_file->getInfo().type != dataType)
    throw std::runtime_error("Unexpected data type for '" + name +
                             "' data set.'");
  if (m_file->getInfo().dims[0] != static_cast<int>(ws->getNPoints()))
    throw std::runtime_error(
        "Inconsistency between the number of points in '" + name +
        "' and the number of bins defined by the dimensions.");
  std::vector<int> start(1, 0);
  std::vector<int> size(1, static_cast<int>(ws->getNPoints()));
  try {
    m_file->getSlab(data, start, size);
  } catch (...) {
    std::cout << " start: " << start[0] << " size: " << size[0] << std::endl;
  }
  m_file->closeData();
}

//----------------------------------------------------------------------------------------------
/** Perform loading for a MDHistoWorkspace.
* The entry should be open already.
*/
void LoadMD::loadHisto() {
  // Create the initial MDHisto.
  MDHistoWorkspace_sptr ws(new MDHistoWorkspace(m_dims));

  // Now the ExperimentInfo
  MDBoxFlatTree::loadExperimentInfos(m_file.get(), m_filename, ws);

  // Coordinate system
  ws->setCoordinateSystem(m_coordSystem);

  // Load the WorkspaceHistory "process"
  ws->history().loadNexus(m_file.get());

  this->loadAffineMatricies(boost::dynamic_pointer_cast<IMDWorkspace>(ws));

  // Load each data slab
  this->loadSlab("signal", ws->getSignalArray(), ws, ::NeXus::FLOAT64);
  this->loadSlab("errors_squared", ws->getErrorSquaredArray(), ws,
                 ::NeXus::FLOAT64);
  this->loadSlab("num_events", ws->getNumEventsArray(), ws, ::NeXus::FLOAT64);
  this->loadSlab("mask", ws->getMaskArray(), ws, ::NeXus::INT8);

  m_file->close();

  // Save to output
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDWorkspace>(ws));
}

//----------------------------------------------------------------------------------------------
/** Load all the dimensions into this->m_dims */
void LoadMD::loadDimensions() {
  m_dims.clear();

  // Load each dimension, as their XML representation
  for (size_t d = 0; d < m_numDims; d++) {
    std::ostringstream mess;
    mess << "dimension" << d;
    std::string dimXML;
    m_file->getAttr(mess.str(), dimXML);
    // Use the dimension factory to read the XML
    m_dims.push_back(createDimension(dimXML));
  }
}

/** Load the coordinate system **/
void LoadMD::loadCoordinateSystem() {
  // Current version stores the coordinate system
  // in its own field. The first version stored it
  // as a log value so fallback on that if it can't
  // be found.
  try {
    uint32_t readCoord(0);
    m_file->readData("coordinate_system", readCoord);
    m_coordSystem = static_cast<SpecialCoordinateSystem>(readCoord);
  } catch (::NeXus::Exception &) {
    auto pathOnEntry = m_file->getPath();
    try {
      m_file->openPath(pathOnEntry + "/experiment0/logs/CoordinateSystem");
      int readCoord(0);
      m_file->readData("value", readCoord);
      m_coordSystem = static_cast<SpecialCoordinateSystem>(readCoord);
    } catch (::NeXus::Exception &) {
    }
    // return to where we started
    m_file->openPath(pathOnEntry);
  }
}

//----------------------------------------------------------------------------------------------
/** Do the loading.
*
* The m_file should be open at the entry level at this point.
*
* @param             ws :: MDEventWorkspace of the given type
*/
template <typename MDE, size_t nd>
void LoadMD::doLoad(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  // Are we using the file back end?
  bool fileBackEnd = getProperty("FileBackEnd");

  if (fileBackEnd && m_BoxStructureAndMethadata)
    throw std::invalid_argument("Both BoxStructureOnly and fileBackEnd were "
                                "set to TRUE: this is not possible.");

  CPUTimer tim;
  Progress *prog = new Progress(this, 0.0, 1.0, 100);

  prog->report("Opening file.");
  std::string title;
  m_file->getAttr("title", title);
  ws->setTitle("title");

  // Load the WorkspaceHistory "process"
  ws->history().loadNexus(m_file.get());

  this->loadAffineMatricies(boost::dynamic_pointer_cast<IMDWorkspace>(ws));

  m_file->closeGroup();
  m_file->close();
  // Add each of the dimension
  for (size_t d = 0; d < nd; d++)
    ws->addDimension(m_dims[d]);

  // Coordinate system
  ws->setCoordinateSystem(m_coordSystem);

  // ----------------------------------------- Box Structure
  // ------------------------------
  prog->report("Reading box structure from HDD.");
  MDBoxFlatTree FlatBoxTree;
  int nDims = static_cast<int>(nd); // should be safe
  FlatBoxTree.loadBoxStructure(m_filename, nDims, MDE::getTypeName());

  BoxController_sptr bc = ws->getBoxController();
  bc->fromXMLString(FlatBoxTree.getBCXMLdescr());

  prog->report("Restoring box structure and connectivity");
  std::vector<API::IMDNode *> boxTree;
  FlatBoxTree.restoreBoxTree(boxTree, bc, fileBackEnd,
                             m_BoxStructureAndMethadata);
  size_t numBoxes = boxTree.size();

  // ---------------------------------------- DEAL WITH BOXES
  // ------------------------------------
  if (fileBackEnd) { // TODO:: call to the file format factory
    auto loader = boost::shared_ptr<API::IBoxControllerIO>(
        new DataObjects::BoxControllerNeXusIO(bc.get()));
    loader->setDataType(sizeof(coord_t), MDE::getTypeName());
    bc->setFileBacked(loader, m_filename);
    // boxes have been already made file-backed when restoring the boxTree;
    // How much memory for the cache?
    {
      // TODO: Clean up, only a write buffer now
      double mb = getProperty("Memory");

      // Defaults have changed, default disk buffer size should be 10 data
      // chunks TODO: find optimal, 100 may be better.
      if (mb <= 0)
        mb = double(10 * loader->getDataChunk() * sizeof(MDE)) /
             double(1024 * 1024);

      // Express the cache memory in units of number of events.
      uint64_t cacheMemory =
          static_cast<uint64_t>((mb * 1024. * 1024.) / sizeof(MDE)) + 1;

      // Set these values in the diskMRU
      bc->getFileIO()->setWriteBufferSize(cacheMemory);

      g_log.information() << "Setting a DiskBuffer cache size of " << mb
                          << " MB, or " << cacheMemory << " events."
                          << std::endl;
    }
  } // Not file back end
  else if (!m_BoxStructureAndMethadata) {
    // ---------------------------------------- READ IN THE BOXES
    // ------------------------------------
    // TODO:: call to the file format factory
    auto loader =
        file_holder_type(new DataObjects::BoxControllerNeXusIO(bc.get()));
    loader->setDataType(sizeof(coord_t), MDE::getTypeName());

    loader->openFile(m_filename, "r");

    const std::vector<uint64_t> &BoxEventIndex = FlatBoxTree.getEventIndex();
    prog->setNumSteps(numBoxes);

    for (size_t i = 0; i < numBoxes; i++) {
      prog->report();
      MDBox<MDE, nd> *box = dynamic_cast<MDBox<MDE, nd> *>(boxTree[i]);
      if (!box)
        continue;

      if (BoxEventIndex[2 * i + 1] >
          0) // Load in memory NOT using the file as the back-end,
      {
        boxTree[i]->reserveMemoryForLoad(BoxEventIndex[2 * i + 1]);
        boxTree[i]->loadAndAddFrom(
            loader.get(), BoxEventIndex[2 * i],
            static_cast<size_t>(BoxEventIndex[2 * i + 1]));
      }
    }
    loader->closeFile();
  } else // box structure and metadata only
  {
  }
  g_log.debug() << tim << " to create all the boxes and fill them with events."
                << std::endl;

  // Box of ID 0 is the head box.
  ws->setBox(boxTree[0]);
  // Make sure the max ID is ok for later ID generation
  bc->setMaxId(numBoxes);

  // end-of bMetaDataOnly
  // Refresh cache
  // TODO:if(!fileBackEnd)ws->refreshCache();
  ws->refreshCache();
  g_log.debug() << tim << " to refreshCache(). " << ws->getNPoints()
                << " points after refresh." << std::endl;

  g_log.debug() << tim << " to finish up." << std::endl;
  delete prog;
}

/**
* Load all of the affine matrices from the file, create the
* appropriate coordinate transform and set those on the workspace.
* @param ws : workspace to set the coordinate transforms on
*/
void LoadMD::loadAffineMatricies(IMDWorkspace_sptr ws) {
  std::map<std::string, std::string> entries;
  m_file->getEntries(entries);

  if (entries.find("transform_to_orig") != entries.end()) {
    CoordTransform *transform = this->loadAffineMatrix("transform_to_orig");
    ws->setTransformToOriginal(transform);
  }
  if (entries.find("transform_from_orig") != entries.end()) {
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
CoordTransform *LoadMD::loadAffineMatrix(std::string entry_name) {
  m_file->openData(entry_name);
  std::vector<coord_t> vec;
  m_file->getData<coord_t>(vec);
  std::string type;
  int inD(0);
  int outD(0);
  m_file->getAttr("type", type);
  m_file->getAttr<int>("rows", outD);
  m_file->getAttr<int>("columns", inD);
  m_file->closeData();
  // Adjust dimensions
  inD--;
  outD--;
  Matrix<coord_t> mat(vec);
  CoordTransform *transform = NULL;
  if (("CoordTransformAffine" == type)||("CoordTransformAligned" == type)) {
    CoordTransformAffine *affine = new CoordTransformAffine(inD, outD);
    affine->setMatrix(mat);
    transform = affine;
  } else {
    g_log.information("Do not know how to process coordinate transform " +
                      type);
  }
  return transform;
}

} // namespace Mantid
} // namespace DataObjects
