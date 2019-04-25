// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/make_unique.h"

#include <H5Cpp.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>

#include <Poco/File.h>
#include <Poco/Path.h>
#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;

namespace {

enum class StoreType { Qx, Qy, I, Idev, Other };

bool isCanSASCompliant(bool isStrict, const std::string &input) {
  auto baseRegex = isStrict ? boost::regex("[a-z_][a-z0-9_]*")
                            : boost::regex("[A-Za-z_][\\w_]*");
  return boost::regex_match(input, baseRegex);
}

void removeSpecialCharacters(std::string &input) {
  boost::regex toReplace("[-\\.]");
  std::string replaceWith("_");
  input = boost::regex_replace(input, toReplace, replaceWith);
}

std::string
makeCompliantName(const std::string &input, bool isStrict,
                  std::function<void(std::string &)> captializeStrategy) {
  auto output = input;
  // Check if input is compliant
  if (!isCanSASCompliant(isStrict, output)) {
    removeSpecialCharacters(output);
    captializeStrategy(output);
    // Check if the changes have made it compliant
    if (!isCanSASCompliant(isStrict, output)) {
      std::string message = "SaveNXcanSAS: The input " + input +
                            "is not compliant with the NXcanSAS format.";
      throw std::runtime_error(message);
    }
  }
  return output;
}

template <typename NumT>
void writeArray1DWithStrAttributes(
    H5::Group &group, const std::string &dataSetName,
    const std::vector<NumT> &values,
    const std::map<std::string, std::string> &attributes) {
  Mantid::DataHandling::H5Util::writeArray1D(group, dataSetName, values);
  auto dataSet = group.openDataSet(dataSetName);
  for (const auto &attribute : attributes) {
    Mantid::DataHandling::H5Util::writeStrAttribute(dataSet, attribute.first,
                                                    attribute.second);
  }
}

H5::DSetCreatPropList setCompression2D(const hsize_t *chunkDims,
                                       const int deflateLevel = 6) {
  H5::DSetCreatPropList propList;
  const int rank = 2;
  propList.setChunk(rank, chunkDims);
  propList.setDeflate(deflateLevel);
  return propList;
}

template <typename Functor>
void write2DWorkspace(H5::Group &group,
                      Mantid::API::MatrixWorkspace_sptr workspace,
                      const std::string &dataSetName, Functor func,
                      const std::map<std::string, std::string> &attributes) {
  using namespace Mantid::DataHandling::H5Util;

  // Set the dimension
  const size_t dimension0 = workspace->getNumberHistograms();
  const size_t dimension1 = workspace->y(0).size();
  const hsize_t rank = 2;
  hsize_t dimensionArray[rank] = {static_cast<hsize_t>(dimension0),
                                  static_cast<hsize_t>(dimension1)};

  // Start position in the 2D data (indexed) data structure
  hsize_t start[rank] = {0, 0};

  // Size of a slab
  hsize_t sizeOfSingleSlab[rank] = {1, dimensionArray[1]};

  // Get the Data Space definition for the 2D Data Set in the file
  auto fileSpace = H5::DataSpace(rank, dimensionArray);
  H5::DataType dataType(getType<double>());

  // Get the proplist with compression settings
  H5::DSetCreatPropList propList = setCompression2D(sizeOfSingleSlab);

  // Create the data set
  auto dataSet =
      group.createDataSet(dataSetName, dataType, fileSpace, propList);

  // Create Data Spae for 1D entry for each row in memory
  hsize_t memSpaceDimension[1] = {dimension1};
  H5::DataSpace memSpace(1, memSpaceDimension);

  // Insert each row of the workspace as a slab
  for (unsigned int index = 0; index < dimension0; ++index) {
    // Need the data space
    fileSpace.selectHyperslab(H5S_SELECT_SET, sizeOfSingleSlab, start);

    // Write the correct data set to file
    dataSet.write(func(workspace, index), dataType, memSpace, fileSpace);
    // Step up the write position
    ++start[0];
  }

  // Add attributes to data set
  for (const auto &attribute : attributes) {
    writeStrAttribute(dataSet, attribute.first, attribute.second);
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
  boost::algorithm::trim(detectorNames);
  detectors.push_back(detectorNames);
  return detectors;
}

//------- SASentry
/**
 * Add the sasEntry to the sasroot.
 * @param file: Handle to the NXcanSAS file
 * @param workspace: the workspace to store
 * @return the sasEntry
 */
H5::Group addSasEntry(H5::H5File &file,
                      Mantid::API::MatrixWorkspace_sptr workspace,
                      const std::string &suffix) {
  using namespace Mantid::DataHandling::NXcanSAS;
  const std::string sasEntryName = sasEntryGroupName + suffix;
  auto sasEntry = Mantid::DataHandling::H5Util::createGroupCanSAS(
      file, sasEntryName, nxEntryClassAttr, sasEntryClassAttr);

  // Add version
  Mantid::DataHandling::H5Util::writeStrAttribute(sasEntry, sasEntryVersionAttr,
                                                  sasEntryVersionAttrValue);

  // Add definition
  Mantid::DataHandling::H5Util::write(sasEntry, sasEntryDefinition,
                                      sasEntryDefinitionFormat);

  // Add title
  auto workspaceTitle = workspace->getTitle();
  Mantid::DataHandling::H5Util::write(sasEntry, sasEntryTitle, workspaceTitle);

  // Add run
  const auto runNumber = workspace->getRunNumber();
  Mantid::DataHandling::H5Util::write(sasEntry, sasEntryRun,
                                      std::to_string(runNumber));

  return sasEntry;
}

//------- SASinstrument
std::string getInstrumentName(Mantid::API::MatrixWorkspace_sptr workspace) {
  auto instrument = workspace->getInstrument();
  return instrument->getFullName();
}

std::string getIDF(Mantid::API::MatrixWorkspace_sptr workspace) {
  auto date = workspace->getWorkspaceStartDate();
  auto instrumentName = getInstrumentName(workspace);
  return workspace->getInstrumentFilename(instrumentName, date);
}

void addDetectors(H5::Group &group, Mantid::API::MatrixWorkspace_sptr workspace,
                  const std::vector<std::string> &detectorNames) {
  // If the group is empty then don't add anything
  if (!detectorNames.empty()) {
    for (const auto &detectorName : detectorNames) {
      if (detectorName.empty()) {
        continue;
      }

      std::string sasDetectorName =
          sasInstrumentDetectorGroupName + detectorName;
      sasDetectorName =
          Mantid::DataHandling::makeCanSASRelaxedName(sasDetectorName);

      auto instrument = workspace->getInstrument();
      auto component = instrument->getComponentByName(detectorName);

      if (component) {
        const auto sample = instrument->getSample();
        const auto distance = component->getDistance(*sample);
        std::map<std::string, std::string> sddAttributes;
        sddAttributes.insert(
            std::make_pair(sasUnitAttr, sasInstrumentDetectorSddUnitAttrValue));
        auto detector = Mantid::DataHandling::H5Util::createGroupCanSAS(
            group, sasDetectorName, nxInstrumentDetectorClassAttr,
            sasInstrumentDetectorClassAttr);
        Mantid::DataHandling::H5Util::write(detector, sasInstrumentDetectorName,
                                            detectorName);
        Mantid::DataHandling::H5Util::writeScalarDataSetWithStrAttributes(
            detector, sasInstrumentDetectorSdd, distance, sddAttributes);
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
 */
void addInstrument(H5::Group &group,
                   Mantid::API::MatrixWorkspace_sptr workspace,
                   const std::string &radiationSource,
                   const std::vector<std::string> &detectorNames) {
  // Setup instrument
  const std::string sasInstrumentNameForGroup = sasInstrumentGroupName;
  auto instrument = Mantid::DataHandling::H5Util::createGroupCanSAS(
      group, sasInstrumentNameForGroup, nxInstrumentClassAttr,
      sasInstrumentClassAttr);
  auto instrumentName = getInstrumentName(workspace);
  Mantid::DataHandling::H5Util::write(instrument, sasInstrumentName,
                                      instrumentName);

  // Setup the detector
  addDetectors(instrument, workspace, detectorNames);

  // Setup source
  const std::string sasSourceName = sasInstrumentSourceGroupName;
  auto source = Mantid::DataHandling::H5Util::createGroupCanSAS(
      instrument, sasSourceName, nxInstrumentSourceClassAttr,
      sasInstrumentSourceClassAttr);
  Mantid::DataHandling::H5Util::write(source, sasInstrumentSourceRadiation,
                                      radiationSource);

  // Add IDF information
  auto idf = getIDF(workspace);
  Mantid::DataHandling::H5Util::write(instrument, sasInstrumentIDF, idf);
}

//------- SASprocess

std::string getDate() {
  time_t rawtime;
  time(&rawtime);
  char temp[25];
  strftime(temp, 25, "%Y-%m-%dT%H:%M:%S", localtime(&rawtime));
  std::string sasDate(temp);
  return sasDate;
}

/**
 * Add the process information to the NXcanSAS file. This information
 * about the run number, the Mantid version and the user file (if available)
 * @param group: the sasEntry
 * @param workspace: the workspace which is being stored
 */
void addProcess(H5::Group &group, Mantid::API::MatrixWorkspace_sptr workspace) {
  // Setup process
  const std::string sasProcessNameForGroup = sasProcessGroupName;
  auto process = Mantid::DataHandling::H5Util::createGroupCanSAS(
      group, sasProcessNameForGroup, nxProcessClassAttr, sasProcessClassAttr);

  // Add name
  Mantid::DataHandling::H5Util::write(process, sasProcessName,
                                      sasProcessNameValue);

  // Add creation date of the file
  auto date = getDate();
  Mantid::DataHandling::H5Util::write(process, sasProcessDate, date);

  // Add Mantid version
  const auto version = std::string(MantidVersion::version());
  Mantid::DataHandling::H5Util::write(process, sasProcessTermSvn, version);

  const auto run = workspace->run();
  if (run.hasProperty(sasProcessUserFileInLogs)) {
    auto userFileProperty = run.getProperty(sasProcessUserFileInLogs);
    auto userFileString = userFileProperty->value();
    Mantid::DataHandling::H5Util::write(process, sasProcessTermUserFile,
                                        userFileString);
  }
}

/**
 * Add the process information to the NXcanSAS file. This information
 * about the run number, the Mantid version and the user file (if available)
 * @param group: the sasEntry
 * @param workspace: the workspace which is being stored
 */
void addProcess(H5::Group &group, Mantid::API::MatrixWorkspace_sptr workspace,
                Mantid::API::MatrixWorkspace_sptr canWorkspace) {
  // Setup process
  const std::string sasProcessNameForGroup = sasProcessGroupName;
  auto process = Mantid::DataHandling::H5Util::createGroupCanSAS(
      group, sasProcessNameForGroup, nxProcessClassAttr, sasProcessClassAttr);

  // Add name
  Mantid::DataHandling::H5Util::write(process, sasProcessName,
                                      sasProcessNameValue);

  // Add creation date of the file
  auto date = getDate();
  Mantid::DataHandling::H5Util::write(process, sasProcessDate, date);

  // Add Mantid version
  const auto version = std::string(MantidVersion::version());
  Mantid::DataHandling::H5Util::write(process, sasProcessTermSvn, version);

  const auto run = workspace->run();
  if (run.hasProperty(sasProcessUserFileInLogs)) {
    auto userFileProperty = run.getProperty(sasProcessUserFileInLogs);
    auto userFileString = userFileProperty->value();
    Mantid::DataHandling::H5Util::write(process, sasProcessTermUserFile,
                                        userFileString);
  }

  // Add can run number
  const auto canRun = canWorkspace->getRunNumber();
  Mantid::DataHandling::H5Util::write(process, sasProcessTermCan,
                                      std::to_string(canRun));
}

WorkspaceDimensionality
getWorkspaceDimensionality(Mantid::API::MatrixWorkspace_sptr workspace) {
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
    return sasIntensity;
  } else {
    return intensityUnitLabel;
  }
}

std::string getIntensityUnit(Mantid::API::MatrixWorkspace_sptr workspace) {
  auto iUnit = workspace->YUnit();
  if (iUnit.empty()) {
    iUnit = workspace->YUnitLabel();
  }
  return iUnit;
}

std::string getMomentumTransferLabel(std::string momentumTransferLabel) {
  if (momentumTransferLabel == "Angstrom^-1") {
    return sasMomentumTransfer;
  } else {
    return momentumTransferLabel;
  }
}

std::string
getUnitFromMDDimension(Mantid::Geometry::IMDDimension_const_sptr dimension) {
  const auto unitLabel = dimension->getMDUnits().getUnitLabel();
  return unitLabel.ascii();
}

void addData1D(H5::Group &data, Mantid::API::MatrixWorkspace_sptr workspace) {
  // Add attributes for @signal, @I_axes, @Q_indices,
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasSignal, sasDataI);
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataIAxesAttr,
                                                  sasDataQ);
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataIUncertaintyAttr,
                                                  sasDataIdev);
  Mantid::DataHandling::H5Util::writeStrAttribute(
      data, sasDataIUncertaintiesAttr, sasDataIdev);
  Mantid::DataHandling::H5Util::writeNumAttribute(data, sasDataQIndicesAttr,
                                                  std::vector<int>{0});

  if (workspace->hasDx(0)) {
    Mantid::DataHandling::H5Util::writeStrAttribute(
        data, sasDataQUncertaintyAttr, sasDataQdev);
    Mantid::DataHandling::H5Util::writeStrAttribute(
        data, sasDataQUncertaintiesAttr, sasDataQdev);
  }

  //-----------------------------------------
  // Add Q with units  + uncertainty definition
  const auto &qValue = workspace->points(0);
  std::map<std::string, std::string> qAttributes;
  auto qUnit = getUnitFromMDDimension(workspace->getDimension(0));
  qUnit = getMomentumTransferLabel(qUnit);
  qAttributes.emplace(sasUnitAttr, qUnit);
  if (workspace->hasDx(0)) {
    qAttributes.emplace(sasUncertaintyAttr, sasDataQdev);
    qAttributes.emplace(sasUncertaintiesAttr, sasDataQdev);
  }

  writeArray1DWithStrAttributes(data, sasDataQ, qValue.rawData(), qAttributes);

  //-----------------------------------------
  // Add I with units + uncertainty definition
  const auto &intensity = workspace->y(0);
  std::map<std::string, std::string> iAttributes;
  auto iUnit = getIntensityUnit(workspace);
  iUnit = getIntensityUnitLabel(iUnit);
  iAttributes.emplace(sasUnitAttr, iUnit);
  iAttributes.emplace(sasUncertaintyAttr, sasDataIdev);
  iAttributes.emplace(sasUncertaintiesAttr, sasDataIdev);

  writeArray1DWithStrAttributes(data, sasDataI, intensity.rawData(),
                                iAttributes);

  //-----------------------------------------
  // Add Idev with units
  const auto &intensityUncertainty = workspace->e(0);
  std::map<std::string, std::string> eAttributes;
  eAttributes.insert(
      std::make_pair(sasUnitAttr, iUnit)); // same units as intensity

  writeArray1DWithStrAttributes(data, sasDataIdev,
                                intensityUncertainty.rawData(), eAttributes);

  //-----------------------------------------
  // Add Qdev with units if available
  if (workspace->hasDx(0)) {
    const auto qResolution = workspace->pointStandardDeviations(0);
    std::map<std::string, std::string> xUncertaintyAttributes;
    xUncertaintyAttributes.emplace(sasUnitAttr, qUnit);

    writeArray1DWithStrAttributes(data, sasDataQdev, qResolution.rawData(),
                                  xUncertaintyAttributes);
  }
}

bool areAxesNumeric(Mantid::API::MatrixWorkspace_sptr workspace) {
  const unsigned indices[] = {0, 1};
  for (const auto index : indices) {
    auto axis = workspace->getAxis(index);
    if (!axis->isNumeric()) {
      return false;
    }
  }
  return true;
}

class SpectrumAxisValueProvider {
public:
  explicit SpectrumAxisValueProvider(
      Mantid::API::MatrixWorkspace_sptr workspace)
      : m_workspace(workspace) {
    setSpectrumAxisValues();
  }

  Mantid::MantidVec::value_type *
  operator()(Mantid::API::MatrixWorkspace_sptr /*unused*/, int index) {
    auto isPointData =
        m_workspace->getNumberHistograms() == m_spectrumAxisValues.size();
    double value = 0;
    if (isPointData) {
      value = m_spectrumAxisValues[index];
    } else {
      value =
          (m_spectrumAxisValues[index + 1] + m_spectrumAxisValues[index]) / 2.0;
    }

    Mantid::MantidVec tempVec(m_workspace->dataY(index).size(), value);
    m_currentAxisValues.swap(tempVec);
    return m_currentAxisValues.data();
  }

private:
  void setSpectrumAxisValues() {
    auto sAxis = m_workspace->getAxis(1);
    for (size_t index = 0; index < sAxis->length(); ++index) {
      m_spectrumAxisValues.push_back((*sAxis)(index));
    }
  }

  Mantid::API::MatrixWorkspace_sptr m_workspace;
  Mantid::MantidVec m_spectrumAxisValues;
  Mantid::MantidVec m_currentAxisValues;
};

/**
 * QxExtractor functor which allows us to convert 2D Qx data into point data.
 */
template <typename T> class QxExtractor {
public:
  T *operator()(Mantid::API::MatrixWorkspace_sptr ws, int index) {
    if (ws->isHistogramData()) {
      qxPointData.clear();
      Mantid::Kernel::VectorHelper::convertToBinCentre(ws->dataX(index),
                                                       qxPointData);
      return qxPointData.data();
    } else {
      return ws->dataX(index).data();
    }
  }

  std::vector<T> qxPointData;
};

/**
 * Stores the 2D data in the HDF5 file. Qx and Qy values need to be stored as a
 *meshgrid.
 * They should be stored as point data.
 * @param data: the hdf5 group
 * @param workspace: the workspace to store
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
 * The layout below is how it would look like in the HDFView, ie vertical axis
 * is first dimension. We map the Mantid Matrix layout 1-to-1. Note that this
 * will swap the matrix indices, but this is how it is done in the other
 *2Dloaders
 *
 * In HDF5 the Qx would need to be stored as:
 * Qx1 Qx2 ... QxM
 * Qx1 Qx2 ... QxM
 * Qx1 Qx2 ... QxM
 *  .
 *  .
 * Qx1 Qx2 ... QxM
 *
 * In HDF5 the Qy would need to be stored as:
 * Qy1 Qy1 ... Qy1
 * Qy2 Qy2 ... Qy2
 * Qy3 Qy3 ... Qy3
 *  .
 *  .
 * QxN QxN ... QxN
 */
void addData2D(H5::Group &data, Mantid::API::MatrixWorkspace_sptr workspace) {
  if (!areAxesNumeric(workspace)) {
    std::invalid_argument("SaveNXcanSAS: The provided 2D workspace needs "
                          "to have 2 numeric axes.");
  }
  // Add attributes for @signal, @I_axes, @Q_indices,
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasSignal, sasDataI);
  const std::string sasDataIAxesAttr2D = sasDataQ + sasSeparator + sasDataQ;
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataIAxesAttr,
                                                  sasDataIAxesAttr2D);
  Mantid::DataHandling::H5Util::writeStrAttribute(data, sasDataIUncertaintyAttr,
                                                  sasDataIdev);
  Mantid::DataHandling::H5Util::writeStrAttribute(
      data, sasDataIUncertaintiesAttr, sasDataIdev);
  // Write the Q Indices as Int Array
  Mantid::DataHandling::H5Util::writeNumAttribute(data, sasDataQIndicesAttr,
                                                  std::vector<int>{0, 1});

  // Store the 2D Qx data + units
  std::map<std::string, std::string> qxAttributes;
  auto qxUnit = getUnitFromMDDimension(workspace->getXDimension());
  qxUnit = getMomentumTransferLabel(qxUnit);
  qxAttributes.emplace(sasUnitAttr, qxUnit);
  QxExtractor<double> qxExtractor;
  write2DWorkspace(data, workspace, sasDataQx, qxExtractor, qxAttributes);

  // Get 2D Qy data and store it
  std::map<std::string, std::string> qyAttributes;
  auto qyUnit = getUnitFromMDDimension(workspace->getDimension(1));
  qyUnit = getMomentumTransferLabel(qyUnit);
  qyAttributes.emplace(sasUnitAttr, qyUnit);

  SpectrumAxisValueProvider spectrumAxisValueProvider(workspace);
  write2DWorkspace(data, workspace, sasDataQy, spectrumAxisValueProvider,
                   qyAttributes);

  // Get 2D I data and store it
  std::map<std::string, std::string> iAttributes;
  auto iUnit = getIntensityUnit(workspace);
  iUnit = getIntensityUnitLabel(iUnit);
  iAttributes.emplace(sasUnitAttr, iUnit);
  iAttributes.emplace(sasUncertaintyAttr, sasDataIdev);
  iAttributes.emplace(sasUncertaintiesAttr, sasDataIdev);

  auto iExtractor = [](Mantid::API::MatrixWorkspace_sptr ws, int index) {
    return ws->dataY(index).data();
  };
  write2DWorkspace(data, workspace, sasDataI, iExtractor, iAttributes);

  // Get 2D Idev data and store it
  std::map<std::string, std::string> eAttributes;
  eAttributes.insert(
      std::make_pair(sasUnitAttr, iUnit)); // same units as intensity

  auto iDevExtractor = [](Mantid::API::MatrixWorkspace_sptr ws, int index) {
    return ws->dataE(index).data();
  };
  write2DWorkspace(data, workspace, sasDataIdev, iDevExtractor, eAttributes);
}

void addData(H5::Group &group, Mantid::API::MatrixWorkspace_sptr workspace) {
  const std::string sasDataName = sasDataGroupName;
  auto data = Mantid::DataHandling::H5Util::createGroupCanSAS(
      group, sasDataName, nxDataClassAttr, sasDataClassAttr);

  auto workspaceDimensionality = getWorkspaceDimensionality(workspace);
  switch (workspaceDimensionality) {
  case (WorkspaceDimensionality::oneD):
    addData1D(data, workspace);
    break;
  case (WorkspaceDimensionality::twoD):
    addData2D(data, workspace);
    break;
  default:
    throw std::runtime_error("SaveNXcanSAS: The provided workspace "
                             "dimensionality is not 1D or 2D.");
  }
}

//------- SAStransmission_spectrum
void addTransmission(H5::Group &group,
                     Mantid::API::MatrixWorkspace_const_sptr workspace,
                     std::string transmissionName) {
  // Setup process
  const std::string sasTransmissionName =
      sasTransmissionSpectrumGroupName + "_" + transmissionName;
  auto transmission = Mantid::DataHandling::H5Util::createGroupCanSAS(
      group, sasTransmissionName, nxTransmissionSpectrumClassAttr,
      sasTransmissionSpectrumClassAttr);

  // Add attributes for @signal, @T_axes, @T_indices, @T_uncertainty,
  // @T_uncertainties, @name, @timestamp
  Mantid::DataHandling::H5Util::writeStrAttribute(transmission, sasSignal,
                                                  sasTransmissionSpectrumT);
  Mantid::DataHandling::H5Util::writeStrAttribute(
      transmission, sasTransmissionSpectrumTIndices, sasTransmissionSpectrumT);
  Mantid::DataHandling::H5Util::writeStrAttribute(
      transmission, sasTransmissionSpectrumTUncertainty,
      sasTransmissionSpectrumTdev);
  Mantid::DataHandling::H5Util::writeStrAttribute(
      transmission, sasTransmissionSpectrumTUncertainties,
      sasTransmissionSpectrumTdev);
  Mantid::DataHandling::H5Util::writeStrAttribute(
      transmission, sasTransmissionSpectrumNameAttr, transmissionName);

  auto date = getDate();
  Mantid::DataHandling::H5Util::writeStrAttribute(
      transmission, sasTransmissionSpectrumTimeStampAttr, date);

  //-----------------------------------------
  // Add T with units + uncertainty definition
  const auto transmissionData = workspace->y(0);
  std::map<std::string, std::string> transmissionAttributes;
  std::string unit;
  if (unit.empty()) {
    unit = sasNone;
  }
  transmissionAttributes.emplace(sasUnitAttr, unit);
  transmissionAttributes.emplace(sasUncertaintyAttr,
                                 sasTransmissionSpectrumTdev);
  transmissionAttributes.emplace(sasUncertaintiesAttr,
                                 sasTransmissionSpectrumTdev);

  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumT,
                                transmissionData.rawData(),
                                transmissionAttributes);

  //-----------------------------------------
  // Add Tdev with units
  const auto &transmissionErrors = workspace->e(0);
  std::map<std::string, std::string> transmissionErrorAttributes;
  transmissionErrorAttributes.emplace(sasUnitAttr, unit);

  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumTdev,
                                transmissionErrors.rawData(),
                                transmissionErrorAttributes);

  //-----------------------------------------
  // Add lambda with units
  const auto &lambda = workspace->x(0);
  std::map<std::string, std::string> lambdaAttributes;
  auto lambdaUnit = getUnitFromMDDimension(workspace->getDimension(0));
  if (lambdaUnit.empty() || lambdaUnit == "Angstrom") {
    lambdaUnit = sasAngstrom;
  }
  lambdaAttributes.emplace(sasUnitAttr, lambdaUnit);

  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumLambda,
                                lambda.rawData(), lambdaAttributes);
}
} // namespace

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNXcanSAS)

/// constructor
SaveNXcanSAS::SaveNXcanSAS() {}

void SaveNXcanSAS::init() {
  auto inputWSValidator = boost::make_shared<Kernel::CompositeValidator>();
  inputWSValidator->add<API::WorkspaceUnitValidator>("MomentumTransfer");
  inputWSValidator->add<API::CommonBinsValidator>();
  declareProperty(
      Mantid::Kernel::make_unique<Mantid::API::WorkspaceProperty<>>(
          "InputWorkspace", "", Kernel::Direction::Input, inputWSValidator),
      "The input workspace, which must be in units of Q");
  declareProperty(Mantid::Kernel::make_unique<Mantid::API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, ".h5"),
                  "The name of the .h5 file to save");

  std::vector<std::string> radiation_source{"Spallation Neutron Source",
                                            "Pulsed Reactor Neutron Source",
                                            "Reactor Neutron Source",
                                            "Synchrotron X-ray Source",
                                            "Pulsed Muon Source",
                                            "Rotating Anode X-ray",
                                            "Fixed Tube X-ray",
                                            "neutron",
                                            "x-ray",
                                            "muon",
                                            "electron"};
  declareProperty(
      "RadiationSource", "Spallation Neutron Source",
      boost::make_shared<Kernel::StringListValidator>(radiation_source),
      "The type of radiation used.");
  declareProperty("DetectorNames", "",
                  "Specify in a comma separated list, which detectors to store "
                  "information about; \nwhere each name must match a name "
                  "given for a detector in the [[IDF|instrument definition "
                  "file (IDF)]]. \nIDFs are located in the instrument "
                  "sub-directory of the MantidPlot install directory.");

  declareProperty(
      Mantid::Kernel::make_unique<API::WorkspaceProperty<>>(
          "Transmission", "", Kernel::Direction::Input, PropertyMode::Optional,
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

std::map<std::string, std::string> SaveNXcanSAS::validateInputs() {
  // The input should be a Workspace2D
  Mantid::API::MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
  std::map<std::string, std::string> result;
  if (!workspace ||
      !boost::dynamic_pointer_cast<const Mantid::DataObjects::Workspace2D>(
          workspace)) {
    result.emplace("InputWorkspace",
                   "The InputWorkspace must be a Workspace2D.");
  }

  // Transmission data should be 1D
  Mantid::API::MatrixWorkspace_sptr transmission = getProperty("Transmission");
  Mantid::API::MatrixWorkspace_sptr transmissionCan =
      getProperty("TransmissionCan");

  auto checkTransmission = [&result](Mantid::API::MatrixWorkspace_sptr trans,
                                     std::string propertyName) {
    if (trans->getNumberHistograms() != 1) {
      result.emplace(propertyName,
                     "The input workspaces for transmissions have to be 1D.");
    }
  };

  if (transmission) {
    checkTransmission(transmission, "Trasmission");
  }

  if (transmissionCan) {
    checkTransmission(transmissionCan, "TransmissionCan");
  }

  return result;
}

void SaveNXcanSAS::exec() {
  Mantid::API::MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
  std::string filename = getPropertyValue("Filename");

  std::string radiationSource = getPropertyValue("RadiationSource");
  std::string detectorNames = getPropertyValue("DetectorNames");

  Mantid::API::MatrixWorkspace_sptr transmissionSample =
      getProperty("Transmission");
  Mantid::API::MatrixWorkspace_sptr transmissionCan =
      getProperty("TransmissionCan");

  // Remove the file if it already exists
  if (Poco::File(filename).exists()) {
    Poco::File(filename).remove();
  }

  H5::H5File file(filename, H5F_ACC_EXCL);

  const std::string suffix("01");

  // Setup progress bar
  int numberOfSteps = 4;
  if (transmissionSample) {
    ++numberOfSteps;
  }

  if (transmissionCan) {
    ++numberOfSteps;
  }

  Progress progress(this, 0.1, 1.0, numberOfSteps);

  // Add a new entry
  progress.report("Adding a new entry.");
  auto sasEntry = addSasEntry(file, workspace, suffix);

  // Add the instrument information
  progress.report("Adding instrument information.");
  const auto detectors = splitDetectorNames(detectorNames);
  addInstrument(sasEntry, workspace, radiationSource, detectors);

  // Add the process information
  progress.report("Adding process information.");
  if (transmissionCan) {
    addProcess(sasEntry, workspace, transmissionCan);
  } else {
    addProcess(sasEntry, workspace);
  }

  // Add the transmissions for sample
  if (transmissionSample) {
    progress.report("Adding sample transmission information.");
    addTransmission(sasEntry, transmissionSample,
                    sasTransmissionSpectrumNameSampleAttrValue);
  }

  // Add the transmissions for can
  if (transmissionCan) {
    progress.report("Adding can transmission information.");
    addTransmission(sasEntry, transmissionCan,
                    sasTransmissionSpectrumNameCanAttrValue);
  }

  // Add the data
  progress.report("Adding data.");
  addData(sasEntry, workspace);

  file.close();
}

/**
 * This makes out of an input a relaxed name, something conforming to
 * "[A-Za-z_][\w_]*"
 * For now "-" is converted to "_", "." is converted to "_", else we throw
 */
std::string makeCanSASRelaxedName(const std::string &input) {
  bool isStrict = false;
  auto emptyCapitalizationStrategy = [](std::string &) {};
  return makeCompliantName(input, isStrict, emptyCapitalizationStrategy);
}

} // namespace DataHandling
} // namespace Mantid
