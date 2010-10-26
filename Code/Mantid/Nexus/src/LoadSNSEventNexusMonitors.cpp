#include "MantidNexus/LoadSNSEventNexusMonitors.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid
{
namespace NeXus
{

DECLARE_ALGORITHM(LoadSNSEventNexusMonitors)

LoadSNSEventNexusMonitors::LoadSNSEventNexusMonitors() : Algorithm(),
nMonitors(0)
{
}

LoadSNSEventNexusMonitors::~LoadSNSEventNexusMonitors()
{
}

/// Initialisation method.
void LoadSNSEventNexusMonitors::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load,
      ".nxs"),
      "The name (including its full or relative path) of the Nexus file to\n"
      "attempt to load. The file extension must either be .nxs or .NXS" );

  declareProperty(
    new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "",
        Kernel::Direction::Output),
    "The name of the output workspace in which to load the EventNexus monitors." );
}

/**
 * Executes the algorithm. Reading in the file and creating and populating
 * the output workspace
 */
void LoadSNSEventNexusMonitors::exec()
{
  // Retrieve the filename from the properties
  this->filename = this->getPropertyValue("Filename");

  // Create the output workspace
  this->WS = API::WorkspaceFactory::Instance().create("Workspace2D",
      1, 1, 1);
  this->WS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  this->WS->setYUnit("Counts");

  this->setProperty("OutputWorkspace", this->WS);
}
/**
 * Load the instrument geometry File
 *  @param instrument instrument name.
 *  @param localWorkspace MatrixWorkspace in which to put the instrument geometry
 */
void LoadSNSEventNexusMonitors::runLoadInstrument(const std::string &instrument,
    API::MatrixWorkspace_sptr localWorkspace)
{
  std::string filename = Kernel::ConfigService::Instance().getInstrumentFilename(instrument);
  if (filename.empty())
  {
    return;
  }
  if (!Poco::File(filename).exists())
  {
    return;
  }

  // do the actual work
  API::IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try
  {
    loadInst->setPropertyValue("Filename", filename);
    loadInst->setProperty<API::MatrixWorkspace_sptr> ("Workspace",
        localWorkspace);
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument& e)
  {
    g_log.information() << "Invalid argument to LoadInstrument sub-algorithm : " << e.what()
        << std::endl;
    executionSuccessful = false;
  } catch (std::runtime_error& e)
  {
    g_log.information() << "Unable to successfully run LoadInstrument sub-algorithm : " << e.what()
        << std::endl;
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful)
  {
    g_log.error() << "Error loading Instrument definition file\n";
  }
  else
  {
    this->instrument_loaded_correctly = true;
  }
}

} // end NeXus
} // end Mantid
