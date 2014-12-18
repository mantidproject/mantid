#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/Matrix.h"
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
#include "MantidMDEvents/BoxControllerNeXusIO.h"

#if defined(__GLIBCXX__) && __GLIBCXX__ >= 20100121 // libstdc++-4.4.3
typedef std::unique_ptr< ::NeXus::File> file_holder_type;
#else
typedef std::auto_ptr< ::NeXus::File> file_holder_type;
#endif

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveMD::SaveMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveMD::~SaveMD() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveMD::init() {
  declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace", "",
                                                      Direction::Input),
                  "An input MDEventWorkspace or MDHistoWorkspace.");

  std::vector<std::string> exts;
  exts.push_back(".nxs");
  declareProperty(
      new FileProperty("Filename", "", FileProperty::OptionalSave, exts),
      "The name of the Nexus file to write, as a full or relative path.\n"
      "Optional if UpdateFileBackEnd is checked.");
  // Filename is NOT used if UpdateFileBackEnd
  setPropertySettings("Filename", new EnabledWhenProperty("UpdateFileBackEnd",
                                                          IS_EQUAL_TO, "0"));

  declareProperty(
      "UpdateFileBackEnd", false,
      "Only for MDEventWorkspaces with a file back end: check this to update "
      "the NXS file on disk\n"
      "to reflect the current data structure. Filename parameter is ignored.");
  setPropertySettings(
      "UpdateFileBackEnd",
      new EnabledWhenProperty("MakeFileBacked", IS_EQUAL_TO, "0"));

  declareProperty("MakeFileBacked", false,
                  "For an MDEventWorkspace that was created in memory:\n"
                  "This saves it to a file AND makes the workspace into a "
                  "file-backed one.");
  setPropertySettings(
      "MakeFileBacked",
      new EnabledWhenProperty("UpdateFileBackEnd", IS_EQUAL_TO, "0"));
}

//----------------------------------------------------------------------------------------------
/** Save the MDEventWorskpace to a file.
 * Based on the Intermediate Data Format Detailed Design Document, v.1.R3 found
 *in SVN.
 *
 * @param ws :: MDEventWorkspace of the given type
 */
template <typename MDE, size_t nd>
void SaveMD::doSaveEvents(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  std::string filename = getPropertyValue("Filename");
  bool update = getProperty("UpdateFileBackEnd");
  bool MakeFileBacked = getProperty("MakeFileBacked");

  bool wsIsFileBacked = ws->isFileBacked();
  if (update && MakeFileBacked)
    throw std::invalid_argument(
        "Please choose either UpdateFileBackEnd or MakeFileBacked, not both.");

  if (MakeFileBacked && wsIsFileBacked)
    throw std::invalid_argument(
        "You picked MakeFileBacked but the workspace is already file-backed!");

  BoxController_sptr bc = ws->getBoxController();

  if (!wsIsFileBacked) { // Erase the file if it exists
    Poco::File oldFile(filename);
    if (oldFile.exists())
      oldFile.remove();
  }

  Progress *prog = new Progress(this, 0.0, 0.05, 1);
  if (update) // workspace has its own file and ignores any changes to the
              // algorithm parameters
  {
    if (!ws->isFileBacked())
      throw std::runtime_error(" attempt to update non-file backed workspace");
    filename = bc->getFileIO()->getFileName();
  }

  //-----------------------------------------------------------------------------------------------------
  // create or open WS group and put there additional information about WS and
  // its dimensions
  int nDims = static_cast<int>(nd);
  bool data_exist;
  auto file = file_holder_type(MDBoxFlatTree::createOrOpenMDWSgroup(
      filename, nDims, MDE::getTypeName(), false, data_exist));

  // Save each NEW ExperimentInfo to a spot in the file
  MDBoxFlatTree::saveExperimentInfos(file.get(), ws);
  if (!update || !data_exist) {
    MDBoxFlatTree::saveWSGenericInfo(file.get(), ws);
  }
  file->closeGroup();
  file->close();

  MDBoxFlatTree BoxFlatStruct;
  //-----------------------------------------------------------------------------------------------------
  if (update) // the workspace is already file backed;
  {
    // remove all boxes from the DiskBuffer. DB will calculate boxes positions
    // on HDD.
    bc->getFileIO()->flushCache();
    // flatten the box structure; this will remember boxes file positions in the
    // box structure
    BoxFlatStruct.initFlatStructure(ws, filename);
  } else // not file backed;
  {
    // the boxes file positions are unknown and we need to calculate it.
    BoxFlatStruct.initFlatStructure(ws, filename);
    // create saver class
    auto Saver = boost::shared_ptr<API::IBoxControllerIO>(
        new MDEvents::BoxControllerNeXusIO(bc.get()));
    Saver->setDataType(sizeof(coord_t), MDE::getTypeName());
    if (MakeFileBacked) {
      // store saver with box controller
      bc->setFileBacked(Saver, filename);
      // get access to boxes array
      std::vector<API::IMDNode *> &boxes = BoxFlatStruct.getBoxes();
      // calculate the position of the boxes on file, indicating to make them
      // saveable and that the boxes were not saved.
      BoxFlatStruct.setBoxesFilePositions(true);
      prog->resetNumSteps(boxes.size(), 0.06, 0.90);
      for (size_t i = 0; i < boxes.size(); i++) {
        auto saveableTag = boxes[i]->getISaveable();
        if (saveableTag) // only boxes can be saveable
        {
          // do not spend time on empty boxes
          if (boxes[i]->getDataInMemorySize() == 0)
            continue;
          // save boxes directly using the boxes file postion, precalculated in
          // boxFlatStructure.
          saveableTag->save();
          // remove boxes data from memory. This will actually correctly set the
          // tag indicatin that data were not loaded.
          saveableTag->clearDataFromMemory();
          // put boxes into write buffer wich will save them when necessary
          // Saver->toWrite(saveTag);
          prog->report("Saving Box");
        }
      }
      // remove everything from diskBuffer;  (not sure if it really necessary
      // but just in case , should not make any harm)
      Saver->flushCache();
      // drop NeXus on HDD (not sure if it really necessary but just in case )
      Saver->flushData();
    } else // just save data, and finish with it
    {
      Saver->openFile(filename, "w");
      BoxFlatStruct.setBoxesFilePositions(false);
      std::vector<API::IMDNode *> &boxes = BoxFlatStruct.getBoxes();
      std::vector<uint64_t> &eventIndex = BoxFlatStruct.getEventIndex();
      prog->resetNumSteps(boxes.size(), 0.06, 0.90);
      for (size_t i = 0; i < boxes.size(); i++) {
        if (eventIndex[2 * i + 1] == 0)
          continue;
        boxes[i]->saveAt(Saver.get(), eventIndex[2 * i]);
        prog->report("Saving Box");
      }
      Saver->closeFile();
    }
  }

  // -------------- Save Box Structure  -------------------------------------
  // OK, we've filled these big arrays of data representing flat box structrre.
  // Save them.
  progress(0.91, "Writing Box Data");
  prog->resetNumSteps(8, 0.92, 1.00);

  // Save box structure;
  BoxFlatStruct.saveBoxStructure(filename);

  delete prog;

  ws->setFileNeedsUpdating(false);
}

//----------------------------------------------------------------------------------------------
/** Save a MDHistoWorkspace to a .nxs file
 *
 * @param ws :: MDHistoWorkspace to save
 */
void SaveMD::doSaveHisto(Mantid::MDEvents::MDHistoWorkspace_sptr ws) {
  std::string filename = getPropertyValue("Filename");

  // Erase the file if it exists
  Poco::File oldFile(filename);
  if (oldFile.exists())
    oldFile.remove();

  // Create a new file in HDF5 mode.
  ::NeXus::File *file;
  file = new ::NeXus::File(filename, NXACC_CREATE5);

  // The base entry. Named so as to distinguish from other workspace types.
  file->makeGroup("MDHistoWorkspace", "NXentry", true);

  // Save the algorithm history under "process"
  ws->getHistory().saveNexus(file);

  // Save all the ExperimentInfos
  for (uint16_t i = 0; i < ws->getNumExperimentInfo(); i++) {
    ExperimentInfo_sptr ei = ws->getExperimentInfo(i);
    std::string groupName = "experiment" + Strings::toString(i);
    if (ei) {
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
  for (size_t d = 0; d < ws->getNumDims(); d++) {
    std::ostringstream mess;
    mess << "dimension" << d;
    file->putAttr(mess.str(), ws->getDimension(d)->toXMLString());
  }

  // Write out the affine matrices
  MDBoxFlatTree::saveAffineTransformMatricies(
      file, boost::dynamic_pointer_cast<const IMDWorkspace>(ws));

  // Check that the typedef has not been changed. The NeXus types would need
  // changing if it does!
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
void SaveMD::exec() {
  IMDWorkspace_sptr ws = getProperty("InputWorkspace");
  IMDEventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(ws);
  MDHistoWorkspace_sptr histoWS =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(ws);

  if (eventWS) {
    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->doSaveEvents, eventWS);
  } else if (histoWS) {
    this->doSaveHisto(histoWS);
  } else
    throw std::runtime_error("SaveMD can only save MDEventWorkspaces and "
                             "MDHistoWorkspaces.\nPlease use SaveNexus or "
                             "another algorithm appropriate for this workspace "
                             "type.");
}

} // namespace Mantid
} // namespace MDEvents
