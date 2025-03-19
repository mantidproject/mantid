// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/NXcanSASHelper.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/H5Util.h"

#include <algorithm>
#include <boost/regex.hpp>
#include <functional>
#include <memory>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace Mantid::NeXus;

namespace {
//------ utility
// For h5 -> hsize_t = unsigned long long
constexpr int COMPRESSION_DEFLATE_LEVEL = 6;

struct SpinStateHelper {
  explicit SpinStateHelper(const std::vector<std::string> &spinStateStr) : spinVec(spinStateStr) {
    if (spinStateStr.size() == 4) {
      Pin = std::vector<int>({-1, 1});
      Pout = std::vector<int>({-1, 1});
    } else if (spinStateStr.size() == 2) {
      if (spinStateStr.begin()->starts_with("0")) {
        Pin = std::vector<int>({0});
        Pout = std::vector<int>({1, -1});
      } else {
        Pin = std::vector<int>({1, -1});
        Pout = std::vector<int>({0});
      }
    }
  }
  std::vector<std::string> spinVec;
  std::vector<int> Pin;
  std::vector<int> Pout;
};
//------- SASFileName

bool isCanSASCompliant(bool isStrict, const std::string &input) {
  auto baseRegex = isStrict ? boost::regex("[a-z_][a-z0-9_]*") : boost::regex("[A-Za-z_][\\w_]*");
  return boost::regex_match(input, baseRegex);
}

void removeSpecialCharacters(std::string &input) {
  boost::regex toReplace("[-\\.]");
  std::string replaceWith("_");
  input = boost::regex_replace(input, toReplace, replaceWith);
}

std::string makeCompliantName(const std::string &input, bool isStrict,
                              const std::function<void(std::string &)> &capitalizeStrategy) {
  auto output = input;
  // Check if input is compliant
  if (!isCanSASCompliant(isStrict, output)) {
    removeSpecialCharacters(output);
    capitalizeStrategy(output);
    // Check if the changes have made it compliant
    if (!isCanSASCompliant(isStrict, output)) {
      std::string message = "SaveNXcanSAS: The input " + input + "is not compliant with the NXcanSAS format.";
      throw std::runtime_error(message);
    }
  }
  return output;
}

//------- SASinstrument

std::string getInstrumentName(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  return workspace->getInstrument()->getFullName();
}

std::string getIDF(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  auto date = workspace->getWorkspaceStartDate();
  auto instrumentName = getInstrumentName(workspace);
  return InstrumentFileFinder::getInstrumentFilename(instrumentName, date);
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

/** Write a property value to the H5 file if the property exists in the run
 *
 * @param run : the run to look for the property in
 * @param propertyName : the name of the property to find
 * @param sasGroup : the group to add the term into in the output file
 * @param sasTerm : the name of the term to add
 */
void addPropertyFromRunIfExists(Run const &run, std::string const &propertyName, H5::Group &sasGroup,
                                std::string const &sasTerm) {
  if (run.hasProperty(propertyName)) {
    const auto *property = run.getProperty(propertyName);
    H5Util::write(sasGroup, sasTerm, property->value());
  }
}

//------- SASData

std::string getIntensityUnitLabel(const std::string &intensityUnitLabel) {
  if (intensityUnitLabel == "I(q) (cm-1)") {
    return sasIntensity;
  }
  return intensityUnitLabel;
}

std::string getIntensityUnit(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  auto iUnit = workspace->YUnit();
  if (iUnit.empty()) {
    iUnit = workspace->YUnitLabel();
  }
  return iUnit;
}

std::string getMomentumTransferLabel(const std::string &momentumTransferLabel) {
  if (momentumTransferLabel == "Angstrom^-1") {
    return sasMomentumTransfer;
  }
  return momentumTransferLabel;
}

std::string getUnitFromMDDimension(const Mantid::Geometry::IMDDimension_const_sptr &dimension) {
  const auto unitLabel = dimension->getMDUnits().getUnitLabel();
  return unitLabel.ascii();
}

bool areAxesNumeric(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  const std::array<int, 2> indices = {0, 1};
  return std::all_of(indices.cbegin(), indices.cend(),
                     [workspace](auto const &index) { return workspace->getAxis(index)->isNumeric(); });
}

template <typename NumT>
void writeArray1DWithStrAttributes(H5::Group &group, const std::string &dataSetName, const std::vector<NumT> &values,
                                   const std::map<std::string, std::string> &attributes) {
  H5Util::writeArray1D(group, dataSetName, values);
  auto dataSet = group.openDataSet(dataSetName);
  for (const auto &attribute : attributes) {
    H5Util::writeStrAttribute(dataSet, attribute.first, attribute.second);
  }
}

H5::DSetCreatPropList setCompression2D(const hsize_t *chunkDims, const int deflateLevel = 6) {
  H5::DSetCreatPropList propList;
  constexpr int rank = 2;
  propList.setChunk(rank, chunkDims);
  propList.setDeflate(deflateLevel);
  return propList;
}

template <typename Functor>
void write2DWorkspace(H5::Group &group, Mantid::API::MatrixWorkspace_sptr workspace, const std::string &dataSetName,
                      Functor func, const std::map<std::string, std::string> &attributes) {
  using namespace Mantid::NeXus::H5Util;

  // Set the dimension
  const size_t dimension0 = workspace->getNumberHistograms();
  const size_t dimension1 = workspace->y(0).size();
  constexpr hsize_t rank = 2;
  hsize_t dimensionArray[rank] = {static_cast<hsize_t>(dimension0), static_cast<hsize_t>(dimension1)};

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
  auto dataSet = group.createDataSet(dataSetName, dataType, fileSpace, propList);

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
  for (const auto &[nameAttr, valueAttr] : attributes) {
    writeStrAttribute(dataSet, nameAttr, valueAttr);
  }
}

template <typename WorkspaceExtractorFunctor>
void writePolarizedData(H5::Group &group, const WorkspaceGroup_sptr &workspaces, WorkspaceExtractorFunctor func,
                        const std::string &dataSetName, const SpinStateHelper &spin,
                        const std::map<std::string, std::string> &attributes = {}) {
  using namespace H5Util;
  using namespace Mantid::Algorithms::PolarizationCorrectionsHelpers;
  auto stateConverter = [](int spin) -> std::string { return (spin == 1) ? "+1" : std::to_string(spin); };

  // Check First Workspace of group for dimensionality
  auto ws0 = std::dynamic_pointer_cast<MatrixWorkspace>(workspaces->getItem(0));
  auto dimSignal = static_cast<hsize_t>(ws0->dataY(0).size());
  auto dimHistogram = static_cast<hsize_t>(ws0->getNumberHistograms());

  auto dataShape = std::vector<hsize_t>({spin.Pin.size(), spin.Pout.size(), dimSignal});
  auto slabShape = std::vector<hsize_t>({1, 1, dimSignal});
  if (dimHistogram > 1) {
    dataShape.insert(dataShape.begin() + 2, dimHistogram);
    slabShape.insert(slabShape.begin() + 2, 1);
  }

  // Set the dimension
  const int rank = static_cast<int>(dataShape.size());
  // Get the Data Space definition for the 2D Data Set in the file
  auto fileSpace = H5::DataSpace(rank, dataShape.data());
  H5::DataType dataType(getType<double>());

  // compression
  H5::DSetCreatPropList propList;
  propList.setChunk(rank, slabShape.data());
  propList.setDeflate(COMPRESSION_DEFLATE_LEVEL);

  // create index of position in hypermatrix
  auto pos = std::vector<hsize_t>(static_cast<hsize_t>(rank), 0);

  // Create the data set
  auto dataSet = group.createDataSet(dataSetName, dataType, fileSpace, propList);
  H5::DataSpace memSpace(rank, slabShape.data());
  for (unsigned int i = 0; i < spin.Pin.size(); i++) {
    for (unsigned int j = 0; j < spin.Pout.size(); j++) {
      auto state = stateConverter(spin.Pin.at(i)) + stateConverter(spin.Pout.at(j));
      auto index = indexOfWorkspaceForSpinState(spin.spinVec, state);
      pos.at(0) = i;
      pos.at(1) = j;

      if (dimHistogram == 1) {
        fileSpace.selectHyperslab(H5S_SELECT_SET, slabShape.data(), pos.data());
        dataSet.write(func(workspaces, static_cast<int>(*index)), dataType, memSpace, fileSpace);
      } else {
        for (unsigned int n = 0; n < dimHistogram; n++) {
          pos.at(2) = n;
          fileSpace.selectHyperslab(H5S_SELECT_SET, slabShape.data(), pos.data());
          dataSet.write(func(workspaces, static_cast<int>(*index), n), dataType, memSpace, fileSpace);
        }
      }
    }
  }

  // Add attributes to data set
  for (const auto &[nameAttr, valueAttr] : attributes) {
    writeStrAttribute(dataSet, nameAttr, valueAttr);
  }
}

class SpectrumAxisValueProvider {
public:
  explicit SpectrumAxisValueProvider(Mantid::API::MatrixWorkspace_sptr workspace) : m_workspace(std::move(workspace)) {
    setSpectrumAxisValues();
  }

  Mantid::MantidVec::value_type *operator()(const Mantid::API::MatrixWorkspace_sptr & /*unused*/, int index) {
    auto isPointData = m_workspace->getNumberHistograms() == m_spectrumAxisValues.size();
    double value = 0;
    if (isPointData) {
      value = m_spectrumAxisValues[index];
    } else {
      value = (m_spectrumAxisValues[index + 1] + m_spectrumAxisValues[index]) / 2.0;
    }

    Mantid::MantidVec tempVec(m_workspace->dataY(index).size(), value);
    m_currentAxisValues.swap(tempVec);
    return m_currentAxisValues.data();
  }

private:
  void setSpectrumAxisValues() {
    auto sAxis = m_workspace->getAxis(1);
    for (size_t index = 0; index < sAxis->length(); ++index) {
      m_spectrumAxisValues.emplace_back((*sAxis)(index));
    }
  }

  Mantid::API::MatrixWorkspace_sptr m_workspace;
  Mantid::MantidVec m_spectrumAxisValues;
  Mantid::MantidVec m_currentAxisValues;
};

template <typename T> class WorkspaceGroupDataExtractor {
public:
  explicit WorkspaceGroupDataExtractor(const WorkspaceGroup_sptr &workspace, bool extractError = false)
      : m_workspace(workspace), m_extractError(extractError) {};
  T *operator()(const Mantid::API::WorkspaceGroup_sptr & /*unused*/, int groupIndex, int spectraIndex = 0) {
    auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(m_workspace->getItem(groupIndex));
    return m_extractError ? ws->dataE(spectraIndex).data() : ws->dataY(spectraIndex).data();
  }

  void setExtractErrors(bool extractError) { m_extractError = extractError; }

private:
  WorkspaceGroup_sptr m_workspace;
  bool m_extractError;
};

/**
 * QxExtractor functor which allows us to convert 2D Qx data into point data.
 */
template <typename T> class QxExtractor {
public:
  T *operator()(const Mantid::API::MatrixWorkspace_sptr &ws, int index) {
    if (ws->isHistogramData()) {
      qxPointData.clear();
      Mantid::Kernel::VectorHelper::convertToBinCentre(ws->dataX(index), qxPointData);
      return qxPointData.data();
    } else {
      return ws->dataX(index).data();
    }
  }

  std::vector<T> qxPointData;
};

void addQ1D(H5::Group &data, const MatrixWorkspace_sptr &workspace) {
  std::map<std::string, std::string> qAttributes;
  // Prepare units
  auto qUnit = getUnitFromMDDimension(workspace->getDimension(0));
  qUnit = getMomentumTransferLabel(qUnit);
  qAttributes.emplace(sasUnitAttr, qUnit);

  // Add Qdev with units if available
  if (workspace->hasDx(0)) {
    H5Util::writeStrAttribute(data, sasDataQUncertaintyAttr, sasDataQdev);
    H5Util::writeStrAttribute(data, sasDataQUncertaintiesAttr, sasDataQdev);

    qAttributes.emplace(sasUncertaintyAttr, sasDataQdev);
    qAttributes.emplace(sasUncertaintiesAttr, sasDataQdev);

    const auto qResolution = workspace->pointStandardDeviations(0);
    std::map<std::string, std::string> xUncertaintyAttributes;
    xUncertaintyAttributes.emplace(sasUnitAttr, qUnit);
    writeArray1DWithStrAttributes(data, sasDataQdev, qResolution.rawData(), xUncertaintyAttributes);
  }

  // We finally add the Q data with necessary attributes
  const auto &qValue = workspace->points(0);
  writeArray1DWithStrAttributes(data, sasDataQ, qValue.rawData(), qAttributes);
}

void addQ2D(H5::Group &data, const MatrixWorkspace_sptr &workspace) {
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
  write2DWorkspace(data, workspace, sasDataQy, spectrumAxisValueProvider, qyAttributes);
}

} // namespace

namespace Mantid::DataHandling::NXcanSAS {
constexpr std::string_view NX_CANSAS_EXTENSION = ".h5";

std::filesystem::path prepareFilename(const std::string &baseFilename, int index, bool isGroup) {
  auto path = std::filesystem::path(baseFilename);
  if (!isGroup) {
    // return early if it doesn't need to add digits
    return path.replace_extension(NX_CANSAS_EXTENSION);
  }

  auto const addDigit = [&](int index) { return index < 10 ? "0" + std::to_string(index) : std::to_string(index); };
  // remove extension if it has any
  path.replace_extension("");
  // add digit for groups
  path += addDigit(index);
  // add correct extension and return path
  return path.replace_extension(NX_CANSAS_EXTENSION);
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

void addDetectors(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                  const std::vector<std::string> &detectorNames) {
  // If the group is empty then don't add anything
  if (!detectorNames.empty()) {
    for (const auto &detectorName : detectorNames) {
      if (detectorName.empty()) {
        continue;
      }

      std::string sasDetectorName = sasInstrumentDetectorGroupName + detectorName;
      sasDetectorName = makeCanSASRelaxedName(sasDetectorName);

      auto instrument = workspace->getInstrument();
      auto component = instrument->getComponentByName(detectorName);

      if (component) {
        const auto sample = instrument->getSample();
        const auto distance = component->getDistance(*sample);
        std::map<std::string, std::string> sddAttributes;
        sddAttributes.insert(std::make_pair(sasUnitAttr, sasInstrumentDetectorSddUnitAttrValue));
        auto detector = H5Util::createGroupCanSAS(group, sasDetectorName, nxInstrumentDetectorClassAttr,
                                                  sasInstrumentDetectorClassAttr);
        H5Util::write(detector, sasInstrumentDetectorName, detectorName);
        H5Util::writeScalarDataSetWithStrAttributes(detector, sasInstrumentDetectorSdd, distance, sddAttributes);
      }
    }
  }
}

/**
 * Add the instrument group to the NXcanSAS file. This adds the
 * instrument name and the IDF
 * @param group: the sasEntry
 * @param workspace: the workspace which is being stored
 * @param radiationSource: the selected radiation source
 * @param detectorNames: the names of the detectors to store
 * @param geometry: Geometry type of collimation
 * @param beamHeight: Height of collimation element in mm
 * @param beamWidth: Width of collimation element in mm
 */
void addInstrument(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                   const std::string &radiationSource, const std::string &geometry, double beamHeight, double beamWidth,
                   const std::vector<std::string> &detectorNames) {
  // Setup instrument
  const std::string sasInstrumentNameForGroup = sasInstrumentGroupName;
  auto instrument =
      H5Util::createGroupCanSAS(group, sasInstrumentNameForGroup, nxInstrumentClassAttr, sasInstrumentClassAttr);
  auto instrumentName = getInstrumentName(workspace);
  H5Util::write(instrument, sasInstrumentName, instrumentName);

  // Setup the detector
  addDetectors(instrument, workspace, detectorNames);

  // Setup source
  const std::string sasSourceName = sasInstrumentSourceGroupName;
  auto source =
      H5Util::createGroupCanSAS(instrument, sasSourceName, nxInstrumentSourceClassAttr, sasInstrumentSourceClassAttr);
  H5Util::write(source, sasInstrumentSourceRadiation, radiationSource);

  // Setup Aperture
  const std::string sasApertureName = sasInstrumentApertureGroupName;
  auto aperture = H5Util::createGroupCanSAS(instrument, sasApertureName, nxInstrumentApertureClassAttr,
                                            sasInstrumentApertureClassAttr);

  H5Util::write(aperture, sasInstrumentApertureShape, geometry);

  std::map<std::string, std::string> beamSizeAttrs;
  beamSizeAttrs.insert(std::make_pair(sasUnitAttr, sasBeamAndSampleSizeUnitAttrValue));
  if (beamHeight != 0) {
    H5Util::writeScalarDataSetWithStrAttributes(aperture, sasInstrumentApertureGapHeight, beamHeight, beamSizeAttrs);
  }
  if (beamWidth != 0) {
    H5Util::writeScalarDataSetWithStrAttributes(aperture, sasInstrumentApertureGapWidth, beamWidth, beamSizeAttrs);
  }

  // Add IDF information
  auto idf = getIDF(workspace);
  H5Util::write(instrument, sasInstrumentIDF, idf);
}

//------- SASpolarization

void addPolarizerComponentMetadata(H5::Group &group, const MatrixWorkspace_sptr &workspace,
                                   const InstrumentPolarizer &polarizer) {

  auto instrument = workspace->getInstrument();
  auto component = instrument->getComponentByName(polarizer.getComponentName());

  if (component) {
    const auto sample = instrument->getSample();
    const auto distance = component->getDistance(*sample);
    const auto type = component->getStringParameter(polarizer.sasPolarizerIDFDeviceType());

    H5Util::write(group, polarizer.sasPolarizerDeviceType(), type.front());
    std::map<std::string, std::string> distanceAttrs;
    distanceAttrs.insert(std::make_pair(sasUnitAttr, polarizer.sasPolarizerDistanceUnitAttr()));
    NeXus::H5Util::writeScalarDataSetWithStrAttributes(group, polarizer.sasPolarizerDistance(), distance,
                                                       distanceAttrs);
  }
}

void addPolarizer(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                  const std::string &componentName, const std::string &componentType) {

  auto const instrumentAttr = InstrumentPolarizer(componentType, componentName);
  auto instrumentGroup =
      group.exists(sasInstrumentGroupName)
          ? group.openGroup(sasInstrumentGroupName)
          : H5Util::createGroupCanSAS(group, sasInstrumentGroupName, nxInstrumentClassAttr, sasInstrumentClassAttr);

  auto polarizerGroup =
      H5Util::createGroupCanSAS(instrumentGroup, instrumentAttr.sasPolarizerGroupAttr(),
                                instrumentAttr.nxPolarizerClassAttr(), instrumentAttr.sasPolarizerClassAttr());

  H5Util::write(polarizerGroup, instrumentAttr.sasPolarizerName(), componentName);
  addPolarizerComponentMetadata(polarizerGroup, workspace, instrumentAttr);
}

void addEMFieldDirection(H5::Group &group, const std::string &emFieldDir) {
  // expect to recibe a comma separated string with directions polar,azimuthal,rotation.
  const auto directions = Mantid::Kernel::VectorHelper::splitStringIntoVector<std::string>(emFieldDir);
  const auto angles = std::vector<std::string>{sasSampleEMFieldDirectionPolar, sasSampleEMFieldDirectionAzimuthal,
                                               sasSampleEMFieldDirectionRotation};

  if (!directions.empty()) {
    std::map<std::string, std::string> magFieldAttrs;
    magFieldAttrs.insert(std::make_pair(sasUnitAttr, sasSampleEMFieldDirectionUnitsAttr));
    for (size_t i = 0; i < directions.size(); i++)
      H5Util::writeScalarDataSetWithStrAttributes(group, angles.at(i), directions.at(i), magFieldAttrs);
  }
}

void addSampleEMFields(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                       const std::string &emFieldStrengthLog, const std::string &emFieldDir) {

  if (workspace->run().hasProperty(emFieldStrengthLog)) {
    auto magFStrength = workspace->run().getLogAsSingleValue(emFieldStrengthLog);
    auto const magFStrengthUnits = workspace->run().getProperty(emFieldStrengthLog)->units();

    std::map<std::string, std::string> magFieldAttrs;
    magFieldAttrs.insert(std::make_pair(sasUnitAttr, magFStrengthUnits));
    // need more information about how to set the direction

    auto sampleGroup = group.exists(sasInstrumentSampleGroupAttr)
                           ? group.openGroup(sasInstrumentSampleGroupAttr)
                           : H5Util::createGroupCanSAS(group, sasInstrumentSampleGroupAttr, nxInstrumentSampleClassAttr,
                                                       sasInstrumentSampleClassAttr);
    H5Util::writeScalarDataSetWithStrAttributes(sampleGroup, sasSampleMagneticField, magFStrength, magFieldAttrs);
    addEMFieldDirection(sampleGroup, emFieldDir);
  }
}

//------- SASsample

void addSample(H5::Group &group, const double &sampleThickness) {
  if (sampleThickness == 0) {
    return;
  }
  std::string const sasSampleNameForGroup = sasInstrumentSampleGroupAttr;

  auto sample = H5Util::createGroupCanSAS(group, sasSampleNameForGroup, nxInstrumentSampleClassAttr,
                                          sasInstrumentSampleClassAttr);

  std::map<std::string, std::string> sampleThicknessAttrs;
  sampleThicknessAttrs.insert(std::make_pair(sasUnitAttr, sasBeamAndSampleSizeUnitAttrValue));
  H5Util::writeScalarDataSetWithStrAttributes(sample, sasInstrumentSampleThickness, sampleThickness,
                                              sampleThicknessAttrs);
}

/**
 * Add the process information to the NXcanSAS file. This information
 * about the run number, the Mantid version and the user file (if available)
 * @param group: the sasEntry
 * @param workspace: the workspace which is being stored
 */
void addProcess(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace) {
  // Setup process
  const std::string sasProcessNameForGroup = sasProcessGroupName;
  auto process = H5Util::createGroupCanSAS(group, sasProcessNameForGroup, nxProcessClassAttr, sasProcessClassAttr);

  // Add name
  H5Util::write(process, sasProcessName, sasProcessNameValue);

  // Add creation date of the file
  auto date = getDate();
  H5Util::write(process, sasProcessDate, date);

  // Add Mantid version
  const auto version = std::string(MantidVersion::version());
  H5Util::write(process, sasProcessTermSvn, version);

  // Add log values
  const auto run = workspace->run();
  addPropertyFromRunIfExists(run, sasProcessUserFileInLogs, process, sasProcessTermUserFile);
  addPropertyFromRunIfExists(run, sasProcessBatchFileInLogs, process, sasProcessTermBatchFile);
}

/**
 * Add the process information to the NXcanSAS file. It contains information
 * about the run number, the Mantid version and the user file (if available)
 * @param group: the sasEntry
 * @param workspace: the workspace which is being stored
 * @param canWorkspace: workspace for the can run
 */
void addProcess(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                const Mantid::API::MatrixWorkspace_sptr &canWorkspace) {
  // Setup process
  const std::string sasProcessNameForGroup = sasProcessGroupName;
  auto process = H5Util::createGroupCanSAS(group, sasProcessNameForGroup, nxProcessClassAttr, sasProcessClassAttr);

  // Add name
  H5Util::write(process, sasProcessName, sasProcessNameValue);

  // Add creation date of the file
  auto date = getDate();
  H5Util::write(process, sasProcessDate, date);

  // Add Mantid version
  const auto version = std::string(MantidVersion::version());
  H5Util::write(process, sasProcessTermSvn, version);

  const auto run = workspace->run();
  addPropertyFromRunIfExists(run, sasProcessUserFileInLogs, process, sasProcessTermUserFile);
  addPropertyFromRunIfExists(run, sasProcessBatchFileInLogs, process, sasProcessTermBatchFile);

  // Add can run number
  const auto canRun = canWorkspace->getRunNumber();
  H5Util::write(process, sasProcessTermCan, std::to_string(canRun));
}

/**
 * Add an entry to the process group.
 * @param group: the sasEntry
 * @param entryName: string containing the name of the value to save
 * @param entryValue: string containing the value to save
 */
void addProcessEntry(H5::Group &group, const std::string &entryName, const std::string &entryValue) {
  auto process = group.openGroup(sasProcessGroupName);
  // Populate process entry
  H5Util::write(process, entryName, entryValue);
}

//------- SAStransmission_spectrum
void addTransmission(H5::Group &group, const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                     const std::string &transmissionName) {
  // Setup process
  const std::string sasTransmissionName = sasTransmissionSpectrumGroupName + "_" + transmissionName;
  auto transmission = H5Util::createGroupCanSAS(group, sasTransmissionName, nxTransmissionSpectrumClassAttr,
                                                sasTransmissionSpectrumClassAttr);

  // Add attributes for @signal, @T_axes, @T_indices, @T_uncertainty,
  // @T_uncertainties, @name, @timestamp
  H5Util::writeStrAttribute(transmission, sasSignal, sasTransmissionSpectrumT);
  H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumTIndices, sasTransmissionSpectrumT);
  H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumTUncertainty, sasTransmissionSpectrumTdev);
  H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumTUncertainties, sasTransmissionSpectrumTdev);
  H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumNameAttr, transmissionName);

  auto date = getDate();
  H5Util::writeStrAttribute(transmission, sasTransmissionSpectrumTimeStampAttr, date);

  //-----------------------------------------
  // Add T with units + uncertainty definition
  const auto transmissionData = workspace->y(0);
  std::map<std::string, std::string> transmissionAttributes;
  std::string unit = sasNone;

  transmissionAttributes.emplace(sasUnitAttr, unit);
  transmissionAttributes.emplace(sasUncertaintyAttr, sasTransmissionSpectrumTdev);
  transmissionAttributes.emplace(sasUncertaintiesAttr, sasTransmissionSpectrumTdev);

  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumT, transmissionData.rawData(),
                                transmissionAttributes);

  //-----------------------------------------
  // Add Tdev with units
  const auto &transmissionErrors = workspace->e(0);
  std::map<std::string, std::string> transmissionErrorAttributes;
  transmissionErrorAttributes.emplace(sasUnitAttr, unit);

  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumTdev, transmissionErrors.rawData(),
                                transmissionErrorAttributes);

  //-----------------------------------------
  // Add lambda with units
  const auto lambda = workspace->points(0);
  std::map<std::string, std::string> lambdaAttributes;
  auto lambdaUnit = getUnitFromMDDimension(workspace->getDimension(0));
  if (lambdaUnit.empty() || lambdaUnit == "Angstrom") {
    lambdaUnit = sasAngstrom;
  }
  lambdaAttributes.emplace(sasUnitAttr, lambdaUnit);

  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumLambda, lambda.rawData(), lambdaAttributes);
}

void addData1D(H5::Group &data, const Mantid::API::MatrixWorkspace_sptr &workspace) {
  // Add attributes for @signal, @I_axes, @Q_indices,
  H5Util::writeStrAttribute(data, sasSignal, sasDataI);
  H5Util::writeStrAttribute(data, sasDataIAxesAttr, sasDataQ);
  H5Util::writeStrAttribute(data, sasDataIUncertaintyAttr, sasDataIdev);
  H5Util::writeStrAttribute(data, sasDataIUncertaintiesAttr, sasDataIdev);
  H5Util::writeNumAttribute(data, sasDataQIndicesAttr, std::vector<int>{0});

  addQ1D(data, workspace);
  //-----------------------------------------
  // Add I with units + uncertainty definition
  const auto &intensity = workspace->y(0);
  std::map<std::string, std::string> iAttributes;
  auto iUnit = getIntensityUnit(workspace);
  iUnit = getIntensityUnitLabel(iUnit);
  iAttributes.emplace(sasUnitAttr, iUnit);
  iAttributes.emplace(sasUncertaintyAttr, sasDataIdev);
  iAttributes.emplace(sasUncertaintiesAttr, sasDataIdev);

  writeArray1DWithStrAttributes(data, sasDataI, intensity.rawData(), iAttributes);

  //-----------------------------------------
  // Add Idev with units
  const auto &intensityUncertainty = workspace->e(0);
  std::map<std::string, std::string> eAttributes;
  eAttributes.insert(std::make_pair(sasUnitAttr, iUnit)); // same units as intensity

  writeArray1DWithStrAttributes(data, sasDataIdev, intensityUncertainty.rawData(), eAttributes);
}

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
 * \.
 * \.
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
 * \.
 * \.
 * Qx1 Qx2 ... QxM
 *
 * In HDF5 the Qy would need to be stored as:
 * Qy1 Qy1 ... Qy1
 * Qy2 Qy2 ... Qy2
 * Qy3 Qy3 ... Qy3
 * \.
 * \.
 * QxN QxN ... QxN
 */
void addData2D(H5::Group &data, const Mantid::API::MatrixWorkspace_sptr &workspace) {
  if (!areAxesNumeric(workspace)) {
    throw std::invalid_argument("SaveNXcanSAS: The provided 2D workspace needs to have 2 numeric axes.");
  }
  // Add attributes for @signal, @I_axes, @Q_indices,
  H5Util::writeStrAttribute(data, sasSignal, sasDataI);
  const std::string sasDataIAxesAttr2D = sasDataQ + sasSeparator + sasDataQ;
  H5Util::writeStrAttribute(data, sasDataIAxesAttr, sasDataIAxesAttr2D);
  H5Util::writeStrAttribute(data, sasDataIUncertaintyAttr, sasDataIdev);
  H5Util::writeStrAttribute(data, sasDataIUncertaintiesAttr, sasDataIdev);
  // Write the Q Indices as Int Array
  H5Util::writeNumAttribute(data, sasDataQIndicesAttr, std::vector<int>{0, 1});

  addQ2D(data, workspace);
  // Get 2D I data and store it
  std::map<std::string, std::string> iAttributes;
  auto iUnit = getIntensityUnit(workspace);
  iUnit = getIntensityUnitLabel(iUnit);
  iAttributes.emplace(sasUnitAttr, iUnit);
  iAttributes.emplace(sasUncertaintyAttr, sasDataIdev);
  iAttributes.emplace(sasUncertaintiesAttr, sasDataIdev);

  auto iExtractor = [](const Mantid::API::MatrixWorkspace_sptr &ws, int index) { return ws->dataY(index).data(); };
  write2DWorkspace(data, workspace, sasDataI, iExtractor, iAttributes);

  // Get 2D Idev data and store it
  std::map<std::string, std::string> eAttributes;
  eAttributes.insert(std::make_pair(sasUnitAttr, iUnit)); // same units as intensity

  auto iDevExtractor = [](const Mantid::API::MatrixWorkspace_sptr &ws, int index) { return ws->dataE(index).data(); };
  write2DWorkspace(data, workspace, sasDataIdev, iDevExtractor, eAttributes);
}

void addPolarizedData(H5::Group &data, const Mantid::API::WorkspaceGroup_sptr &wsGroup,
                      const std::string &inputSpinStates) {
  //  Add attributes for @signal, @I_axes, @Q_indices,
  H5Util::writeStrAttribute(data, sasSignal, sasDataI);
  const std::string sasDataIAxesAttrSpin = sasDataPin + sasSeparator + sasDataPout + sasSeparator + sasDataQ;
  H5Util::writeStrAttribute(data, sasDataIAxesAttr, sasDataIAxesAttrSpin);
  H5Util::writeStrAttribute(data, sasDataIUncertaintyAttr, sasDataIdev);
  H5Util::writeStrAttribute(data, sasDataIUncertaintiesAttr, sasDataIdev);
  //  Write the Q Indices as Int Array
  H5Util::writeNumAttribute(data, sasDataPinIndicesAttr, 0);
  H5Util::writeNumAttribute(data, sasDataPoutIndicesAttr, 1);
  H5Util::writeNumAttribute(data, sasDataQIndicesAttr, std::vector<int>{0, 1, 2});

  // store Pin/ Pout
  std::map<std::string, std::string> polAttributes;
  polAttributes.emplace(sasUnitAttr, sasDataPolarizationUnitAttr);
  auto const inputSpinOrder = Algorithms::PolarizationCorrectionsHelpers::splitSpinStateString(inputSpinStates);
  auto const spinPairs = SpinStateHelper(inputSpinOrder);
  writeArray1DWithStrAttributes(data, sasDataPin, spinPairs.Pin, polAttributes);
  writeArray1DWithStrAttributes(data, sasDataPout, spinPairs.Pout, polAttributes);

  // add Q
  auto const ws0 = std::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(0));
  auto dim = getWorkspaceDimensionality(ws0);
  if (dim == WorkspaceDimensionality::oneD) {
    addQ1D(data, ws0);
  } else {
    // Store the 2D Qx data + units
    addQ2D(data, ws0);
  }

  // add signal
  WorkspaceGroupDataExtractor<double> wsGroupExtractor(wsGroup);
  writePolarizedData(data, wsGroup, wsGroupExtractor, sasDataI, spinPairs);
  // add signal error
  wsGroupExtractor.setExtractErrors(true);
  writePolarizedData(data, wsGroup, wsGroupExtractor, sasDataIdev, spinPairs);
}

WorkspaceDimensionality getWorkspaceDimensionality(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  auto numberOfHistograms = workspace->getNumberHistograms();
  WorkspaceDimensionality dimensionality(WorkspaceDimensionality::other);
  if (numberOfHistograms == 1) {
    dimensionality = WorkspaceDimensionality::oneD;
  } else if (numberOfHistograms > 1) {
    dimensionality = WorkspaceDimensionality::twoD;
  }
  return dimensionality;
}

} // namespace Mantid::DataHandling::NXcanSAS
