// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/SaveMD.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Strings.h"
#include <Poco/File.h>

using file_holder_type = std::unique_ptr<::NeXus::File>;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace {
template <typename MDE, size_t nd>
void prepareUpdate(MDBoxFlatTree &BoxFlatStruct, BoxController *bc, typename MDEventWorkspace<MDE, nd>::sptr ws,
                   const std::string &filename) {
  // remove all boxes from the DiskBuffer. DB will calculate boxes positions
  // on HDD.
  bc->getFileIO()->flushCache();
  // flatten the box structure; this will remember boxes file positions in the
  // box structure
  BoxFlatStruct.initFlatStructure(ws, filename);
}
} // namespace

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveMD)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace or MDHistoWorkspace.");

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::OptionalSave, ".nxs"),
                  "The name of the Nexus file to write, as a full or relative path.\n"
                  "Optional if UpdateFileBackEnd is checked.");
  // Filename is NOT used if UpdateFileBackEnd
  setPropertySettings("Filename", std::make_unique<EnabledWhenProperty>("UpdateFileBackEnd", IS_EQUAL_TO, "0"));

  declareProperty("UpdateFileBackEnd", false,
                  "Only for MDEventWorkspaces with a file back end: check this to update "
                  "the NXS file on disk\n"
                  "to reflect the current data structure. Filename parameter is ignored.");
  setPropertySettings("UpdateFileBackEnd", std::make_unique<EnabledWhenProperty>("MakeFileBacked", IS_EQUAL_TO, "0"));

  declareProperty("MakeFileBacked", false,
                  "For an MDEventWorkspace that was created in memory:\n"
                  "This saves it to a file AND makes the workspace into a "
                  "file-backed one.");
  setPropertySettings("MakeFileBacked", std::make_unique<EnabledWhenProperty>("UpdateFileBackEnd", IS_EQUAL_TO, "0"));
}

//----------------------------------------------------------------------------------------------
/** Save the MDEventWorskpace to a file.
 * Based on the Intermediate Data Format Detailed Design Document, v.1.R3 found
 *in SVN.
 *
 * @param ws :: MDEventWorkspace of the given type
 */
template <typename MDE, size_t nd> void SaveMD::doSaveEvents(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  bool updateFileBackend = getProperty("UpdateFileBackEnd");
  bool makeFileBackend = getProperty("MakeFileBacked");
  if (updateFileBackend && makeFileBackend)
    throw std::invalid_argument("Please choose either UpdateFileBackEnd or MakeFileBacked, not both.");

  bool wsIsFileBacked = ws->isFileBacked();
  std::string filename = getPropertyValue("Filename");
  BoxController_sptr bc = ws->getBoxController();
  auto copyFile = wsIsFileBacked && !filename.empty() && filename != bc->getFilename();
  if (wsIsFileBacked) {
    if (makeFileBackend) {
      throw std::runtime_error("MakeFileBacked selected but workspace is already file backed.");
    }
  } else {
    if (updateFileBackend) {
      throw std::runtime_error("UpdateFileBackEnd selected but workspace is not file backed.");
    }
  }

  if (!wsIsFileBacked) {
    Poco::File oldFile(filename);
    if (oldFile.exists())
      oldFile.remove();
  }

  auto prog = std::make_unique<Progress>(this, 0.0, 0.05, 1);
  if (updateFileBackend) // workspace has its own file and ignores any changes
                         // to the
                         // algorithm parameters
  {
    if (!ws->isFileBacked())
      throw std::runtime_error(" attempt to update non-file backed workspace");
    filename = bc->getFileIO()->getFileName();
  }

  //-----------------------------------------------------------------------------------------------------
  // create or open WS group and put there additional information about WS and
  // its dimensions
  auto nDims = static_cast<int>(nd);
  bool data_exist;
  auto file =
      file_holder_type(MDBoxFlatTree::createOrOpenMDWSgroup(filename, nDims, MDE::getTypeName(), false, data_exist));

  // Save each NEW ExperimentInfo to a spot in the file
  MDBoxFlatTree::saveExperimentInfos(file.get(), ws);
  if (!updateFileBackend || !data_exist) {
    MDBoxFlatTree::saveWSGenericInfo(file.get(), ws);
  }
  file->closeGroup();
  file->close();

  MDBoxFlatTree BoxFlatStruct;
  //-----------------------------------------------------------------------------------------------------
  if (updateFileBackend) // the workspace is already file backed;
  {
    prepareUpdate<MDE, nd>(BoxFlatStruct, bc.get(), ws, filename);
  } else if (copyFile) {
    // Update the original file
    if (ws->fileNeedsUpdating()) {
      prepareUpdate<MDE, nd>(BoxFlatStruct, bc.get(), ws, filename);
      BoxFlatStruct.saveBoxStructure(filename);
    }
    bc->getFileIO()->copyFileTo(filename);
  } else // not file backed;
  {
    // the boxes file positions are unknown and we need to calculate it.
    BoxFlatStruct.initFlatStructure(ws, filename);
    // create saver class
    auto Saver = std::shared_ptr<API::IBoxControllerIO>(new DataObjects::BoxControllerNeXusIO(bc.get()));
    Saver->setDataType(sizeof(coord_t), MDE::getTypeName());
    if (makeFileBackend) {
      // store saver with box controller
      bc->setFileBacked(Saver, filename);
      // get access to boxes array
      std::vector<API::IMDNode *> &boxes = BoxFlatStruct.getBoxes();
      // calculate the position of the boxes on file, indicating to make them
      // saveable and that the boxes were not saved.
      BoxFlatStruct.setBoxesFilePositions(true);
      prog->resetNumSteps(boxes.size(), 0.06, 0.90);
      for (auto &boxe : boxes) {
        auto saveableTag = boxe->getISaveable();
        if (saveableTag) // only boxes can be saveable
        {
          // do not spend time on empty or masked boxes
          if (boxe->getDataInMemorySize() == 0 || boxe->getIsMasked())
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
        if (eventIndex[2 * i + 1] == 0 || boxes[i]->getIsMasked())
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
  if (!copyFile) {
    BoxFlatStruct.saveBoxStructure(filename);
  }

  ws->setFileNeedsUpdating(false);
}

//----------------------------------------------------------------------------------------------
/** Save a MDHistoWorkspace to a .nxs file
 *
 * @param ws :: MDHistoWorkspace to save
 */
void SaveMD::doSaveHisto(const Mantid::DataObjects::MDHistoWorkspace_sptr &ws) {
  std::string filename = getPropertyValue("Filename");

  // Erase the file if it exists
  Poco::File oldFile(filename);
  if (oldFile.exists())
    oldFile.remove();

  // Create a new file in HDF5 mode.
  auto file = std::make_unique<::NeXus::File>(filename, NXACC_CREATE5);

  // The base entry. Named so as to distinguish from other workspace types.
  file->makeGroup("MDHistoWorkspace", "NXentry", true);

  // Write out the coordinate system
  file->writeData("coordinate_system", static_cast<uint32_t>(ws->getSpecialCoordinateSystem()));

  // Write out the set display normalization
  file->writeData("visual_normalization", static_cast<uint32_t>(ws->displayNormalization()));

  // Save the algorithm history under "process"
  ws->getHistory().saveNexus(file.get());

  // Save all the ExperimentInfos
  for (uint16_t i = 0; i < ws->getNumExperimentInfo(); i++) {
    ExperimentInfo_sptr ei = ws->getExperimentInfo(i);
    std::string groupName = "experiment" + Strings::toString(i);
    if (ei) {
      // Can't overwrite entries. Just add the new ones
      file->makeGroup(groupName, "NXgroup", true);
      file->putAttr("version", 1);
      ei->saveExperimentInfoNexus(file.get());
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
  MDBoxFlatTree::saveAffineTransformMatricies(file.get(), std::dynamic_pointer_cast<const IMDWorkspace>(ws));

  // Check that the typedef has not been changed. The NeXus types would need
  // changing if it does!
  static_assert(sizeof(signal_t) == sizeof(double), "signal_t using has been changed!");

  // Number of data points
  auto nPoints = static_cast<int>(ws->getNPoints());

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
  IMDEventWorkspace_sptr eventWS = std::dynamic_pointer_cast<IMDEventWorkspace>(ws);
  MDHistoWorkspace_sptr histoWS = std::dynamic_pointer_cast<MDHistoWorkspace>(ws);

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

} // namespace Mantid::MDAlgorithms
