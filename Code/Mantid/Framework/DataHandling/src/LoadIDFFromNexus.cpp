/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadIDFFromNexus.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidNexus/MuonNexusReader.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>


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

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
		  "The name (including its full or relative path) of the Nexus file to "
		  "attempt to load the instrument from. The file extension must either be "
		  ".nxs or .NXS" );

  declareProperty("InstrumentPath",std::string(""),"Path name within the Nexus tree of the folder containing the instrument folder."
      "This is usually 'raw_data_1' for a raw Nexus file and 'mantid_workspace_1' for a processed nexus file",Direction::Input);
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadIDFFromNexus::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // Get the instrument path
  std::string instrumentPath = getPropertyValue("InstrumentPath");

  // Code to be added.

  return;
}


} // namespace DataHandling
} // namespace Mantid
