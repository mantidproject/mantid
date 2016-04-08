#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MantidVersion.h"

#include <boost/make_shared.hpp>
#include <H5Cpp.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace
{
/**
 * Add the sasEntry to the sasroot.
 * @param file: Handle to the NXcanSAS file
 * @param workspace: the workspace to store
 * @param version: the NXcanSAS version
 * @param suffix: the suffix of the current entry
 * @return the sasEntry
 */
H5::Group addSasEntry(H5::H5File &file,
                      Mantid::API::MatrixWorkspace_sptr workspace,
                      const std::string &version, const std::string &suffix)
{
    const std::string sasEntryName = "sasentry" + suffix;
    // TODO: refactor once PR of Pete is in
    // auto sasEntry = H5Util::createGroupNXS(file, sasEntryName, "SASentry");
    // writeStrAttribute(sasEntry, "version", version);
    // auto workspaceName = workspace->name();
    // H5Util::write(group, "Title", workspaceName);
    auto sasEntry = file.createGroup(sasEntryName);
    return sasEntry;
}

std::string getInstrumentName(Mantid::API::MatrixWorkspace_sptr workspace) {
  auto instrument = workspace->getInstrument();
  return instrument->getFullName();
}

std::string
getIDF(Mantid::API::MatrixWorkspace_sptr workspace) {
    auto instrument = workspace->getInstrument();
    return instrument->getFileName();
}

/**
 * Add the instrument group to the NXcanSAS file. This adds the
 * instrument name and the IDF
 * @param group: the sasEntry
 * @param workspace: the workspace which is being stored
 * @param radiationSource: the selcted radiation source
 * @param suffix: the suffix of the current entry
 */
void addInstrument(H5::Group &group,
                   Mantid::API::MatrixWorkspace_sptr workspace,
                   const std::string &radiationSource,
                   const std::string &suffix) {
    const std::string sasInstrumentName = "sasinstrument" + suffix;
    auto instrument = group.createGroup(sasInstrumentName);

    auto instrumentName = getInstrumentName(workspace);
    // H5Util::write(instrument, "instrument_name", instrumentName);

    // H5Util::write(instrument, "source", radiationSource);

    auto idf = getIDF(workspace);
    // H5Util::write(instrument, "idf", idf);
}

/**
 * Add the process information to the NXcanSAS file. This information
 * about the run number, the Mantid version and the user file (if available)
 * @param group: the sasEntry
 * @param workspace: the workspace which is being stored
 * @param suffix: the suffix of the current entry
 */
void addProcess(H5::Group &group,
                   Mantid::API::MatrixWorkspace_sptr workspace,
                   const std::string &suffix){
  const std::string sasProcessName = "sasprocess" + suffix;
  auto process = group.createGroup(sasProcessName);

  const auto version = std::string(MantidVersion::version());
  // H5Util::write(process, "svn", version);

  const auto runNumber = workspace->getRunNumber();
  // H5Util::write(process, "run_number", version);

  const auto run = workspace->run();
  if (run.hasProperty("UserFile")) {
      const auto userFile = run.getProperty("UserFile");
      // H5Util::write(process, "user_file", userFile);
  }
}

class enum WorkspaceDimensionality{1D, 2D, Other};

WorkspaceDimensionality getWorkspaceDimensionality(Mantid::API::MatrixWorkspace_sptr workspace) {
  // TODO: create read out
  return WorkspaceDimensionality::1D;
}

void addData1D(H5::Group &data, Mantid::API::MatrixWorkspace_sptr workspace) {
}

void addData2D(H5::Group &data, Mantid::API::MatrixWorkspace_sptr workspace) {
}

void addData(H5::Group &group, Mantid::API::MatrixWorkspace_sptr workspace,
             const std::string &suffix){
  const std::string sasDataName = "sasdata" + suffix;
  auto data = group.createGroup(sasDataName);

  // Check if the data is 1D or 2D
  auto isData1D = isData1D(workspace);
  if (isData1D) {
    add1DData(data, workspace);
  } else {
    add2DData(data, workspace);
  }
}

}
}

namespace Mantid
{
namespace DataHandling
{

void SaveNXcanSAS::init()
{
    declareProperty(
        Mantid::Kernel::make_unique<Mantid::API::WorkspaceProperty<>>(
            "InputWorkspace", "", Kernel::Direction::Input,
            boost::make_shared<API::WorkspaceUnitValidator>(
                "MomentumTransfer")),
        "The input workspace, which must be in units of Q");
    declareProperty(Mantid::Kernel::make_unique<Mantid::API::FileProperty>(
                        "Filename", "", API::FileProperty::Save, ".nxs"),
                    "The name of the .nxs file to save");

    std::vector<std::string> radiation_source{
        "Spallation Neutron Source", "Pulsed Reactor Neutron Source",
        "Reactor Neutron Source", "Synchrotron X-ray Source",
        "Pulsed Muon Source", "Rotating Anode X-ray", "Fixed Tube X-ray",
        "neutron", "x-ray", "muon", "electron"};
    declareProperty(
        "RadiationSource", "Spallation Neutron Source",
        boost::make_shared<Kernel::StringListValidator>(radiation_source),
        "The type of radiation used.");

    declareProperty("Process", "", "Text to append to Process section");
    declareProperty(
        "DetectorNames", "",
        "Specify in a comma separated list, which detectors to store "
        "information about; \nwhere each name must match a name "
        "given for a detector in the [[IDF|instrument definition "
        "file (IDF)]]. \nIDFs are located in the instrument "
        "sub-directory of the MantidPlot install directory.");

    declareProperty(
        Mantid::Kernel::make_unique<API::WorkspaceProperty<>>(
            "Transmission", "", Kernel::Direction::Input,
            PropertyMode::Optional,
            boost::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
        "The transmission workspace. Optional. If given, will be saved at "
        "TransmissionSpectrum");

    declareProperty(
        Mantid::Kernel::make_unique<API::WorkspaceProperty<>>(
            "TransmissionCan", "", Kernel::Direction::Input,
            PropertyMode::Optional,
            boost::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
        "The transmission workspace of the Can. Optional. If given, will be "
        "saved at TransmissionSpectrum");
}

std::map<std::string, std::string> SaveNXcanSAS::validateInputs()
{
    // The input should be a Workspace2D
    Mantid::API::MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
    std::map<std::string, std::string> result;
    if (!workspace
        || !boost::dynamic_pointer_cast<const Mantid::DataObjects::Workspace2D>(
               workspace)) {
        result.insert(std::make_pair(
            "InputWorkspace", "The InputWorkspace must be a Workspace2D."));
    }

    // Don't allow ragged workspaces for now
    if (!API::WorkspaceHelpers::commonBoundaries(workspace)) {
        result.insert(
            std::make_pair("InputWorkspace",
                           "The InputWorkspace cannot be a ragged workspace."));
    }

    return result;
}

void SaveNXcanSAS::exec()
{
    Mantid::API::MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
    std::string filename = getPropertyValue("Filename");

    std::string radiationSource = getPropertyValue("RadiationSource");
    std::string process = getPropertyValue("Process");
    std::string detectorNames = getPropertyValue("DetectorNames");

    // Mantid::API::MatrixWorkspace_sptr transmissionSample =
    // getPropertyValue("Transmission");
    // Mantid::API::MatrixWorkspace_sptr transmissionCan =
    // getPropertyValue("TransmissionCan");

    // Remove the file if it already exists
    if (Poco::File(filename).exists()) {
        Poco::File(filename).remove();
    }

    H5::H5File file(filename, H5F_ACC_EXCL);

    const std::string suffix("1");
    const std::string version("1.0");

    // Add a new entry
    auto sasEntry = addSasEntry(file, workspace, version, suffix);

    // Add data the data
    addData(sasEntry, workspace, suffix);

    // Add the instrument information
    addInstrument(sasEntry, workspace, radiationSource, suffix);

    // Add the process information
    addProcess(sasEntry, workspace, process, suffix);

    // Add the transmission --> HOW?
}

} // namespace DataHandling
} // namespace Mantid
