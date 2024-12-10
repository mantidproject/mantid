// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/LoadMD.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/MDUnitFactory.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidMDAlgorithms/SetMDFrame.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <nexus/NeXusException.hpp>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

using file_holder_type = std::unique_ptr<Mantid::DataObjects::BoxControllerNeXusIO>;

namespace Mantid::MDAlgorithms {

DECLARE_NEXUS_HDF5_FILELOADER_ALGORITHM(LoadMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadMD::LoadMD()
    : m_numDims(0),                                          // uninitialized incorrect value
      m_coordSystem(None), m_BoxStructureAndMethadata(true), // this is faster but rarely needed.
      m_saveMDVersion(false), m_requiresMDFrameCorrection(false) {}

/**
 * Return the confidence with which this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMD::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {
  int confidence = 0;
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();
  if (allEntries.count("NXentry") == 1) {
    if (descriptor.isEntry("/MDEventWorkspace") || descriptor.isEntry("/MDHistoWorkspace")) {
      confidence = 95;
    }
  }

  return confidence;
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadMD::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "The name of the Nexus file to load, as a full or relative path");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("MetadataOnly", false),
                  "Load Box structure and other metadata without events. The "
                  "loaded workspace will be empty and not file-backed.");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("BoxStructureOnly", false),
                  "Load partial information about the boxes and events. Redundant property "
                  "currently equivalent to MetadataOnly");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("FileBackEnd", false),
                  "Set to true to load the data only on demand.");
  setPropertySettings("FileBackEnd", std::make_unique<EnabledWhenProperty>("MetadataOnly", IS_EQUAL_TO, "0"));

  declareProperty(std::make_unique<PropertyWithValue<double>>("Memory", -1),
                  "For FileBackEnd only: the amount of memory (in MB) to allocate to the "
                  "in-memory cache.\n"
                  "If not specified, a default of 40% of free physical memory is used.");
  setPropertySettings("Memory", std::make_unique<EnabledWhenProperty>("FileBackEnd", IS_EQUAL_TO, "1"));

  declareProperty("LoadHistory", true, "If true, the workspace history will be loaded");

  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output MDEventWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadMD::execLoader() {
  m_filename = getPropertyValue("Filename");
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
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
    throw Kernel::Exception::FileError("Can not open file " + for_access, m_filename);

  // The main entry
  const std::shared_ptr<Mantid::Kernel::NexusHDF5Descriptor> fileInfo = getFileInfo();

  std::string entryName;
  if (fileInfo->isEntry("/MDEventWorkspace", "NXentry")) {
    entryName = "MDEventWorkspace";
  } else if (fileInfo->isEntry("/MDHistoWorkspace", "NXentry")) {
    entryName = "MDHistoWorkspace";
  } else {
    throw std::runtime_error("Unexpected NXentry name. Expected "
                             "'MDEventWorkspace' or 'MDHistoWorkspace'.");
  }

  // Open the entry
  m_file->openGroup(entryName, "NXentry");

  // Check is SaveMD version 2 was used
  m_saveMDVersion = 0;
  if (m_file->hasAttr("SaveMDVersion"))
    m_file->getAttr("SaveMDVersion", m_saveMDVersion);

  if (m_saveMDVersion == 2)
    this->loadDimensions2();
  else {
    // How many dimensions?
    std::vector<int32_t> vecDims;
    m_file->readData("dimensions", vecDims);
    if (vecDims.empty())
      throw std::runtime_error("LoadMD:: Error loading number of dimensions.");
    m_numDims = vecDims[0];
    if (m_numDims == 0)
      throw std::runtime_error("LoadMD:: number of dimensions == 0.");

    // Now load all the dimension xml
    this->loadDimensions();
  }

  // Coordinate system
  this->loadCoordinateSystem();

  // QConvention (Inelastic or Crystallography)
  this->loadQConvention();

  // Display normalization settting
  if (fileInfo->isEntry("/" + entryName + "/" + VISUAL_NORMALIZATION_KEY)) {
    this->loadVisualNormalization(VISUAL_NORMALIZATION_KEY, m_visualNormalization);
  }

  if (entryName == "MDEventWorkspace") {
    // The type of event
    std::string eventType;
    m_file->getAttr("event_type", eventType);

    if (fileInfo->isEntry("/" + entryName + "/" + VISUAL_NORMALIZATION_KEY_HISTO)) {
      this->loadVisualNormalization(VISUAL_NORMALIZATION_KEY_HISTO, m_visualNormalizationHisto);
    }

    // Use the factory to make the workspace of the right type
    IMDEventWorkspace_sptr ws;
    if (m_visualNormalizationHisto && m_visualNormalization) {
      ws = MDEventFactory::CreateMDWorkspace(m_numDims, eventType, m_visualNormalization.value(),
                                             m_visualNormalizationHisto.value());
    } else {
      ws = MDEventFactory::CreateMDWorkspace(m_numDims, eventType);
    }

    // Now the ExperimentInfo
    auto prog = std::make_unique<Progress>(this, 0.0, 0.1, 1);
    prog->report("Load experiment information.");
    bool lazyLoadExpt = fileBacked;
    MDBoxFlatTree::loadExperimentInfos(m_file.get(), m_filename, ws, *fileInfo.get(), "MDEventWorkspace", lazyLoadExpt);

    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->doLoad, ws);

    // Check if a MDFrame adjustment is required
    checkForRequiredLegacyFixup(ws);
    if (m_requiresMDFrameCorrection) {
      setMDFrameOnWorkspaceFromLegacyFile(ws);
    }
    // Write out the Qconvention
    // ki-kf for Inelastic convention; kf-ki for Crystallography convention
    std::string pref_QConvention = Kernel::ConfigService::Instance().getString("Q.convention");
    g_log.information() << "Convention for Q in Preferences is " << pref_QConvention
                        << "; Convention of Q in NeXus file is " << m_QConvention << '\n';

    if (pref_QConvention != m_QConvention) {
      std::vector<double> scaling(m_numDims);
      scaling = qDimensions(ws);
      g_log.information() << "Transforming Q\n";
      Algorithm_sptr transform_alg = createChildAlgorithm("TransformMD");
      transform_alg->setProperty("InputWorkspace", std::dynamic_pointer_cast<IMDWorkspace>(ws));
      transform_alg->setProperty("Scaling", scaling);
      transform_alg->executeAsChildAlg();
      IMDWorkspace_sptr tmp = transform_alg->getProperty("OutputWorkspace");
      ws = std::dynamic_pointer_cast<IMDEventWorkspace>(tmp);
    }
    // Save to output
    setProperty("OutputWorkspace", std::dynamic_pointer_cast<IMDWorkspace>(ws));
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
void LoadMD::loadSlab(const std::string &name, void *data, const MDHistoWorkspace_sptr &ws,
                      ::NeXus::NXnumtype dataType) {
  m_file->openData(name);
  if (m_file->getInfo().type != dataType)
    throw std::runtime_error("Unexpected data type for '" + name + "' data set.'");

  int nPoints = 1;
  size_t numDims = m_file->getInfo().dims.size();
  std::vector<int> size(numDims);
  for (size_t d = 0; d < numDims; d++) {
    nPoints *= static_cast<int>(m_file->getInfo().dims[d]);
    size[d] = static_cast<int>(m_file->getInfo().dims[d]);
  }
  if (nPoints != static_cast<int>(ws->getNPoints()))
    throw std::runtime_error("Inconsistency between the number of points in '" + name +
                             "' and the number of bins defined by the dimensions.");
  std::vector<int> start(numDims, 0);
  try {
    m_file->getSlab(data, start, size);
  } catch (...) {
    g_log.debug() << " start: " << start[0] << " size: " << size[0] << '\n';
  }
  m_file->closeData();
}

//----------------------------------------------------------------------------------------------
/** Perform loading for a MDHistoWorkspace.
 * The entry should be open already.
 */
void LoadMD::loadHisto() {
  // Create the initial MDHisto.
  MDHistoWorkspace_sptr ws;
  // If display normalization has been provided. Use that.
  if (m_visualNormalization) {
    ws = std::make_shared<MDHistoWorkspace>(m_dims, m_visualNormalization.value());
  } else {
    ws = std::make_shared<MDHistoWorkspace>(m_dims); // Whatever MDHistoWorkspace defaults to.
  }

  // Now the ExperimentInfo
  auto prog = std::make_unique<Progress>(this, 0.0, 0.1, 1);
  prog->report("Load experiment information.");
  MDBoxFlatTree::loadExperimentInfos(m_file.get(), m_filename, ws);

  // Coordinate system
  ws->setCoordinateSystem(m_coordSystem);

  // Load the WorkspaceHistory "process"
  if (this->getProperty("LoadHistory")) {
    ws->history().loadNexus(m_file.get());
  }

  this->loadAffineMatricies(std::dynamic_pointer_cast<IMDWorkspace>(ws));

  if (m_saveMDVersion == 2)
    m_file->openGroup("data", "NXdata");
  // Load each data slab
  this->loadSlab("signal", ws->mutableSignalArray(), ws, ::NeXus::FLOAT64);
  this->loadSlab("errors_squared", ws->mutableErrorSquaredArray(), ws, ::NeXus::FLOAT64);
  this->loadSlab("num_events", ws->mutableNumEventsArray(), ws, ::NeXus::FLOAT64);
  this->loadSlab("mask", ws->mutableMaskArray(), ws, ::NeXus::INT8);

  m_file->close();

  // Check if a MDFrame adjustment is required
  checkForRequiredLegacyFixup(ws);
  if (m_requiresMDFrameCorrection) {
    setMDFrameOnWorkspaceFromLegacyFile(ws);
  }

  // Write out the Qconvention
  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string pref_QConvention = Kernel::ConfigService::Instance().getString("Q.convention");
  g_log.information() << "Convention for Q in Preferences is " << pref_QConvention
                      << "; Convention of Q in NeXus file is " << m_QConvention << '\n';

  if (pref_QConvention != m_QConvention) {
    std::vector<double> scaling(m_numDims);
    scaling = qDimensions(ws);
    g_log.information() << "Transforming Q\n";
    Algorithm_sptr transform_alg = createChildAlgorithm("TransformMD");
    transform_alg->setProperty("InputWorkspace", std::dynamic_pointer_cast<IMDWorkspace>(ws));
    transform_alg->setProperty("Scaling", scaling);
    transform_alg->executeAsChildAlg();
    IMDWorkspace_sptr tmp = transform_alg->getProperty("OutputWorkspace");
    ws = std::dynamic_pointer_cast<MDHistoWorkspace>(tmp);
  }

  // Save to output
  setProperty("OutputWorkspace", std::dynamic_pointer_cast<IMDWorkspace>(ws));
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
    m_dims.emplace_back(createDimension(dimXML));
  }
  // Since this is an old algorithm we will
  // have to provide an MDFrame correction
  m_requiresMDFrameCorrection = true;
}

//----------------------------------------------------------------------------------------------
/** Load all the dimensions into this->m_dims
 * The dimensions are stored as an nxData array */
void LoadMD::loadDimensions2() {
  using namespace Geometry;
  m_dims.clear();

  std::string axes;

  m_file->openGroup("data", "NXdata");
  m_file->openData("signal");
  m_file->getAttr("axes", axes);
  m_file->closeData();

  std::vector<std::string> splitAxes;
  boost::split(splitAxes, axes, boost::is_any_of(":"));
  // Create each dimension from axes data
  // We loop axes backwards because Mantid
  for (size_t d = splitAxes.size(); d > 0; d--) {
    std::string long_name;
    std::string units;
    std::string frame;
    std::vector<double> axis;
    m_file->openData(splitAxes[d - 1]);
    m_file->getAttr("long_name", long_name);
    m_file->getAttr("units", units);
    try {
      m_file->getAttr("frame", frame);
    } catch (std::exception &) {
      frame = Mantid::Geometry::UnknownFrame::UnknownFrameName;
      m_requiresMDFrameCorrection = true;
    }
    Geometry::MDFrame_const_uptr mdFrame = Geometry::makeMDFrameFactoryChain()->create(MDFrameArgument(frame, units));
    m_file->getData(axis);
    m_file->closeData();
    m_dims.emplace_back(std::make_shared<MDHistoDimension>(long_name, long_name, *mdFrame,
                                                           static_cast<coord_t>(axis.front()),
                                                           static_cast<coord_t>(axis.back()), axis.size() - 1));
  }
  m_file->closeGroup();
  m_numDims = m_dims.size();
}

void LoadMD::loadVisualNormalization(const std::string &key,
                                     std::optional<Mantid::API::MDNormalization> &normalization) {
  try {
    uint32_t readVisualNormalization(0);
    m_file->readData(key, readVisualNormalization);
    normalization = static_cast<Mantid::API::MDNormalization>(readVisualNormalization);
  } catch (::NeXus::Exception &) {

  } catch (std::exception &) {
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

/** Load the convention for Q  **/
void LoadMD::loadQConvention() {
  try {
    m_file->getAttr("QConvention", m_QConvention);
  } catch (std::exception &) {
    m_QConvention = "Inelastic";
  }
}

//----------------------------------------------------------------------------------------------
/** Do the loading.
 *
 * The m_file should be open at the entry level at this point.
 *
 * @param             ws :: MDEventWorkspace of the given type
 */
template <typename MDE, size_t nd> void LoadMD::doLoad(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  // Are we using the file back end?
  bool fileBackEnd = getProperty("FileBackEnd");

  if (fileBackEnd && m_BoxStructureAndMethadata)
    throw std::invalid_argument("Combination of BoxStructureOnly or "
                                "MetaDataOnly were set to TRUE with "
                                "fileBackEnd "
                                ": this is not possible.");

  CPUTimer tim;
  auto prog = std::make_unique<Progress>(this, 0.0, 1.0, 100);

  prog->report("Opening file.");
  std::string title;
  try {
    m_file->getAttr("title", title);
  } catch (std::exception &) {
    // Leave the title blank if error on loading
  }
  ws->setTitle(title);

  // Load the WorkspaceHistory "process"
  if (this->getProperty("LoadHistory")) {
    ws->history().loadNexus(m_file.get());
  }

  this->loadAffineMatricies(std::dynamic_pointer_cast<IMDWorkspace>(ws));

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
  auto nDims = static_cast<int>(nd); // should be safe
  FlatBoxTree.loadBoxStructure(m_filename, nDims, MDE::getTypeName());

  BoxController_sptr bc = ws->getBoxController();
  bc->fromXMLString(FlatBoxTree.getBCXMLdescr());

  prog->report("Restoring box structure and connectivity");
  std::vector<API::IMDNode *> boxTree;
  FlatBoxTree.restoreBoxTree(boxTree, bc, fileBackEnd, m_BoxStructureAndMethadata);
  size_t numBoxes = boxTree.size();

  // ---------------------------------------- DEAL WITH BOXES
  // ------------------------------------
  if (fileBackEnd) { // TODO:: call to the file format factory
    auto loader = std::shared_ptr<DataObjects::BoxControllerNeXusIO>(new DataObjects::BoxControllerNeXusIO(bc.get()));
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
        mb = double(10 * loader->getDataChunk() * sizeof(MDE)) / double(1024 * 1024);

      // Express the cache memory in units of number of events.
      uint64_t cacheMemory = static_cast<uint64_t>((mb * 1024. * 1024.) / sizeof(MDE)) + 1;

      // Set these values in the diskMRU
      bc->getFileIO()->setWriteBufferSize(cacheMemory);

      g_log.information() << "Setting a DiskBuffer cache size of " << mb << " MB, or " << cacheMemory << " events.\n";
    }
  } // Not file back end
  else if (!m_BoxStructureAndMethadata) {
    // ---------------------------------------- READ IN THE BOXES
    // ------------------------------------
    // TODO:: call to the file format factory
    auto loader = file_holder_type(new DataObjects::BoxControllerNeXusIO(bc.get()));
    loader->setDataType(sizeof(coord_t), MDE::getTypeName());
    loader->openFile(m_filename, "r");

    const std::vector<uint64_t> &BoxEventIndex = FlatBoxTree.getEventIndex();
    prog->setNumSteps(numBoxes);
    std::vector<coord_t> boxTemp;

    for (size_t i = 0; i < numBoxes; i++) {
      prog->report();
      auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxTree[i]);
      if (!box)
        continue;

      if (BoxEventIndex[2 * i + 1] > 0) // Load in memory NOT using the file as the back-end,
      {
        boxTree[i]->reserveMemoryForLoad(BoxEventIndex[2 * i + 1]);
        boxTree[i]->loadAndAddFrom(loader.get(), BoxEventIndex[2 * i], static_cast<size_t>(BoxEventIndex[2 * i + 1]),
                                   boxTemp);
      }
    }
    loader->closeFile();
  } else // box structure and metadata only
  {
  }
  g_log.debug() << tim << " to create all the boxes and fill them with events.\n";

  // Box of ID 0 is the head box.
  ws->setBox(boxTree[0]);
  // Make sure the max ID is ok for later ID generation
  bc->setMaxId(numBoxes);

  // end-of bMetaDataOnly
  // Refresh cache
  // TODO:if(!fileBackEnd)ws->refreshCache();
  ws->refreshCache();
  g_log.debug() << tim << " to refreshCache(). " << ws->getNPoints() << " points after refresh.\n";

  g_log.debug() << tim << " to finish up.\n";
}

/**
 * Load all of the affine matrices from the file, create the
 * appropriate coordinate transform and set those on the workspace.
 * @param ws : workspace to set the coordinate transforms on
 */
void LoadMD::loadAffineMatricies(const IMDWorkspace_sptr &ws) {
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
CoordTransform *LoadMD::loadAffineMatrix(const std::string &entry_name) {
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
  Matrix<coord_t> mat(vec, outD, inD);
  // Adjust dimensions
  inD--;
  outD--;
  CoordTransform *transform = nullptr;
  if (("CoordTransformAffine" == type) || ("CoordTransformAligned" == type)) {
    auto affine = new CoordTransformAffine(inD, outD);
    affine->setMatrix(mat);
    transform = affine;
  } else {
    g_log.information("Do not know how to process coordinate transform " + type);
  }
  return transform;
}

/**
 * Set MDFrames for workspaces from legacy files
 * @param ws:: poitner to the workspace which needs to be corrected
 */
void LoadMD::setMDFrameOnWorkspaceFromLegacyFile(const API::IMDWorkspace_sptr &ws) {

  g_log.information() << "LoadMD: Encountered a legacy file which has a mismatch between "
                         "its MDFrames and its Special Coordinate System. "
                         "Attempting to convert MDFrames.\n";
  auto numberOfDimensions = ws->getNumDims();

  // Select an MDFrame based on the special coordinates.
  // Note that for None, we select a General Coordinate System,
  // unless the name is "Unknown frame"
  std::string selectedFrame;

  switch (m_coordSystem) {
  case Mantid::Kernel::QLab:
    selectedFrame = Mantid::Geometry::QLab::QLabName;
    break;
  case Mantid::Kernel::QSample:
    selectedFrame = Mantid::Geometry::QSample::QSampleName;
    break;
  case Mantid::Kernel::HKL:
    selectedFrame = Mantid::Geometry::HKL::HKLName;
    break;
  default:
    selectedFrame = Mantid::Geometry::GeneralFrame::GeneralFrameName;
  }

  // Get the old frames just in case something goes wrong. In this case we
  // reset the frames.

  std::vector<std::string> oldFrames(numberOfDimensions, Mantid::Geometry::GeneralFrame::GeneralFrameName);
  for (size_t index = 0; index < numberOfDimensions; ++index) {
    oldFrames[index] = ws->getDimension(index)->getMDFrame().name();
  }

  // We want to set only up to the first three dimensions to the selected Frame;
  // Everything else will be set to a General Frame
  std::vector<std::string> framesToSet(numberOfDimensions, Mantid::Geometry::GeneralFrame::GeneralFrameName);
  auto fillUpTo = numberOfDimensions > 3 ? 3 : numberOfDimensions;
  std::fill_n(framesToSet.begin(), fillUpTo, selectedFrame);

  try {
    // Set the MDFrames for each axes
    Algorithm_sptr setMDFrameAlg = this->createChildAlgorithm("SetMDFrame");
    int axesCounter = 0;
    for (auto &frame : framesToSet) {
      setMDFrameAlg->setProperty("InputWorkspace", ws);
      setMDFrameAlg->setProperty("MDFrame", frame);
      setMDFrameAlg->setProperty("Axes", std::vector<int>(1, axesCounter));
      ++axesCounter;
      setMDFrameAlg->executeAsChildAlg();
    }
  } catch (...) {
    g_log.warning() << "LoadMD: An issue occured while trying to correct "
                       "MDFrames. Trying to revert to original.\n";
    // Revert to the old frames.
    Algorithm_sptr setMDFrameAlg = this->createChildAlgorithm("SetMDFrame");
    int axesCounter = 0;
    for (auto &oldFrame : oldFrames) {
      setMDFrameAlg->setProperty("InputWorkspace", ws);
      setMDFrameAlg->setProperty("MDFrame", oldFrame);
      setMDFrameAlg->setProperty("Axes", std::vector<int>(1, axesCounter));
      ++axesCounter;
      setMDFrameAlg->executeAsChildAlg();
    }
  }
}

/**
 * Check for required legacy fix up for certain file types. Namely the case
 * where
 * all MDFrames were stored as MDFrames
 */
void LoadMD::checkForRequiredLegacyFixup(const API::IMDWorkspace_sptr &ws) {
  // Check if the special coordinate is not none
  auto isQBasedSpecialCoordinateSystem = true;
  if (m_coordSystem == Mantid::Kernel::SpecialCoordinateSystem::None) {
    isQBasedSpecialCoordinateSystem = false;
  }

  // Check if all MDFrames are of type Unknown frame
  auto containsOnlyUnkownFrames = true;
  for (size_t index = 0; index < ws->getNumDims(); ++index) {
    if (ws->getDimension(index)->getMDFrame().name() != Mantid::Geometry::UnknownFrame::UnknownFrameName) {
      containsOnlyUnkownFrames = false;
      break;
    }
  }

  // Check if a fix up is required
  if (isQBasedSpecialCoordinateSystem && containsOnlyUnkownFrames) {
    m_requiresMDFrameCorrection = true;
  }
}

/**
 * Find scaling for Q dimensions
 */
std::vector<double> LoadMD::qDimensions(const API::IMDWorkspace_sptr &ws) {
  std::vector<double> scaling(m_numDims);
  for (size_t d = 0; d < m_numDims; d++) {
    std::string dimd = ws->getDimension(d)->getName();

    // Assume the Q dimensions are those that have names starting with [
    // such as [H,0.5H,0], or Q_ such as Q_sample_x.
    // The change in sign should apply only to those.
    boost::regex re("\\[.*|Q_");
    if (boost::regex_search(dimd.begin(), dimd.begin() + 2, re))
      scaling[d] = -1.0;
    else
      scaling[d] = 1.0;
  }
  return scaling;
}
const std::string LoadMD::VISUAL_NORMALIZATION_KEY = "visual_normalization";
const std::string LoadMD::VISUAL_NORMALIZATION_KEY_HISTO = "visual_normalization_histo";

} // namespace Mantid::MDAlgorithms
