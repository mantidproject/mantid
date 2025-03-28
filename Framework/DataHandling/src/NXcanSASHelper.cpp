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

#include <functional>
#include <regex>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace Mantid::NeXus;

namespace {
//=== UTILITY ===//

// For h5 -> hsize_t = unsigned long long

using AttrMap = std::map<std::string, std::string>;

std::string getIntensityUnit(const MatrixWorkspace_sptr &workspace) {
  auto iUnit = workspace->YUnit();
  if (iUnit.empty()) {
    iUnit = workspace->YUnitLabel();
  }
  return iUnit == "I(q) (cm-1)" ? sasIntensity : std::string(iUnit);
}

std::string getMDUnit(const Mantid::Geometry::IMDDimension_const_sptr &dimension,
                      const std::string &expectedUnit = "Angstrom^-1",
                      const std::string &sasFormatUnit = sasMomentumTransfer) {

  const auto unitLabel = dimension->getMDUnits().getUnitLabel().ascii();
  return (unitLabel.empty() || unitLabel == expectedUnit) ? sasFormatUnit : std::string(unitLabel);
}

template <typename NumT>
void writeArray1DWithStrAttributes(H5::Group &group, const std::string &dataSetName, const std::vector<NumT> &values,
                                   const AttrMap &attributes) {
  H5Util::writeArray1D(group, dataSetName, values);
  auto dataSet = group.openDataSet(dataSetName);
  for (const auto &[attributeName, attributeValue] : attributes) {
    H5Util::writeStrAttribute(dataSet, attributeName, attributeValue);
  }
}

void writeDataSetAttributes(const H5::H5Object &dataSet, const AttrMap &attributes) {
  // Add attributes to data set
  for (const auto &[nameAttr, valueAttr] : attributes) {
    H5Util::writeStrAttribute(dataSet, nameAttr, valueAttr);
  }
}

H5::DSetCreatPropList setCompression(int rank, const hsize_t *chunkDims, int deflateLevel = 6) {
  H5::DSetCreatPropList propList;
  propList.setChunk(rank, chunkDims);
  propList.setDeflate(deflateLevel);
  return propList;
}

//=== Functors to Extract Data From Workspaces ===//

/**
 * QxExtractor functor which allows us to convert 2D Qx data into point data.
 */
template <typename T> class QxExtractor {
public:
  T *operator()(const MatrixWorkspace_sptr &ws, size_t index) {
    if (ws->isHistogramData()) {
      qxPointData.clear();
      VectorHelper::convertToBinCentre(ws->dataX(index), qxPointData);
      return qxPointData.data();
    }
    return ws->dataX(index).data();
  }

  std::vector<T> qxPointData;
};

/**
 * Spectrum Axis Value provider allows to convert 2D Qy data into point data.
 */
class SpectrumAxisValueProvider {
public:
  explicit SpectrumAxisValueProvider(const MatrixWorkspace_sptr &workspace) : m_workspace(std::move(workspace)) {
    setSpectrumAxisValues();
  }

  Mantid::MantidVec::value_type *operator()(const MatrixWorkspace_sptr & /*unused*/, size_t index) {
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

  MatrixWorkspace_sptr m_workspace;
  Mantid::MantidVec m_spectrumAxisValues;
  Mantid::MantidVec m_currentAxisValues;
};

/**
 * WorkspaceGroupDataExtractor allows to extract signal or error signal from a spectra in a workspace within a group.
 * Used in extracting polarized data.
 */
template <typename T> class WorkspaceGroupDataExtractor {
public:
  explicit WorkspaceGroupDataExtractor(const WorkspaceGroup_sptr &workspace, bool extractError = false)
      : m_workspace(std::move(workspace)), m_extractError(extractError) {}

  T *operator()(size_t groupIndex, size_t spectraIndex = 0) {
    auto const ws = std::dynamic_pointer_cast<MatrixWorkspace>(m_workspace->getItem(groupIndex));
    return m_extractError ? ws->dataE(spectraIndex).data() : ws->dataY(spectraIndex).data();
  }

  void setExtractErrors(bool extractError) { m_extractError = extractError; }

private:
  WorkspaceGroup_sptr m_workspace;
  bool m_extractError;
};

//=== SASFilename ===//

bool isCanSASCompliant(bool isStrict, const std::string &input) {
  auto const baseRegex = isStrict ? std::regex("[a-z_][a-z0-9_]*") : std::regex("[A-Za-z_][\\w_]*");
  return std::regex_match(input, baseRegex);
}

void removeSpecialCharacters(std::string &input) {
  std::regex const toReplace("[-\\.]");
  std::string const replaceWith("_");
  input = std::regex_replace(input, toReplace, replaceWith);
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
      auto const message = "SaveNXcanSAS: The input " + input + "is not compliant with the NXcanSAS format.";
      throw std::runtime_error(message);
    }
  }
  return output;
}

//=== SASinstrument ===//

std::string getInstrumentName(const MatrixWorkspace_sptr &workspace) {
  return workspace->getInstrument()->getFullName();
}

std::string getIDF(const MatrixWorkspace_sptr &workspace) {
  auto const date = workspace->getWorkspaceStartDate();
  auto const instrumentName = getInstrumentName(workspace);
  return InstrumentFileFinder::getInstrumentFilename(instrumentName, date);
}

//=== SASprocess ===//

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
void addPropertyFromRunIfExists(const Run &run, const std::string &propertyName, H5::Group &sasGroup,
                                const std::string &sasTerm) {
  if (run.hasProperty(propertyName)) {
    const auto *property = run.getProperty(propertyName);
    H5Util::write(sasGroup, sasTerm, property->value());
  }
}

//=== SASpolarization ===//

struct SpinStateHelper {
  explicit SpinStateHelper(const std::vector<std::string> &spinStateStr) : spinVec(spinStateStr) {
    auto const spinVec = std::vector<int>({-1, 1});
    if (spinStateStr.size() == 4) {
      Pin = spinVec;
      Pout = spinVec;
    } else if (spinStateStr.size() == 2) {
      if (spinStateStr.begin()->starts_with("0")) {
        Pin = std::vector<int>(1, 0);
        Pout = spinVec;
      } else {
        Pin = spinVec;
        Pout = std::vector<int>(1, 0);
      }
    }
  }
  std::vector<std::string> spinVec;
  std::vector<int> Pin;
  std::vector<int> Pout;
};

template <typename WorkspaceExtractorFunctor>
void writePolarizedData(H5::Group &group, const WorkspaceGroup_sptr &workspaces, WorkspaceExtractorFunctor func,
                        const std::string &dataSetName, const SpinStateHelper &spin, const AttrMap &attributes = {}) {
  using namespace Mantid::Algorithms::PolarizationCorrectionsHelpers;
  auto stateConverter = [](int spin) -> std::string { return (spin == 1) ? "+1" : std::to_string(spin); };

  // Check First Workspace of group for dimensionality
  const auto ws0 = std::dynamic_pointer_cast<MatrixWorkspace>(workspaces->getItem(0));
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
  const H5::DataType dataType(H5Util::getType<double>());

  // compression: 6 by default.
  const auto propList = setCompression(rank, slabShape.data());
  // create index of position in hypermatrix
  auto pos = std::vector<hsize_t>(static_cast<hsize_t>(rank), 0);

  // Create the data set
  auto dataSet = group.createDataSet(dataSetName, dataType, fileSpace, propList);
  H5::DataSpace memSpace(rank, slabShape.data());
  for (size_t i = 0; i < spin.Pin.size(); i++) {
    for (size_t j = 0; j < spin.Pout.size(); j++) {
      auto state = stateConverter(spin.Pin.at(i)) + stateConverter(spin.Pout.at(j));
      auto index = indexOfWorkspaceForSpinState(spin.spinVec, state);
      if (!index.has_value()) {
        throw std::runtime_error("Couldn't find workspace for spin state " + state);
      }

      pos.at(0) = i;
      pos.at(1) = j;

      if (dimHistogram == 1) {
        fileSpace.selectHyperslab(H5S_SELECT_SET, slabShape.data(), pos.data());
        dataSet.write(func(*index), dataType, memSpace, fileSpace);
      } else {
        for (size_t n = 0; n < dimHistogram; n++) {
          pos.at(2) = n;
          fileSpace.selectHyperslab(H5S_SELECT_SET, slabShape.data(), pos.data());
          dataSet.write(func(*index, n), dataType, memSpace, fileSpace);
        }
      }
    }
  }
  writeDataSetAttributes(dataSet, attributes);
}

//=== SASdata ===//

void writeStandardDataAttributes(const H5::Group &data, const std::string &IaxesAttr = "Q",
                                 const std::vector<int> &qIndices = {0}) {
  H5Util::writeStrAttribute(data, sasSignal, sasDataI);
  H5Util::writeStrAttribute(data, sasDataIAxesAttr, IaxesAttr);
  H5Util::writeStrAttribute(data, sasDataIUncertaintyAttr, sasDataIdev);
  H5Util::writeStrAttribute(data, sasDataIUncertaintiesAttr, sasDataIdev);
  H5Util::writeNumAttribute(data, sasDataQIndicesAttr, qIndices);
}

AttrMap prepareUnitAttributes(const MatrixWorkspace_sptr &workspace, std::string iUnit = "") {
  AttrMap iAttributes;
  if (iUnit.empty()) {
    iUnit = getIntensityUnit(workspace);
  }
  iAttributes.emplace(sasUnitAttr, iUnit);
  iAttributes.emplace(sasUncertaintyAttr, sasDataIdev);
  iAttributes.emplace(sasUncertaintiesAttr, sasDataIdev);
  return iAttributes;
}

template <typename Functor>
void write2DWorkspace(H5::Group &group, const MatrixWorkspace_sptr &workspace, const std::string &dataSetName,
                      Functor func, const AttrMap &attributes) {
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
  H5::DataType dataType(H5Util::getType<double>());

  // Get the proplist with compression settings
  auto const propList = setCompression(rank, sizeOfSingleSlab);

  // Create the data set
  auto dataSet = group.createDataSet(dataSetName, dataType, fileSpace, propList);

  // Create Data Space for 1D entry for each row in memory
  hsize_t memSpaceDimension[1] = {dimension1};
  H5::DataSpace memSpace(1, memSpaceDimension);

  // Insert each row of the workspace as a slab
  for (size_t index = 0; index < dimension0; ++index) {
    // Need the data space
    fileSpace.selectHyperslab(H5S_SELECT_SET, sizeOfSingleSlab, start);

    // Write the correct data set to file
    dataSet.write(func(workspace, index), dataType, memSpace, fileSpace);
    // Step up the write position
    ++start[0];
  }
  writeDataSetAttributes(dataSet, attributes);
}

void addQ1D(H5::Group &data, const MatrixWorkspace_sptr &workspace) {
  AttrMap qAttributes;
  // Prepare units
  auto qUnit = getMDUnit(workspace->getDimension(0));
  qAttributes.emplace(sasUnitAttr, qUnit);

  // Add Qdev with units if available
  if (workspace->hasDx(0)) {
    H5Util::writeStrAttribute(data, sasDataQUncertaintyAttr, sasDataQdev);
    H5Util::writeStrAttribute(data, sasDataQUncertaintiesAttr, sasDataQdev);

    qAttributes.emplace(sasUncertaintyAttr, sasDataQdev);
    qAttributes.emplace(sasUncertaintiesAttr, sasDataQdev);

    const auto qResolution = workspace->pointStandardDeviations(0);
    AttrMap xUncertaintyAttributes;
    xUncertaintyAttributes.emplace(sasUnitAttr, qUnit);
    writeArray1DWithStrAttributes(data, sasDataQdev, qResolution.rawData(), xUncertaintyAttributes);
  }

  // We finally add the Q data with necessary attributes
  const auto &qValue = workspace->points(0);
  writeArray1DWithStrAttributes(data, sasDataQ, qValue.rawData(), qAttributes);
}

void addQ2D(H5::Group &data, const MatrixWorkspace_sptr &workspace) {
  // Store the 2D Qx data + units
  AttrMap qxAttributes;
  auto qxUnit = getMDUnit(workspace->getDimension(0));
  qxAttributes.emplace(sasUnitAttr, qxUnit);
  const QxExtractor<double> qxExtractor;
  write2DWorkspace(data, workspace, sasDataQx, qxExtractor, qxAttributes);

  // Get 2D Qy data and store it
  AttrMap qyAttributes;
  auto qyUnit = getMDUnit(workspace->getDimension(1));
  qyAttributes.emplace(sasUnitAttr, qyUnit);

  const SpectrumAxisValueProvider spectrumAxisValueProvider(workspace);
  write2DWorkspace(data, workspace, sasDataQy, spectrumAxisValueProvider, qyAttributes);
}

} // namespace

namespace Mantid::DataHandling::NXcanSAS {

std::string addDigit(size_t index) { return index < 10 ? "0" + std::to_string(index) : std::to_string(index); }

std::filesystem::path prepareFilename(const std::string &baseFilename, bool addDigitSuffix, size_t index) {
  auto path = std::filesystem::path(baseFilename);
  if (!addDigitSuffix) {
    // return early if it doesn't need to add digits
    return path.replace_extension(NX_CANSAS_EXTENSION);
  }
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
 * @param input: Input string in which to apply the relaxed format
 */
std::string makeCanSASRelaxedName(const std::string &input) {
  bool isStrict = false;
  auto emptyCapitalizationStrategy = [](std::string &) {};
  return makeCompliantName(input, isStrict, emptyCapitalizationStrategy);
}

/**
 * Adds detector info to the sas group
 * @param group: The sasEntry
 * @param workspace: The workspace from which to extract detector info
 * @param detectorNames: The names of the detectors to store
 */
void addDetectors(H5::Group &group, const MatrixWorkspace_sptr &workspace,
                  const std::vector<std::string> &detectorNames) {
  // If the group is empty then don't add anything
  for (const auto &detectorName : detectorNames) {
    if (!detectorName.empty()) {
      std::string sasDetectorName = sasInstrumentDetectorGroupName + detectorName;
      sasDetectorName = makeCanSASRelaxedName(sasDetectorName);

      auto instrument = workspace->getInstrument();
      auto component = instrument->getComponentByName(detectorName);

      if (component) {
        const auto sample = instrument->getSample();
        const auto distance = component->getDistance(*sample);
        AttrMap sddAttributes;
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
 * @param group: The sasEntry
 * @param workspace: The workspace which is being stored
 * @param radiationSource: The selected radiation source
 * @param detectorNames: The names of the detectors to store
 * @param geometry: Geometry type of collimation
 * @param beamHeight: Height of collimation element in mm
 * @param beamWidth: Width of collimation element in mm
 */
void addInstrument(H5::Group &group, const MatrixWorkspace_sptr &workspace, const std::string &radiationSource,
                   const std::string &geometry, double beamHeight, double beamWidth,
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

  AttrMap beamSizeAttrs;
  beamSizeAttrs.insert(std::make_pair(sasUnitAttr, sasBeamAndSampleSizeUnitAttrValue));
  if (beamHeight != 0) {
    H5Util::writeScalarDataSetWithStrAttributes(aperture, sasInstrumentApertureGapHeight, beamHeight, beamSizeAttrs);
  }
  if (beamWidth != 0) {
    H5Util::writeScalarDataSetWithStrAttributes(aperture, sasInstrumentApertureGapWidth, beamWidth, beamSizeAttrs);
  }

  // Add IDF information
  std::string idf;
  // try-catch added to use test instrument in testing
  try {
    idf = getIDF(workspace);
  } catch (const std::runtime_error &) {
    idf = "unknown";
  }

  H5Util::write(instrument, sasInstrumentIDF, idf);
}

/**
 * Add the polarizer component information to the instrument cansas group
 * @param group: The sasEntry
 * @param workspace: The workspace from which component information is extracted
 * @param componentName: The name of the polarizer component as found in the IDF
 * @param componentType: Type of component: flipper, analyzer or polarizer
 * @param groupSuffix: Suffix to add to the component group when there is more than one component per type
 */
void addPolarizer(H5::Group &group, const MatrixWorkspace_sptr &workspace, const std::string &componentName,
                  const std::string &componentType, const std::string &groupSuffix) {

  auto const instrumentAttr = InstrumentPolarizer(componentType, componentName);
  auto instrumentGroup =
      group.exists(sasInstrumentGroupName)
          ? group.openGroup(sasInstrumentGroupName)
          : H5Util::createGroupCanSAS(group, sasInstrumentGroupName, nxInstrumentClassAttr, sasInstrumentClassAttr);

  auto const instrument = workspace->getInstrument();
  auto const component = instrument->getComponentByName(instrumentAttr.getComponentName());

  if (component) {
    auto polarizerGroup =
        H5Util::createGroupCanSAS(instrumentGroup, instrumentAttr.sasPolarizerGroupAttr() + groupSuffix,
                                  instrumentAttr.nxPolarizerClassAttr(), instrumentAttr.sasPolarizerClassAttr());

    auto const type = component->getStringParameter(instrumentAttr.sasPolarizerIDFDeviceType());
    H5Util::write(polarizerGroup, instrumentAttr.sasPolarizerName(), componentName);
    H5Util::write(polarizerGroup, instrumentAttr.sasPolarizerDeviceType(), type.front());

    // Calculate Z distance from component to sample
    auto const samplePos = instrument->getSample()->getPos();
    auto const compPos = component->getPos();
    auto distance = samplePos.Z() - compPos.Z();

    AttrMap distanceAttrs;
    distanceAttrs.insert(std::make_pair(sasUnitAttr, instrumentAttr.sasPolarizerDistanceUnitAttr()));
    H5Util::writeScalarDataSetWithStrAttributes(polarizerGroup, instrumentAttr.sasPolarizerDistance(), distance,
                                                distanceAttrs);
  }
}

/**
 * Adds the Field directon of either the magnetic or the electric field on the sample.
 * @param group: The sas group to add the data to.
 * @param emFieldDir: Comma separated string representing spherical vector with directions polar,azimuthal androtation.
 */
void addEMFieldDirection(H5::Group &group, const std::string &emFieldDir) {
  // Expect to receive a comma separated string with directions polar,azimuthal and rotation.
  const auto directions = Mantid::Kernel::VectorHelper::splitStringIntoVector<double>(emFieldDir);
  const auto angles = std::vector<std::string>{sasSampleEMFieldDirectionPolar, sasSampleEMFieldDirectionAzimuthal,
                                               sasSampleEMFieldDirectionRotation};

  if (!directions.empty()) {
    AttrMap magFieldAttrs;
    magFieldAttrs.insert(std::make_pair(sasUnitAttr, sasSampleEMFieldDirectionUnitsAttr));
    for (size_t i = 0; i < directions.size(); i++)
      H5Util::writeScalarDataSetWithStrAttributes(group, angles.at(i), directions.at(i), magFieldAttrs);
  }
}

/**
 * Adds the direction and strength of either magnetic or electric field on the sample.
 * @param group: The sas group to add the data to.
 * @param workspace: The workspace from which to extract the information
 * @param emFieldDir: Comma separated string representing spherical vector with directions polar,azimuthal androtation.
 */
void addSampleEMFields(H5::Group &group, const MatrixWorkspace_sptr &workspace, const std::string &emFieldStrengthLog,
                       const std::string &emFieldDir) {
  auto sampleGroup = group.exists(sasInstrumentSampleGroupAttr)
                         ? group.openGroup(sasInstrumentSampleGroupAttr)
                         : H5Util::createGroupCanSAS(group, sasInstrumentSampleGroupAttr, nxInstrumentSampleClassAttr,
                                                     sasInstrumentSampleClassAttr);

  // Field Strength
  if (workspace->run().hasProperty(emFieldStrengthLog)) {
    auto magFStrength = workspace->run().getLogAsSingleValue(emFieldStrengthLog);
    auto const magFStrengthUnits = workspace->run().getProperty(emFieldStrengthLog)->units();

    AttrMap magFieldAttrs;
    if (!magFStrengthUnits.empty()) {
      magFieldAttrs.insert(std::make_pair(sasUnitAttr, magFStrengthUnits));
    }
    H5Util::writeScalarDataSetWithStrAttributes(sampleGroup, sasSampleMagneticField, magFStrength, magFieldAttrs);
  }

  // Field Direction
  addEMFieldDirection(sampleGroup, emFieldDir);
}

/**
 * Adds sample thickness information to the sas sample group
 * @param group: The parent sas group in which to store the sample group
 * @param sampleThickness: Sample thickness
 */

void addSample(H5::Group &group, const double &sampleThickness) {
  if (sampleThickness == 0) {
    return;
  }
  std::string const sasSampleNameForGroup = sasInstrumentSampleGroupAttr;

  auto sample = H5Util::createGroupCanSAS(group, sasSampleNameForGroup, nxInstrumentSampleClassAttr,
                                          sasInstrumentSampleClassAttr);

  AttrMap sampleThicknessAttrs;
  sampleThicknessAttrs.insert(std::make_pair(sasUnitAttr, sasBeamAndSampleSizeUnitAttrValue));
  H5Util::writeScalarDataSetWithStrAttributes(sample, sasInstrumentSampleThickness, sampleThickness,
                                              sampleThicknessAttrs);
}

/**
 * Add the process information to the NXcanSAS file. It contains information
 * about the run number, the Mantid version and the user file (if available)
 * @param group: The sasEntry
 * @param workspace: The workspace which is being stored
 * @param canWorkspace: Workspace for the can run
 */
void addProcess(H5::Group &group, const MatrixWorkspace_sptr &workspace, const MatrixWorkspace_sptr &canWorkspace) {
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

  if (canWorkspace) {
    // Add can run number
    const auto canRun = canWorkspace->getRunNumber();
    H5Util::write(process, sasProcessTermCan, std::to_string(canRun));
  }
}

/**
 * Add a transmission group to the cansas file, including metadata extracted from the transmission workspace
 * @param group: The sasEntry
 * @param workspace: The transmission workspace
 * @param transmissionName: Suffix for the transmission group
 */
void addTransmission(H5::Group &group, const MatrixWorkspace_const_sptr &workspace,
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
  AttrMap transmissionAttributes;
  std::string unit = sasNone;

  transmissionAttributes.emplace(sasUnitAttr, unit);
  transmissionAttributes.emplace(sasUncertaintyAttr, sasTransmissionSpectrumTdev);
  transmissionAttributes.emplace(sasUncertaintiesAttr, sasTransmissionSpectrumTdev);

  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumT, transmissionData.rawData(),
                                transmissionAttributes);

  //-----------------------------------------
  // Add Tdev with units
  const auto &transmissionErrors = workspace->e(0);
  AttrMap transmissionErrorAttributes;
  transmissionErrorAttributes.emplace(sasUnitAttr, unit);

  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumTdev, transmissionErrors.rawData(),
                                transmissionErrorAttributes);

  //-----------------------------------------
  // Add lambda with units
  const auto lambda = workspace->points(0);
  AttrMap lambdaAttributes;
  auto lambdaUnit = getMDUnit(workspace->getDimension(0), "Angstrom", sasAngstrom);
  lambdaAttributes.emplace(sasUnitAttr, lambdaUnit);
  writeArray1DWithStrAttributes(transmission, sasTransmissionSpectrumLambda, lambda.rawData(), lambdaAttributes);
}

/**
 * Adds signal and Q data to the data group from 1D reduced SANS data
 * @param group: The data group
 * @param workspace: The reduced SANS workspace
 */
void addData1D(H5::Group &data, const MatrixWorkspace_sptr &workspace) {
  // Add attributes for @signal, @I_axes, @Q_indices,
  writeStandardDataAttributes(data);
  addQ1D(data, workspace);
  //-----------------------------------------
  // Add I with units + uncertainty definition
  const auto &intensity = workspace->y(0);
  auto iAttributes = prepareUnitAttributes(workspace);
  writeArray1DWithStrAttributes(data, sasDataI, intensity.rawData(), iAttributes);

  //-----------------------------------------
  // Add Idev with units
  const auto &intensityUncertainty = workspace->e(0);
  AttrMap eAttributes;
  eAttributes.insert(std::make_pair(sasUnitAttr, iAttributes[sasUnitAttr])); // same units as intensity

  writeArray1DWithStrAttributes(data, sasDataIdev, intensityUncertainty.rawData(), eAttributes);
}

/**
 * Stores the 2D signal and Q data in the HDF5 file. Qx and Qy values are stored as a
 * meshgrid of point data.
 * @param data: The data group
 * @param workspace: The reduced 2D sans workspace
 *
 * Workspace shape Mantid Matrix
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
 * The layout below is how it would appear in the HDFView. The vertical axis
 * is the first dimension. We map the Mantid Matrix layout 1-to-1. Note that this
 * will swap the matrix indices, but this is how it is done in the other 2Dloaders
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
void addData2D(H5::Group &data, const MatrixWorkspace_sptr &workspace) {

  const std::string sasDataIAxesAttr2D = sasDataQ + sasSeparator + sasDataQ;
  // Add attributes for @signal, @I_axes, @Q_indices,
  writeStandardDataAttributes(data, sasDataIAxesAttr2D, std::vector<int>{0, 1});

  addQ2D(data, workspace);
  // Get 2D I data and store it
  auto iAttributes = prepareUnitAttributes(workspace);

  auto iExtractor = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->dataY(index).data(); };
  write2DWorkspace(data, workspace, sasDataI, iExtractor, iAttributes);

  // Get 2D Idev data and store it
  AttrMap eAttributes;
  eAttributes.insert(std::make_pair(sasUnitAttr, iAttributes[sasUnitAttr])); // same units as intensity

  auto iDevExtractor = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->dataE(index).data(); };
  write2DWorkspace(data, workspace, sasDataIdev, iDevExtractor, eAttributes);
}

/**
 * Adds signal, Q and spin data to the data group from 1D or 2D reduced polarized SANS data
 * @param data: The data group
 * @param wsGroup: The reduced SANS workspace group
 * @param inputSpinStates: A string of comma separated PinPout pairs, each corresponding to a workspace on wsGroup.
 */
void addPolarizedData(H5::Group &data, const WorkspaceGroup_sptr &wsGroup, const std::string &inputSpinStates) {

  // Workspace form which to extract metadata
  auto const ws0 = std::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(0));
  auto const dim = getWorkspaceDimensionality(ws0);

  //  Add attributes for @signal, @I_axes, @Q_indices,
  std::string sasDataIAxesAttrSpin = sasDataPin + sasSeparator + sasDataPout + sasSeparator + sasDataQ;
  std::vector<int> qIndices = {0, 1, 2};
  if (dim == WorkspaceDimensionality::twoD) {
    sasDataIAxesAttrSpin += sasSeparator + sasDataQ;
    qIndices.push_back(3);
  }

  writeStandardDataAttributes(data, sasDataIAxesAttrSpin, qIndices);

  // store Pin/ Pout
  H5Util::writeNumAttribute(data, sasDataPinIndicesAttr, sasDataPinIndicesValue);
  H5Util::writeNumAttribute(data, sasDataPoutIndicesAttr, sasDataPoutIndicesValue);
  AttrMap polAttributes;
  polAttributes.emplace(sasUnitAttr, sasDataPolarizationUnitAttr);
  auto const inputSpinOrder = Algorithms::PolarizationCorrectionsHelpers::splitSpinStateString(inputSpinStates);
  auto const spinPairs = SpinStateHelper(inputSpinOrder);
  writeArray1DWithStrAttributes(data, sasDataPin, spinPairs.Pin, polAttributes);
  writeArray1DWithStrAttributes(data, sasDataPout, spinPairs.Pout, polAttributes);

  // add Q
  switch (dim) {
  case (WorkspaceDimensionality::oneD):
    addQ1D(data, ws0);
    break;
  case (WorkspaceDimensionality::twoD):
    addQ2D(data, ws0);
    break;
  default:
    throw(std::invalid_argument("Incorrect dimension for workspace"));
  }

  // Add I with units + uncertainty definition
  auto iAttributes = prepareUnitAttributes(ws0);

  // add signal
  WorkspaceGroupDataExtractor<double> wsGroupExtractor(wsGroup);
  writePolarizedData(data, wsGroup, wsGroupExtractor, sasDataI, spinPairs, iAttributes);

  // add signal error
  wsGroupExtractor.setExtractErrors(true);
  AttrMap eAttributes;
  eAttributes.insert(std::make_pair(sasUnitAttr, iAttributes[sasUnitAttr]));
  writePolarizedData(data, wsGroup, wsGroupExtractor, sasDataIdev, spinPairs, eAttributes);
}

/**
 * Retrieves workspace dimensionality enum value: oneD , twoD, other (error)
 * @param workspace: The workspace from which to get the data dimensionality
 *
 */
WorkspaceDimensionality getWorkspaceDimensionality(const MatrixWorkspace_sptr &workspace) {
  auto const numberOfHists = workspace->getNumberHistograms();
  return static_cast<WorkspaceDimensionality>(numberOfHists > 1 ? 2 : numberOfHists);
}

/**
 * Creates and opens a H5 File in the given path
 * @param path: Path in which to create the file
 */
H5::H5File prepareFile(const std::filesystem::path &path) {
  // Prepare file
  if (!path.empty()) {
    std::filesystem::remove(path);
  }
  return H5::H5File(path.string(), H5F_ACC_EXCL, NeXus::H5Util::defaultFileAcc());
}

} // namespace Mantid::DataHandling::NXcanSAS
