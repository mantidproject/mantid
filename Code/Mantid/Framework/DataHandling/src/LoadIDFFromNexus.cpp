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

  // Take instrument info from nexus file. 
  // If the nexus file also contains a instrument parameter map entry this 
  // is returned as parameterString
  std::string parameterString;
  localWorkspace->loadInstrumentInfoNexus( &nxfile, parameterString );
  // at present loadInstrumentInfoNexus does not populate any instrument params
  // into the workspace including those that are defined in the IDF.
  // Here populate inst params defined in IDF
  localWorkspace->populateInstrumentParameters();

  // if no parameter map in nexus file then attempt to load a 'fallback' 
  // parameter file from hard-disk. You may argue whether this should be
  // done at all from an algorithm called LoadIDFFromNexus but that is
  // for another day to possible change
  if ( parameterString.empty() )
  {
    // Create the 'fallback' parameter file name to look for
    const std::string directory = ConfigService::Instance().getString("parameterDefinition.directory");
    const std::string instrumentName = localWorkspace->getInstrument()->getName();
    const std::string paramFile = directory + instrumentName + "_Parameters.xml";

    try {
      // load and also populate instrument parameters from this 'fallback' parameter file 
      LoadParameterFile::execManually(false, paramFile,"", localWorkspace);
      g_log.notice() << "Instrument parameter file: " << paramFile << " has been loaded" << std::endl;
    } catch ( std::runtime_error& ) {
      g_log.debug() << "Instrument parameter file: " << paramFile << " not found or un-parsable. ";
    }
  }
  else
  {
    g_log.notice() << "Found Instrument parameter map entry in Nexus file, which is loaded" << std::endl;
    // process parameterString into parameters in workspace
    localWorkspace->readParameterMap(parameterString);
  }

  return;
}


} // namespace DataHandling
} // namespace Mantid
