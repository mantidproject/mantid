#include "MantidKernel/Strings.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/FileBackedExperimentInfo.h"
#include "MantidDataObjects/MDEventFactory.h"
#include <Poco/File.h>

#if defined(__GLIBCXX__) && __GLIBCXX__ >= 20100121 // libstdc++-4.4.3
typedef std::unique_ptr< ::NeXus::File> file_holder_type;
#else
typedef std::auto_ptr< ::NeXus::File> file_holder_type;
#endif

namespace Mantid {
namespace DataObjects {
namespace {
/// static logger
Kernel::Logger g_log("MDBoxFlatTree");
}

MDBoxFlatTree::MDBoxFlatTree() : m_nDim(-1) {}

/**The method initiates the MDBoxFlatTree class internal structure in the form
 *ready for saving this structure to HDD
 *
 * @param pws  -- the shared pointer to the MD workspace which is the source of
 *the flat box structure
 * @param fileName -- the name of the file, where this structure should be
 *written. TODO: It is here for the case of file based workspaces
 */
void MDBoxFlatTree::initFlatStructure(API::IMDEventWorkspace_sptr pws,
                                      const std::string &fileName) {
  m_bcXMLDescr = pws->getBoxController()->toXMLString();
  m_FileName = fileName;

  m_nDim = int(pws->getNumDims());
  // flatten the box structure
  pws->getBoxes(m_Boxes, 1000, false);

  API::IMDNode::sortObjByID(m_Boxes);

  size_t maxBoxes = m_Boxes.size();
  // Box type (0=None, 1=MDBox, 2=MDGridBox
  m_BoxType.assign(maxBoxes, 0);
  // Recursion depth
  m_Depth.assign(maxBoxes, -1);
  // Start/end indices into the list of events
  m_BoxEventIndex.assign(maxBoxes * 2, 0);
  // Min/Max extents in each dimension
  m_Extents.assign(maxBoxes * m_nDim * 2, 0);
  // Inverse of the volume of the cell
  m_InverseVolume.assign(maxBoxes, 0);
  // Box cached signal/error squared
  m_BoxSignalErrorsquared.assign(maxBoxes * 2, 0);
  // Start/end children IDs
  m_BoxChildren.assign(maxBoxes * 2, 0);

  API::IMDNode *Box;
  size_t ic(0);
  bool filePositionDefined(true);
  for (size_t i = 0; i < maxBoxes; i++) {
    Box = m_Boxes[i];
    // currently ID is the number of the box, but it may change in a future.
    // TODO: uint64_t
    size_t id = Box->getID();
    size_t numChildren = Box->getNumChildren();
    if (numChildren > 0) // MDGridBox have children
    {
      // DEBUG:
      //// Make sure that all children are ordered. TODO: This might not be
      /// needed if the IDs are rigorously done
      // size_t lastId = Box->getChild(0)->getId();
      // for (size_t i = 1; i < numChildren; i++)
      //{
      //  if (Box->getChild(i)->getId() != lastId+1)
      //    throw std::runtime_error("Non-sequential child ID encountered!");
      //  lastId = Box->getChild(i)->getId();
      //}
      // TODO! id != ic
      m_BoxType[ic] = 2;
      m_BoxChildren[ic * 2] = int(Box->getChild(0)->getID());
      m_BoxChildren[ic * 2 + 1] = int(Box->getChild(numChildren - 1)->getID());

      // no events but index defined -- TODO -- The proper file has to have
      // consequent indexes for all boxes too.
      m_BoxEventIndex[ic * 2] = 0;
      m_BoxEventIndex[ic * 2 + 1] = 0;
    } else {
      m_BoxType[ic] = 1;
      m_BoxChildren[ic * 2] = 0;
      m_BoxChildren[ic * 2 + 1] = 0;

      // MDBox<MDE,nd> * mdBox = dynamic_cast<MDBox<MDE,nd> *>(Box);
      // if(!mdBox) throw std::runtime_error("found unfamiliar type of box");
      // Store the index

      uint64_t nPoints = Box->getNPoints();
      Kernel::ISaveable *pSaver = Box->getISaveable();
      if (pSaver)
        m_BoxEventIndex[ic * 2] = pSaver->getFilePosition();
      else
        filePositionDefined = false;

      m_BoxEventIndex[ic * 2 + 1] = nPoints;
    }

    // Various bits of data about the box
    m_Depth[ic] = int(Box->getDepth());
    m_BoxSignalErrorsquared[ic * 2] = double(Box->getSignal());
    m_BoxSignalErrorsquared[ic * 2 + 1] = double(Box->getErrorSquared());
    m_InverseVolume[ic] = Box->getInverseVolume();
    for (int d = 0; d < m_nDim; d++) {
      size_t newIndex = id * size_t(m_nDim * 2) + d * 2;
      m_Extents[newIndex] = Box->getExtents(d).getMin();
      m_Extents[newIndex + 1] = Box->getExtents(d).getMax();
    }
    ic++;
  }
  // file position have to be calculated afresh
  if (!filePositionDefined) {
    uint64_t boxPosition(0);
    for (size_t i = 0; i < maxBoxes; i++) {
      if (m_BoxType[i] == 1) {
        m_BoxEventIndex[2 * i] = boxPosition;
        boxPosition += m_BoxEventIndex[2 * i + 1];
      }
    }
  }
}
/*** this function tries to set file positions of the boxes to
     make data physically located close to each other to be as close as possible
   on the HDD
     @param setFileBacked  -- initiate the boxes to be fileBacked. The boxes
   assumed not to be saved before.
*/
void MDBoxFlatTree::setBoxesFilePositions(bool setFileBacked) {
  // this will preserve file-backed workspace and information in it as we are
  // not loading old box data and not?
  // this would be right for binary access but questionable for Nexus --TODO:
  // needs testing
  // Done in INIT--> need check if ID and index in the tree are always the same.
  // Kernel::ISaveable::sortObjByFilePos(m_Boxes);
  // calculate the box positions in the resulting file and save it on place
  uint64_t eventsStart = 0;
  for (size_t i = 0; i < m_Boxes.size(); i++) {
    API::IMDNode *mdBox = m_Boxes[i];
    size_t ID = mdBox->getID();

    // avoid grid boxes;
    if (m_BoxType[ID] == 2)
      continue;

    size_t nEvents = mdBox->getTotalDataSize();
    m_BoxEventIndex[ID * 2] = eventsStart;
    m_BoxEventIndex[ID * 2 + 1] = nEvents;
    if (setFileBacked)
      mdBox->setFileBacked(eventsStart, nEvents, false);

    eventsStart += nEvents;
  }
}

void MDBoxFlatTree::saveBoxStructure(const std::string &fileName) {
  m_FileName = fileName;
  bool old_group;
  auto hFile = file_holder_type(createOrOpenMDWSgroup(
      fileName, m_nDim, m_Boxes[0]->getEventType(), false, old_group));

  // Save box structure;
  this->saveBoxStructure(hFile.get());
  // close workspace group
  hFile->closeGroup();
  // close file
  hFile->close();
}

void MDBoxFlatTree::saveBoxStructure(::NeXus::File *hFile) {
  size_t maxBoxes = this->getNBoxes();
  if (maxBoxes == 0)
    return;

  std::map<std::string, std::string> groupEntries;
  hFile->getEntries(groupEntries);

  bool create(false);
  if (groupEntries.find("box_structure") ==
      groupEntries.end()) // dimensions dataset exist
    create = true;

  // Start the box data group
  if (create) {
    hFile->makeGroup("box_structure", "NXdata", true);
    hFile->putAttr("version", "1.0");
    // Add box controller info to this group
    hFile->putAttr("box_controller_xml", m_bcXMLDescr);

  } else {
    hFile->openGroup("box_structure", "NXdata");
    // update box controller information
    hFile->putAttr("box_controller_xml", m_bcXMLDescr);
  }

  std::vector<int64_t> exents_dims(2, 0);
  exents_dims[0] = (int64_t(maxBoxes));
  exents_dims[1] = (m_nDim * 2);
  std::vector<int64_t> exents_chunk(2, 0);
  exents_chunk[0] = int64_t(16384);
  exents_chunk[1] = (m_nDim * 2);

  std::vector<int64_t> box_2_dims(2, 0);
  box_2_dims[0] = int64_t(maxBoxes);
  box_2_dims[1] = (2);
  std::vector<int64_t> box_2_chunk(2, 0);
  box_2_chunk[0] = int64_t(16384);
  box_2_chunk[1] = (2);

  if (create) {
    // Write it for the first time
    hFile->writeExtendibleData("box_type", m_BoxType);
    hFile->writeExtendibleData("depth", m_Depth);
    hFile->writeExtendibleData("inverse_volume", m_InverseVolume);
    hFile->writeExtendibleData("extents", m_Extents, exents_dims, exents_chunk);
    hFile->writeExtendibleData("box_children", m_BoxChildren, box_2_dims,
                               box_2_chunk);
    hFile->writeExtendibleData("box_signal_errorsquared",
                               m_BoxSignalErrorsquared, box_2_dims,
                               box_2_chunk);
    hFile->writeExtendibleData("box_event_index", m_BoxEventIndex, box_2_dims,
                               box_2_chunk);
  } else {
    // Update the expendable data sets
    hFile->writeUpdatedData("box_type", m_BoxType);
    hFile->writeUpdatedData("depth", m_Depth);
    hFile->writeUpdatedData("inverse_volume", m_InverseVolume);
    hFile->writeUpdatedData("extents", m_Extents, exents_dims);
    hFile->writeUpdatedData("box_children", m_BoxChildren, box_2_dims);
    hFile->writeUpdatedData("box_signal_errorsquared", m_BoxSignalErrorsquared,
                            box_2_dims);
    hFile->writeUpdatedData("box_event_index", m_BoxEventIndex, box_2_dims);
  }
  // close the box group.
  hFile->closeGroup();
}

/**load box structure from the file, defined by file name
 @param fileName       :: The name of the file with the box information
 @param nDim           :: number of dimensions the boxes  have.
                          If this number is <=0 on input, method loads existing
 number of box dimensions from the file, if it is a number, method verifies if
                          if the number of dimensions provided equal to this
 number in  the file. (leftover from the time when it was templated method)
 @param EventType      :: "MDEvent" or "MDLeanEvent"  -- describe the type of
 events the workspace contains, similarly to nDim, used to check the data
 integrity
 @param onlyEventInfo  :: load only box controller information and the events
 locations -- do not restore boxes themselves
 @param restoreExperimentInfo :: load also experiment information
 */
void MDBoxFlatTree::loadBoxStructure(const std::string &fileName, int &nDim,
                                     const std::string &EventType,
                                     bool onlyEventInfo,
                                     bool restoreExperimentInfo) {

  m_FileName = fileName;
  m_nDim = nDim;
  m_eventType = EventType;

  bool old_group;
  // open the file and the MD workspace group.
  auto hFile = file_holder_type(
      createOrOpenMDWSgroup(fileName, nDim, m_eventType, true, old_group));
  if (!old_group)
    throw Kernel::Exception::FileError(
        "MD workspace box structure data are not present in the file",
        fileName);

  m_nDim = nDim;

  this->loadBoxStructure(hFile.get(), onlyEventInfo);

  if (restoreExperimentInfo) {
    if (!m_mEI)
      m_mEI = boost::make_shared<Mantid::API::MultipleExperimentInfos>(
          Mantid::API::MultipleExperimentInfos());

    loadExperimentInfos(hFile.get(), fileName, m_mEI);
  }

  // close workspace group
  hFile->closeGroup();

  // close the NeXus file
  hFile->close();
}
void MDBoxFlatTree::loadBoxStructure(::NeXus::File *hFile, bool onlyEventInfo) {
  // ----------------------------------------- Box Structure
  // ------------------------------
  hFile->openGroup("box_structure", "NXdata");

  if (onlyEventInfo) {
    // Load the box controller description
    hFile->getAttr("box_controller_xml", m_bcXMLDescr);
    hFile->readData("box_type", m_BoxType);
    hFile->readData("box_event_index", m_BoxEventIndex);
    return;
  }
  // Load the box controller description
  hFile->getAttr("box_controller_xml", m_bcXMLDescr);

  // Read all the data blocks
  hFile->readData("box_type", m_BoxType);
  size_t numBoxes = m_BoxType.size();
  if (numBoxes == 0)
    throw std::runtime_error("Zero boxes found. There must have been an error "
                             "reading or writing the file.");

  hFile->readData("depth", m_Depth);
  hFile->readData("inverse_volume", m_InverseVolume);
  hFile->readData("extents", m_Extents);

  m_nDim = int(m_Extents.size() / (numBoxes * 2));
  hFile->readData("box_children", m_BoxChildren);
  hFile->readData("box_signal_errorsquared", m_BoxSignalErrorsquared);
  hFile->readData("box_event_index", m_BoxEventIndex);

  // Check all vector lengths match
  if (m_Depth.size() != numBoxes)
    throw std::runtime_error("Incompatible size for data: depth.");
  if (m_InverseVolume.size() != numBoxes)
    throw std::runtime_error("Incompatible size for data: inverse_volume.");
  // if (boxType.size() != numBoxes) throw std::runtime_error("Incompatible size
  // for data: boxType.");
  // if (m_Extents.size() != numBoxes*m_nDim*2) throw
  // std::runtime_error("Incompatible size for data: extents.");
  if (m_BoxChildren.size() != numBoxes * 2)
    throw std::runtime_error("Incompatible size for data: box_children.");
  if (m_BoxEventIndex.size() != numBoxes * 2)
    throw std::runtime_error("Incompatible size for data: box_event_index.");
  if (m_BoxSignalErrorsquared.size() != numBoxes * 2)
    throw std::runtime_error(
        "Incompatible size for data: box_signal_errorsquared.");

  hFile->closeGroup();
}

/** Save each NEW ExperimentInfo to a spot in the file
 *@param file -- NeXus file pointer to the file, opened within appropriate group
 *where one going to place experiment infos
 *@param ws   -- the shared pointer to the workspace with experiment infos to
 *write.
*/
void MDBoxFlatTree::saveExperimentInfos(::NeXus::File *const file,
                                        API::IMDEventWorkspace_const_sptr ws) {

  std::map<std::string, std::string> entries;
  file->getEntries(entries);
  for (uint16_t i = 0; i < ws->getNumExperimentInfo(); i++) {
    API::ExperimentInfo_const_sptr ei = ws->getExperimentInfo(i);
    std::string groupName = "experiment" + Kernel::Strings::toString(i);
    if (entries.find(groupName) == entries.end()) {
      // Can't overwrite entries. Just add the new ones
      file->makeGroup(groupName, "NXgroup", true);
      file->putAttr("version", 1);
      ei->saveExperimentInfoNexus(file);
      file->closeGroup();

      // Warning for high detector IDs.
      // The routine in MDEvent::saveVectorToNexusSlab() converts detector IDs
      // to single-precision floats
      // Floats only have 24 bits of int precision = 16777216 as the max,
      // precise detector ID
      detid_t min = 0;
      detid_t max = 0;
      try {
        ei->getInstrument()->getMinMaxDetectorIDs(min, max);
      } catch (std::runtime_error &) { /* Ignore error. Min/max will be 0 */
      }

      if (max > 16777216) {
        g_log.warning() << "This instrument (" << ei->getInstrument()->getName()
                        << ") has detector IDs that are higher than can be "
                           "saved in the .NXS file as single-precision floats."
                        << std::endl;
        g_log.warning() << "Detector IDs above 16777216 will not be precise. "
                           "Please contact the developers." << std::endl;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Load the ExperimentInfo blocks, if any, in the NXS file
*
* @param file :: the pointer to the properly opened nexus data file where the
*experiment info groups can be found.
* @param filename :: the filename of the opened NeXus file. Use for the file-backed case
* @param mei :: MDEventWorkspace/MDHisto to load experiment infos to or rather
*pointer to the base class of this workspaces (which is an experimentInfo)
* @param lazy :: If true, use the FileBackedExperimentInfo class to only load
* the data from the file when it is requested
*/
void MDBoxFlatTree::loadExperimentInfos(::NeXus::File *const file, const std::string &filename,
    boost::shared_ptr<Mantid::API::MultipleExperimentInfos> mei,
    bool lazy) {
  // First, find how many experimentX blocks there are
  std::map<std::string, std::string> entries;
  file->getEntries(entries);
  std::list<uint16_t> ExperimentBlockNum;
  std::map<std::string, std::string>::iterator it = entries.begin();
  for (; it != entries.end(); ++it) {
    std::string name = it->first;
    if (boost::starts_with(name, "experiment")) {
      try {
        uint16_t num =
            boost::lexical_cast<uint16_t>(name.substr(10, name.size() - 10));
        if (num < std::numeric_limits<uint16_t>::max() - 1) {
          // dublicated experiment info names are impossible due to the
          // structure of the nexus file but missing -- can be found.
          ExperimentBlockNum.push_back(num);
        }
      } catch (boost::bad_lexical_cast &) { /* ignore */
      }
    }
  }

  ExperimentBlockNum.sort();

  // check if all subsequent experiment infos numbers are present
  auto itr = ExperimentBlockNum.begin();
  size_t ic = 0;
  for (; itr != ExperimentBlockNum.end(); itr++) {
    if (*itr != ic) {
      for (size_t i = ic + 1; i < *itr; i++) {
        std::string groupName = "experiment" + Kernel::Strings::toString(i);
        g_log.warning() << "NXS file is missing a ExperimentInfo block "
                        << groupName
                        << ". Workspace will be missing ExperimentInfo."
                        << std::endl;
      }
    }
    ic++;
  }

  // Now go through in order, loading and adding
  itr = ExperimentBlockNum.begin();
  for (; itr != ExperimentBlockNum.end(); itr++) {
    std::string groupName = "experiment" + Kernel::Strings::toString(*itr);
    if (lazy) {
      auto ei = boost::make_shared<API::FileBackedExperimentInfo>(
        filename, file->getPath() + "/" + groupName);
      // And add it to the mutliple experiment info.
      mei->addExperimentInfo(ei);
     }
    else {
      auto ei = boost::make_shared<API::ExperimentInfo>();
      file->openGroup(groupName, "NXgroup");
      std::string parameterStr;
      try {
        // Get the sample, logs, instrument
        ei->loadExperimentInfoNexus(file, parameterStr);
        // Now do the parameter map
        ei->readParameterMap(parameterStr);
        // And add it to the mutliple experiment info.
        mei->addExperimentInfo(ei);
      } catch (std::exception &e) {
        g_log.information("Error loading section '" + groupName +
                          "' of nxs file.");
        g_log.information(e.what());
      }
      file->closeGroup();
    }
  }
}
/**Export existing experiment info defined in the box structure to target
 * workspace (or other experiment info as workspace is an experiment info) */
void
MDBoxFlatTree::exportExperiment(Mantid::API::IMDEventWorkspace_sptr &targetWS) {
  // copy experiment infos
  targetWS->copyExperimentInfos(*m_mEI);
  // free this Experiment info as it has been already exported
  m_mEI.reset();
}

/** Method recovers the interconnected box structure from the plain tree into
 box tree, recovering both boxes and their connectivity
  * does the opposite to the initFlatStructure operation (the class contents
 remains unchanged)
 @param  Boxes       :: the return vector of pointers to interconnected boxes.
 All previous pointers found in the vector will be overwritten (beware of memory
 loss)
 @param  bc          :: shard pointer to the box controller, which each box uses
 @param  FileBackEnd :: if one should make the data file backed, namely
 restore/calculate the data, necessary to obtain events file positions
 @param BoxStructureOnly :: restore box tree only ignoring information about the
 box events


 @returns   totalNumEvents :: total number of events the box structure should
 contain and allocated memory for.
*/
uint64_t MDBoxFlatTree::restoreBoxTree(std::vector<API::IMDNode *> &Boxes,
                                       API::BoxController_sptr &bc,
                                       bool FileBackEnd,
                                       bool BoxStructureOnly) {

  size_t numBoxes = this->getNBoxes();
  Boxes.assign(numBoxes, NULL);

  uint64_t totalNumEvents(0);
  m_nDim = static_cast<int>(bc->getNDims());
  int maxNdim = int(MDEventFactory::getMaxNumDim());
  if (m_nDim <= 0 || m_nDim > maxNdim)
    throw std::runtime_error(
        "Workspace dimesnions are not defined properly in the box controller");

  int iEventType(0);
  if (m_eventType == "MDLeanEvent")
    iEventType = 0;
  else if (m_eventType == "MDEvent")
    iEventType = 2;
  else
    throw std::invalid_argument(
        " Unknown event type provided for MDBoxFlatTree::restoreBoxTree");

  for (size_t i = 0; i < numBoxes; i++) {

    size_t box_type = m_BoxType[i];
    if (box_type == 0)
      continue;

    API::IMDNode *ibox = NULL;

    // Extents of the box, as a vector
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extentsVector(
        m_nDim);
    for (size_t d = 0; d < size_t(m_nDim); d++)
      extentsVector[d].setExtents(
          static_cast<double>(m_Extents[i * m_nDim * 2 + d * 2]),
          static_cast<double>(m_Extents[i * m_nDim * 2 + d * 2 + 1]));

    // retrieve initial and file location and the numner of the events which
    // belong to this box stored on the HDD
    uint64_t indexStart = m_BoxEventIndex[i * 2];
    uint64_t numEvents = m_BoxEventIndex[i * 2 + 1];

    totalNumEvents += numEvents;
    if (box_type == 1) {
      // --- Make a MDBox -----
      if (BoxStructureOnly) { // create box with undefined numer of events --
                              // differs from 0 number of events by not calling
                              // reserve(0) on underlying vectors.
        ibox = MDEventFactory::createBox(size_t(m_nDim),
                                         MDEventFactory::BoxType(iEventType),
                                         bc, extentsVector, m_Depth[i]);
      } else // !BoxStructureOnly)
      {

        if (FileBackEnd) {
          ibox = MDEventFactory::createBox(size_t(m_nDim),
                                           MDEventFactory::BoxType(iEventType),
                                           bc, extentsVector, m_Depth[i]);
          // Mark the box as file backed and indicate that the box was saved
          ibox->setFileBacked(indexStart, numEvents, true);
        } else {
          ibox = MDEventFactory::createBox(
              size_t(m_nDim), MDEventFactory::BoxType(iEventType), bc,
              extentsVector, m_Depth[i], numEvents);
        }
      } // ifBoxStructureOnly
    } else if (box_type == 2) {
      // --- Make a MDGridBox -----
      ibox = MDEventFactory::createBox(size_t(m_nDim),
                                       MDEventFactory::BoxType(iEventType + 1),
                                       bc, extentsVector, m_Depth[i]);
    } else
      continue;
    // Force correct ID
    ibox->setID(i);
    // calculate volume from extents;
    ibox->calcVolume();
    double vol = m_InverseVolume[i];
    if (vol <= FLT_EPSILON)
      vol = 1;
    if (std::fabs((ibox->getInverseVolume() - vol) / vol) > 1.e-5) {
      g_log.debug() << " Accuracy warning for box N " << i
                    << " as stored inverse volume is : " << m_InverseVolume[i]
                    << " and calculated from extents: "
                    << ibox->getInverseVolume() << std::endl;
      ibox->setInverseVolume(coord_t(m_InverseVolume[i]));
    }

    // Set the cached values
    ibox->setSignal(m_BoxSignalErrorsquared[i * 2]);
    ibox->setErrorSquared(m_BoxSignalErrorsquared[i * 2 + 1]);

    // Save the box at its index in the vector.
    Boxes[i] = ibox;

  } // end Box loop

  // Go again, giving the children to the parents
  for (size_t i = 0; i < numBoxes; i++) {
    if (m_BoxType[i] == 2) {
      size_t indexStart = m_BoxChildren[i * 2];
      size_t indexEnd = m_BoxChildren[i * 2 + 1] + 1;
      Boxes[i]->setChildren(Boxes, indexStart, indexEnd);
    }
  }
  bc->setMaxId(numBoxes);
  return totalNumEvents;
}
/** The function to create a NeXus MD workspace group with specified events type
 and number of dimensions or opens the existing group,
    which corresponds to the input parameters.
 *@param fileName -- the name of the file to create  or open WS group
 *@param nDims     -- number of workspace dimensions;
 *@param WSEventType -- the string describing event type
 *@param readOnly    -- true if the file is opened for read-only access
 *@param alreadyExists -- return true, if opened existing group or false if new
 group has been created.

 *@return   NeXus pointer to properly opened NeXus data file and group.
 *
 *@throws if group or its component do not exist and the file is opened
 read-only or if the existing file parameters are not equal to the
            input parameters.
*/
::NeXus::File *
MDBoxFlatTree::createOrOpenMDWSgroup(const std::string &fileName, int &nDims,
                                     const std::string &WSEventType,
                                     bool readOnly, bool &alreadyExists) {
  alreadyExists = false;
  Poco::File oldFile(fileName);
  bool fileExists = oldFile.exists();
  if (!fileExists && readOnly)
    throw Kernel::Exception::FileError(
        "Attempt to open non-existing file in read-only mode", fileName);

  NXaccess access(NXACC_RDWR);
  if (readOnly)
    access = NXACC_READ;

  file_holder_type hFile;
  try {
    if (fileExists)
      hFile = file_holder_type(new ::NeXus::File(fileName, access));
    else
      hFile = file_holder_type(new ::NeXus::File(fileName, NXACC_CREATE5));
  } catch (...) {
    throw Kernel::Exception::FileError("Can not open NeXus file", fileName);
  }

  std::map<std::string, std::string> groupEntries;

  hFile->getEntries(groupEntries);
  if (groupEntries.find("MDEventWorkspace") !=
      groupEntries.end()) // WS group exist
  {
    // Open and check ws group
    // -------------------------------------------------------------------------------->>>
    hFile->openGroup("MDEventWorkspace", "NXentry");
    alreadyExists = true;

    std::string eventType;
    if (hFile->hasAttr("event_type")) {
      hFile->getAttr("event_type", eventType);

      if (eventType != WSEventType)
        throw Kernel::Exception::FileError(
            "Trying to open MDWorkspace nexus file with the the events: " +
                eventType + "\n different from workspace type: " + WSEventType,
            fileName);
    } else // it is possible that workspace group has been created by somebody
           // else and there are no this kind of attribute attached to it.
    {
      if (readOnly)
        throw Kernel::Exception::FileError(
            "The NXdata group: MDEventWorkspace opened in read-only mode but \n"
            " does not have necessary attribute describing the event type used",
            fileName);
      hFile->putAttr("event_type", WSEventType);
    }
    // check dimensions dataset
    bool dimDatasetExist(false);
    hFile->getEntries(groupEntries);
    if (groupEntries.find("dimensions") !=
        groupEntries.end()) // dimensions dataset exist
      dimDatasetExist = true;

    if (dimDatasetExist) {
      int32_t nFileDims;
      hFile->readData<int32_t>("dimensions", nFileDims);
      if (nDims != 0) // check against dimensions provided
      {
        if (nFileDims != static_cast<int32_t>(nDims))
          throw Kernel::Exception::FileError(
              "The NXdata group: MDEventWorkspace initiated for different "
              "number of dimensions then requested ",
              fileName);
      } else // read what is already there
      {
        nDims = static_cast<int>(nFileDims);
      }
    } else {
      auto nFileDim = static_cast<int32_t>(nDims);
      if (nFileDim <= 0)
        throw std::invalid_argument("MDBoxFlatTree::createOrOpenMDWSgrou: "
                                    "Invalid number of workspace dimensions "
                                    "provided to save into file ");

      // Write out  # of dimensions
      hFile->writeData("dimensions", nFileDim);
    }
    // END Open and check ws group
    // --------------------------------------------------------------------------------<<<<
  } else {
    // create new WS group
    // ------------------------------------------------------------------------------->>>>>
    if (readOnly)
      throw Kernel::Exception::FileError("The NXdata group: MDEventWorkspace "
                                         "does not exist in the read-only file",
                                         fileName);

    try {
      alreadyExists = false;
      hFile->makeGroup("MDEventWorkspace", "NXentry", true);
      hFile->putAttr("event_type", WSEventType);

      auto nDim = int32_t(nDims);
      // Write out  # of dimensions
      hFile->writeData("dimensions", nDim);
    } catch (...) {
      throw Kernel::Exception::FileError(
          "Can not create new NXdata group: MDEventWorkspace", fileName);
    }
    // END create new WS group
    // -------------------------------------------------------------------------------<<<
  }
  return hFile.release();
}

/**Save workspace generic info like dimension structure, history, title
 * dimensions etc.*/
void MDBoxFlatTree::saveWSGenericInfo(::NeXus::File *const file,
                                      API::IMDWorkspace_const_sptr ws) {
  // Write out the coordinate system
  file->writeData("coordinate_system",
                  static_cast<uint32_t>(ws->getSpecialCoordinateSystem()));

  // Save the algorithm history under "process"
  ws->getHistory().saveNexus(file);

  // Write out the affine matrices
  saveAffineTransformMatricies(
      file, boost::dynamic_pointer_cast<const API::IMDWorkspace>(ws));

  // Save some info as attributes. (Note: need to use attributes, not data sets
  // because those cannot be resized).
  file->putAttr("definition", ws->id());
  file->putAttr("title", ws->getTitle());
  // Save each dimension, as their XML representation
  size_t nDim = ws->getNumDims();
  for (size_t d = 0; d < nDim; d++) {
    std::ostringstream mess;
    mess << "dimension" << d;
    file->putAttr(mess.str(), ws->getDimension(d)->toXMLString());
  }
}

/**
 * Save the affine matrices to both directional conversions to the
 * data.
 * @param file : pointer to the NeXus file
 * @param ws : workspace to get matrix from
 */
void
MDBoxFlatTree::saveAffineTransformMatricies(::NeXus::File *const file,
                                            API::IMDWorkspace_const_sptr ws) {
  try {
    saveAffineTransformMatrix(file, ws->getTransformToOriginal(),
                              "transform_to_orig");
  } catch (std::runtime_error &) {
    // Do nothing
  }
  try {
    saveAffineTransformMatrix(file, ws->getTransformFromOriginal(),
                              "transform_from_orig");
  } catch (std::runtime_error &) {
    // Do nothing
  }
}
/**
 * Extract and save the requested affine matrix.
 * @param file : pointer to the NeXus file
 * @param transform : the object to extract the affine matrix from
 * @param entry_name : the tag in the NeXus file to save under
 */
void MDBoxFlatTree::saveAffineTransformMatrix(::NeXus::File *const file,
                                              API::CoordTransform *transform,
                                              std::string entry_name) {
  Kernel::Matrix<coord_t> matrix = transform->makeAffineMatrix();
  g_log.debug() << "TRFM: " << matrix.str() << std::endl;
  saveMatrix<coord_t>(file, entry_name, matrix, ::NeXus::FLOAT32,
                      transform->id());
}

/**
 * Save routine for a generic matrix
 * @param file : pointer to the NeXus file
 * @param name : the tag in the NeXus file to save under
 * @param m : matrix to save
 * @param type : NXnumtype for the matrix data
 * @param tag : id for an affine matrix conversion
 */
template <typename T>
void saveMatrix(::NeXus::File *const file, std::string name,
                Kernel::Matrix<T> &m, ::NeXus::NXnumtype type,
                std::string tag) {
  std::vector<T> v = m.getVector();
  // Number of data points
  int nPoints = static_cast<int>(v.size());

  file->makeData(name, type, nPoints, true);
  // Need a pointer
  file->putData(&v[0]);
  if (!tag.empty()) {
    file->putAttr("type", tag);
    file->putAttr("rows", static_cast<int>(m.numRows()));
    file->putAttr("columns", static_cast<int>(m.numCols()));
  }
  file->closeData();
}
}
}
