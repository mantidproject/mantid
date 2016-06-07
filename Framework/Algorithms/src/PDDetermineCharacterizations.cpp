#include "MantidAlgorithms/PDDetermineCharacterizations.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

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
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDDetermineCharacterizations)

//----------------------------------------------------------------------------------------------
/// Constructor
PDDetermineCharacterizations::PDDetermineCharacterizations() {}

//----------------------------------------------------------------------------------------------
/// Destructor
PDDetermineCharacterizations::~PDDetermineCharacterizations() = default;

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PDDetermineCharacterizations::name() const {
  return "PDDetermineCharacterizations";
}

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
 * -  "wavelength" (double)
 * -  "bank" (integer)
 * -  "container" (string)
 * -  "vanadium" (string)
 * -  "empty" (string)
 * -  "d_min" (string)
 * -  "d_max" (string)
 * -  "tof_min" (double)
 * -  "tof_max" (double)
 * @return The list of expected column names
 */
std::vector<std::string> getColumnNames() {
  return {"frequency", "wavelength", "bank",  "container", "vanadium",
          "empty",     "d_min",      "d_max", "tof_min",   "tof_max"};
}

/// More intesive input checking. @see Algorithm::validateInputs
std::map<std::string, std::string>
PDDetermineCharacterizations::validateInputs() {
  std::map<std::string, std::string> result;

  ITableWorkspace_const_sptr characterizations = getProperty(CHAR_PROP_NAME);

  if (!bool(characterizations))
    return result;

  std::vector<std::string> expectedNames = getColumnNames();
  std::vector<std::string> names = characterizations->getColumnNames();
  if (names.size() < expectedNames.size()) { // allow for extra columns
    std::stringstream msg;
    msg << "Encountered invalid number of columns in "
        << "TableWorkspace. Found " << names.size() << " expected "
        << expectedNames.size();
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
      Kernel::make_unique<WorkspaceProperty<>>(
          "InputWorkspace", "", Direction::Input, API::PropertyMode::Optional),
      "Workspace with logs to help identify frequency and wavelength");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
          CHAR_PROP_NAME, "", Direction::Input, API::PropertyMode::Optional),
      "Table of characterization information");

  declareProperty("ReductionProperties", "__pd_reduction_properties",
                  "Property manager name for the reduction");

  const std::string defaultMsg =
      " run to use. 0 to use value in table, -1 to not use.";

  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<int32_t>>("BackRun", "0"),
      "Empty container" + defaultMsg);
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<int32_t>>("NormRun", "0"),
      "Normalization" + defaultMsg);
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<int32_t>>("NormBackRun", "0"),
      "Normalization background" + defaultMsg);

  std::vector<std::string> defaultFrequencyNames{"SpeedRequest1", "Speed1",
                                                 "frequency"};
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
                      FREQ_PROP_NAME, defaultFrequencyNames),
                  "Candidate log names for frequency");

  std::vector<std::string> defaultWavelengthNames{"LambdaRequest", "lambda"};
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
                      WL_PROP_NAME, defaultWavelengthNames),
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
void PDDetermineCharacterizations::getInformationFromTable(
    const double frequency, const double wavelength) {
  size_t numRows = m_characterizations->rowCount();

  for (size_t i = 0; i < numRows; ++i) {
    const double rowFrequency =
        m_characterizations->getRef<double>("frequency", i);
    const double rowWavelength =
        m_characterizations->getRef<double>("wavelength", i);

    if (closeEnough(frequency, rowFrequency) &&
        closeEnough(wavelength, rowWavelength)) {
      g_log.information() << "Using information from row " << i
                          << " with frequency = " << rowFrequency
                          << " and wavelength = " << rowWavelength << "\n";

      m_propertyManager->setProperty("frequency", frequency);
      m_propertyManager->setProperty("wavelength", wavelength);

      m_propertyManager->setProperty(
          "bank", m_characterizations->getRef<int>("bank", i));

      m_propertyManager->setProperty(
          "vanadium", m_characterizations->getRef<std::string>("vanadium", i));
      m_propertyManager->setProperty(
          "container",
          m_characterizations->getRef<std::string>("container", i));
      m_propertyManager->setProperty(
          "empty", m_characterizations->getRef<std::string>("empty", i));

      m_propertyManager->setPropertyValue(
          "d_min", m_characterizations->getRef<std::string>("d_min", i));
      m_propertyManager->setPropertyValue(
          "d_max", m_characterizations->getRef<std::string>("d_max", i));

      m_propertyManager->setProperty(
          "tof_min", m_characterizations->getRef<double>("tof_min", i));
      m_propertyManager->setProperty(
          "tof_max", m_characterizations->getRef<double>("tof_max", i));
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
double PDDetermineCharacterizations::getLogValue(API::Run &run,
                                                 const std::string &propName) {
  std::vector<std::string> names = getProperty(propName);

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

  for (auto &name : names) {
    if (run.hasProperty(name)) {
      const std::string units = run.getProperty(name)->units();

      if (validUnits.find(units) != validUnits.end()) {
        double value = run.getLogAsSingleValue(name);
        if (value == 0.) {
          std::stringstream msg;
          msg << "'" << name << "' has a mean value of zero " << units;
          g_log.information(msg.str());
        } else {
          std::stringstream msg;
          msg << "Found " << label << " in log '" << name
              << "' with mean value " << value << " " << units;
          g_log.information(msg.str());
          return value;
        }
      } else {
        std::stringstream msg;
        msg << "When looking at " << name
            << " log encountered unknown units for " << label << ":" << units;
        g_log.warning(msg.str());
      }
    }
  }
  g_log.warning("Failed to determine " + label);
  return 0.;
}

/// Set the default values in the property manager
void PDDetermineCharacterizations::setDefaultsInPropManager() {
  if (!m_propertyManager->existsProperty("frequency")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<PropertyWithValue<double>>("frequency", 0.));
  }
  if (!m_propertyManager->existsProperty("wavelength")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<PropertyWithValue<double>>("wavelength", 0.));
  }
  if (!m_propertyManager->existsProperty("bank")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<PropertyWithValue<int>>("bank", 1));
  }
  if (!m_propertyManager->existsProperty("vanadium")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<ArrayProperty<int32_t>>("vanadium", "0"));
  }
  if (!m_propertyManager->existsProperty("container")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<ArrayProperty<int32_t>>("container", "0"));
  }
  if (!m_propertyManager->existsProperty("empty")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<ArrayProperty<int32_t>>("empty", "0"));
  }
  if (!m_propertyManager->existsProperty("d_min")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<ArrayProperty<double>>("d_min"));
  }
  if (!m_propertyManager->existsProperty("d_max")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<ArrayProperty<double>>("d_max"));
  }
  if (!m_propertyManager->existsProperty("tof_min")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<PropertyWithValue<double>>("tof_min", 0.));
  }
  if (!m_propertyManager->existsProperty("tof_max")) {
    m_propertyManager->declareProperty(
        Kernel::make_unique<PropertyWithValue<double>>("tof_max", 0.));
  }
}

/**
 * Set the run number in the property manager from algoritm inputs.
 * @param inputName
 * @param propName
 */
void PDDetermineCharacterizations::overrideRunNumProperty(
    const std::string &inputName, const std::string &propName) {
  if (this->isDefault(inputName))
    return;

  std::vector<int32_t> runnumbers = this->getProperty(inputName);
  if ((!runnumbers.empty()) && (runnumbers[0] != 0)) {
    if (runnumbers[0] < 0) {
      runnumbers.erase(runnumbers.begin(), runnumbers.end());
      runnumbers.push_back(0);
    }
    m_propertyManager->setProperty(propName, runnumbers);
  }
}

/// Execute the algorithm.
void PDDetermineCharacterizations::exec() {
  // setup property manager to return
  const std::string managerName = getPropertyValue("ReductionProperties");
  if (PropertyManagerDataService::Instance().doesExist(managerName)) {
    m_propertyManager =
        PropertyManagerDataService::Instance().retrieve(managerName);
  } else {
    m_propertyManager = boost::make_shared<Kernel::PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(managerName,
                                                        m_propertyManager);
  }

  setDefaultsInPropManager();

  m_characterizations = getProperty(CHAR_PROP_NAME);
  if (bool(m_characterizations) && (m_characterizations->rowCount() > 0)) {
    API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    auto run = inputWS->mutableRun();

    double frequency = getLogValue(run, FREQ_PROP_NAME);

    double wavelength = getLogValue(run, WL_PROP_NAME);

    getInformationFromTable(frequency, wavelength);
  }

  overrideRunNumProperty("BackRun", "container");
  overrideRunNumProperty("NormRun", "vanadium");
  overrideRunNumProperty("NormBackRun", "empty");

  std::vector<std::string> expectedNames = getColumnNames();
  for (auto &expectedName : expectedNames) {
    if (m_propertyManager->existsProperty(expectedName)) {
      g_log.debug() << expectedName << ":"
                    << m_propertyManager->getPropertyValue(expectedName)
                    << "\n";
    } else {
      g_log.warning() << expectedName << " DOES NOT EXIST\n";
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
