#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/MDUnit.h"

#include <boost/make_shared.hpp>
#include <H5Cpp.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;

namespace
{

template <typename NumT>
void writeArray1DWithStrAttributes(H5::Group &group, const std::string &name,
                                   const std::vector<NumT> &values, std::map<std::string,std::string> attributes) {
#if 0
  DataType dataType(getType<NumT>());
  DataSpace dataSpace = getDataSpace(values);

  DSetCreatPropList propList = setCompressionAttributes(values.size());

  auto data = group.createDataSet(name, dataType, dataSpace, propList);

  for (const auto& attribute : attributes) {
    data.createAttribute(attribute.first, attribute.second);
  }
  data.write(&(values[0]), dataType);
#endif

void writeWithStrAttr(Group &group, const std::string &name, const std::string &value, std::map<std::string,std::string> attribute) {
#if 0
   StrType dataType(0, value.length() + 1);
   DataSpace dataSpace = getDataSpace(1);
   H5::DataSet data = group.createDataSet(name, dataType, dataSpace);

   for (const auto& attribute : attributes) {
     data.createAttribute(attribute.first, attribute.second);
   }

   data.write(value, dataType);
#endif
}
}


std::vector<std::string> splitDetectorNames(std::string detectorNames) {
  const std::string delimiter = ",";
  std::vector<std::string> detectors;
  size_t pos(0);
  std::string detectorName;
  while ((pos = detectorNames.find(delimiter)) != std::string::npos) {
      detectorName = detectorNames.substr(0, pos);
      boost::algorithm::trim(detectorName);
      detectors.push_back(detectorName);
      detectorNames.erase(0, pos + delimiter.length());
  }
  // Push remaining element
  detectors.push_back(boost::algorithm::trim(detectorNames));
  return detectors;
}

//------- SASentry
/**
 * Add the sasEntry to the sasroot.
 * @param file: Handle to the NXcanSAS file
 * @param workspace: the workspace to store
 * @param suffix: the suffix of the current entry
 * @return the sasEntry
 */
H5::Group addSasEntry(H5::H5File &file,
                      Mantid::API::MatrixWorkspace_sptr workspace, const std::string &suffix)
{
    const std::string sasEntryName = sasEntryName + suffix;
    // auto sasEntry = H5Util::createGroupNXS(file, sasEntryName, sasEntryClassAttr);

    // H5Util::writeStrAttribute(sasEntry, sasEntryVersionAttr, sasEntryVersionAttrValue);

    // H5Util::write(process, sasEntryDefinition, sasEntryDefinitionFormat);

    // auto workspaceName = workspace->name();
    // H5Util::write(group, sasEntryTitle, workspaceName);

    // const auto runNumber = workspace->getRunNumber();
    // H5Util::write(process, sasEntryRun, runNumber);

    auto sasEntry = file.createGroup(sasEntryName);
    return sasEntry;
}


//------- SASinstrument
std::string getInstrumentName(Mantid::API::MatrixWorkspace_sptr workspace) {
  auto instrument = workspace->getInstrument();
  return instrument->getFullName();
}

std::string
getIDF(Mantid::API::MatrixWorkspace_sptr workspace) {
    const auto instrument = workspace->getInstrument();
    //instrument->getFileName();
    // TODO get file name
    return "";
    //return instrument->getFileName();
}


void addDetectors(H5::Group &group,
                  Mantid::API::MatrixWorkspace_sptr workspace,
                  const std::string &radiationSource,
                  std::vector<std::string>& detectorNames,
                  const std::string &suffix) {
  // If the group is empty then have a dummy instrument name
  if (detectorNames.empty()) {
    const std::string sasDetectorName = sasInstrumentDetectorGroupName + suffix;
    // H5Util::createGroupNXS(group, sasDetectorName, sasInstrumentDetectorClassAttr);
    // H5Util::write(group, sasInstrumentDetectorName, "");
  } else {
    for (const auto& detectorName : detectorNames) {
      const std::string sasDetectorName = sasInstrumentDetectorGroupName + suffix + detectorName;
      // H5Util::createGroupNXS(group, sasDetectorName, sasInstrumentDetectorClassAttr);

      auto instrument = workspace->getInstrument();
      auto component = instrument->getComponentByName(detectorName);

      if (component) {
        const auto sample= instrument->getSample();
        const auto distance = component->getDistance(*sample);
        std::map<std::string, std::string> sddAttributes;
        sddAtrributes.insert(std::make_pair(sasUnitAttr, sasInstrumentDetectorSddUnitAttrValue));
        // H5Util::write(group, sasInstrumentDetectorName, detectorName);
        // H5Util::writeWithStrAttr(group, sasInstrumentDetectorSdd, std::toString(distance), sddAttributes);
      } else {
        // H5Util::write(group, sasInstrumentDetectorName, "");
      }
    }
  }
}

/**
 * Add the instrument group to the NXcanSAS file. This adds the
 * instrument name and the IDF
 * @param group: the sasEntry
 * @param workspace: the workspace which is being stored
 * @param radiationSource: the selcted radiation source
 * @param detectorNames: the names of the detectors to store
 * @param suffix: the suffix of the current entry
 */
void addInstrument(H5::Group &group,
                   Mantid::API::MatrixWorkspace_sptr workspace,
                   const std::string &radiationSource,
                   std::vector<std::string>& detectorNames,
                   const std::string &suffix) {
  // Setup instrument
  const std::string sasInstrumentName = sasInstrumentGroupName + suffix;
  //auto instrument = H5Util::createGroupNXS(group, sasInstrumentName, sasInstrumentClassAttr);
  auto instrumentName = getInstrumentName(workspace);
  // H5Util::write(instrument, sasInstrumentName, instrumentName);

  // Setup collimation (is blank)
  const std::string sasCollimationName = sasInstrumentCollimationGroupName + suffix;
  //H5Util::createGroupNXS(instrument, sasCollimationName, sasInstrumentCollimationClassAttr);

  // Setup the detector TODO: change group to instrument
  addDetectors(group, workspace, detectorNames, suffix);

  // Setup source
  const std::string sasSourceName = sasInstrumentSourceGroupName + suffix;
  // auto source = H5Util::createGroupNXS(instrument, sasSourceName, sasInstrumentSourceClassAttr);
  // H5Util::write(source, sasInstrumentSourceRadiation, radiationSource);


  // Don't do it now
    //auto idf = getIDF(workspace);
    //// H5Util::write(instrument, "idf", idf);
}


//------- SASprocess
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

enum class WorkspaceDimensionality { oneD, twoD, other};

WorkspaceDimensionality getWorkspaceDimensionality(Mantid::API::MatrixWorkspace_sptr workspace) {
  auto numberOfHistograms = workspace->getNumberHistograms();
  WorkspaceDimensionality dimensionality(WorkspaceDimensionality::other);
  if (numberOfHistograms == 1) {
    dimensionality = WorkspaceDimensionality::oneD;
  } else if (numberOfHistograms > 1) {
    dimensionality = WorkspaceDimensionality::twoD;
  }
  return dimensionality;
}


//------- SASdata
void addData1D(H5::Group &data,Mantid::API::MatrixWorkspace_sptr workspace, const std::string& radiationSource) {
  // Add attributes for @I_axes and @Q_indices
  //H5Util::writeStrAttribute(data, "I_axes", "Q");
  //H5Util::writeStrAttribute(data, "Q_indices", "0");

  // Add Q with units  + uncertainty definition
  const auto qValue = workspace->readX(0);
  std::map<std::string, std::string> qAttributes;

  auto qDimension = workspace->getDimension(0);
  auto qUnitLabel = qDimension->getMDUnits().getUnitLabel();
  auto qUnit = qUnitLabel.ascii();
  qAttributes.insert(std::make_pair(Mantid::DataHandling::SaveNXcanSAS::unitTag, qUnit));

  if (workspace->hasDx(0)) {
    qAttributes.insert(std::make_pair(Mantid::DataHandling::SaveNXcanSAS::uncertaintyTag, Mantid::DataHandling::SaveNXcanSAS::qDevTag));
  }

  writeArray1DWithStrAttributes(data, Mantid::DataHandling::SaveNXcanSAS::qTag, qValue, qAttributes);

  // Add I with units + uncertainty definition
  const auto intensity = workspace->readY(0);
  //H5Util::writeArray1D(data, "I", intensity);

  // Add Idev with units
  const auto intensityUncertainty = workspace->readE(0);
  //H5Util::writeArray1D(data, "Idev", intensityUncertainty);

  // Add Qdev with units if available
  if (workspace->hasDx(0)) {
    const auto qResolution = workspace->readDx(0);
  }

  // Add probe type
  // H5Util::write(data, "probe_type", radiationSource);
}

void addData2D(H5::Group &data, Mantid::API::MatrixWorkspace_sptr workspace, const std::string& radiationSource) {
}

void addData(H5::Group &group, Mantid::API::MatrixWorkspace_sptr workspace,
             const std::string& radiationSource,
             const std::string &suffix){
  const std::string sasDataName = "sasdata" + suffix;
  auto data = group.createGroup(sasDataName);

  auto workspaceDimensionality = getWorkspaceDimensionality(workspace);
  switch(workspaceDimensionality) {
    case(WorkspaceDimensionality::oneD):
      addData1D(data, workspace, radiationSource);
      break;
    case(WorkspaceDimensionality::twoD):
      addData2D(data, workspace, radiationSource);
      break;
    default:
      throw std::runtime_error("SaveNXcanSAS: The provided workspace dimensionality is not 1D or 2D.");
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

    // Units have to be momentum transfer


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

    const std::string suffix("01");

    // Add a new entry
    auto sasEntry = addSasEntry(file, workspace, suffix);

    // Add the data
    addData(sasEntry, workspace, radiationSource, suffix);

    // Add the instrument information
    detectors = splitDetectorNames(detectorNames);
    addInstrument(sasEntry, workspace, radiationSource, detectors, suffix);

    // Add the process information
    addProcess(sasEntry, workspace, suffix);

    // Add the transmission
}

} // namespace DataHandling
} // namespace Mantid
