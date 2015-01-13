#include "MantidAPI/ExperimentInfo.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/ModeratorModel.h"

#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>

#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/SAXParser.h>
#include <Poco/ScopedLock.h>
#include <nexus/NeXusException.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Poco::XML;

namespace Mantid {
namespace API {
namespace {
/// static logger object
Kernel::Logger g_log("ExperimentInfo");
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ExperimentInfo::ExperimentInfo()
    : m_moderatorModel(), m_choppers(), m_sample(new Sample()),
      m_run(new Run()), m_parmap(new ParameterMap()),
      sptr_instrument(new Instrument()) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ExperimentInfo::~ExperimentInfo() {}

//---------------------------------------------------------------------------------------
/**
 * Constructs the object from a copy if the input. This leaves the new mutex
 * unlocked.
 * @param source The source object from which to initialize
 */
ExperimentInfo::ExperimentInfo(const ExperimentInfo &source) {
  this->copyExperimentInfoFrom(&source);
}

//---------------------------------------------------------------------------------------
/** Copy the experiment info data from another ExperimentInfo instance,
 * e.g. a MatrixWorkspace.
 * @param other :: the source from which to copy ExperimentInfo
 */
void ExperimentInfo::copyExperimentInfoFrom(const ExperimentInfo *other) {
  m_sample = other->m_sample;
  m_run = other->m_run;
  this->setInstrument(other->getInstrument());
  if (other->m_moderatorModel)
    m_moderatorModel = other->m_moderatorModel->clone();
  m_choppers.clear();
  for (auto iter = other->m_choppers.begin(); iter != other->m_choppers.end();
       ++iter) {
    m_choppers.push_back((*iter)->clone());
  }
}

//---------------------------------------------------------------------------------------
/** Clone this ExperimentInfo class into a new one
 */
ExperimentInfo *ExperimentInfo::cloneExperimentInfo() const {
  ExperimentInfo *out = new ExperimentInfo();
  out->copyExperimentInfoFrom(this);
  return out;
}

//---------------------------------------------------------------------------------------

/// @returns A human-readable description of the object
const std::string ExperimentInfo::toString() const {
  std::ostringstream out;

  Geometry::Instrument_const_sptr inst = this->getInstrument();
  out << "Instrument: " << inst->getName() << " ("
      << inst->getValidFromDate().toFormattedString("%Y-%b-%d") << " to "
      << inst->getValidToDate().toFormattedString("%Y-%b-%d") << ")";
  out << "\n";
  if (!inst->getFilename().empty()) {
    out << "Instrument from: " << inst->getFilename();
    out << "\n";
  }

  // parameter files loaded
  auto paramFileVector = this->instrumentParameters().getParameterFilenames();
  for (auto itFilename = paramFileVector.begin();
       itFilename != paramFileVector.end(); ++itFilename) {
    out << "Parameters from: " << *itFilename;
    out << "\n";
  }

  std::string runStart = getAvailableWorkspaceStartDate();
  std::string runEnd = getAvailableWorkspaceEndDate();
  std::string msgNA = "not available";
  if (runStart.empty())
    runStart = msgNA;
  if (runEnd.empty())
    runEnd = msgNA;
  out << "Run start: " << runStart << "\n";
  out << "Run end:  " << runEnd
      << "\n"; // note extra space for pseudo/approx-alignment

  if (this->sample().hasOrientedLattice()) {
    const Geometry::OrientedLattice &latt = this->sample().getOrientedLattice();
    out << "Sample: a " << std::fixed << std::setprecision(1) << latt.a()
        << ", b " << latt.b() << ", c " << latt.c();
    out << "; alpha " << std::fixed << std::setprecision(0) << latt.alpha()
        << ", beta " << latt.beta() << ", gamma " << latt.gamma();
    out << "\n";
  }
  return out.str();
}

//---------------------------------------------------------------------------------------
/** Set the instrument
* @param instr :: Shared pointer to an instrument.
*/
void ExperimentInfo::setInstrument(const Instrument_const_sptr &instr) {
  if (instr->isParametrized()) {
    sptr_instrument = instr->baseInstrument();
    m_parmap = instr->getParameterMap();
  } else {
    sptr_instrument = instr;
  }
}

//---------------------------------------------------------------------------------------
/** Get a shared pointer to the parametrized instrument associated with this
*workspace
*
*  @return The instrument class
*/
Instrument_const_sptr ExperimentInfo::getInstrument() const {
  return Geometry::ParComponentFactory::createInstrument(sptr_instrument,
                                                         m_parmap);
}

//---------------------------------------------------------------------------------------
/**  Returns a new copy of the instrument parameters
*    @return a (new) copy of the instruments parameter map
*/
Geometry::ParameterMap &ExperimentInfo::instrumentParameters() {
  // TODO: Here duplicates cow_ptr. Figure out if there's a better way

  // Use a double-check for sharing so that we only

  // enter the critical region if absolutely necessary
  if (!m_parmap.unique()) {
    Poco::Mutex::ScopedLock lock(m_mutex);
    // Check again because another thread may have taken copy
    // and dropped reference count since previous check
    if (!m_parmap.unique()) {
      ParameterMap_sptr oldData = m_parmap;
      m_parmap = boost::make_shared<ParameterMap>(*oldData);
    }
  }
  return *m_parmap;
}

//---------------------------------------------------------------------------------------
/**  Returns a const reference to the instrument parameters.
*    @return a const reference to the instrument ParameterMap.
*/
const Geometry::ParameterMap &ExperimentInfo::instrumentParameters() const {
  return *m_parmap.get();
}

//---------------------------------------------------------------------------------------
/**  Returns a const reference to the instrument parameters.
*    @return a const reference to the instrument ParameterMap.
*/
const Geometry::ParameterMap &
ExperimentInfo::constInstrumentParameters() const {
  return *m_parmap;
}

namespace {
///@cond

/// Used for storing info about "r-position", "t-position" and "p-position"
/// parameters
/// These are translated to X,Y,Z and so must all be processed together
struct RTP {
  RTP() : radius(0.0), haveRadius(false), theta(0.0), phi(0.0) {}
  double radius;
  bool haveRadius;
  double theta;
  double phi;
};

struct ParameterValue {
  ParameterValue(const Geometry::XMLInstrumentParameter &paramInfo,
                 const API::Run &run)
      : info(paramInfo), runData(run) {}

  operator double() {
    if (info.m_logfileID.empty())
      return boost::lexical_cast<double>(info.m_value);
    else
      return info.createParamValue(
          runData.getTimeSeriesProperty<double>(info.m_logfileID));
  }
  operator int() { return boost::lexical_cast<int>(info.m_value); }
  operator bool() {
    if (boost::iequals(info.m_value, "true"))
      return true;
    else if (boost::iequals(info.m_value, "yes"))
      return true;
    else
      return false;
  }
  const Geometry::XMLInstrumentParameter &info;
  const Run &runData;
};
///@endcond
}

//---------------------------------------------------------------------------------------
/** Add parameters to the instrument parameter map that are defined in
* instrument
*   definition file or parameter file, which may contain parameters that require
*   logfile data to be available. Logs must be loaded before running this
* method.
*/
void ExperimentInfo::populateInstrumentParameters() {
  // Get instrument and sample
  boost::shared_ptr<const Instrument> instrument =
      getInstrument()->baseInstrument();

  // Reference to the run
  const auto &runData = run();

  // Get pointer to parameter map that we may add parameters to and information
  // about
  // the parameters that my be specified in the instrument definition file (IDF)
  Geometry::ParameterMap &paramMap = instrumentParameters();
  const auto &paramInfoFromIDF = instrument->getLogfileCache();

  const double deg2rad(M_PI / 180.0);
  std::map<const IComponent *, RTP> rtpParams;

  auto cacheEnd = paramInfoFromIDF.end();
  for (auto cacheItr = paramInfoFromIDF.begin(); cacheItr != cacheEnd;
       ++cacheItr) {
    const auto &nameComp = cacheItr->first;
    const auto &paramInfo = cacheItr->second;
    const std::string &paramN = nameComp.first;

    try {
      // Special case where user has specified r-position,t-position, and/or
      // p-position.
      // We need to know all three first to calculate a set of X,Y,Z
      if (paramN.compare(1, 9, "-position") == 0) {
        auto &rtpValues = rtpParams[paramInfo->m_component]; // If not found,
                                                             // constructs
                                                             // default
        double value = ParameterValue(*paramInfo, runData);
        if (paramN.compare(0, 1, "r") == 0) {
          rtpValues.radius = value;
          rtpValues.haveRadius = true;
        } else if (paramN.compare(0, 1, "t") == 0)
          rtpValues.theta = deg2rad * value;
        else if (paramN.compare(0, 1, "p") == 0)
          rtpValues.phi = deg2rad * value;
        else {
        }
        if (rtpValues.haveRadius) // Just overwrite x,y,z
        {
          // convert spherical coordinates to cartesian coordinate values
          double x = rtpValues.radius * std::sin(rtpValues.theta) *
                     std::cos(rtpValues.phi);
          paramMap.addPositionCoordinate(paramInfo->m_component, "x", x);
          double y = rtpValues.radius * std::sin(rtpValues.theta) *
                     std::sin(rtpValues.phi);
          paramMap.addPositionCoordinate(paramInfo->m_component, "y", y);
          double z = rtpValues.radius * std::cos(rtpValues.theta);
          paramMap.addPositionCoordinate(paramInfo->m_component, "z", z);
        }
      } else {
        populateWithParameter(paramMap, paramN, *paramInfo, runData);
      }
    } catch (std::exception &exc) {
      g_log.information() << "Unable to add component parameter '"
                          << nameComp.first << "'. Error: " << exc.what();
      continue;
    }
  }
}

//---------------------------------------------------------------------------------------
/**
 * Replaces current parameter map with a copy of the given map
 * @ pmap const reference to parameter map whose copy replaces the current
 * parameter map
 */
void ExperimentInfo::replaceInstrumentParameters(
    const Geometry::ParameterMap &pmap) {
  this->m_parmap.reset(new ParameterMap(pmap));
}
//---------------------------------------------------------------------------------------
/**
 * exchanges contents of current parameter map with contents of other map)
 * @ pmap reference to parameter map which would exchange its contents with
 * current map
 */
void ExperimentInfo::swapInstrumentParameters(Geometry::ParameterMap &pmap) {
  this->m_parmap->swap(pmap);
}

//---------------------------------------------------------------------------------------
/**
 * Caches a lookup for the detector IDs of the members that are part of the same
 * group
 * @param mapping :: A map between a detector ID and the other IDs that are part
 * of the same
 * group.
 */
void ExperimentInfo::cacheDetectorGroupings(const det2group_map &mapping) {
  m_detgroups = mapping;
}

//---------------------------------------------------------------------------------------
/// Returns the detector IDs that make up the group that this ID is part of
const std::vector<detid_t> &
ExperimentInfo::getGroupMembers(const detid_t detID) const {
  auto iter = m_detgroups.find(detID);
  if (iter != m_detgroups.end()) {
    return iter->second;
  } else {
    throw std::runtime_error(
        "ExperimentInfo::getGroupMembers - Unable to find ID " +
        boost::lexical_cast<std::string>(detID) + " in lookup");
  }
}

//---------------------------------------------------------------------------------------
/**
 * Get a detector or detector group from an ID
 * @param detID ::
 * @returns A single detector or detector group depending on the mapping set.
 * @see set
 */
Geometry::IDetector_const_sptr
ExperimentInfo::getDetectorByID(const detid_t detID) const {
  if (m_detgroups.empty()) {
    return getInstrument()->getDetector(detID);
  } else {
    const std::vector<detid_t> &ids = this->getGroupMembers(detID);
    return getInstrument()->getDetectorG(ids);
  }
}

//---------------------------------------------------------------------------------------

/**
 * Set an object describing the moderator properties and take ownership
 * @param source :: A pointer to an object describing the source. Ownership is
 * transferred to this object
 */
void ExperimentInfo::setModeratorModel(ModeratorModel *source) {
  if (!source) {
    throw std::invalid_argument(
        "ExperimentInfo::setModeratorModel - NULL source object found.");
  }
  m_moderatorModel = boost::shared_ptr<ModeratorModel>(source);
}

/// Returns a reference to the source properties object
ModeratorModel &ExperimentInfo::moderatorModel() const {
  if (!m_moderatorModel) {
    throw std::runtime_error("ExperimentInfo::moderatorModel - No source "
                             "desciption has been defined");
  }
  return *m_moderatorModel;
}

//---------------------------------------------------------------------------------------
/**
 * Sets a new chopper description at a given point. The point is given by index
 * where 0 is
 * closest to the source
 * @param chopper :: A pointer to a new chopper object, this class takes
 * ownership of the pointer
 * @param index :: An optional index that specifies which chopper point the
 * chopper belongs to (default=0)
 */
void ExperimentInfo::setChopperModel(ChopperModel *chopper,
                                     const size_t index) {
  if (!chopper) {
    throw std::invalid_argument(
        "ExperimentInfo::setChopper - NULL chopper object found.");
  }
  auto iter = m_choppers.begin();
  std::advance(iter, index);
  if (index < m_choppers.size()) // Replacement
  {
    (*iter) = boost::shared_ptr<ChopperModel>(chopper);
  } else // Insert it
  {
    m_choppers.insert(iter, boost::shared_ptr<ChopperModel>(chopper));
  }
}

/**
 * Returns a const reference to a chopper description
 * @param index :: An optional index giving the point within the instrument this
 * chopper describes (default=0)
 * @return A reference to a const chopper object
 */
ChopperModel &ExperimentInfo::chopperModel(const size_t index) const {
  if (index < m_choppers.size()) {
    auto iter = m_choppers.begin();
    std::advance(iter, index);
    return **iter;
  } else {
    std::ostringstream os;
    os << "ExperimentInfo::chopper - Invalid index=" << index << ". "
       << m_choppers.size() << " chopper descriptions have been set.";
    throw std::invalid_argument(os.str());
  }
}

//---------------------------------------------------------------------------------------
/** Get a constant reference to the Sample associated with this workspace.
* @return const reference to Sample object
*/
const Sample &ExperimentInfo::sample() const {
  Poco::Mutex::ScopedLock lock(m_mutex);
  return *m_sample;
}

/** Get a reference to the Sample associated with this workspace.
*  This non-const method will copy the sample if it is shared between
*  more than one workspace, and the reference returned will be to the copy.
*  Can ONLY be taken by reference!
* @return reference to sample object
*/
Sample &ExperimentInfo::mutableSample() {
  // Use a double-check for sharing so that we only
  // enter the critical region if absolutely necessary
  if (!m_sample.unique()) {
    Poco::Mutex::ScopedLock lock(m_mutex);
    // Check again because another thread may have taken copy
    // and dropped reference count since previous check
    if (!m_sample.unique()) {
      boost::shared_ptr<Sample> oldData = m_sample;
      m_sample = boost::make_shared<Sample>(*oldData);
    }
  }
  return *m_sample;
}

//---------------------------------------------------------------------------------------
/** Get a constant reference to the Run object associated with this workspace.
* @return const reference to run object
*/
const Run &ExperimentInfo::run() const {
  Poco::Mutex::ScopedLock lock(m_mutex);
  return *m_run;
}

/** Get a reference to the Run object associated with this workspace.
*  This non-const method will copy the Run object if it is shared between
*  more than one workspace, and the reference returned will be to the copy.
*  Can ONLY be taken by reference!
* @return reference to Run object
*/
Run &ExperimentInfo::mutableRun() {
  // Use a double-check for sharing so that we only
  // enter the critical region if absolutely necessary
  if (!m_run.unique()) {
    Poco::Mutex::ScopedLock lock(m_mutex);
    // Check again because another thread may have taken copy
    // and dropped reference count since previous check
    if (!m_run.unique()) {
      boost::shared_ptr<Run> oldData = m_run;
      m_run = boost::make_shared<Run>(*oldData);
    }
  }
  return *m_run;
}

/**
 * Get an experimental log either by log name or by type, e.g.
 *   - temperature_log
 *   - chopper_speed_log
 * The logs are first checked for one matching the given string and if that
 * fails then the instrument is checked for a parameter of the same name
 * and if this exists then its value is assume to be the actual log required
 * @param log :: A string giving either a specific log name or instrument
 * parameter whose
 * value is to be retrieved
 * @return A pointer to the property
 */
Kernel::Property *ExperimentInfo::getLog(const std::string &log) const {
  try {
    return run().getProperty(log);
  } catch (Kernel::Exception::NotFoundError &) {
    // No log with that name
  }
  // If the instrument has a parameter with that name then take the value as a
  // log name
  const std::string logName =
      instrumentParameters().getString(sptr_instrument.get(), log);
  if (logName.empty()) {
    throw std::invalid_argument(
        "ExperimentInfo::getLog - No instrument parameter named \"" + log +
        "\". Cannot access full log name");
  }
  return run().getProperty(logName);
}

/**
 * Get an experimental log as a single value either by log name or by type. @see
 * getLog
 * @param log :: A string giving either a specific log name or instrument
 * parameter whose
 * value is to be retrieved
 * @return A pointer to the property
 */
double ExperimentInfo::getLogAsSingleValue(const std::string &log) const {
  try {
    return run().getPropertyAsSingleValue(log);
  } catch (Kernel::Exception::NotFoundError &) {
    // No log with that name
  }
  // If the instrument has a parameter with that name then take the value as a
  // log name
  const std::string logName =
      instrumentParameters().getString(sptr_instrument.get(), log);
  if (logName.empty()) {
    throw std::invalid_argument(
        "ExperimentInfo::getLog - No instrument parameter named \"" + log +
        "\". Cannot access full log name");
  }
  return run().getPropertyAsSingleValue(logName);
}

//---------------------------------------------------------------------------------------
/** Utility method to get the run number
 *
 * @return the run number (int) or 0 if not found.
 */
int ExperimentInfo::getRunNumber() const {
  const Run &thisRun = run();
  if (!thisRun.hasProperty("run_number")) {
    // No run_number property, default to 0
    return 0;
  } else {
    Property *prop = m_run->getProperty("run_number");
    if (prop) {
      // Use the string representation. That way both a string and a number
      // property will work.
      int val;
      if (Strings::convert(prop->value(), val))
        return val;
      else
        return 0;
    }
  }
  return 0;
}

/**
 * Returns the emode for this run. It first searchs the run logs for a
 * "deltaE-mode" log and falls back to
 * the instrument if one is not found. If neither exist then the run is
 * considered Elastic.
 * @return The emode enum for the energy transfer mode of this run. Currently
 * only checks the instrument
 */
Kernel::DeltaEMode::Type ExperimentInfo::getEMode() const {
  static const char *emodeTag = "deltaE-mode";
  std::string emodeStr;
  if (run().hasProperty(emodeTag)) {
    emodeStr = run().getPropertyValueAsType<std::string>(emodeTag);
  } else if (sptr_instrument &&
             instrumentParameters().contains(sptr_instrument.get(), emodeTag)) {
    Geometry::Parameter_sptr param =
        instrumentParameters().get(sptr_instrument.get(), emodeTag);
    emodeStr = param->asString();
  } else {
    return Kernel::DeltaEMode::Elastic;
  }
  return Kernel::DeltaEMode::fromString(emodeStr);
}

/**
 * Easy access to the efixed value for this run & detector ID
 * @param detID :: The detector ID to ask for the efixed mode (ignored in Direct
 * & Elastic mode). The
 * detector with ID matching that given is pulled from the instrument with this
 * method and it will
 * throw a Exception::NotFoundError if the ID is unknown.
 * @return The current EFixed value
 */
double ExperimentInfo::getEFixed(const detid_t detID) const {
  IDetector_const_sptr det = getInstrument()->getDetector(detID);
  return getEFixed(det);
}

/**
 * Easy access to the efixed value for this run & detector
 * @param detector :: The detector object to ask for the efixed mode. Only
 * required for Indirect mode
 * @return The current efixed value
 */
double
ExperimentInfo::getEFixed(const Geometry::IDetector_const_sptr detector) const {
  Kernel::DeltaEMode::Type emode = getEMode();
  if (emode == Kernel::DeltaEMode::Direct) {
    try {
      return this->run().getPropertyValueAsType<double>("Ei");
    } catch (Kernel::Exception::NotFoundError &) {
      throw std::runtime_error(
          "Experiment logs do not contain an Ei value. Have you run GetEi?");
    }
  } else if (emode == Kernel::DeltaEMode::Indirect) {
    if (!detector)
      throw std::runtime_error("ExperimentInfo::getEFixed - Indirect mode "
                               "efixed requested without a valid detector.");
    Parameter_sptr par =
        constInstrumentParameters().getRecursive(detector.get(), "Efixed");
    if (par) {
      return par->value<double>();
    } else {
      std::vector<double> efixedVec = detector->getNumberParameter("Efixed");
      if (efixedVec.empty()) {
        int detid = detector->getID();
        IDetector_const_sptr detectorSingle =
            getInstrument()->getDetector(detid);
        efixedVec = detectorSingle->getNumberParameter("Efixed");
      }
      if (!efixedVec.empty()) {
        return efixedVec.at(0);
      } else {
        std::ostringstream os;
        os << "ExperimentInfo::getEFixed - Indirect mode efixed requested but "
              "detector has no Efixed parameter attached. ID="
           << detector->getID();
        throw std::runtime_error(os.str());
      }
    }
  } else {
    throw std::runtime_error("ExperimentInfo::getEFixed - EFixed requested for "
                             "elastic mode, don't know what to do!");
  }
}

void ExperimentInfo::setEFixed(const detid_t detID, const double value) {
  IDetector_const_sptr det = getInstrument()->getDetector(detID);
  Geometry::ParameterMap &pmap = instrumentParameters();
  pmap.addDouble(det.get(), "Efixed", value);
}

// used to terminate SAX process
class DummyException {
public:
  std::string m_validFrom;
  std::string m_validTo;
  DummyException(std::string validFrom, std::string validTo)
      : m_validFrom(validFrom), m_validTo(validTo) {}
};

// SAX content handler for grapping stuff quickly from IDF
class myContentHandler : public Poco::XML::ContentHandler {
  virtual void startElement(const XMLString &, const XMLString &localName,
                            const XMLString &, const Attributes &attrList) {
    if (localName == "instrument") {
      throw DummyException(
          static_cast<std::string>(attrList.getValue("", "valid-from")),
          static_cast<std::string>(attrList.getValue("", "valid-to")));
    }
  }
  virtual void endElement(const XMLString &, const XMLString &,
                          const XMLString &) {}
  virtual void startDocument() {}
  virtual void endDocument() {}
  virtual void characters(const XMLChar[], int, int) {}
  virtual void endPrefixMapping(const XMLString &) {}
  virtual void ignorableWhitespace(const XMLChar[], int, int) {}
  virtual void processingInstruction(const XMLString &, const XMLString &) {}
  virtual void setDocumentLocator(const Locator *) {}
  virtual void skippedEntity(const XMLString &) {}
  virtual void startPrefixMapping(const XMLString &, const XMLString &) {}
};

//---------------------------------------------------------------------------------------
/** Return from an IDF the values of the valid-from and valid-to attributes
*
*  @param IDFfilename :: Full path of an IDF
*  @param[out] outValidFrom :: Used to return valid-from date
*  @param[out] outValidTo :: Used to return valid-to date
*/
void ExperimentInfo::getValidFromTo(const std::string &IDFfilename,
                                    std::string &outValidFrom,
                                    std::string &outValidTo) {
  SAXParser pParser;
  // Create on stack to ensure deletion. Relies on pParser also being local
  // variable.
  myContentHandler conHand;
  pParser.setContentHandler(&conHand);

  try {
    pParser.parse(IDFfilename);
  } catch (DummyException &e) {
    outValidFrom = e.m_validFrom;
    outValidTo = e.m_validTo;
  } catch (...) {
    // should throw some sensible here
  }
}

//---------------------------------------------------------------------------------------
/** Return workspace start date as an ISO 8601 string. If this info not stored
*in workspace the
*   method returns current date. This date is used for example to retrieve the
*instrument file.
*
*  @return workspace start date as a string (current time if start date not
*available)
*/
std::string ExperimentInfo::getWorkspaceStartDate() const {
  std::string date;
  try {
    date = run().startTime().toISO8601String();
  } catch (std::runtime_error &) {
    g_log.information("run_start/start_time not stored in workspace. Default "
                      "to current date.");
    date = Kernel::DateAndTime::getCurrentTime().toISO8601String();
  }
  return date;
}

//---------------------------------------------------------------------------------------
/** Return workspace start date as a formatted string (strftime, as
 *  returned by Kernel::DateAndTime) string, if available. If
 *  unavailable, an empty string is returned
 *
 *  @return workspace start date as a string (empty if no date available)
 */
std::string ExperimentInfo::getAvailableWorkspaceStartDate() const {
  std::string date;
  try {
    date = run().startTime().toFormattedString();
  } catch (std::runtime_error &) {
    g_log.information("Note: run_start/start_time not stored in workspace.");
  }
  return date;
}

//---------------------------------------------------------------------------------------
/** Return workspace end date as a formatted string (strftime style,
 *  as returned by Kernel::DateAdnTime) string, if available. If
 *  unavailable, an empty string is returned
 *
 *  @return workspace end date as a string (empty if no date available)
 */
std::string ExperimentInfo::getAvailableWorkspaceEndDate() const {
  std::string date;
  try {
    date = run().endTime().toFormattedString();
  } catch (std::runtime_error &) {
    g_log.information("Note: run_start/start_time not stored in workspace.");
  }
  return date;
}

//---------------------------------------------------------------------------------------
/** A given instrument may have multiple IDFs associated with it. This method
*return an
*  identifier which identify a given IDF for a given instrument. An IDF filename
*is
*  required to be of the form IDFname + _Definition + Identifier + .xml, the
*identifier
*  then is the part of a filename that identifies the IDF valid at a given date.
*
*  If several IDF files are valid at the given date the file with the most
*recent from
*  date is selected. If no such files are found the file with the latest from
*date is
*  selected.
*
*  If no file is found for the given instrument, an empty string is returned.
*
*  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
*  @param date :: ISO 8601 date
*  @return full path of IDF
*/
std::string
ExperimentInfo::getInstrumentFilename(const std::string &instrumentName,
                                      const std::string &date) {
  if (date.empty()) {
    // Just use the current date
    g_log.debug() << "No date specified, using current date and time."
                  << std::endl;
    const std::string now =
        Kernel::DateAndTime::getCurrentTime().toISO8601String();
    // Recursively call this method, but with both parameters.
    return ExperimentInfo::getInstrumentFilename(instrumentName, now);
  }

  g_log.debug() << "Looking for instrument XML file for " << instrumentName
                << " that is valid on '" << date << "'\n";
  // Lookup the instrument (long) name
  std::string instrument(
      Kernel::ConfigService::Instance().getInstrument(instrumentName).name());

  // Get the search directory for XML instrument definition files (IDFs)
  const std::vector<std::string> &directoryNames =
      Kernel::ConfigService::Instance().getInstrumentDirectories();

  boost::regex regex(instrument + "_Definition.*\\.xml",
                     boost::regex_constants::icase);
  Poco::DirectoryIterator end_iter;
  DateAndTime d(date);
  bool foundGoodFile =
      false; // True if we have found a matching file (valid at the given date)
  std::string mostRecentIDF; // store most recently starting matching IDF if
                             // found, else most recently starting IDF.
  DateAndTime refDate("1900-01-31 23:59:00"); // used to help determine the most
                                              // recently starting IDF, if none
                                              // match
  DateAndTime refDateGoodFile("1900-01-31 23:59:00"); // used to help determine
                                                      // the most recently
                                                      // starting matching IDF
  for (auto instDirs_itr = directoryNames.begin();
       instDirs_itr != directoryNames.end(); ++instDirs_itr) {
    // This will iterate around the directories from user ->etc ->install, and
    // find the first beat file
    std::string directoryName = *instDirs_itr;
    for (Poco::DirectoryIterator dir_itr(directoryName); dir_itr != end_iter;
         ++dir_itr) {
      if (!Poco::File(dir_itr->path()).isFile())
        continue;

      std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();
      if (regex_match(l_filenamePart, regex)) {
        g_log.debug() << "Found file: '" << dir_itr->path() << "'\n";
        std::string validFrom, validTo;
        getValidFromTo(dir_itr->path(), validFrom, validTo);
        g_log.debug() << "File '" << dir_itr->path() << " valid dates: from '"
                      << validFrom << "' to '" << validTo << "'\n";
        DateAndTime from(validFrom);
        // Use a default valid-to date if none was found.
        DateAndTime to;
        if (validTo.length() > 0)
          to.setFromISO8601(validTo);
        else
          to.setFromISO8601("2100-01-01T00:00:00");

        if (from <= d && d <= to) {
          if (from > refDateGoodFile) { // We'd found a matching file more
                                        // recently starting than any other
                                        // matching file found
            foundGoodFile = true;
            refDateGoodFile = from;
            mostRecentIDF = dir_itr->path();
          }
        }
        if (!foundGoodFile && (from > refDate)) { // Use most recently starting
                                                  // file, in case we don't find
                                                  // a matching file.
          refDate = from;
          mostRecentIDF = dir_itr->path();
        }
      }
    }
  }
  g_log.debug() << "IDF selected is " << mostRecentIDF << std::endl;
  return mostRecentIDF;
}

//--------------------------------------------------------------------------------------------
/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 */
void ExperimentInfo::saveExperimentInfoNexus(::NeXus::File *file) const {
  Instrument_const_sptr instrument = getInstrument();
  instrument->saveNexus(file, "instrument");
  sample().saveNexus(file, "sample");
  run().saveNexus(file, "logs");
}

//--------------------------------------------------------------------------------------------
/** Load the sample and log info from an open NeXus file.
 * @param file :: open NeXus file
 */
void ExperimentInfo::loadSampleAndLogInfoNexus(::NeXus::File *file) {
  // First, the sample and then the logs
  int sampleVersion = mutableSample().loadNexus(file, "sample");
  if (sampleVersion == 0) {
    // Old-style (before Sep-9-2011) NXS processed
    // sample field contains both the logs and the sample details
    file->openGroup("sample", "NXsample");
    this->mutableRun().loadNexus(file, "");
    file->closeGroup();
  } else {
    // Newer style: separate "logs" field for the Run object
    this->mutableRun().loadNexus(file, "logs");
  }
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param[out] parameterStr :: special string for all the parameters.
 *             Feed that to ExperimentInfo::readParameterMap() after the
 * instrument is done.
 * @throws Exception::NotFoundError If instrument definition is not in the nexus
 * file and cannot
 *                                  be loaded from the IDF.
 */
void ExperimentInfo::loadExperimentInfoNexus(::NeXus::File *file,
                                             std::string &parameterStr) {
  // load sample and log info
  loadSampleAndLogInfoNexus(file);

  loadInstrumentInfoNexus(file, parameterStr);
}

//--------------------------------------------------------------------------------------------
/** Load the instrument from an open NeXus file.
 * @param file :: open NeXus file
 * @param[out] parameterStr :: special string for all the parameters.
 *             Feed that to ExperimentInfo::readParameterMap() after the
 * instrument is done.
 * @throws Exception::NotFoundError If instrument definition is not in the nexus
 * file and cannot
 *                                  be loaded from the IDF.
 */
void ExperimentInfo::loadInstrumentInfoNexus(::NeXus::File *file,
                                             std::string &parameterStr) {
  std::string instrumentName;
  std::string instrumentXml;
  std::string instrumentFilename;

  // Try to get the instrument file
  file->openGroup("instrument", "NXinstrument");
  file->readData("name", instrumentName);

  try {
    file->openGroup("instrument_xml", "NXnote");
    file->readData("data", instrumentXml);
    file->closeGroup();
  } catch (NeXus::Exception &ex) {
    // Just carry on - it might not be there (e.g. old-style processed files)
    g_log.debug(ex.what());
  }

  // We first assume this is a new version file, but if the next step fails we
  // assume its and old version file.
  int version = 1;
  try {
    file->readData("instrument_source", instrumentFilename);
  } catch (NeXus::Exception &) {
    version = 0;
    // In the old version 'processed' file, this was held at the top level (as
    // was the parameter map)
    file->closeGroup();
    try {
      file->readData("instrument_source", instrumentFilename);
    } catch (NeXus::Exception &ex) {
      // Just carry on - it might not be there (e.g. for SNS files)
      g_log.debug(ex.what());
    }
  }

  try {
    file->openGroup("instrument_parameter_map", "NXnote");
    file->readData("data", parameterStr);
    file->closeGroup();
  } catch (NeXus::Exception &ex) {
    // Just carry on - it might not be there (e.g. for SNS files)
    g_log.debug(ex.what());
  }

  if (version == 1) {
    file->closeGroup();
  }

  instrumentFilename = Strings::strip(instrumentFilename);
  instrumentXml = Strings::strip(instrumentXml);
  instrumentName = Strings::strip(instrumentName);
  if (instrumentXml.empty() && !instrumentName.empty()) {
    // XML was not included or was empty.
    // Use the instrument name to find the file
    try {
      std::string filename =
          getInstrumentFilename(instrumentName, getWorkspaceStartDate());
      // And now load the contents
      instrumentFilename = filename;
      instrumentXml = Strings::loadFile(filename);
    } catch (std::exception &e) {
      g_log.error() << "Error loading instrument IDF file for '"
                    << instrumentName << "'.\n";
      g_log.debug() << e.what() << std::endl;
      throw;
    }
  } else {
    if (!instrumentFilename.empty())
      instrumentFilename = ConfigService::Instance().getInstrumentDirectory() +
                           "/" + instrumentFilename;
    g_log.debug() << "Using instrument IDF XML text contained in nexus file.\n";
  }

  // ---------- Now parse that XML to make the instrument -------------------
  if (!instrumentXml.empty() && !instrumentName.empty()) {
    InstrumentDefinitionParser parser;
    parser.initialize(instrumentFilename, instrumentName, instrumentXml);
    std::string instrumentNameMangled = parser.getMangledName();
    Instrument_sptr instr;
    // Check whether the instrument is already in the InstrumentDataService
    if (InstrumentDataService::Instance().doesExist(instrumentNameMangled)) {
      // If it does, just use the one from the one stored there
      instr = InstrumentDataService::Instance().retrieve(instrumentNameMangled);
    } else {
      // Really create the instrument
      instr = parser.parseXML(NULL);
      // Add to data service for later retrieval
      InstrumentDataService::Instance().add(instrumentNameMangled, instr);
    }
    // Now set the instrument
    this->setInstrument(instr);
  }
}

//-------------------------------------------------------------------------------------------------
/** Parse the result of ParameterMap.asString() into the ParameterMap
 * of the current instrument. The instrument needs to have been loaded
 * already, of course.
 *
 * @param parameterStr :: result of ParameterMap.asString()
 */
void ExperimentInfo::readParameterMap(const std::string &parameterStr) {
  Geometry::ParameterMap &pmap = this->instrumentParameters();
  Instrument_const_sptr instr = this->getInstrument()->baseInstrument();

  int options = Poco::StringTokenizer::TOK_IGNORE_EMPTY;
  options += Poco::StringTokenizer::TOK_TRIM;
  Poco::StringTokenizer splitter(parameterStr, "|", options);

  Poco::StringTokenizer::Iterator iend = splitter.end();
  // std::string prev_name;
  for (Poco::StringTokenizer::Iterator itr = splitter.begin(); itr != iend;
       ++itr) {
    Poco::StringTokenizer tokens(*itr, ";");
    if (tokens.count() < 4)
      continue;
    std::string comp_name = tokens[0];
    // if( comp_name == prev_name ) continue; this blocks reading in different
    // parameters of the same component. RNT
    // prev_name = comp_name;
    const Geometry::IComponent *comp = 0;
    if (comp_name.find("detID:") != std::string::npos) {
      int detID = atoi(comp_name.substr(6).c_str());
      comp = instr->getDetector(detID).get();
      if (!comp) {
        g_log.warning() << "Cannot find detector " << detID << '\n';
        continue;
      }
    } else {
      comp = instr->getComponentByName(comp_name).get();
      if (!comp) {
        g_log.warning() << "Cannot find component " << comp_name << '\n';
        continue;
      }
    }
    if (!comp)
      continue;
    // create parameter's value as a sum of all tokens with index 3 or larger
    // this allow a parameter's value to contain ";"
    std::string paramValue = tokens[3];
    int size = static_cast<int>(tokens.count());
    for (int i = 4; i < size; i++)
      paramValue += ";" + tokens[4];
    pmap.add(tokens[1], comp, tokens[2], paramValue);
  }
}

//------------------------------------------------------------------------------------------------------
// Private members
//------------------------------------------------------------------------------------------------------

/**
 * Fill map with instrument parameter first set in xml file
 * Where this is appropriate a parameter value is dependent on values in a log
 * entry
 * @param paramMap Map to populate
 * @param name The name of the parameter
 * @param paramInfo A reference to the object describing this parameter
 * @param runData A reference to the run object, which stores log value entries
 */
void ExperimentInfo::populateWithParameter(
    Geometry::ParameterMap &paramMap, const std::string &name,
    const Geometry::XMLInstrumentParameter &paramInfo, const Run &runData) {
  const std::string &category = paramInfo.m_type;
  ParameterValue paramValue(paramInfo,
                            runData); // Defines implicit conversion operator

  // Some names are special. Values should be convertible to double
  if (name.compare("x") == 0 || name.compare("y") == 0 ||
      name.compare("z") == 0) {
    paramMap.addPositionCoordinate(paramInfo.m_component, name, paramValue);
  } else if (name.compare("rot") == 0 || name.compare("rotx") == 0 ||
             name.compare("roty") == 0 || name.compare("rotz") == 0) {
    paramMap.addRotationParam(paramInfo.m_component, name, paramValue);
  } else if (category.compare("fitting") == 0) {
    std::ostringstream str;
    str << paramInfo.m_value << " , " << paramInfo.m_fittingFunction << " , "
        << name << " , " << paramInfo.m_constraint[0] << " , "
        << paramInfo.m_constraint[1] << " , " << paramInfo.m_penaltyFactor
        << " , " << paramInfo.m_tie << " , " << paramInfo.m_formula << " , "
        << paramInfo.m_formulaUnit << " , " << paramInfo.m_resultUnit << " , "
        << (*(paramInfo.m_interpolation));
    paramMap.add("fitting", paramInfo.m_component, name, str.str());
  } else if (category.compare("string") == 0) {
    paramMap.addString(paramInfo.m_component, name, paramInfo.m_value);
  } else if (category.compare("bool") == 0) {
    paramMap.addBool(paramInfo.m_component, name, paramValue);
  } else if (category.compare("int") == 0) {
    paramMap.addInt(paramInfo.m_component, name, paramValue);
  } else // assume double
  {
    paramMap.addDouble(paramInfo.m_component, name, paramValue);
  }
}

} // namespace Mantid
} // namespace API

namespace Mantid {
namespace Kernel {

template <>
MANTID_API_DLL Mantid::API::ExperimentInfo_sptr
IPropertyManager::getValue<Mantid::API::ExperimentInfo_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::ExperimentInfo_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::ExperimentInfo_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected ExperimentInfo.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL Mantid::API::ExperimentInfo_const_sptr
IPropertyManager::getValue<Mantid::API::ExperimentInfo_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::ExperimentInfo_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::ExperimentInfo_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected const ExperimentInfo.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
