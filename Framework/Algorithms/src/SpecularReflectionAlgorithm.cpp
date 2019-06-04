// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SpecularReflectionAlgorithm.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace {

const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
const std::string lineDetectorAnalysis = "LineDetectorAnalysis";
const std::string pointDetectorAnalysis = "PointDetectorAnalysis";

/**
 * Check the spectrum numbers are contiguous.
 * @param spectrumNumbers : Spectrum numbers to check
 * @param strictSpectrumChecking : True to throw if there are errors.
 * @param logger : Reference to log object
 */
void checkSpectrumNumbers(const std::vector<int> &spectrumNumbers,
                          bool strictSpectrumChecking, Logger &logger) {
  std::unordered_set<int> uniqueSpectrumNumbers(spectrumNumbers.begin(),
                                                spectrumNumbers.end());
  if (uniqueSpectrumNumbers.size() != spectrumNumbers.size()) {
    throw std::invalid_argument("Spectrum numbers are not unique.");
  }
  auto maxIt = std::max_element(uniqueSpectrumNumbers.begin(),
                                uniqueSpectrumNumbers.end());
  auto minIt = std::min_element(uniqueSpectrumNumbers.begin(),
                                uniqueSpectrumNumbers.end());
  const int max = *maxIt;
  const int min = *minIt;
  for (int i = min; i <= max; ++i) {
    if (uniqueSpectrumNumbers.find(i) == uniqueSpectrumNumbers.end()) {
      const std::string message =
          "Spectrum numbers are not a contiguous linear sequence.";
      if (strictSpectrumChecking) {
        throw std::invalid_argument(message);
      } else {
        logger.warning(message);
      }
    }
  }
}
} // namespace

namespace Mantid {
namespace Algorithms {

/**
 * Initialize common properties
 */
void SpecularReflectionAlgorithm::initCommonProperties() {
  std::stringstream message;
  message << "The type of analysis to perform. " << multiDetectorAnalysis
          << ", " << lineDetectorAnalysis << " or " << multiDetectorAnalysis
          << ". Used to help automatically determine the detector components "
             "to move";

  std::vector<std::string> propOptions;
  propOptions.push_back(pointDetectorAnalysis);
  propOptions.push_back(lineDetectorAnalysis);
  propOptions.push_back(multiDetectorAnalysis);

  declareProperty("AnalysisMode", pointDetectorAnalysis,
                  boost::make_shared<StringListValidator>(propOptions),
                  message.str());

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "DetectorComponentName", "", Direction::Input),
                  "Name of the detector component i.e. point-detector. If "
                  "these are not specified, the algorithm will attempt lookup "
                  "using a standard naming convention.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "SampleComponentName", "", Direction::Input),
                  "Name of the sample component i.e. some-surface-holder. If "
                  "these are not specified, the algorithm will attempt lookup "
                  "using a standard naming convention.");

  auto boundedArrayValidator = boost::make_shared<ArrayBoundedValidator<int>>();
  boundedArrayValidator->setLower(0);
  declareProperty(
      std::make_unique<ArrayProperty<int>>("SpectrumNumbersOfDetectors",
                                      boundedArrayValidator, Direction::Input),
      "A list of spectrum numbers making up an effective point detector.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("StrictSpectrumChecking",
                                                       true, Direction::Input),
                  "Enable, disable strict spectrum checking. Strict spectrum "
                  "checking protects against non-sequential integers in which "
                  "spectrum numbers are not in {min, min+1, ..., max}");

  setPropertySettings("SpectrumNumbersOfDetectors",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SampleComponentName", IS_NOT_DEFAULT));
}

/**
 * Get the sample component. Use the name provided as a property as the basis
 *for the lookup as a priority.
 *
 * Throws if the name is invalid.
 * @param inst : Instrument to search through
 * @return : The component : The component object found.
 */
Mantid::Geometry::IComponent_const_sptr
SpecularReflectionAlgorithm::getSurfaceSampleComponent(
    Mantid::Geometry::Instrument_const_sptr inst) const {
  std::string sampleComponent = "some-surface-holder";
  if (!isPropertyDefault("SampleComponentName")) {
    sampleComponent = this->getPropertyValue("SampleComponentName");
  }
  auto searchResult = inst->getComponentByName(sampleComponent);
  if (searchResult == nullptr) {
    throw std::invalid_argument(sampleComponent +
                                " does not exist. Check input properties.");
  }
  return searchResult;
}

/**
 * Get the detector component. Use the name provided as a property as the basis
 *for the lookup as a priority.
 *
 * Throws if the name is invalid.
 * @param workspace : Workspace from instrument with detectors
 * @param isPointDetector : True if this is a point detector. Used to guess a
 *name.
 * @return The component : The component object found.
 */
boost::shared_ptr<const Mantid::Geometry::IComponent>
SpecularReflectionAlgorithm::getDetectorComponent(
    MatrixWorkspace_sptr workspace, const bool isPointDetector) const {
  boost::shared_ptr<const IComponent> searchResult;
  if (!isPropertyDefault("SpectrumNumbersOfDetectors")) {
    const std::vector<int> spectrumNumbers =
        this->getProperty("SpectrumNumbersOfDetectors");
    const bool strictSpectrumChecking =
        this->getProperty("StrictSpectrumChecking");
    checkSpectrumNumbers(spectrumNumbers, strictSpectrumChecking, g_log);
    auto specToWorkspaceIndex = workspace->getSpectrumToWorkspaceIndexMap();
    DetectorGroup_sptr allDetectors = boost::make_shared<DetectorGroup>();
    const auto &spectrumInfo = workspace->spectrumInfo();
    for (auto index : spectrumNumbers) {
      const size_t spectrumNumber{static_cast<size_t>(index)};
      auto it = specToWorkspaceIndex.find(index);
      if (it == specToWorkspaceIndex.end()) {
        std::stringstream message;
        message << "Spectrum number " << spectrumNumber
                << " does not exist in the InputWorkspace";
        throw std::invalid_argument(message.str());
      }
      const size_t workspaceIndex = it->second;
      auto detector = workspace->getDetector(workspaceIndex);
      if (spectrumInfo.isMasked(workspaceIndex))
        g_log.warning() << "Adding a detector (ID:" << detector->getID()
                        << ") that is flagged as masked.\n";
      allDetectors->addDetector(detector);
    }
    searchResult = allDetectors;
  } else {
    Mantid::Geometry::Instrument_const_sptr inst = workspace->getInstrument();
    std::string componentToCorrect =
        isPointDetector ? "point-detector" : "linedetector";

    if (!isPropertyDefault("DetectorComponentName")) {
      componentToCorrect = this->getPropertyValue("DetectorComponentName");
    }
    searchResult = inst->getComponentByName(componentToCorrect);
    if (searchResult == nullptr) {
      throw std::invalid_argument(componentToCorrect +
                                  " does not exist. Check input properties.");
    }
  }

  return searchResult;
}

/**
 * Determine if the property value is the same as the default value.
 * This can be used to determine if the property has not been set.
 * @param propertyName : Name of property to query
 * @return: True only if the property has it's default value.
 */
bool SpecularReflectionAlgorithm::isPropertyDefault(
    const std::string &propertyName) const {
  Property *property = this->getProperty(propertyName);
  return property->isDefault();
}

/**
 * Calculate the twoTheta angle from the detector and sample locations.
 * @return: twoTheta
 */
double SpecularReflectionAlgorithm::calculateTwoTheta() const {
  MatrixWorkspace_sptr inWS = this->getProperty("InputWorkspace");

  const std::string analysisMode = this->getProperty("AnalysisMode");

  Instrument_const_sptr instrument = inWS->getInstrument();

  IComponent_const_sptr detector =
      this->getDetectorComponent(inWS, analysisMode == pointDetectorAnalysis);

  IComponent_const_sptr sample = this->getSurfaceSampleComponent(instrument);

  const V3D detSample = detector->getPos() - sample->getPos();

  boost::shared_ptr<const ReferenceFrame> refFrame =
      instrument->getReferenceFrame();

  const double upoffset = refFrame->vecPointingUp().scalar_prod(detSample);
  const double beamoffset =
      refFrame->vecPointingAlongBeam().scalar_prod(detSample);

  const double twoTheta = std::atan2(upoffset, beamoffset) * 180 / M_PI;

  return twoTheta;
}

} // namespace Algorithms
} // namespace Mantid
