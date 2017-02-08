#include "MantidAPI/ExperimentInfo.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/ModeratorModel.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"

#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"

#include "MantidBeamline/DetectorInfo.h"
#include "MantidBeamline/SpectrumInfo.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/make_unique.h"

#include "MantidTypes/SpectrumDefinition.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>

#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/SAX/Attributes.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/SAXParser.h>
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

/** Constructor
 */
ExperimentInfo::ExperimentInfo()
    : m_moderatorModel(), m_choppers(), m_sample(new Sample()),
      m_run(new Run()), m_parmap(new ParameterMap()),
      sptr_instrument(new Instrument()),
      m_detectorInfo(boost::make_shared<Beamline::DetectorInfo>(0)) {}

/**
 * Constructs the object from a copy if the input. This leaves the new mutex
 * unlocked.
 * @param source The source object from which to initialize
 */
ExperimentInfo::ExperimentInfo(const ExperimentInfo &source) {
  this->copyExperimentInfoFrom(&source);
  setSpectrumDefinitions(source.spectrumInfo().sharedSpectrumDefinitions());
}

// Defined as default in source for forward declaration with std::unique_ptr.
ExperimentInfo::~ExperimentInfo() = default;

/** Copy the experiment info data from another ExperimentInfo instance,
 * e.g. a MatrixWorkspace.
 * @param other :: the source from which to copy ExperimentInfo
 */
void ExperimentInfo::copyExperimentInfoFrom(const ExperimentInfo *other) {
  m_sample = other->m_sample;
  m_run = other->m_run->clone();
  this->setInstrument(other->getInstrument());
  if (other->m_moderatorModel)
    m_moderatorModel = other->m_moderatorModel->clone();
  m_choppers.clear();
  for (const auto &chopper : other->m_choppers) {
    m_choppers.push_back(chopper->clone());
  }
  *m_detectorInfo = *other->m_detectorInfo;
  // We do not copy Beamline::SpectrumInfo (which contains detector grouping
  // information) for now:
  // - For MatrixWorkspace, grouping information is still stored in ISpectrum
  //   and should not be overridden (copy is done in ExperimentInfo ctor, but
  //   not here since we just copy the experiment data).
  // - For cached groupings (for MDWorkspaces), grouping was not copied in the
  //   old implementation either.
}

/** Clone this ExperimentInfo class into a new one
 */
ExperimentInfo *ExperimentInfo::cloneExperimentInfo() const {
  return new ExperimentInfo(*this);
}

/// @returns A human-readable description of the object
const std::string ExperimentInfo::toString() const {
  try {
    populateIfNotLoaded();
  } catch (std::exception &) {
    // Catch any errors so that the string returned has as much information
    // as possible
  }

  std::ostringstream out;

  Geometry::Instrument_const_sptr inst = this->getInstrument();
  const auto instName = inst->getName();
  out << "Instrument: ";
  if (!instName.empty()) {
    out << instName << " ("
        << inst->getValidFromDate().toFormattedString("%Y-%b-%d") << " to "
        << inst->getValidToDate().toFormattedString("%Y-%b-%d") << ")";
    const auto instFilename = inst->getFilename();
    if (!instFilename.empty()) {
      out << "Instrument from: " << instFilename;
      out << "\n";
    }
  } else {
    out << "None";
  }
  out << "\n";

  // parameter files loaded
  auto paramFileVector =
      this->constInstrumentParameters().getParameterFilenames();
  for (auto &itFilename : paramFileVector) {
    out << "Parameters from: " << itFilename;
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

// Helpers for setInstrument and getInstrument
namespace {
void checkDetectorInfoSize(const Instrument &instr,
                           const Beamline::DetectorInfo &detInfo) {
  const auto numDets = instr.getNumberDetectors();
  if (numDets != detInfo.size())
    throw std::runtime_error("ExperimentInfo: size mismatch between "
                             "DetectorInfo and number of detectors in "
                             "instrument");
}

std::unique_ptr<Beamline::DetectorInfo>
makeDetectorInfo(const Instrument &oldInstr, const Instrument &newInstr) {
  if (newInstr.hasDetectorInfo()) {
    // We allocate a new DetectorInfo in case there is an Instrument holding a
    // reference to our current DetectorInfo.
    const auto &detInfo = newInstr.detectorInfo();
    checkDetectorInfoSize(oldInstr, detInfo);
    return Kernel::make_unique<Beamline::DetectorInfo>(detInfo);
  } else {
    // If there is no DetectorInfo in the instrument we create a default one.
    const auto numDets = oldInstr.getNumberDetectors();
    // Currently monitors flags are stored in the detector cache of the base
    // instrument. The copy being made here is strictly speaking duplicating
    // that data, but with future refactoring this will no longer be the case.
    // Note that monitors will not change after creating a workspace.
    // Instrument::markAsMonitor works only for the base instrument and it is
    // not possible to obtain a non-const reference to the base instrument in a
    // workspace. Thus we do not need to worry about the two copies of monitor
    // flags running out of sync.
    std::vector<size_t> monitors;
    for (size_t i = 0; i < numDets; ++i)
      if (newInstr.isMonitorViaIndex(i))
        monitors.push_back(i);
    return Kernel::make_unique<Beamline::DetectorInfo>(numDets, monitors);
  }
}
}

/** Set the instrument
* @param instr :: Shared pointer to an instrument.
*/
void ExperimentInfo::setInstrument(const Instrument_const_sptr &instr) {
  m_spectrumInfoWrapper = nullptr;
  m_detectorInfoWrapper = nullptr;
  if (instr->isParametrized()) {
    sptr_instrument = instr->baseInstrument();
    m_parmap = instr->getParameterMap();
  } else {
    sptr_instrument = instr;
    m_parmap = boost::make_shared<ParameterMap>();
  }
  m_detectorInfo = makeDetectorInfo(*sptr_instrument, *instr);
  // Detector IDs that were previously dropped because they were not part of the
  // instrument may now suddenly be valid, so we have to reinitialize the
  // detector grouping. Also the index corresponding to specific IDs may have
  // changed.
  invalidateAllSpectrumDefinitions();
}

/** Get a shared pointer to the parametrized instrument associated with this
*workspace
*
*  @return The instrument class
*/
Instrument_const_sptr ExperimentInfo::getInstrument() const {
  populateIfNotLoaded();
  checkDetectorInfoSize(*sptr_instrument, *m_detectorInfo);
  auto instrument = Geometry::ParComponentFactory::createInstrument(
      sptr_instrument, m_parmap);
  instrument->setDetectorInfo(m_detectorInfo);
  return instrument;
}

/**  Returns a new copy of the instrument parameters
*    @return a (new) copy of the instruments parameter map
*/
Geometry::ParameterMap &ExperimentInfo::instrumentParameters() {
  populateIfNotLoaded();
  // TODO: Here duplicates cow_ptr. Figure out if there's a better way

  // Use a double-check for sharing so that we only

  // enter the critical region if absolutely necessary
  if (!m_parmap.unique()) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // Check again because another thread may have taken copy
    // and dropped reference count since previous check
    if (!m_parmap.unique()) {
      m_spectrumInfoWrapper = nullptr;
      m_detectorInfoWrapper = nullptr;
    }
    if (!m_parmap.unique()) {
      ParameterMap_sptr oldData = m_parmap;
      m_parmap = boost::make_shared<ParameterMap>(*oldData);
    }
  }
  return *m_parmap;
}

/**  Returns a const reference to the instrument parameters.
*    @return a const reference to the instrument ParameterMap.
*/
const Geometry::ParameterMap &ExperimentInfo::instrumentParameters() const {
  populateIfNotLoaded();
  return *m_parmap;
}

/**  Returns a const reference to the instrument parameters.
*    @return a const reference to the instrument ParameterMap.
*/
const Geometry::ParameterMap &
ExperimentInfo::constInstrumentParameters() const {
  populateIfNotLoaded();
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

/** Add parameters to the instrument parameter map that are defined in
* instrument
*   definition file or parameter file, which may contain parameters that require
*   logfile data to be available. Logs must be loaded before running this
* method.
*/
void ExperimentInfo::populateInstrumentParameters() {
  populateIfNotLoaded();
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
          // convert spherical coordinates to Cartesian coordinate values
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

/**
 * Replaces current parameter map with a copy of the given map
 * Careful: Parameters that are stored in DetectorInfo are not automatically
 * handled.
 * @ pmap const reference to parameter map whose copy replaces the current
 * parameter map
 */
void ExperimentInfo::replaceInstrumentParameters(
    const Geometry::ParameterMap &pmap) {
  populateIfNotLoaded();
  m_spectrumInfoWrapper = nullptr;
  m_detectorInfoWrapper = nullptr;
  this->m_parmap.reset(new ParameterMap(pmap));
}

/**
 * exchanges contents of current parameter map with contents of other map)
 * Careful: Parameters that are stored in DetectorInfo are not automatically
 * handled.
 * @ pmap reference to parameter map which would exchange its contents with
 * current map
 */
void ExperimentInfo::swapInstrumentParameters(Geometry::ParameterMap &pmap) {
  populateIfNotLoaded();
  m_spectrumInfoWrapper = nullptr;
  m_detectorInfoWrapper = nullptr;
  this->m_parmap->swap(pmap);
}

/**
 * Caches a lookup for the detector IDs of the members that are part of the same
 * group
 * @param mapping :: A map between a detector ID and the other IDs that are part
 * of the same
 * group.
 */
void ExperimentInfo::cacheDetectorGroupings(const det2group_map &mapping) {
  populateIfNotLoaded();
  if (mapping.empty()) {
    cacheDefaultDetectorGrouping();
    return;
  }
  setNumberOfDetectorGroups(mapping.size());
  size_t specIndex = 0;
  for (const auto &item : mapping) {
    m_det2group[item.first] = specIndex;
    setDetectorGrouping(specIndex, item.second);
    specIndex++;
  }
}

/** Sets the number of detector groups.
 *
 * This method should not need to be called explicitly. The number of detector
 * groups will be set either when initializing a MatrixWorkspace, or by calling
 * `cacheDetectorGroupings` for an ExperimentInfo stored in an MDWorkspace. */
void ExperimentInfo::setNumberOfDetectorGroups(const size_t count) const {
  populateIfNotLoaded();
  if (m_spectrumInfo)
    m_spectrumDefinitionNeedsUpdate.clear();
  m_spectrumDefinitionNeedsUpdate.resize(count, 1);
  m_spectrumInfo = Kernel::make_unique<Beamline::SpectrumInfo>(count);
  m_spectrumInfoWrapper = nullptr;
}

/** Sets the detector grouping for the spectrum with the given `index`.
 *
 * This method should not need to be called explicitly. Groupings are updated
 * automatically when modifying detector IDs in a workspace (via ISpectrum). */
void ExperimentInfo::setDetectorGrouping(
    const size_t index, const std::set<detid_t> &detIDs) const {
  SpectrumDefinition specDef;
  for (const auto detID : detIDs) {
    try {
      const size_t detIndex = detectorInfo().indexOf(detID);
      specDef.add(detIndex);
    } catch (std::out_of_range &) {
      // Silently strip bad detector IDs
    }
  }
  m_spectrumInfo->setSpectrumDefinition(index, std::move(specDef));
  m_spectrumDefinitionNeedsUpdate.at(index) = 0;
}

/** Update detector grouping for spectrum with given index.
 *
 * This method is called when the detector grouping stored in SpectrumDefinition
 * at `index` in Beamline::SpectrumInfo is not initialized or outdated. The
 * implementation throws, since no grouping information for update is available
 * when grouping comes from a call to `cacheDetectorGroupings`. This method is
 * overridden in MatrixWorkspace. */
void ExperimentInfo::updateCachedDetectorGrouping(const size_t) const {
  throw std::runtime_error("ExperimentInfo::updateCachedDetectorGrouping: "
                           "Cannot update -- grouping information not "
                           "available");
}

/**
 * Set an object describing the moderator properties and take ownership
 * @param source :: A pointer to an object describing the source. Ownership is
 * transferred to this object
 */
void ExperimentInfo::setModeratorModel(ModeratorModel *source) {
  populateIfNotLoaded();
  if (!source) {
    throw std::invalid_argument(
        "ExperimentInfo::setModeratorModel - NULL source object found.");
  }
  m_moderatorModel = boost::shared_ptr<ModeratorModel>(source);
}

/// Returns a reference to the source properties object
ModeratorModel &ExperimentInfo::moderatorModel() const {
  populateIfNotLoaded();
  if (!m_moderatorModel) {
    throw std::runtime_error("ExperimentInfo::moderatorModel - No source "
                             "desciption has been defined");
  }
  return *m_moderatorModel;
}

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
  populateIfNotLoaded();
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
  populateIfNotLoaded();
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

/** Get a constant reference to the Sample associated with this workspace.
* @return const reference to Sample object
*/
const Sample &ExperimentInfo::sample() const {
  populateIfNotLoaded();
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return *m_sample;
}

/** Get a reference to the Sample associated with this workspace.
*  This non-const method will copy the sample if it is shared between
*  more than one workspace, and the reference returned will be to the copy.
*  Can ONLY be taken by reference!
* @return reference to sample object
*/
Sample &ExperimentInfo::mutableSample() {
  populateIfNotLoaded();
  // Use a double-check for sharing so that we only
  // enter the critical region if absolutely necessary
  if (!m_sample.unique()) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // Check again because another thread may have taken copy
    // and dropped reference count since previous check
    if (!m_sample.unique()) {
      boost::shared_ptr<Sample> oldData = m_sample;
      m_sample = boost::make_shared<Sample>(*oldData);
    }
  }
  return *m_sample;
}

/** Get a constant reference to the Run object associated with this workspace.
* @return const reference to run object
*/
const Run &ExperimentInfo::run() const {
  populateIfNotLoaded();
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return *m_run;
}

/** Get a reference to the Run object associated with this workspace.
*  This non-const method will copy the Run object if it is shared between
*  more than one workspace, and the reference returned will be to the copy.
*  Can ONLY be taken by reference!
* @return reference to Run object
*/
Run &ExperimentInfo::mutableRun() {
  populateIfNotLoaded();
  // Use a double-check for sharing so that we only
  // enter the critical region if absolutely necessary
  if (!m_run.unique()) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
  populateIfNotLoaded();
  try {
    return run().getProperty(log);
  } catch (Kernel::Exception::NotFoundError &) {
    // No log with that name
  }
  // If the instrument has a parameter with that name then take the value as a
  // log name
  const std::string logName =
      constInstrumentParameters().getString(sptr_instrument.get(), log);
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
  populateIfNotLoaded();
  try {
    return run().getPropertyAsSingleValue(log);
  } catch (Kernel::Exception::NotFoundError &) {
    // No log with that name
  }
  // If the instrument has a parameter with that name then take the value as a
  // log name
  const std::string logName =
      constInstrumentParameters().getString(sptr_instrument.get(), log);
  if (logName.empty()) {
    throw std::invalid_argument(
        "ExperimentInfo::getLog - No instrument parameter named \"" + log +
        "\". Cannot access full log name");
  }
  return run().getPropertyAsSingleValue(logName);
}

/** Utility method to get the run number
 *
 * @return the run number (int) or 0 if not found.
 */
int ExperimentInfo::getRunNumber() const {
  populateIfNotLoaded();
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
 * checks the sample log & instrument in this order
 */
Kernel::DeltaEMode::Type ExperimentInfo::getEMode() const {
  populateIfNotLoaded();
  static const char *emodeTag = "deltaE-mode";
  std::string emodeStr;
  if (run().hasProperty(emodeTag)) {
    emodeStr = run().getPropertyValueAsType<std::string>(emodeTag);
  } else if (sptr_instrument &&
             constInstrumentParameters().contains(sptr_instrument.get(),
                                                  emodeTag)) {
    Geometry::Parameter_sptr param =
        constInstrumentParameters().get(sptr_instrument.get(), emodeTag);
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
  populateIfNotLoaded();
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
  populateIfNotLoaded();
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
  populateIfNotLoaded();
  IDetector_const_sptr det = getInstrument()->getDetector(detID);
  Geometry::ParameterMap &pmap = instrumentParameters();
  pmap.addDouble(det.get(), "Efixed", value);
}

// used to terminate SAX process
class DummyException {
public:
  std::string m_validFrom;
  std::string m_validTo;
  DummyException(const std::string &validFrom, const std::string &validTo)
      : m_validFrom(validFrom), m_validTo(validTo) {}
};

// SAX content handler for grapping stuff quickly from IDF
class myContentHandler : public Poco::XML::ContentHandler {
  void startElement(const XMLString &, const XMLString &localName,
                    const XMLString &, const Attributes &attrList) override {
    if (localName == "instrument") {
      throw DummyException(
          static_cast<std::string>(attrList.getValue("", "valid-from")),
          static_cast<std::string>(attrList.getValue("", "valid-to")));
    }
  }
  void endElement(const XMLString &, const XMLString &,
                  const XMLString &) override {}
  void startDocument() override {}
  void endDocument() override {}
  void characters(const XMLChar[], int, int) override {}
  void endPrefixMapping(const XMLString &) override {}
  void ignorableWhitespace(const XMLChar[], int, int) override {}
  void processingInstruction(const XMLString &, const XMLString &) override {}
  void setDocumentLocator(const Locator *) override {}
  void skippedEntity(const XMLString &) override {}
  void startPrefixMapping(const XMLString &, const XMLString &) override {}
};

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

/** Return workspace start date as an ISO 8601 string. If this info not stored
*in workspace the
*   method returns current date. This date is used for example to retrieve the
*instrument file.
*
*  @return workspace start date as a string (current time if start date not
*available)
*/
std::string ExperimentInfo::getWorkspaceStartDate() const {
  populateIfNotLoaded();
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

/** Return workspace start date as a formatted string (strftime, as
 *  returned by Kernel::DateAndTime) string, if available. If
 *  unavailable, an empty string is returned
 *
 *  @return workspace start date as a string (empty if no date available)
 */
std::string ExperimentInfo::getAvailableWorkspaceStartDate() const {
  populateIfNotLoaded();
  std::string date;
  try {
    date = run().startTime().toFormattedString();
  } catch (std::runtime_error &) {
    g_log.information("Note: run_start/start_time not stored in workspace.");
  }
  return date;
}

/** Return workspace end date as a formatted string (strftime style,
 *  as returned by Kernel::DateAdnTime) string, if available. If
 *  unavailable, an empty string is returned
 *
 *  @return workspace end date as a string (empty if no date available)
 */
std::string ExperimentInfo::getAvailableWorkspaceEndDate() const {
  populateIfNotLoaded();
  std::string date;
  try {
    date = run().endTime().toFormattedString();
  } catch (std::runtime_error &) {
    g_log.information("Note: run_start/start_time not stored in workspace.");
  }
  return date;
}

/** A given instrument may have multiple IDFs associated with it. This method
*return an identifier which identify a given IDF for a given instrument.
* An IDF filename is required to be of the form IDFname + _Definition +
*Identifier + .xml, the identifier then is the part of a filename that
*identifies the IDF valid at a given date.
*
*  If several IDF files are valid at the given date the file with the most
*recent from date is selected. If no such files are found the file with the
*latest from date is selected.
*
*  If no file is found for the given instrument, an empty string is returned.
*
*  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
*  @param date :: ISO 8601 date
*  @return full path of IDF
*
* @throws Exception::NotFoundError If no valid instrument definition filename is
* found
*/
std::string
ExperimentInfo::getInstrumentFilename(const std::string &instrumentName,
                                      const std::string &date) {
  if (date.empty()) {
    // Just use the current date
    g_log.debug() << "No date specified, using current date and time.\n";
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
  for (const auto &directoryName : directoryNames) {
    // This will iterate around the directories from user ->etc ->install, and
    // find the first beat file
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
  g_log.debug() << "IDF selected is " << mostRecentIDF << '\n';
  return mostRecentIDF;
}

/** Return a const reference to the DetectorInfo object.
 *
 * Any modifications of the instrument or instrument parameters will invalidate
 * this reference.
 */
const DetectorInfo &ExperimentInfo::detectorInfo() const {
  populateIfNotLoaded();
  if (!m_detectorInfoWrapper) {
    std::lock_guard<std::mutex> lock{m_detectorInfoMutex};
    if (!m_detectorInfoWrapper)
      m_detectorInfoWrapper =
          Kernel::make_unique<DetectorInfo>(*m_detectorInfo, getInstrument());
  }
  return *m_detectorInfoWrapper;
}

/** Return a non-const reference to the DetectorInfo object. Not thread safe.
 */
DetectorInfo &ExperimentInfo::mutableDetectorInfo() {
  populateIfNotLoaded();
  // No locking here since this non-const method is not thread safe.

  // We get the non-const ParameterMap reference *first* such that no copy is
  // triggered unless really necessary. The call to `instrumentParameters`
  // releases the old m_detectorInfoWrapper to drop the reference count to the
  // ParameterMap by 1 (DetectorInfo contains a parameterized Instrument, so the
  // reference count to the ParameterMap is at least 2 if m_detectorInfoWrapper
  // is not nullptr: 1 from the ExperimentInfo, 1 from DetectorInfo). If then
  // the ExperimentInfo is not the sole owner of the ParameterMap a copy is
  // triggered.
  auto pmap = &instrumentParameters();
  // Here `getInstrument` creates a parameterized instrument, increasing the
  // reference count to the ParameterMap. This has do be done *after* getting
  // the ParameterMap.
  m_detectorInfoWrapper =
      Kernel::make_unique<DetectorInfo>(*m_detectorInfo, getInstrument(), pmap);
  return *m_detectorInfoWrapper;
}

/** Return a reference to the SpectrumInfo object.
 *
 * Any modifications of the instrument or instrument parameters will invalidate
 * this reference.
 */
const SpectrumInfo &ExperimentInfo::spectrumInfo() const {
  populateIfNotLoaded();
  if (!m_spectrumInfoWrapper) {
    std::lock_guard<std::mutex> lock{m_spectrumInfoMutex};
    if (!m_spectrumInfo) // this should happen only if not MatrixWorkspace
      cacheDefaultDetectorGrouping();
    if (!m_spectrumInfoWrapper)
      m_spectrumInfoWrapper =
          Kernel::make_unique<SpectrumInfo>(*m_spectrumInfo, *this);
  }
  // Rebuild any spectrum definitions that are out of date. Accessing
  // `API::SpectrumInfo` will rebuild invalid spectrum definitions as it
  // encounters them (if detector IDs in an `ISpectrum` are changed), however we
  // need to deal with one special case here:
  // If two algorithms (or two threads in the same algorithm) access the same
  // workspace for reading at the same time, calls to
  // `updateSpectrumDefinitionIfNecessary` done by `API::SpectrumInfo` break
  // thread-safety. `Algorithm` sets a read-lock, but this lazy update method is
  // `const` and will modify internals of the workspace nevertheless. We thus
  // need explicit locking here. Note that we do not need extra locking in the
  // case of `ExperimentInfo::mutableSpectrumInfo` or other calls to
  // `updateSpectrumDefinitionIfNecessary` done by `API::SpectrumInfo`: If the
  // workspace is only read-locked, this update will ensure that no updates will
  // be triggered by SpectrumInfo, since changing detector IDs in an `ISpectrum`
  // is not possible for a read-only workspace. If the workspace is write-locked
  // detector IDs in ISpectrum may change, but the write-lock by `Algorithm`
  // guarantees that there is no concurrent reader and thus updating is safe.
  if (std::any_of(m_spectrumDefinitionNeedsUpdate.cbegin(),
                  m_spectrumDefinitionNeedsUpdate.cend(),
                  [](char i) { return i == 1; })) {
    std::lock_guard<std::mutex> lock{m_spectrumInfoMutex};
    if (std::any_of(m_spectrumDefinitionNeedsUpdate.cbegin(),
                    m_spectrumDefinitionNeedsUpdate.cend(),
                    [](char i) { return i == 1; })) {
      for (size_t i = 0; i < m_spectrumInfoWrapper->size(); ++i)
        updateSpectrumDefinitionIfNecessary(i);
    }
  }
  return *m_spectrumInfoWrapper;
}

/** Return a non-const reference to the SpectrumInfo object. Not thread safe.
 */
SpectrumInfo &ExperimentInfo::mutableSpectrumInfo() {
  populateIfNotLoaded();
  // Creating SpectrumInfo with a non-const reference to a MatrixWorkspace will
  // call ExperimentInfo::mutableDetectorInfo() which will later be used by
  // modifications. This will trigger a copy if required. Note that the
  // following happens internally:
  // 1. make_unique creates a new SpectrumInfo, which calls
  // ExperimentInfo::mutableDetectorInfo(). In the latter method, the reference
  // count to the ParameterMap is typically not 1, so
  // invalidateInstrumentReferences() is called.
  // 2. invalidateInstrumentReferences() resets m_spectrumInfoWrapper, releasing
  // any parameterized detectors and thus dropping any unneeded references to
  // the ParameterMap.
  // 3. Construction of SpectrumInfo continues and the result is assigned to
  // m_spectrumInfoWrapper.

  // No locking here since this non-const method is not thread safe.
  if (!m_spectrumInfo) // this should happen only if not MatrixWorkspace
    cacheDefaultDetectorGrouping();
  m_spectrumInfoWrapper =
      Kernel::make_unique<SpectrumInfo>(*m_spectrumInfo, *this);
  return *m_spectrumInfoWrapper;
}

/// Sets the SpectrumDefinition for all spectra.
void ExperimentInfo::setSpectrumDefinitions(
    Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinitions) {
  if (spectrumDefinitions) {
    m_spectrumInfo = Kernel::make_unique<Beamline::SpectrumInfo>(
        std::move(spectrumDefinitions));
    m_spectrumDefinitionNeedsUpdate.resize(0);
    m_spectrumDefinitionNeedsUpdate.resize(m_spectrumInfo->size(), 0);
  } else {
    // Keep the old m_spectrumInfo which should have the correct size, but
    // invalidate all definitions.
    invalidateAllSpectrumDefinitions();
  }
  m_spectrumInfoWrapper = nullptr;
}

/** Notifies the ExperimentInfo that a spectrum definition has changed.
 *
 * ExperimentInfo will rebuild its spectrum definitions before the next use. In
 * general it should not be necessary to use this method: ISpectrum will take
 * care of this when its detector IDs are modified. */
void ExperimentInfo::invalidateSpectrumDefinition(const size_t index) {
  // This uses a vector of char, such that flags for different indices can be
  // set from different threads (std::vector<bool> is not thread-safe).
  m_spectrumDefinitionNeedsUpdate.at(index) = 1;
}

void ExperimentInfo::updateSpectrumDefinitionIfNecessary(
    const size_t index) const {
  if (m_spectrumDefinitionNeedsUpdate.at(index) != 0)
    updateCachedDetectorGrouping(index);
}

/** Sets up a default detector grouping.
 *
 * The purpose of this method is to work around potential issues of MDWorkspaces
 * that do not have grouping information. In such cases a default 1:1
 * mapping/grouping is generated by this method. See also issue #18252. */
void ExperimentInfo::cacheDefaultDetectorGrouping() const {
  if (m_spectrumInfo && (m_spectrumInfo->size() != 0))
    return;
  const auto &detIDs = sptr_instrument->getDetectorIDs();
  setNumberOfDetectorGroups(detIDs.size());
  size_t specIndex = 0;
  for (const auto detID : detIDs) {
    m_det2group[detID] = specIndex;
    const size_t detIndex = detectorInfo().indexOf(detID);
    SpectrumDefinition specDef;
    specDef.add(detIndex);
    m_spectrumInfo->setSpectrumDefinition(specIndex, std::move(specDef));
    m_spectrumDefinitionNeedsUpdate.at(specIndex) = 0;
    specIndex++;
  }
}

/** Returns the index of the (first) group the detID is part of.
 *
 * The purpose of this method is access to grouping information for
 * MDWorkspaces. */
size_t ExperimentInfo::groupOfDetectorID(const detid_t detID) const {
  if (!m_spectrumInfo || (m_spectrumInfo->size() == 0))
    return detectorInfo().indexOf(detID);

  auto iter = m_det2group.find(detID);
  if (iter != m_det2group.end()) {
    return iter->second;
  } else {
    throw std::out_of_range(
        "ExperimentInfo::groupOfDetectorID - Unable to find ID " +
        std::to_string(detID) + " in lookup");
  }
}

/// Sets flags for all spectrum definitions indicating that they need to be
/// updated.
void ExperimentInfo::invalidateAllSpectrumDefinitions() {
  std::fill(m_spectrumDefinitionNeedsUpdate.begin(),
            m_spectrumDefinitionNeedsUpdate.end(), 1);
}

/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 */
void ExperimentInfo::saveExperimentInfoNexus(::NeXus::File *file) const {
  Instrument_const_sptr instrument = getInstrument();
  instrument->saveNexus(file, "instrument");
  sample().saveNexus(file, "sample");
  run().saveNexus(file, "logs");
}

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

/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param nxFilename :: the filename of the nexus file
 * @param[out] parameterStr :: special string for all the parameters.
 *             Feed that to ExperimentInfo::readParameterMap() after the
 * instrument is done.
 * @throws Exception::NotFoundError If instrument definition is not in the nexus
 * file and cannot
 *                                  be loaded from the IDF.
 */
void ExperimentInfo::loadExperimentInfoNexus(const std::string &nxFilename,
                                             ::NeXus::File *file,
                                             std::string &parameterStr) {
  // load sample and log info
  loadSampleAndLogInfoNexus(file);

  loadInstrumentInfoNexus(nxFilename, file, parameterStr);
}

/** Load the instrument from an open NeXus file.
 * @param nxFilename :: the filename of the nexus file
 * @param file :: open NeXus file
 * @param[out] parameterStr :: special string for all the parameters.
 *             Feed that to ExperimentInfo::readParameterMap() after the
 * instrument is done.
 * @throws Exception::NotFoundError If instrument definition is not in the nexus
 * file and cannot
 *                                  be loaded from the IDF.
 */
void ExperimentInfo::loadInstrumentInfoNexus(const std::string &nxFilename,
                                             ::NeXus::File *file,
                                             std::string &parameterStr) {

  // Open instrument group
  file->openGroup("instrument", "NXinstrument");

  // Try to get the instrument embedded in the Nexus file
  std::string instrumentName;
  std::string instrumentXml;
  loadEmbeddedInstrumentInfoNexus(file, instrumentName, instrumentXml);

  // load parameters if found
  loadInstrumentParametersNexus(file, parameterStr);

  // Close the instrument group
  file->closeGroup();

  // Set the instrument given the name and and XML obtained
  setInstumentFromXML(nxFilename, instrumentName, instrumentXml);
}

/** Load the instrument from an open NeXus file without reading any parameters
 * (yet).
 * @param nxFilename :: the filename of the nexus file
 * @param file :: open NeXus file
 * instrument is done.
 * @throws Exception::NotFoundError If instrument definition is not in the nexus
 * file and cannot
 *                                  be loaded from the IDF.
 */
void ExperimentInfo::loadInstrumentInfoNexus(const std::string &nxFilename,
                                             ::NeXus::File *file) {

  // Open instrument group
  file->openGroup("instrument", "NXinstrument");

  // Try to get the instrument embedded in the Nexus file
  std::string instrumentName;
  std::string instrumentXml;
  loadEmbeddedInstrumentInfoNexus(file, instrumentName, instrumentXml);

  // Close the instrument group
  file->closeGroup();

  // Set the instrument given the name and and XML obtained
  setInstumentFromXML(nxFilename, instrumentName, instrumentXml);
}

/** Attempt to load an IDF embedded in the Nexus file.
 * @param file :: open NeXus file with instrument group open
 * @param[out] instrumentName :: name of instrument
 * @param[out] instrumentXml  :: XML string of embedded instrument definition or
 * empty if not found
 */
void ExperimentInfo::loadEmbeddedInstrumentInfoNexus(
    ::NeXus::File *file, std::string &instrumentName,
    std::string &instrumentXml) {

  file->readData("name", instrumentName);

  try {
    file->openGroup("instrument_xml", "NXnote");
    file->readData("data", instrumentXml);
    file->closeGroup();
  } catch (NeXus::Exception &ex) {
    g_log.debug(std::string("Unable to load instrument_xml: ") + ex.what());
  }
}

/** Set the instrument given its name and definition in XML
 *  If the XML string is empty the definition is loaded from the IDF file
 *  specified by the name
 * @param nxFilename :: the filename of the nexus file, needed to check whether
 * instrument already exists in ADS.
 * @param instrumentName :: name of instrument
 * @param instrumentXml  :: XML string of instrument or empty to indicate load
 * of instrument definition file
 */
void ExperimentInfo::setInstumentFromXML(const std::string &nxFilename,
                                         std::string &instrumentName,
                                         std::string &instrumentXml) {

  instrumentXml = Strings::strip(instrumentXml);
  instrumentName = Strings::strip(instrumentName);
  std::string instrumentFilename;
  if (!instrumentXml.empty()) {
    // instrument xml is being loaded from the nxs file, set the
    // instrumentFilename
    // to identify the Nexus file as the source of the data
    instrumentFilename = nxFilename;
    g_log.debug() << "Using instrument IDF XML text contained in nexus file.\n";
  } else {
    // XML was not included or was empty
    // Use the instrument name to find the file
    instrumentFilename =
        getInstrumentFilename(instrumentName, getWorkspaceStartDate());
    // And now load the contents
    instrumentXml = loadInstrumentXML(instrumentFilename);
  }

  // ---------- Now parse that XML to make the instrument -------------------
  if (!instrumentXml.empty() && !instrumentName.empty()) {
    InstrumentDefinitionParser parser(instrumentFilename, instrumentName,
                                      instrumentXml);

    std::string instrumentNameMangled = parser.getMangledName();
    Instrument_sptr instr;
    // Check whether the instrument is already in the InstrumentDataService
    if (InstrumentDataService::Instance().doesExist(instrumentNameMangled)) {
      // If it does, just use the one from the one stored there
      instr = InstrumentDataService::Instance().retrieve(instrumentNameMangled);
    } else {
      // Really create the instrument
      instr = parser.parseXML(nullptr);
      // Add to data service for later retrieval
      InstrumentDataService::Instance().add(instrumentNameMangled, instr);
    }
    // Now set the instrument
    this->setInstrument(instr);
  }
}

/** Loads the contents of a file and returns the string
 *  The file is assumed to be an IDF, and already checked that
 *  the path is correct.
 *
 * @param filename :: the path to the file
 */
std::string ExperimentInfo::loadInstrumentXML(const std::string &filename) {
  try {
    return Strings::loadFile(filename);
  } catch (std::exception &e) {
    g_log.error() << "Error loading instrument IDF file: " << filename << ".\n";
    g_log.debug() << e.what() << '\n';
    throw;
  }
}

/** Load the instrument parameters from an open NeXus file if found there.
 * @param file :: open NeXus file in its Instrument group
 * @param[out] parameterStr :: special string for all the parameters.
 *             Feed that to ExperimentInfo::readParameterMap() after the
 * instrument is done.
 */
void ExperimentInfo::loadInstrumentParametersNexus(::NeXus::File *file,
                                                   std::string &parameterStr) {
  try {
    file->openGroup("instrument_parameter_map", "NXnote");
    file->readData("data", parameterStr);
    file->closeGroup();
  } catch (NeXus::Exception &ex) {
    g_log.debug(std::string("Unable to load instrument_parameter_map: ") +
                ex.what());
    g_log.information(
        "Parameter map entry missing from NeXus file. Continuing without it.");
  }
}

/** Parse the result of ParameterMap.asString() into the ParameterMap
 * of the current instrument. The instrument needs to have been loaded
 * already, of course.
 *
 * @param parameterStr :: result of ParameterMap.asString()
 */
void ExperimentInfo::readParameterMap(const std::string &parameterStr) {
  Geometry::ParameterMap &pmap = this->instrumentParameters();
  Instrument_const_sptr instr = this->getInstrument()->baseInstrument();

  int options = Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY;
  options += Mantid::Kernel::StringTokenizer::TOK_TRIM;
  Mantid::Kernel::StringTokenizer splitter(parameterStr, "|", options);

  auto iend = splitter.end();
  // std::string prev_name;
  for (auto itr = splitter.begin(); itr != iend; ++itr) {
    Mantid::Kernel::StringTokenizer tokens(*itr, ";");
    if (tokens.count() < 4)
      continue;
    std::string comp_name = tokens[0];
    // if( comp_name == prev_name ) continue; this blocks reading in different
    // parameters of the same component. RNT
    // prev_name = comp_name;
    const Geometry::IComponent *comp = nullptr;
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

    // create parameter's value as a sum of all tokens with index 3 or larger
    // this allow a parameter's value to contain ";"
    std::string paramValue = tokens[3];
    int size = static_cast<int>(tokens.count());
    for (int i = 4; i < size; i++)
      paramValue += ";" + tokens[4];

    if (tokens[2].compare("masked") == 0) {
      const auto &type = tokens[1];
      const std::string name = "dummy";
      auto param = ParameterFactory::create(type, name);
      param->fromString(paramValue);
      bool value = param->value<bool>();
      if (value) {
        // Do not add masking to ParameterMap, it is stored in DetectorInfo
        const auto det = dynamic_cast<const Detector *const>(comp);
        if (!det) {
          throw std::runtime_error("Found masking for a non-detector "
                                   "component. This is not possible");
        } else
          m_detectorInfo->setMasked(detectorInfo().indexOf(det->getID()),
                                    value);
      }
    } else {
      pmap.add(tokens[1], comp, tokens[2], paramValue);
    }
  }
}

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

  const std::string *pDescription = nullptr;
  if (!paramInfo.m_description.empty())
    pDescription = &paramInfo.m_description;

  // Some names are special. Values should be convertible to double
  if (name.compare("masked") == 0) {
    bool value(paramValue);
    if (value) {
      // Do not add masking to ParameterMap, it is stored in DetectorInfo
      const auto det =
          dynamic_cast<const Detector *const>(paramInfo.m_component);
      if (!det)
        throw std::runtime_error(
            "Found masking for a non-detector component. This is not possible");
      m_detectorInfo->setMasked(detectorInfo().indexOf(det->getID()),
                                paramValue);
    }
  } else if (name.compare("x") == 0 || name.compare("y") == 0 ||
             name.compare("z") == 0) {
    paramMap.addPositionCoordinate(paramInfo.m_component, name, paramValue);
  } else if (name.compare("rot") == 0 || name.compare("rotx") == 0 ||
             name.compare("roty") == 0 || name.compare("rotz") == 0) {
    paramMap.addRotationParam(paramInfo.m_component, name, paramValue,
                              pDescription);
  } else if (category.compare("fitting") == 0) {
    std::ostringstream str;
    str << paramInfo.m_value << " , " << paramInfo.m_fittingFunction << " , "
        << name << " , " << paramInfo.m_constraint[0] << " , "
        << paramInfo.m_constraint[1] << " , " << paramInfo.m_penaltyFactor
        << " , " << paramInfo.m_tie << " , " << paramInfo.m_formula << " , "
        << paramInfo.m_formulaUnit << " , " << paramInfo.m_resultUnit << " , "
        << (*(paramInfo.m_interpolation));
    paramMap.add("fitting", paramInfo.m_component, name, str.str(),
                 pDescription);
  } else if (category.compare("string") == 0) {
    paramMap.addString(paramInfo.m_component, name, paramInfo.m_value,
                       pDescription);
  } else if (category.compare("bool") == 0) {
    paramMap.addBool(paramInfo.m_component, name, paramValue, pDescription);
  } else if (category.compare("int") == 0) {
    paramMap.addInt(paramInfo.m_component, name, paramValue, pDescription);
  } else { // assume double
    paramMap.addDouble(paramInfo.m_component, name, paramValue, pDescription);
  }
}

void ExperimentInfo::populateIfNotLoaded() const {
  // The default implementation does nothing. Used by subclasses
  // (FileBackedExperimentInfo) to load content from files upon access.
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
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<ExperimentInfo>.";
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
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<ExperimentInfo>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
