#include "MantidAlgorithms/PDDetermineCharacterizations2.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::ITableWorkspace_const_sptr;
using Mantid::API::PropertyManagerDataService;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;

namespace { // anonymous namespace
/// this should match those in LoadPDCharacterizations
const std::vector<std::string> COL_NAMES = {
    "frequency",  // double
    "wavelength", // double
    "bank",       // integer
    "container",  // string
    "vanadium",   // string
    "empty",      // string
    "d_min",      // string
    "d_max",      // string
    "tof_min",    // double
    "tof_max"     // double
};
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDDetermineCharacterizations2)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PDDetermineCharacterizations2::PDDetermineCharacterizations2() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PDDetermineCharacterizations2::~PDDetermineCharacterizations2() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PDDetermineCharacterizations2::name() const {
  return "PDDetermineCharacterizations";
}

/// Algorithm's version for identification. @see Algorithm::version
int PDDetermineCharacterizations2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PDDetermineCharacterizations2::category() const {
  return "Workflow/Diffraction/UsesPropertyManager";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PDDetermineCharacterizations2::summary() const {
  return "Determines the characterizations of a workspace.";
}

//----------------------------------------------------------------------------------------------

std::map<std::string, std::string>
PDDetermineCharacterizations2::validateInputs() {
  std::map<std::string, std::string> result;

  ITableWorkspace_const_sptr characterizations =
      getProperty("Characterizations");

  std::vector<std::string> names = characterizations->getColumnNames();
  if (names.size() < COL_NAMES.size()) { // allow for extra columns
    std::stringstream msg;
    msg << "Encountered invalid number of columns in "
        << "TableWorkspace. Found " << names.size() << " expected "
        << COL_NAMES.size();
    result["Characterizations"] = msg.str();
  } else {
    for (auto it = COL_NAMES.begin(); it != COL_NAMES.end(); ++it) {
      if (std::find(names.begin(), names.end(), *it) == names.end()) {
        std::stringstream msg;
        msg << "Failed to find column named " << (*it);
        result["Characterizations"] = msg.str();
      }
    }
  }
  return result;
}

/** Initialize the algorithm's properties.
 */
void PDDetermineCharacterizations2::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Workspace with logs to help identify frequency and wavelength");

  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
                      "Characterizations", "", Direction::Input),
                  "Table of characterization information");

  declareProperty("ReductionProperties", "__pd_reduction_properties",
                  "Property manager name for the reduction");

  const std::string defaultMsg =
      " run to use. 0 to use value in table, -1 to not use.";

  declareProperty("BackRun", 0, "Empty container" + defaultMsg);
  declareProperty("NormRun", 0, "Normalization" + defaultMsg);
  declareProperty("NormBackRun", 0, "Normalization background" + defaultMsg);

  const std::vector<std::string> DEFAULT_FREQUENCY_NAMES = {
      "SpeedRequest1", "Speed1", "frequency"};
  declareProperty(new Kernel::ArrayProperty<std::string>(
                      "FrequencyLogNames", DEFAULT_FREQUENCY_NAMES),
                  "Candidate log names for frequency");

  const std::vector<std::string> DEFAULT_WAVELENGTH_NAMES = {"LambdaRequest",
                                                             "lambda"};
  declareProperty(new Kernel::ArrayProperty<std::string>(
                      "WaveLengthLogNames", DEFAULT_WAVELENGTH_NAMES),
                  "Candidate log names for wave length");
}

//----------------------------------------------------------------------------------------------

/*
    def processInformation(self, prop_man, info_dict):
        for key in COL_NAMES:
            val = info_dict[key]
            # Convert comma-delimited list to array, else return the original
            # value.
            if type("") == type(val):
                if (len(val)==0) and  (key in DEF_INFO.keys()):
                    val = DEF_INFO[key]
                else:
                    try:
                        val = [float(x) for x in val.split(',')]
                    except ValueError, err:
                        self.log().error("Error to parse key: '%s' value = '%s'.
   " % (str(key), str(val)))
                        raise NotImplementedError(str(err))

            try:
                prop_man[key] = val
            except TypeError:
                # Converter error, so remove old value first
                del prop_man[key]
                prop_man[key] = val

    def closeEnough(self, left, right):
        left = float(left)
        right = float(right)
        if abs(left-right) == 0.:
            return True
        if 100. * abs(left-right)/left < 5.:
            return True
        return False

    def getLine(self, char, frequency, wavelength):
        """ Get line in the characterization file with given frequency and
   wavelength
        """
        # empty dictionary if things are wrong
        if frequency is None or wavelength is None:
            return dict(DEF_INFO)

        # go through every row looking for a match
        result = dict(DEF_INFO)
        icount = 0
        for i in xrange(char.rowCount()):
            row = char.row(i)
            if not self.closeEnough(frequency, row['frequency']):
                continue
            if not self.closeEnough(wavelength, row['wavelength']):
                continue
            result = dict(row)
            icount += 1

        self.log().information("Total %d rows are parsed for frequency = %f,
   wavelength = %f" % (icount, frequency, wavelength))
        return result

    def getFrequency(self, logs, wkspName):
        """ Get frequency from log
        """
        frequencynames = self.getProperty("FrequencyLogNames").value
        self.log().notice("Type of frequency names is %s" %
   (str(type(frequencynames))))
        for name in frequencynames:
            if name in logs.keys():
                frequency = logs[name]
                if frequency.units != "Hz":
                    msg = "When looking at " + name \
                        + " log encountered unknown units for frequency. " \
                        + "Only know how to deal with " \
                        + "frequency in Hz, not " + frequency.units
                    self.log().information(msg)
                else:
                    frequency = frequency.getStatistics().mean
                    if frequency == 0.:
                        self.log().information("'%s' mean value is zero" % name)
                    else:
                        self.log().information("Found frequency in %s log" \
                                                   % name)
                        return frequency
        self.log().warning("Failed to determine frequency in \"%s\"" \
                               % wkspName)
        return None

    def getWavelength(self, logs, dummy_wkspName):
        """ Get wave length
        Wavelength can be given by 2 sample logs, either LambdaRequest or
   lambda.
        And its unit can be either Angstrom or A.
        """
        wavelengthnames = self.getProperty("WaveLengthLogNames").value

        for name in wavelengthnames:
            # skip if not exists
            if name not in logs.keys():
                continue

            # get value
            wavelength = logs[name]

            # unit
            if name == "LambdaRequest":
                if wavelength.units != "Angstrom":
                    msg = "Only know how to deal with LambdaRequest in Angstrom,
   not %s" % (wavelength.units)
                    self.log().warning(msg)
                    break

            elif name == "lambda":
                if wavelength.units != "A":
                    msg = "Only know how to deal with lambda in A, not %s" %
   (wavelength.units)
                    self.log().warning(msg)
                    break

            else:
                if wavelength.units != "Angstrom" and wavelength.units != "A":
                    msg = "Only know how to deal with %s in Angstrom (A) but not
   %s" % (name,\
                            wavelength.units)
                    self.log().warning(msg)
                    break

            # return
            wavelength = wavelength.getStatistics().mean
            if wavelength == 0.:
                self.log().warning("'%s' mean value is zero" % name)
                break
            else:
                return wavelength

        # ENDFOR

        return None
 */

void PDDetermineCharacterizations2::setDefaultsInPropManager() {
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
        new PropertyWithValue<int32_t>("vanadium", 0));
  }
  if (!m_propertyManager->existsProperty("container")) {
    m_propertyManager->declareProperty(
        new PropertyWithValue<int32_t>("container", 0));
  }
  if (!m_propertyManager->existsProperty("empty")) {
    m_propertyManager->declareProperty(
        new PropertyWithValue<int32_t>("empty", 0));
  }
  if (!m_propertyManager->existsProperty("d_min")) {
    m_propertyManager->declareProperty(
        new ArrayProperty<std::vector<double>>("d_min"));
  }
  if (!m_propertyManager->existsProperty("d_max")) {
    m_propertyManager->declareProperty(
        new ArrayProperty<std::vector<double>>("d_max"));
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

void PDDetermineCharacterizations2::overrideRunNumProperty(
    const std::string &inputName, const std::string &propName) {
  int32_t runnumber = this->getProperty(inputName);
  if (runnumber != 0) {
    if (runnumber < 0)
      runnumber = 0;
    m_propertyManager->setProperty(propName, runnumber);
  }
}

/** Execute the algorithm.
 */
void PDDetermineCharacterizations2::exec() {
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

  m_characterizations = getProperty("Characterizations");
  if (bool(m_characterizations) && (m_characterizations->rowCount() > 0)) {
  }

  overrideRunNumProperty("BackRun", "container");
  overrideRunNumProperty("NormRun", "vanadium");
  overrideRunNumProperty("NormBackRun", "empty");

  /*

        # empty characterizations table means return the default values
        char = self.getProperty("Characterizations").value
        if char.rowCount() <= 0:
            for key in COL_NAMES:
                if not manager.existsProperty(key):
                    manager[key] = DEF_INFO[key]
            PropertyManagerDataService.addOrReplace(manager_name, manager)
            return
        wksp = self.getProperty("InputWorkspace").value

        # determine wavelength and frequency
        frequency = self.getFrequency(wksp.getRun(), str(wksp))
        wavelength = self.getWavelength(wksp.getRun(), str(wksp))
        self.log().information("Determined frequency: " + str(frequency) \
                                   + " Hz, center wavelength:" \
                                   + str(wavelength) + " Angstrom")

        # get a row of the table
        info = self.getLine(char, frequency, wavelength)

        # update the characterization runs as necessary
        propNames = ("BackRun",   "NormRun",  "NormBackRun")
        dictNames = ("container", "vanadium", "empty")
        for (propName, dictName) in zip(propNames, dictNames):
            runNum = self.getProperty(propName).value
            if runNum < 0: # reset value
                info[dictName] = 0
            elif runNum > 0: # override value
                info[dictName] = runNum

        # convert to a property manager
        self.processInformation(manager, info)
        PropertyManagerDataService.addOrReplace(manager_name, manager)
*/
  for (auto it = COL_NAMES.begin(); it != COL_NAMES.end(); ++it) {
    if (m_propertyManager->existsProperty(*it)) {
      std::cout << (*it) << ":" << m_propertyManager->getPropertyValue(*it)
                << std::endl;
    } else {
      std::cout << (*it) << " DOES NOT EXIST" << std::endl;
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
