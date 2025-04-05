// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/SaveMD2.h"
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
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Strings.h"
#include <Poco/File.h>

using file_holder_type = std::unique_ptr<::NeXus::File>;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveMD2)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveMD2::init() {
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
  declareProperty("SaveHistory", true, "Option to not save the Mantid history in the file. Only for MDHisto");
  declareProperty("SaveInstrument", true, "Option to not save the instrument in the file. Only for MDHisto");
  declareProperty("SaveSample", true, "Option to not save the sample in the file. Only for MDHisto");
  declareProperty("SaveLogs", true, "Option to not save the logs in the file. Only for MDHisto");
}

//----------------------------------------------------------------------------------------------
/** Save a MDHistoWorkspace to a .nxs file
 *
 * @param ws :: MDHistoWorkspace to save
 */
void SaveMD2::doSaveHisto(const Mantid::DataObjects::MDHistoWorkspace_sptr &ws) {
  std::string filename = getPropertyValue("Filename");

  // Erase the file if it exists
  Poco::File oldFile(filename);
  if (oldFile.exists())
    oldFile.remove();

  // Create a new file in HDF5 mode.
  auto file = std::make_unique<::NeXus::File>(filename, NXACC_CREATE5);

  // The base entry. Named so as to distinguish from other workspace types.
  file->makeGroup("MDHistoWorkspace", "NXentry", true);
  file->putAttr("SaveMDVersion", 2);

  // Write out the coordinate system
  if (getProperty("SaveSample"))
    file->writeData("coordinate_system", static_cast<uint32_t>(ws->getSpecialCoordinateSystem()));

  // Write out the Qconvention
  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string m_QConvention = Kernel::ConfigService::Instance().getString("Q.convention");
  file->putAttr("QConvention", m_QConvention);

  // Write out the visual normalization
  if (getProperty("SaveSample"))
    file->writeData("visual_normalization", static_cast<uint32_t>(ws->displayNormalization()));

  // Save the algorithm history under "process"
  if (getProperty("SaveHistory"))
    ws->getHistory().saveNexus(file.get());

  // Save all the ExperimentInfos
  if (getProperty("SaveInstrument") || (getProperty("SaveSample")) || getProperty("SaveLogs")) {
    for (uint16_t i = 0; i < ws->getNumExperimentInfo(); i++) {
      ExperimentInfo_sptr ei = ws->getExperimentInfo(i);
      std::string groupName = "experiment" + Strings::toString(i);
      if (ei) {
        // Can't overwrite entries. Just add the new ones
        file->makeGroup(groupName, "NXgroup", true);
        file->putAttr("version", 1);
        ei->saveExperimentInfoNexus(file.get(), getProperty("SaveInstrument"), getProperty("SaveSample"),
                                    getProperty("SaveLogs"));
        file->closeGroup();
      }
    }
  }

  // Write out the affine matrices
  if (getProperty("SaveSample"))
    MDBoxFlatTree::saveAffineTransformMatricies(file.get(), std::dynamic_pointer_cast<const IMDWorkspace>(ws));

  file->makeGroup("data", "NXdata", true);

  // Save each axis dimension as an array
  size_t numDims = ws->getNumDims();
  std::string axes_label;
  for (size_t d = 0; d < numDims; d++) {
    std::vector<double> axis;
    IMDDimension_const_sptr dim = ws->getDimension(d);
    // NeXus field names must be alphanumeric only
    std::string axis_title("D" + std::to_string(d));
    auto nbounds = dim->getNBoundaries();
    for (size_t n = 0; n < nbounds; n++)
      axis.emplace_back(dim->getX(n));
    file->makeData(axis_title, NXnumtype::FLOAT64, static_cast<int>(dim->getNBoundaries()), true);
    file->putData(&axis[0]);
    file->putAttr("units", std::string(dim->getUnits()));
    file->putAttr("long_name", std::string(dim->getName()));
    file->putAttr("frame", dim->getMDFrame().name());
    file->closeData();
    if (d != 0)
      axes_label.insert(0, ":");
    axes_label.insert(0, axis_title);
  }

  // Number of data points
  // Size in each dimension (in the "C" style order, so z,y,x
  // That is, data[z][y][x] = etc.
  ::NeXus::DimVector size(numDims);
  for (size_t d = 0; d < numDims; d++) {
    IMDDimension_const_sptr dim = ws->getDimension(d);
    // Size in each dimension (reverse order for RANK)
    size[numDims - 1 - d] = int(dim->getNBins());
  }

  ::NeXus::DimSizeVector chunks = size;
  chunks[0] = 1; // Drop the largest stride for chunking, I don't know
                 // if this is the best but appears to work

  file->makeCompData("signal", NXnumtype::FLOAT64, size, ::NeXus::LZW, chunks, true);
  file->putData(ws->getSignalArray());
  file->putAttr("signal", 1);
  file->putAttr("axes", axes_label);
  file->closeData();

  file->makeCompData("errors_squared", NXnumtype::FLOAT64, size, ::NeXus::LZW, chunks, true);
  file->putData(ws->getErrorSquaredArray());
  file->closeData();

  file->makeCompData("num_events", NXnumtype::FLOAT64, size, ::NeXus::LZW, chunks, true);
  file->putData(ws->getNumEventsArray());
  file->closeData();

  file->makeCompData("mask", NXnumtype::INT8, size, ::NeXus::LZW, chunks, true);
  file->putData(ws->getMaskArray());
  file->closeData();

  file->closeGroup();

  // TODO: Links to original workspace???

  file->closeGroup();
  file->close();
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveMD2::exec() {
  IMDWorkspace_sptr ws = getProperty("InputWorkspace");
  IMDEventWorkspace_sptr eventWS = std::dynamic_pointer_cast<IMDEventWorkspace>(ws);
  MDHistoWorkspace_sptr histoWS = std::dynamic_pointer_cast<MDHistoWorkspace>(ws);

  if (eventWS) {
    // If event workspace use SaveMD version 1.
    auto saveMDv1 = createChildAlgorithm("SaveMD", -1, -1, true, 1);
    saveMDv1->setProperty<IMDWorkspace_sptr>("InputWorkspace", ws);
    saveMDv1->setProperty<std::string>("Filename", getProperty("Filename"));
    saveMDv1->setProperty<bool>("UpdateFileBackEnd", getProperty("UpdateFileBackEnd"));
    saveMDv1->setProperty<bool>("MakeFileBacked", getProperty("MakeFileBacked"));
    saveMDv1->execute();
  } else if (histoWS) {
    this->doSaveHisto(histoWS);
  } else
    throw std::runtime_error("SaveMD can only save MDEventWorkspaces and "
                             "MDHistoWorkspaces.\nPlease use SaveNexus or "
                             "another algorithm appropriate for this workspace "
                             "type.");
}

} // namespace Mantid::MDAlgorithms
