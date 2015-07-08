//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadIDFFromNexus.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadIDFFromNexus)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadIDFFromNexus::LoadIDFFromNexus() {}

/// Initialisation method.
void LoadIDFFromNexus::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("Workspace", "Anonymous",
                                             Direction::InOut),
      "The name of the workspace in which to attach the imported instrument");

  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".nxs.h5");
  declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name (including its full or relative path) of the Nexus file to "
      "attempt to load the instrument from.");

  declareProperty("InstrumentParentPath", std::string(""),
                  "Path name within the Nexus tree of the folder containing "
                  "the instrument folder. "
                  "For example it is 'raw_data_1' for an ISIS raw Nexus file "
                  "and 'mantid_workspace_1' for a processed nexus file. "
                  "Only a one level path is curently supported",
                  Direction::Input);
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadIDFFromNexus::exec() {
  // Retrieve the filename from the properties
  const std::string filename = getPropertyValue("Filename");

  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // Get the instrument path
  std::string instrumentParentPath = getPropertyValue("InstrumentParentPath");

  // Get the instrument group in the Nexus file
  ::NeXus::File nxfile(filename);
  // Assume one level in instrument path
  nxfile.openPath(instrumentParentPath);

  // Take instrument info from nexus file.
  localWorkspace->loadInstrumentInfoNexus(filename, &nxfile );

  LoadParameters(  &nxfile, localWorkspace );

  return;
}


/** Loads the parameters from the Nexus file if possible, else from a parameter file
 *  into the specified workspace
 * @param nxfile :: open NeXus file
 * @param localWorkspace :: workspace into which loading occurs
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadIDFFromNexus::LoadParameters( ::NeXus::File *nxfile, const MatrixWorkspace_sptr localWorkspace ) {

  std::string parameterString;

  // First attempt to load parameters from nexus file.                                    
  nxfile->openGroup("instrument", "NXinstrument");
  localWorkspace->loadInstrumentParametersNexus( nxfile, parameterString );
  nxfile->closeGroup();

  // loadInstrumentParametersNexus does not populate any instrument params
  // so we do it here.
  localWorkspace->populateInstrumentParameters();

  if (parameterString.empty()) {
    // No parameters have been found in Nexus file, so we look for them in a parameter file.
    std::vector<std::string> directoryNames =
        ConfigService::Instance().getInstrumentDirectories();
    const std::string instrumentName =
        localWorkspace->getInstrument()->getName();
    for (auto instDirs_itr = directoryNames.begin();
         instDirs_itr != directoryNames.end(); ++instDirs_itr) {
      // This will iterate around the directories from user ->etc ->install, and
      // find the first beat file
      std::string directoryName = *instDirs_itr;
      const std::string paramFile =
          directoryName + instrumentName + "_Parameters.xml";

      try {
        // load and also populate instrument parameters from this 'fallback'
        // parameter file
        Algorithm_sptr loadParamAlg = createChildAlgorithm("LoadParameterFile");
        loadParamAlg->setProperty("Filename", paramFile);
        loadParamAlg->setProperty("Workspace", localWorkspace);
        loadParamAlg->execute();
        g_log.notice() << "Instrument parameter file: " << paramFile
                       << " has been loaded" << std::endl;
        break; // stop at the first one
      } catch (std::runtime_error &) {
        g_log.debug() << "Instrument parameter file: " << paramFile
                      << " not found or un-parsable. ";
      }
    }
  } else { // We do have parameters from the Nexus file
    g_log.notice()
        << "Found Instrument parameter map entry in Nexus file, which is loaded"
        << std::endl;
    // process parameterString into parameters in workspace
    localWorkspace->readParameterMap(parameterString);
  }

}

} // namespace DataHandling
} // namespace Mantid
