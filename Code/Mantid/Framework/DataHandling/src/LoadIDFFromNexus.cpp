/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadIDFFromNexus.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"

namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(LoadIDFFromNexus)

/// Sets documentation strings for this algorithm
void LoadIDFFromNexus::initDocs()
{
  this->setWikiSummary("Load an IDF from a Nexus file, if found there.");
  this->setOptionalMessage("Load an IDF from a Nexus file, if found there. You may need to tell this algorithm where to find the Instrument folder in the Nexus file");
}


using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadIDFFromNexus::LoadIDFFromNexus()
{}

/// Initialisation method.
void LoadIDFFromNexus::init()
{
  // When used as a Child Algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace in which to attach the imported instrument" );

  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".nxs.h5");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name (including its full or relative path) of the Nexus file to "
                  "attempt to load the instrument from.");

  declareProperty("InstrumentParentPath",std::string(""),"Path name within the Nexus tree of the folder containing the instrument folder."
      "For example it is 'raw_data_1' for an ISIS raw Nexus file and 'mantid_workspace_1' for a processed nexus file."
      "Only a one level path is curently supported",Direction::Input);
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadIDFFromNexus::exec()
{
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

  std::string parameterString;
  localWorkspace->loadExperimentInfoNexus( &nxfile, parameterString );
  localWorkspace->readParameterMap(parameterString);

  runLoadParameterFile(localWorkspace);

  return;
}

void LoadIDFFromNexus::runLoadParameterFile(const MatrixWorkspace_sptr & workspace)
{
  const std::string directory = ConfigService::Instance().getString("parameterDefinition.directory");
  const std::string instrumentName = workspace->getInstrument()->getName();
  const std::string paramFile = directory + instrumentName + "_Parameters.xml";

  try {
    LoadParameterFile::execManually(false, paramFile,"", workspace);
  } catch ( std::runtime_error& ) {
    g_log.notice() << "File " << paramFile << " not found or un-parsable. "
                       "However, the instrument has been loaded successfully.\n";
    // This next function needs to have been called. If LoadParameterFile succeeds then it will have been called inside that.
    workspace->populateInstrumentParameters();
  }

}


} // namespace DataHandling
} // namespace Mantid
