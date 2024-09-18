// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PDDetermineCharacterizations.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid::Algorithms {

using Mantid::API::ITableWorkspace_const_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyManagerDataService;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::TimeSeriesProperty;

namespace { // anonymous namespace
const std::string CHAR_PROP_NAME("Characterizations");
const std::string FREQ_PROP_NAME("FrequencyLogNames");
const std::string WL_PROP_NAME("WaveLengthLogNames");
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDDetermineCharacterizations)

/// Algorithms name for identification. @see Algorithm::name
const std::string PDDetermineCharacterizations::name() const { return "PDDetermineCharacterizations"; }

/// Algorithm's version for identification. @see Algorithm::version
int PDDetermineCharacterizations::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PDDetermineCharacterizations::category() const {
  return "Workflow\\Diffraction\\UsesPropertyManager";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PDDetermineCharacterizations::summary() const {
  return "Determines the characterizations of a workspace.";
}

/**
 * These should match those in LoadPDCharacterizations
 * - "frequency" double
 * - "wavelength" (double)
 * - "bank" (integer)
 * - "container" (string)
 * - "vanadium" (string)
 * - "vanadium_background" (string)
 * - "empty" (string)
 * - "d_min" (string)
 * - "d_max" (string)
 * - "tof_min" (double)
 * - "tof_max" (double)
 * - "wavelength_min" (double)
 * - "wavelength_max" (double)
 * @return The list of expected column names
 */
std::vector<std::string> getColumnNames() {
  return {"frequency",         "wavelength",       "bank",  "container", "vanadium", "vanadium_background",
          "empty_environment", "empty_instrument", "d_min", "d_max",     "tof_min",  "tof_max",
          "wavelength_min",    "wavelength_max"};
}

/// More intesive input checking. @see Algorithm::validateInputs
std::map<std::string, std::string> PDDetermineCharacterizations::validateInputs() {
  std::map<std::string, std::string> result;

  ITableWorkspace_const_sptr characterizations = getProperty(CHAR_PROP_NAME);

  if (!bool(characterizations))
    return result;

  std::vector<std::string> expectedNames = getColumnNames();
  std::vector<std::string> names = characterizations->getColumnNames();
  if (names.size() < expectedNames.size()) { // allow for extra columns
    std::stringstream msg;
    msg << "Encountered invalid number of columns in "
        << "TableWorkspace. Found " << names.size() << " expected " << expectedNames.size();
    result[CHAR_PROP_NAME] = msg.str();
  } else {
    for (auto &expectedName : expectedNames) {
      if (std::find(names.begin(), names.end(), expectedName) == names.end()) {
        std::stringstream msg;
        msg << "Failed to find column named " << expectedName;
        result[CHAR_PROP_NAME] = msg.str();
      }
    }
  }
  return result;
}

/// Initialize the algorithm's properties.
void PDDetermineCharacterizations::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, API::PropertyMode::Optional),
      "Workspace with logs to help identify frequency and wavelength");

  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(CHAR_PROP_NAME, "", Direction::Input,
                                                                            API::PropertyMode::Optional),
                  "Table of characterization information");

  declareProperty("ReductionProperties", "__pd_reduction_properties", "Property manager name for the reduction");

  const std::string defaultMsg = " run to use. 0 to use value in table, -1 to not use.";

  declareProperty(std::make_unique<Kernel::ArrayProperty<int32_t>>("BackRun", "0"), "Empty container" + defaultMsg);
  declareProperty(std::make_unique<Kernel::ArrayProperty<int32_t>>("NormRun", "0"), "Normalization" + defaultMsg);
  declareProperty(std::make_unique<Kernel::ArrayProperty<int32_t>>("NormBackRun", "0"),
                  "Normalization background" + defaultMsg);
  declareProperty(std::make_unique<Kernel::ArrayProperty<int32_t>>("EmptyEnv", "0"),
                  "Empty sample environment" + defaultMsg);
  declareProperty(std::make_unique<Kernel::ArrayProperty<int32_t>>("EmptyInstr", "0"), "Empty instrument" + defaultMsg);

  std::vector<std::string> defaultFrequencyNames{"SpeedRequest1", "Speed1", "frequency", "skf1.speed"};
  declareProperty(
      std::make_unique<Kernel::ArrayProperty<std::string>>(FREQ_PROP_NAME, std::move(defaultFrequencyNames)),
      "Candidate log names for frequency");
  // NOTE: beamline specific log names goes to the end of the list
  std::vector<std::string> defaultWavelengthNames{"LambdaRequest", "lambda", "skf12.lambda", "BL1B:Det:TH:BL:Lambda",
                                                  "freq"};
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>(WL_PROP_NAME, std::move(defaultWavelengthNames)),
                  "Candidate log names for wave length");
}

/**
 * Compare two numbers to be in agreement within 5%
 * @param left
 * @param right
 * @return
 */
bool closeEnough(const double left, const double right) {
  // the same value
  const double diff = fabs(left - right);
  if (diff == 0.)
    return true;

  // same within 5%
  const double relativeDiff = diff * 2 / (left + right);
  return relativeDiff < .05;
}

/// Fill in the property manager from the correct line in the table
void PDDetermineCharacterizations::getInformationFromTable(const double frequency, const double wavelength,
                                                           const std::string &canName) {
  size_t numRows = m_characterizations->rowCount();

  for (size_t i = 0; i < numRows; ++i) {
    const double rowFrequency = m_characterizations->getRef<double>("frequency", i);
    const double rowWavelength = m_characterizations->getRef<double>("wavelength", i);

    if (closeEnough(frequency, rowFrequency) && closeEnough(wavelength, rowWavelength)) {

      // declare how the row was chosen
      g_log.information() << "Using information from row " << i << " with frequency = " << rowFrequency
                          << " and wavelength = " << rowWavelength << "\n";
      m_propertyManager->setProperty("frequency", frequency);
      m_propertyManager->setProperty("wavelength", wavelength);

      // what bank number this should be called - only used at POWGEN
      m_propertyManager->setProperty("bank", m_characterizations->getRef<int>("bank", i));

      // data ranges
      m_propertyManager->setPropertyValue("d_min", m_characterizations->getRef<std::string>("d_min", i));
      m_propertyManager->setPropertyValue("d_max", m_characterizations->getRef<std::string>("d_max", i));
      m_propertyManager->setProperty("tof_min", m_characterizations->getRef<double>("tof_min", i));
      m_propertyManager->setProperty("tof_max", m_characterizations->getRef<double>("tof_max", i));
      m_propertyManager->setProperty("wavelength_min", m_characterizations->getRef<double>("wavelength_min", i));
      m_propertyManager->setProperty("wavelength_max", m_characterizations->getRef<double>("wavelength_max", i));

      // characterization run numbers
      m_propertyManager->setProperty("vanadium", m_characterizations->getRef<std::string>("vanadium", i));
      m_propertyManager->setProperty("vanadium_background",
                                     m_characterizations->getRef<std::string>("vanadium_background", i));
      m_propertyManager->setProperty("container", m_characterizations->getRef<std::string>("container", i));
      m_propertyManager->setProperty("empty_environment",
                                     m_characterizations->getRef<std::string>("empty_environment", i));
      m_propertyManager->setProperty("empty_instrument",
                                     m_characterizations->getRef<std::string>("empty_instrument", i));

      // something special if the container was specified
      if (!canName.empty()) {
        const auto columnNames = m_characterizations->getColumnNames();
        if (std::find(columnNames.begin(), columnNames.end(), canName) == columnNames.end()) {
          g_log.warning() << "Failed to find container name \"" << canName << "\" in characterizations table \""
                          << m_characterizations->getName() << " - using default container value\n";
        } else {
          const auto canRuns = m_characterizations->getRef<std::string>(canName, i);
          g_log.information() << "Updating container identifier to \"" << canRuns << "\"\n";
          m_propertyManager->setProperty("container", canRuns);
        }
      }

      return;
    }
  }
  g_log.warning("Failed to find compatible row in characterizations table");
}

/**
 * Get a value from one of a set of logs.
 * @param run
 * @param propName
 * @return
 */
double PDDetermineCharacterizations::getLogValue(const API::Run &run, const std::string &propName) {
  std::vector<std::string> propNames = getProperty(propName);

  std::string label = "frequency";
  if (propName == WL_PROP_NAME)
    label = "wavelength";

  std::unordered_set<std::string> validUnits;
  if (propName == WL_PROP_NAME) {
    validUnits.insert("Angstrom");
    validUnits.insert("A");
  } else {
    validUnits.insert("Hz");
  }

  for (auto &propertyName : propNames) {
    if (run.hasProperty(propertyName)) {
      const std::string units = run.getProperty(propertyName)->units();

      if (validUnits.find(units) != validUnits.end()) {
        const double value = run.getLogAsSingleValue(propertyName, Kernel::Math::TimeAveragedMean);
        if (value == 0.) {
          std::stringstream msg;
          msg << "'" << propertyName << "' has a mean value of zero " << units;
          g_log.information(msg.str());
        } else {
          std::stringstream msg;
          msg << "Found " << label << " in log '" << propertyName << "' with mean value " << value << " " << units;
          g_log.information(msg.str());
          return value;
        }
      } else {
        std::stringstream msg;
        msg << "When looking at " << propertyName << " log encountered unknown units '" << units << "' for " << label
            << ":" << units;
        g_log.warning(msg.str());
      }
    }
  }

  // generate an exception if it gets here because the log wasn't found
  std::stringstream msg;
  msg << "Failed to determine " << label << " because none of the logs ";
  for (auto &propertyName : propNames) {
    msg << "\"" << propertyName << "\" ";
  }
  msg << "exist";
  throw std::runtime_error(msg.str());
}

/// Set the default values in the property manager
void PDDetermineCharacterizations::setDefaultsInPropManager() {
  if (!m_propertyManager->existsProperty("frequency")) {
    m_propertyManager->declareProperty(std::make_unique<PropertyWithValue<double>>("frequency", 0.));
  }
  if (!m_propertyManager->existsProperty("wavelength")) {
    m_propertyManager->declareProperty(std::make_unique<PropertyWithValue<double>>("wavelength", 0.));
  }

  if (!m_propertyManager->existsProperty("bank")) {
    m_propertyManager->declareProperty(std::make_unique<PropertyWithValue<int>>("bank", 1));
  }

  if (!m_propertyManager->existsProperty("d_min")) {
    m_propertyManager->declareProperty(std::make_unique<ArrayProperty<double>>("d_min"));
  }
  if (!m_propertyManager->existsProperty("d_max")) {
    m_propertyManager->declareProperty(std::make_unique<ArrayProperty<double>>("d_max"));
  }
  if (!m_propertyManager->existsProperty("tof_min")) {
    m_propertyManager->declareProperty(std::make_unique<PropertyWithValue<double>>("tof_min", 0.));
  }
  if (!m_propertyManager->existsProperty("tof_max")) {
    m_propertyManager->declareProperty(std::make_unique<PropertyWithValue<double>>("tof_max", 0.));
  }
  if (!m_propertyManager->existsProperty("wavelength_min")) {
    m_propertyManager->declareProperty(std::make_unique<PropertyWithValue<double>>("wavelength_min", 0.));
  }
  if (!m_propertyManager->existsProperty("wavelength_max")) {
    m_propertyManager->declareProperty(std::make_unique<PropertyWithValue<double>>("wavelength_max", 0.));
  }

  if (!m_propertyManager->existsProperty("vanadium")) {
    m_propertyManager->declareProperty(std::make_unique<ArrayProperty<int32_t>>("vanadium", "0"));
  }
  if (!m_propertyManager->existsProperty("vanadium_background")) {
    m_propertyManager->declareProperty(std::make_unique<ArrayProperty<int32_t>>("vanadium_background", "0"));
  }
  if (!m_propertyManager->existsProperty("container")) {
    m_propertyManager->declareProperty(std::make_unique<ArrayProperty<int32_t>>("container", "0"));
  }
  if (!m_propertyManager->existsProperty("empty_environment")) {
    m_propertyManager->declareProperty(std::make_unique<ArrayProperty<int32_t>>("empty_environment", "0"));
  }
  if (!m_propertyManager->existsProperty("empty_instrument")) {
    m_propertyManager->declareProperty(std::make_unique<ArrayProperty<int32_t>>("empty_instrument", "0"));
  }
}

/**
 * Set the run number in the property manager from algoritm inputs.
 * @param inputName
 * @param propName
 */
void PDDetermineCharacterizations::overrideRunNumProperty(const std::string &inputName, const std::string &propName) {
  if (this->isDefault(inputName))
    return;

  std::vector<int32_t> runnumbers = this->getProperty(inputName);
  if ((!runnumbers.empty()) && (runnumbers[0] != 0)) {
    if (runnumbers[0] < 0) {
      runnumbers.erase(runnumbers.begin(), runnumbers.end());
      runnumbers.emplace_back(0);
    }
    m_propertyManager->setProperty(propName, runnumbers);
  }
}

/// Execute the algorithm.
void PDDetermineCharacterizations::exec() {
  // setup property manager to return
  const std::string managerName = getPropertyValue("ReductionProperties");
  if (PropertyManagerDataService::Instance().doesExist(managerName)) {
    m_propertyManager = PropertyManagerDataService::Instance().retrieve(managerName);
  } else {
    m_propertyManager = std::make_shared<Kernel::PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(managerName, m_propertyManager);
  }

  setDefaultsInPropManager();

  m_characterizations = getProperty(CHAR_PROP_NAME);
  if (bool(m_characterizations) && (m_characterizations->rowCount() > 0)) {
    API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    auto run = inputWS->mutableRun();

    double frequency = getLogValue(run, FREQ_PROP_NAME);

    double wavelength = getLogValue(run, WL_PROP_NAME);

    // determine the container name
    std::string container;
    if (run.hasProperty("SampleContainer")) {
      const auto containerProp = run.getLogData("SampleContainer");

      // the property is normally a TimeSeriesProperty
      const auto containerPropSeries = dynamic_cast<TimeSeriesProperty<std::string> *>(containerProp);
      if (containerPropSeries) {
        // assume that only the first value matters
        container = containerPropSeries->valuesAsVector().front();
      } else {
        // try as a normal Property
        container = containerProp->value();
      }

      // remove whitespace from the value
      container = Kernel::Strings::replaceAll(container, " ", "");
    }

    getInformationFromTable(frequency, wavelength, container);
  }

  overrideRunNumProperty("BackRun", "container");
  overrideRunNumProperty("NormRun", "vanadium");
  overrideRunNumProperty("NormBackRun", "vanadium_background");
  overrideRunNumProperty("EmptyEnv", "empty_environment");
  overrideRunNumProperty("EmptyInstr", "empty_instrument");

  std::vector<std::string> expectedNames = getColumnNames();
  for (auto &expectedName : expectedNames) {
    if (m_propertyManager->existsProperty(expectedName)) {
      g_log.debug() << expectedName << ":" << m_propertyManager->getPropertyValue(expectedName) << "\n";
    } else {
      g_log.warning() << expectedName << " DOES NOT EXIST\n";
    }
  }
}

} // namespace Mantid::Algorithms
