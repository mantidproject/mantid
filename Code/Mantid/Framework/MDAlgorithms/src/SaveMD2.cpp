#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/SaveMD2.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <Poco/File.h>
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"

#if defined(__GLIBCXX__) && __GLIBCXX__ >= 20100121 // libstdc++-4.4.3
typedef std::unique_ptr<::NeXus::File> file_holder_type;
#else
typedef std::auto_ptr<::NeXus::File> file_holder_type;
#endif

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveMD2)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveMD2::SaveMD2() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveMD2::~SaveMD2() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveMD2::init() {
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
/** Save a MDHistoWorkspace to a .nxs file
 *
 * @param ws :: MDHistoWorkspace to save
 */
void SaveMD2::doSaveHisto(Mantid::DataObjects::MDHistoWorkspace_sptr ws) {
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
  file->putAttr("SaveMDVersion", 2);

  // Write out the coordinate system
  file->writeData("coordinate_system",
                  static_cast<uint32_t>(ws->getSpecialCoordinateSystem()));

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
  size_t numDims = ws->getNumDims();
  file->writeData("dimensions", int32_t(numDims));

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
  // Size in each dimension (in the "C" style order, so z,y,x
  // That is, data[z][y][x] = etc.
  std::vector<int> size(numDims);
  for (size_t d = 0; d < numDims; d++) {
    IMDDimension_const_sptr dim = ws->getDimension(d);
    // Size in each dimension (reverse order for RANK)
    size[numDims - 1 - d] = int(dim->getNBins());
    g_log.debug() << "Dim: " << d << " Bins: " << size[numDims - 1 - d]
                  << std::endl;
  }

  file->makeData("signal", ::NeXus::FLOAT64, size, true);
  file->putData(ws->getSignalArray());
  file->closeData();

  file->makeData("errors_squared", ::NeXus::FLOAT64, size, true);
  file->putData(ws->getErrorSquaredArray());
  file->closeData();

  file->makeData("num_events", ::NeXus::FLOAT64, size, true);
  file->putData(ws->getNumEventsArray());
  file->closeData();

  file->makeData("mask", ::NeXus::INT8, size, true);
  file->putData(ws->getMaskArray());
  file->closeData();

  // TODO: Links to original workspace???

  file->closeGroup();
  file->close();
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveMD2::exec() {
  IMDWorkspace_sptr ws = getProperty("InputWorkspace");
  IMDEventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(ws);
  MDHistoWorkspace_sptr histoWS =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(ws);

  if (eventWS) {
    // If event workspace use SaveMD version 1.
    IAlgorithm_sptr saveMDv1 = createChildAlgorithm("SaveMD",-1,-1,true,1);
    saveMDv1->setProperty<IMDWorkspace_sptr>("InputWorkspace",ws);
    saveMDv1->setProperty<std::string>("Filename",getProperty("Filename"));
    saveMDv1->setProperty<bool>("UpdateFileBackEnd",getProperty("UpdateFileBackEnd"));
    saveMDv1->setProperty<bool>("MakeFileBacked",getProperty("MakeFileBacked"));
    saveMDv1->execute();
  } else if (histoWS) {
    this->doSaveHisto(histoWS);
  } else
    throw std::runtime_error("SaveMD can only save MDEventWorkspaces and "
                             "MDHistoWorkspaces.\nPlease use SaveNexus or "
                             "another algorithm appropriate for this workspace "
                             "type.");
}

} // namespace Mantid
} // namespace DataObjects
