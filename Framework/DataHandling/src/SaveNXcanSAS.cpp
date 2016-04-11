#include "MantidAPI/Axis.h"
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
using namespace H5;

namespace {
/// static logger object
Mantid::Kernel::Logger g_log("H5Util");
const std::string NX_ATTR_CLASS("NX_class");
}

namespace Mantid {
namespace DataHandling {
namespace H5Util {
  // -------------------------------------------------------------------
  // convert primitives to HDF5 enum
  // -------------------------------------------------------------------

  template <typename NumT> DataType getType() { throw DataTypeIException(); }

  template <> MANTID_DATAHANDLING_DLL DataType getType<float>() {
    return PredType::NATIVE_FLOAT;
  }

  template <> MANTID_DATAHANDLING_DLL DataType getType<double>() {
    return PredType::NATIVE_DOUBLE;
  }

  template <> MANTID_DATAHANDLING_DLL DataType getType<int32_t>() {
    return PredType::NATIVE_INT32;
  }

  template <> MANTID_DATAHANDLING_DLL DataType getType<uint32_t>() {
    return PredType::NATIVE_UINT32;
  }

  template <> MANTID_DATAHANDLING_DLL DataType getType<int64_t>() {
    return PredType::NATIVE_INT64;
  }

  template <> MANTID_DATAHANDLING_DLL DataType getType<uint64_t>() {
    return PredType::NATIVE_UINT64;
  }

  // -------------------------------------------------------------------
  // write methods
  // -------------------------------------------------------------------

  Group createGroupNXS(H5File &file, const std::string &name,
                       const std::string &nxtype) {
    auto group = file.createGroup(name);
    writeStrAttribute(group, NX_ATTR_CLASS, nxtype);
    return group;
  }

  Group createGroupNXS(Group &group, const std::string &name,
                       const std::string &nxtype) {
    auto outGroup = group.createGroup(name);
    writeStrAttribute(outGroup, NX_ATTR_CLASS, nxtype);
    return outGroup;
  }

  DataSpace getDataSpace(const size_t length) {
    hsize_t dims[] = {length};
    return DataSpace(1, dims);
  }

  template <typename NumT> DataSpace getDataSpace(const std::vector<NumT> &data) {
    return H5Util::getDataSpace(data.size());
  }

  DSetCreatPropList setCompressionAttributes(const std::size_t length,
                                             const int deflateLevel) {
    DSetCreatPropList propList;
    hsize_t chunk_dims[1] = {length};
    propList.setChunk(1, chunk_dims);
    propList.setDeflate(deflateLevel);
    return propList;
  }

  void writeStrAttribute(Group &location, const std::string &name,
                         const std::string &value) {
    StrType attrType(0, H5T_VARIABLE);
    DataSpace attrSpace(H5S_SCALAR);
    auto groupAttr = location.createAttribute(name, attrType, attrSpace);
    groupAttr.write(attrType, value);
  }

  void write(Group &group, const std::string &name, const std::string &value) {
    StrType dataType(0, value.length() + 1);
    DataSpace dataSpace = getDataSpace(1);
    H5::DataSet data = group.createDataSet(name, dataType, dataSpace);
    data.write(value, dataType);
  }

  void writeWithStrAttr(H5::Group &group, const std::string &name, const std::string &value, const std::map<std::string,std::string>& attributes) {
     StrType dataType(0, value.length() + 1);
     DataSpace dataSpace = getDataSpace(1);
     H5::DataSet data = group.createDataSet(name, dataType, dataSpace);

     for (const auto& attribute : attributes) {
      // TODO: Add attribute here
     }

     data.write(value, dataType);
  }

  template <typename NumT>
  void writeArray1D(Group &group, const std::string &name,
                    const std::vector<NumT> &values) {
    DataType dataType(getType<NumT>());
    DataSpace dataSpace = getDataSpace(values);

    DSetCreatPropList propList = setCompressionAttributes(values.size());

    auto data = group.createDataSet(name, dataType, dataSpace, propList);
    data.write(&(values[0]), dataType);
  }

  template <typename NumT>
  void writeArray1DWithStrAttributes(H5::Group &group, const std::string &name,
                                     const std::vector<NumT> &values, std::map<std::string,std::string> attributes) {
    DataType dataType(getType<NumT>());
    DataSpace dataSpace = getDataSpace(values);

    DSetCreatPropList propList = setCompressionAttributes(values.size());

    auto data = group.createDataSet(name, dataType, dataSpace, propList);

    for (const auto& attribute : attributes) {
      //data.createAttribute(attribute.first, attribute.second);
    }
    data.write(&(values[0]), dataType);
  }




  // -------------------------------------------------------------------
  // instantiations for writeArray1D
  // -------------------------------------------------------------------
  template MANTID_DATAHANDLING_DLL void
  writeArray1D(H5::Group &group, const std::string &name,
               const std::vector<float> &values);
  template MANTID_DATAHANDLING_DLL void
  writeArray1D(H5::Group &group, const std::string &name,
               const std::vector<double> &values);
  template MANTID_DATAHANDLING_DLL void
  writeArray1D(H5::Group &group, const std::string &name,
               const std::vector<int32_t> &values);
  template MANTID_DATAHANDLING_DLL void
  writeArray1D(H5::Group &group, const std::string &name,
               const std::vector<uint32_t> &values);
  template MANTID_DATAHANDLING_DLL void
  writeArray1D(H5::Group &group, const std::string &name,
               const std::vector<int64_t> &values);
  template MANTID_DATAHANDLING_DLL void
  writeArray1D(H5::Group &group, const std::string &name,
               const std::vector<uint64_t> &values);

  // -------------------------------------------------------------------
  // instantiations for getDataSpace
  // -------------------------------------------------------------------
  template MANTID_DATAHANDLING_DLL DataSpace
  getDataSpace(const std::vector<float> &data);
  template MANTID_DATAHANDLING_DLL DataSpace
  getDataSpace(const std::vector<double> &data);
  template MANTID_DATAHANDLING_DLL DataSpace
  getDataSpace(const std::vector<int32_t> &data);
  template MANTID_DATAHANDLING_DLL DataSpace
  getDataSpace(const std::vector<uint32_t> &data);
  template MANTID_DATAHANDLING_DLL DataSpace
  getDataSpace(const std::vector<int64_t> &data);
  template MANTID_DATAHANDLING_DLL DataSpace
  getDataSpace(const std::vector<uint64_t> &data);

}
}
}


namespace
{



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
  boost::algorithm::trim(detectorNames);
  detectors.push_back(detectorNames);
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
                      Mantid::API::MatrixWorkspace_sptr workspace,
                      const std::string &suffix)
{
    const std::string sasEntryName = sasEntryName + suffix;
    auto sasEntry
        = Mantid::DataHandling::H5Util::createGroupNXS(file, sasEntryName, sasEntryClassAttr);

    // Add version
    Mantid::DataHandling::H5Util::writeStrAttribute(sasEntry, sasEntryVersionAttr,
                              sasEntryVersionAttrValue);

    // Add definition
    Mantid::DataHandling::H5Util::write(sasEntry, sasEntryDefinition, sasEntryDefinitionFormat);

    // Add title
    auto workspaceName = workspace->name();
    Mantid::DataHandling::H5Util::write(sasEntry, sasEntryTitle, workspaceName);

    // Add run
    const auto runNumber = workspace->getRunNumber();
    Mantid::DataHandling::H5Util::write(sasEntry, sasEntryRun, std::to_string(runNumber));

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
                  const std::vector<std::string>& detectorNames,
                  const std::string &suffix) {
  // If the group is empty then don't add anything
  if (!detectorNames.empty()) {
    for (const auto& detectorName : detectorNames) {
      const std::string sasDetectorName = sasInstrumentDetectorGroupName + suffix + detectorName;
      auto instrument = workspace->getInstrument();
      auto component = instrument->getComponentByName(detectorName);

      if (component) {
        const auto sample= instrument->getSample();
        const auto distance = component->getDistance(*sample);
        std::map<std::string, std::string> sddAttributes;
        sddAttributes.insert(std::make_pair(sasUnitAttr, sasInstrumentDetectorSddUnitAttrValue));
        auto detector = Mantid::DataHandling::H5Util::createGroupNXS(group, sasDetectorName, sasInstrumentDetectorClassAttr);
        Mantid::DataHandling::H5Util::write(detector, sasInstrumentDetectorName, detectorName);
        Mantid::DataHandling::H5Util::writeWithStrAttr(detector, sasInstrumentDetectorSdd, std::to_string(distance), sddAttributes);
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
                   const std::vector<std::string>& detectorNames,
                   const std::string &suffix) {
  // Setup instrument
  const std::string sasInstrumentName = sasInstrumentGroupName + suffix;
  auto instrument = Mantid::DataHandling::H5Util::createGroupNXS(group, sasInstrumentName, sasInstrumentClassAttr);
  auto instrumentName = getInstrumentName(workspace);
  Mantid::DataHandling::H5Util::write(instrument, sasInstrumentName, instrumentName);

  // Setup collimation (is blank)
  const std::string sasCollimationName = sasInstrumentCollimationGroupName + suffix;
  Mantid::DataHandling::H5Util::createGroupNXS(instrument, sasCollimationName, sasInstrumentCollimationClassAttr);

  // Setup the detector
  addDetectors(instrument, workspace, detectorNames, suffix);

  // Setup source
  const std::string sasSourceName = sasInstrumentSourceGroupName + suffix;
  auto source = Mantid::DataHandling::H5Util::createGroupNXS(instrument, sasSourceName, sasInstrumentSourceClassAttr);
  Mantid::DataHandling::H5Util::write(source, sasInstrumentSourceRadiation, radiationSource);

  // TODO Don't do it now
    //auto idf = getIDF(workspace);
    //Mantid::DataHandling::H5Util::write(instrument, sasInstrumentIDF, idf);
}


//------- SASprocess

std::string getDate() {
  time_t rawtime;
  time(&rawtime);
  char temp[25];
  strftime(temp, 25, "%d-%b-%Y %H:%M:%S", localtime(&rawtime));
  std::string sasDate(temp);
  return sasDate;
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
  // Setup process
  const std::string sasProcessName = sasProcessGroupName + suffix;
  auto process = Mantid::DataHandling::H5Util::createGroupNXS(group, sasProcessName, sasProcessClassAttr);

  // Add name
  Mantid::DataHandling::H5Util::write(process, sasProcessName, sasProcessNameValue);

  // Add creation date of the file
  auto date = getDate();
  Mantid::DataHandling::H5Util::write(process, sasProcessDate, date);

  // Add Mantid version
  const auto version = std::string(MantidVersion::version());
  Mantid::DataHandling::H5Util::write(process, sasProcessTermSvn, version);


  const auto run = workspace->run();
  if (run.hasProperty(sasProcessUserFileInLogs)) {
      // TODO
      //const std::string& userFile = run.getPropertyValue(sasProcessUserFileInLogs);
      //Mantid::DataHandling::H5Util::write(process, sasProcessTermUserFile, userFile);
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

std::string getIntensityUnitLabel(std::string intensityUnitLabel) {
  if (intensityUnitLabel == "I(q) (cm-1)") {
    return "1/cm";
  } else {
    return intensityUnitLabel;
  }
}

std::string getUnitFromMDDimension(Mantid::Geometry::IMDDimension_const_sptr dimension) {
  const auto unitLabel = dimension->getMDUnits().getUnitLabel();
  return unitLabel.ascii();
}


void addData1D(H5::Group &data,Mantid::API::MatrixWorkspace_sptr workspace) {
  // Add attributes for @signal, @I_axes, @Q_indices,
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasSignal, sasDataI);
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataIAxesAttr, sasDataQ);
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataIUncertaintyAttr, sasDataIdev);
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataQIndicesAttr, "0");
  if (workspace->hasDx(0)) {
    Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataQUncertaintyAttr, sasDataQdev);
  }

  //-----------------------------------------
  // Add Q with units  + uncertainty definition
  const auto qValue = workspace->readX(0);
  std::map<std::string, std::string> qAttributes;
  const auto qUnit = getUnitFromMDDimension(workspace->getDimension(0));
  qAttributes.insert(std::make_pair(sasUnitAttr, qUnit));
  if (workspace->hasDx(0)) {
    qAttributes.insert(std::make_pair(sasUncertaintyAttr, sasDataQdev));
  }

  Mantid::DataHandling::H5Util::writeArray1DWithStrAttributes(data, sasDataQ, qValue, qAttributes);


  //-----------------------------------------
  // Add I with units + uncertainty definition
  const auto intensity = workspace->readY(0);
  std::map<std::string, std::string> iAttributes;
  auto iUnit = getUnitFromMDDimension(workspace->getYDimension());
  iUnit = getIntensityUnitLabel(iUnit);
  iAttributes.insert(std::make_pair(sasUnitAttr, iUnit));
  iAttributes.insert(std::make_pair(sasUncertaintyAttr, sasDataIdev));

  Mantid::DataHandling::H5Util::writeArray1DWithStrAttributes(data, sasDataI, intensity, iAttributes);


  //-----------------------------------------
  // Add Idev with units
  const auto intensityUncertainty = workspace->readE(0);
  std::map<std::string, std::string> eAttributes;
  eAttributes.insert(std::make_pair(sasUnitAttr, iUnit)); // same units as intensity

  Mantid::DataHandling::H5Util::writeArray1DWithStrAttributes(data, sasDataIdev, intensityUncertainty, eAttributes);


  //-----------------------------------------
  // Add Qdev with units if available
  if (workspace->hasDx(0)) {
    const auto qResolution = workspace->readDx(0);
    std::map<std::string, std::string> xUncertaintyAttributes;
    xUncertaintyAttributes.insert(std::make_pair(sasUnitAttr, qUnit));

    Mantid::DataHandling::H5Util::writeArray1DWithStrAttributes(data, sasDataQdev, qResolution, xUncertaintyAttributes);
  }
}


bool areAxesNumeric(Mantid::API::MatrixWorkspace_sptr workspace)
{
    const unsigned indices[] = {0, 1};
    for (const auto index : indices) {
        auto axis = workspace->getAxis(index);
        if (!axis->isNumeric()) {
            return false;
        }
    }
    return true;
}


enum class StoreType{Qx, Qy, I, Idev, Other};

int storeTypeToInt(StoreType type) {
  int value(-1);
  switch(type) {
    case(Qx):
      value = 0;
      break;
    case(Qy):
      value = 1;
      break;
    default:
      value = -1;
  }

  return value;
}

/**
 * Stores the 2D data in the HDF5 file. Qx and Qy values need to be stored as a meshgrid.
 * They should be stored as point data.
 * @param data: the hdf5 group
 * @param workspace: the workspace to store
 * @param type: the type of data we want to store
 *
 * Workspace looks like this in Mantid Matrix
 *    (Qx)  0       1          2     ...   M   (first dimension)
 * (QY)
 *  0    IQx0Qy0  IQx1Qy0   IQx2Qy0  ...  IQxMQy0
 *  1    IQx0Qy1  IQx1Qy1   IQx2Qy1  ...  IQxMQy1
 *  2    IQx0Qy2  IQx1Qy2   IQx2Qy2  ...  IQxMQy2
 *  3    IQx0Qy3  IQx1Qy3   IQx2Qy3  ...  IQxMQy3
 *  .
 *  .
 *  N    IQx0QyN  IQx1QyN   IQx2QyN  ...  IQxMQyN
 *  (second dimension)
 *
 * The layout below is how it would look like in the HDFView, ie vertical axis is first dimension
 *
 * In HDF5 the Qx would need to be stored as:
 * Qx1 Qx1 ... Qx1 : N times
 * Qx2 Qx2 ... Qx2 : N times
 * Qx3 Qx3 ... Qx3 : N times
 *  .
 *  .
 * QxM QxM ... QxM : N times
 *
 * In HDF5 the Qy would need to be stored as:
 * Qy1 Qy2 ... QyN
 * Qy1 Qy2 ... QyN
 * Qy1 Qy2 ... QyN
 *  .
 *  .
 * Qx1 Qx2 ... QxN
 * M times
 *
 *
 * In HDF5 the I would need to be stored as:
 * Qy1 Qy2 ... QyN
 * Qy1 Qy2 ... QyN
 * Qy1 Qy2 ... QyN
 *
 */
void write2Ddata(H5::Group &data, Mantid::API::MatrixWorkspace_sptr workspace, StoreType type) {
  const auto isHistogram = workspace->isHistogramData();
  const auto maxDim0 = workspace->blocksize();
  const auto maxDim1 = workspace->getNumberHistograms();

  auto index = storeTypeToInt(type);

  switch(type) {
    case(StoreType::Qx):
      auto qX = workspace->readX(0);
      const size_t otherDimension = maxDim1;
      //write2DqValue(data);
      break;
    case(StoreType::Qx):
      auto qX = workspace->readX(0);
      const size_t otherDimension = maxDim1;
      //write2DqValue(data);
      break;
    case(StoreType::I):
      break;
    case(StoreType::Idev):
      break;
    default:
      std::invalid_argument("SaveNXcanSAS: Cannot handle the supplied the data type. Currently only Qx, Qy, I and Idev can be handled for 2D data.");
  }
}


void addData2D(H5::Group &data, Mantid::API::MatrixWorkspace_sptr workspace) {
  if (!areAxesNumeric(workspace)) {
    std::invalid_argument("SaveNXcanSAS: The provided 2D workspace needs to have 2 numeric axes.");
  }

  // Add attributes for @signal, @I_axes, @Q_indices,
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasSignal, sasDataI);
  const std::string sasDataIAxesAttr2D = sasDataQ + sasSeparator + sasDataQ;
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataIAxesAttr, sasDataIAxesAttr2D);
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataIUncertaintyAttr, sasDataIdev);
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataQIndicesAttr, "0,1");

  // Store the 2D Qx data
  write2Ddata(data, workspace, StoreType::Qx);

  // Get 2D Qy data and store it
  write2Ddata(data, workspace, StoreType::Qy);

  // Get 2D I data and store it
  write2Ddata(data, workspace, StoreType::I);
}

void addData(H5::Group &group, Mantid::API::MatrixWorkspace_sptr workspace,
             const std::string &suffix){
  const std::string sasDataName = sasDataClassGroupName + suffix;
  auto data = group.createGroup(sasDataName);

  auto workspaceDimensionality = getWorkspaceDimensionality(workspace);
  switch(workspaceDimensionality) {
    case(WorkspaceDimensionality::oneD):
      addData1D(data, workspace);
      break;
    case(WorkspaceDimensionality::twoD):
      addData2D(data, workspace);
      break;
    default:
      throw std::runtime_error("SaveNXcanSAS: The provided workspace dimensionality is not 1D or 2D.");
  } 
}

//------- SAStransmission_spectrum
void addTransmission(H5::Group &group, Mantid::API::MatrixWorkspace_const_sptr workspace, std::string transmissionName, std::string suffix) {
  // Setup process
  const std::string sasTransmissionName = sasTransmissionSpectrumGroupName + suffix;
  auto transmission = Mantid::DataHandling::H5Util::createGroupNXS(group, sasTransmissionName, sasTransmissionSpectrumClassAttr);

  // Add attributes for @signal, @T_axes, @T_indices, @T_uncertainty, @name, @timestamp
  Mantid::DataHandling::H5Util::writeStrAttribute(transmission, sasSignal, sasTransmissionSpectrumT);
  Mantid::DataHandling::H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumTIndies, sasTransmissionSpectrumT);
  Mantid::DataHandling::H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumTUncertainty, sasTransmissionSpectrumTdev);
  Mantid::DataHandling::H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumNameAttr, transmissionName);

  auto date = getDate();
  Mantid::DataHandling::H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumTimeStampAttr, date);


  //-----------------------------------------
  // Add T with units + uncertainty definition
  const auto transmissionData = workspace->readY(0);
  std::map<std::string, std::string> transmissionAttributes;
  const std::string unit = "";
  transmissionAttributes.insert(std::make_pair(sasUnitAttr, unit));
  transmissionAttributes.insert(std::make_pair(sasUncertaintyAttr, sasTransmissionSpectrumTdev));

  Mantid::DataHandling::H5Util::writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumT, transmissionData, transmissionAttributes);


  //-----------------------------------------
  // Add Tdev with units
  const auto transmissionErrors = workspace->readE(0);
  std::map<std::string, std::string> transmissionErrorAttributes;
  transmissionErrorAttributes.insert(std::make_pair(sasUnitAttr, unit));

  Mantid::DataHandling::H5Util::writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumTdev, transmissionErrors, transmissionErrorAttributes);


  //-----------------------------------------
  // Add lambda with units
  const auto lambda = workspace->readX(0);
  std::map<std::string, std::string> lambdaAttributes;
  const auto lambdaUnit = getUnitFromMDDimension(workspace->getDimension(0));
  transmissionErrorAttributes.insert(std::make_pair(sasUnitAttr, lambdaUnit));

  Mantid::DataHandling::H5Util::writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumLambda, lambda, lambdaAttributes);
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

   // Mantid::API::MatrixWorkspace_const_sptr transmissionSample = getPropertyValue("Transmission");
   // Mantid::API::MatrixWorkspace_const_sptr transmissionCan = getPropertyValue("TransmissionCan");

    // Remove the file if it already exists
    if (Poco::File(filename).exists()) {
        Poco::File(filename).remove();
    }

    H5::H5File file(filename, H5F_ACC_EXCL);

    const std::string suffix("01");

    // Add a new entry
    auto sasEntry = addSasEntry(file, workspace, suffix);

    // Add the data
    addData(sasEntry, workspace, suffix);

    // Add the instrument information
    const auto detectors = splitDetectorNames(detectorNames);
    addInstrument(sasEntry, workspace, radiationSource, detectors, suffix);

    // Add the process information
    addProcess(sasEntry, workspace, suffix);
#if 0
    // Add the transmissions for sample and can
    if (transmissionSample) {
      addTransmission(sasEntry, transmissionSample, sasTransmissionSpectrumNameSampleAttrValue, suffix);
    }

    if (transmissionCan) {
      addTransmission(sasEntry, transmissionCan, sasTransmissionSpectrumNameCanAttrValue, suffix);
    }
#endif
}

} // namespace DataHandling
} // namespace Mantid
