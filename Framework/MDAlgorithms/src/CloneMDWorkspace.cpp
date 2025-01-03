// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/CloneMDWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CloneMDWorkspace)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CloneMDWorkspace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace/MDHistoWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output MDEventWorkspace/MDHistoWorkspace.");

  std::vector<std::string> exts(1, ".nxs");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::OptionalSave, exts),
                  "If the input workspace is file-backed, specify a file to which to save "
                  "the cloned workspace.\n"
                  "If the workspace is file-backed but this parameter is NOT specified, "
                  "then a new filename with '_clone' appended is created next to the "
                  "original file.\n"
                  "No effect if the input workspace is NOT file-backed.\n"
                  "");
}

//----------------------------------------------------------------------------------------------

/** Perform the cloning
 *
 * @param ws ::  MDEventWorkspace to clone
 */
template <typename MDE, size_t nd> void CloneMDWorkspace::doClone(const typename MDEventWorkspace<MDE, nd>::sptr &ws) {
  BoxController_sptr bc = ws->getBoxController();

  if (!bc)
    throw std::runtime_error("Error with InputWorkspace: no BoxController!");
  if (bc->isFileBacked()) {
    if (ws->fileNeedsUpdating()) {
      // Data was modified! You need to save first.
      g_log.notice() << "InputWorkspace's file-backend being updated. \n";
      auto alg = createChildAlgorithm("SaveMD", 0.0, 0.4, false);
      alg->setProperty("InputWorkspace", ws);
      alg->setProperty("UpdateFileBackEnd", true);
      alg->executeAsChildAlg();
    }

    // Generate a new filename to copy to
    std::string originalFile = bc->getFilename();
    std::string outFilename = getPropertyValue("Filename");
    if (outFilename.empty()) {
      // Auto-generated name
      Poco::Path path = Poco::Path(originalFile).absolute();
      std::string newName = path.getBaseName() + "_clone." + path.getExtension();
      path.setFileName(newName);
      outFilename = path.toString();
    }

    // Perform the copying. HDF5 takes a file lock out when opening the file. On Windows this
    // prevents the a read handle being opened to perform the clone so we need to close the
    // file,  do the copy, then reopen it on the original box controller
    g_log.notice() << "Cloned workspace file being copied to: " << outFilename << '\n';
    bc->getFileIO()->copyFileTo(outFilename);
    g_log.information() << "File copied successfully.\n";

    // Now load it back
    auto alg = createChildAlgorithm("LoadMD", 0.5, 1.0, false);
    alg->setPropertyValue("Filename", outFilename);
    alg->setPropertyValue("FileBackEnd", "1");
    alg->setPropertyValue("Memory", "0"); // TODO: How much memory?
    alg->executeAsChildAlg();

    // Set the output workspace to this
    IMDWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    this->setProperty("OutputWorkspace", std::dynamic_pointer_cast<IMDWorkspace>(outWS));
  } else {
    // Perform the clone in memory.
    IMDWorkspace_sptr outWS(ws->clone());
    setProperty("OutputWorkspace", outWS);
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CloneMDWorkspace::exec() {
  IMDWorkspace_sptr inBaseWS = getProperty("InputWorkspace");
  IMDEventWorkspace_sptr inWS = std::dynamic_pointer_cast<IMDEventWorkspace>(inBaseWS);
  MDHistoWorkspace_sptr inHistoWS = std::dynamic_pointer_cast<MDHistoWorkspace>(inBaseWS);

  if (inWS) {
    CALL_MDEVENT_FUNCTION(this->doClone, inWS);
  } else if (inHistoWS) {
    // Polymorphic clone().
    IMDWorkspace_sptr outWS(inHistoWS->clone());
    // And set to the output. Easy.
    this->setProperty("OutputWorkspace", outWS);
  } else {
    // Call CloneWorkspace as a fall-back?
    throw std::runtime_error("CloneMDWorkspace can only clone a "
                             "MDEventWorkspace or MDHistoWorkspace. Try "
                             "CloneWorkspace.");
  }
}

} // namespace Mantid::MDAlgorithms
