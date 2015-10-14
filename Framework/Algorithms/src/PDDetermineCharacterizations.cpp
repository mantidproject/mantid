#include "MantidAlgorithms/PDDetermineCharacterizations.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::ITableWorkspace_const_sptr;
using Mantid::API::PropertyManagerDataService;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;
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
PDDetermineCharacterizations::~PDDetermineCharacterizations() {}

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
 * @return The list of expected column names
 */
std::vector<std::string> getColumnNames() {
  std::vector<std::string> names;
  names.push_back("frequency");  // double
  names.push_back("wavelength"); // double
  names.push_back("bank");       // integer
  names.push_back("container");  // string
  names.push_back("vanadium");   // string
  names.push_back("empty");      // string
  names.push_back("d_min");      // string
  names.push_back("d_max");      // string
  names.push_back("tof_min");    // double
  names.push_back("tof_max");    // double
  return names;
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
    for (auto it = expectedNames.begin(); it != expectedNames.end(); ++it) {
      if (std::find(names.begin(), names.end(), *it) == names.end()) {
        std::stringstream msg;
        msg << "Failed to find column named " << (*it);
        result[CHAR_PROP_NAME] = msg.str();
      }
    }
  }
  return result;
}

/// Initialize the algorithm's properties.
void PDDetermineCharacterizations::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                              API::PropertyMode::Optional),
      "Workspace with logs to help identify frequency and wavelength");

  declareProperty(
      new WorkspaceProperty<API::ITableWorkspace>(
          CHAR_PROP_NAME, "", Direction::Input, API::PropertyMode::Optional),
      "Table of characterization information");

  declareProperty("ReductionProperties", "__pd_reduction_properties",
                  "Property manager name for the reduction");

  const std::string defaultMsg =
      " run to use. 0 to use value in table, -1 to not use.";

  declareProperty(new Kernel::ArrayProperty<int32_t>("BackRun", "0"),
                  "Empty container" + defaultMsg);
  declareProperty(new Kernel::ArrayProperty<int32_t>("NormRun", "0"),
                  "Normalization" + defaultMsg);
  declareProperty(new Kernel::ArrayProperty<int32_t>("NormBackRun", "0"),
                  "Normalization background" + defaultMsg);

  std::vector<std::string> defaultFrequencyNames;
  defaultFrequencyNames.push_back("SpeedRequest1");
  defaultFrequencyNames.push_back("Speed1");
  defaultFrequencyNames.push_back("frequency");
  declareProperty(new Kernel::ArrayProperty<std::string>(FREQ_PROP_NAME,
                                                         defaultFrequencyNames),
                  "Candidate log names for frequency");

  std::vector<std::string> defaultWavelengthNames;
  defaultWavelengthNames.push_back("LambdaRequest");
  defaultWavelengthNames.push_back("lambda");
  declareProperty(new Kernel::ArrayProperty<std::string>(
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
  if (relativeDiff < .05)
    return true;

  return false;
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

  std::set<std::string> validUnits;
  if (propName == WL_PROP_NAME) {
    validUnits.insert("Angstrom");
    validUnits.insert("A");
  } else {
    validUnits.insert("Hz");
  }

  for (auto name = names.begin(); name != names.end(); ++name) {
    if (run.hasProperty(*name)) {
      const std::string units = run.getProperty(*name)->units();

      if (validUnits.find(units) != validUnits.end()) {
        double value = run.getLogAsSingleValue(*name);
        if (value == 0.) {
          std::stringstream msg;
          msg << "'" << *name << "' has a mean value of zero " << units;
          g_log.information(msg.str());
        } else {
          std::stringstream msg;
          msg << "Found " << label << " in log '" << *name
              << "' with mean value " << value << " " << units;
          g_log.information(msg.str());
          return value;
        }
      } else {
        std::stringstream msg;
        msg << "When looking at " << *name
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
        new PropertyWithValue<double>("frequency", 0.));
  }
  if (!m_propertyManager->existsProperty("wavelength")) {
    m_propertyManager->declareProperty(
        new PropertyWithValue<double>("wavelength", 0.));
  }
  if (!m_propertyManager->existsProperty("bank")) {
    m_propertyManager->declareProperty(new PropertyWithValue<int>("bank", 1));
  }
  if (!m_propertyManager->existsProperty("vanadium")) {
    m_propertyManager->declareProperty(
        new ArrayProperty<int32_t>("vanadium", "0"));
  }
  if (!m_propertyManager->existsProperty("container")) {
    m_propertyManager->declareProperty(
        new ArrayProperty<int32_t>("container", "0"));
  }
  if (!m_propertyManager->existsProperty("empty")) {
    m_propertyManager->declareProperty(
        new ArrayProperty<int32_t>("empty", "0"));
  }
  if (!m_propertyManager->existsProperty("d_min")) {
    m_propertyManager->declareProperty(new ArrayProperty<double>("d_min"));
  }
  if (!m_propertyManager->existsProperty("d_max")) {
    m_propertyManager->declareProperty(new ArrayProperty<double>("d_max"));
  }
  if (!m_propertyManager->existsProperty("tof_min")) {
    m_propertyManager->declareProperty(
        new PropertyWithValue<double>("tof_min", 0.));
  }
  if (!m_propertyManager->existsProperty("tof_max")) {
    m_propertyManager->declareProperty(
        new PropertyWithValue<double>("tof_max", 0.));
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
  for (auto it = expectedNames.begin(); it != expectedNames.end(); ++it) {
    if (m_propertyManager->existsProperty(*it)) {
      g_log.debug() << (*it) << ":" << m_propertyManager->getPropertyValue(*it)
                    << "\n";
    } else {
      g_log.warning() << (*it) << " DOES NOT EXIST\n";
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
