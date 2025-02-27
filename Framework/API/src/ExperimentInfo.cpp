// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidBeamline/SpectrumInfo.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
#include "MantidNexus/NeXusException.hpp"

#include "MantidTypes/SpectrumDefinition.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <Poco/Path.h>

#include <algorithm>
#include <memory>
#include <tuple>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Types::Core;

namespace Mantid::API {
namespace {
/// static logger object
Kernel::Logger g_log("ExperimentInfo");

} // namespace

/** Constructor
 */
ExperimentInfo::ExperimentInfo() : m_parmap(new ParameterMap()), sptr_instrument(new Instrument()) {
  m_parmap->setInstrument(sptr_instrument.get());
}

/**
 * Constructs the object from a copy if the input. This leaves the new mutex
 * unlocked.
 * @param source The source object from which to initialize
 */
ExperimentInfo::ExperimentInfo(const ExperimentInfo &source) { *this = source; }

/**
 * Implements the copy assignment operator
 */
ExperimentInfo &ExperimentInfo::operator=(const ExperimentInfo &source) {
  this->copyExperimentInfoFrom(&source);
  setSpectrumDefinitions(source.spectrumInfo().sharedSpectrumDefinitions());
  return *this;
}

// Defined as default in source for forward declaration with std::unique_ptr.
ExperimentInfo::~ExperimentInfo() = default;

/** Copy the experiment info data from another ExperimentInfo instance,
 * e.g. a MatrixWorkspace.
 * @param other :: the source from which to copy ExperimentInfo
 */
void ExperimentInfo::copyExperimentInfoFrom(const ExperimentInfo *other) {
  m_sample = other->m_sample;
  m_run = other->m_run;
  this->setInstrument(other->getInstrument());
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
ExperimentInfo *ExperimentInfo::cloneExperimentInfo() const { return new ExperimentInfo(*this); }

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
    out << instName << " (" << inst->getValidFromDate().toFormattedString("%Y-%b-%d") << " to "
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
  auto paramFileVector = this->constInstrumentParameters().getParameterFilenames();
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
  out << "Run end:  " << runEnd << "\n"; // note extra space for pseudo/approx-alignment

  if (this->sample().hasOrientedLattice()) {
    const Geometry::OrientedLattice &latt = this->sample().getOrientedLattice();
    out << "Sample: a " << std::fixed << std::setprecision(1) << latt.a() << ", b " << latt.b() << ", c " << latt.c();
    out << "; alpha " << std::fixed << std::setprecision(0) << latt.alpha() << ", beta " << latt.beta() << ", gamma "
        << latt.gamma();
    out << "\n";
  }
  return out.str();
}

// Helpers for setInstrument and getInstrument
namespace {
void checkDetectorInfoSize(const Instrument &instr, const Geometry::DetectorInfo &detInfo) {
  const auto numDets = instr.getNumberDetectors();
  if (numDets != detInfo.size())
    throw std::runtime_error("ExperimentInfo: size mismatch between "
                             "DetectorInfo and number of detectors in "
                             "instrument");
}
} // namespace

/** Set the instrument
 * @param instr :: Shared pointer to an instrument.
 */
void ExperimentInfo::setInstrument(const Instrument_const_sptr &instr) {
  m_spectrumInfoWrapper = nullptr;

  // Detector IDs that were previously dropped because they were not part of the
  // instrument may now suddenly be valid, so we have to reinitialize the
  // detector grouping. Also the index corresponding to specific IDs may have
  // changed.
  if (sptr_instrument != (instr->isParametrized() ? instr->baseInstrument() : instr))
    invalidateAllSpectrumDefinitions();
  if (instr->isParametrized()) {
    sptr_instrument = instr->baseInstrument();
    // We take a *copy* of the ParameterMap since we are modifying it by setting
    // a pointer to our DetectorInfo, and in case it contains legacy parameters
    // such as positions or rotations.
    m_parmap = std::make_shared<ParameterMap>(*instr->getParameterMap());
  } else {
    sptr_instrument = instr;
    m_parmap = std::make_shared<ParameterMap>();
  }
  m_parmap->setInstrument(sptr_instrument.get());
}

/** Get a shared pointer to the parametrized instrument associated with this
 *workspace
 *
 *  @return The instrument class
 */
Instrument_const_sptr ExperimentInfo::getInstrument() const {
  populateIfNotLoaded();
  checkDetectorInfoSize(*sptr_instrument, detectorInfo());
  return Geometry::ParComponentFactory::createInstrument(sptr_instrument, m_parmap);
}

/**  Returns a new copy of the instrument parameters
 *    @return a (new) copy of the instruments parameter map
 */
Geometry::ParameterMap &ExperimentInfo::instrumentParameters() {
  populateIfNotLoaded();
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
const Geometry::ParameterMap &ExperimentInfo::constInstrumentParameters() const {
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
  ParameterValue(const Geometry::XMLInstrumentParameter &paramInfo, const API::Run &run)
      : info(paramInfo), runData(run) {}

  operator double() {
    if (info.m_logfileID.empty())
      return boost::lexical_cast<double>(info.m_value);
    else {
      const TimeROI *roi = &runData.getTimeROI();
      return info.createParamValue(runData.getTimeSeriesProperty<double>(info.m_logfileID), roi);
    }
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
} // namespace

namespace {
bool isPositionParameter(const std::string &name) { return ParameterMap::pos() == name; }

bool isRotationParameter(const std::string &name) { return ParameterMap::rot() == name; }

bool isScaleParameter(const std::string &name) { return (name == "scalex" || name == "scaley"); }

bool isRedundantPosOrRot(const std::string &name) {
  // Check size first as a small optimization.
  return (name.size() == 4) &&
         (name == "posx" || name == "posy" || name == "posz" || name == "rotx" || name == "roty" || name == "rotz");
}

template <class T> T getParam(const std::string &paramType, const std::string &paramValue) {
  const std::string name = "dummy";
  auto param = ParameterFactory::create(paramType, name);
  param->fromString(paramValue);
  return param->value<T>();
}

void updatePosition(ComponentInfo &componentInfo, const IComponent *component, const V3D &newRelPos) {
  const auto compIndex = componentInfo.indexOf(component->getComponentID());
  V3D position = newRelPos;
  if (componentInfo.hasParent(compIndex)) {
    const auto parentIndex = componentInfo.parent(compIndex);
    componentInfo.rotation(parentIndex).rotate(position);
    position += componentInfo.position(parentIndex);
  }
  componentInfo.setPosition(compIndex, position);
}

void updateRotation(ComponentInfo &componentInfo, const IComponent *component, const Quat &newRelRot) {
  const auto compIndex = componentInfo.indexOf(component->getComponentID());

  auto rotation = newRelRot;
  if (componentInfo.hasParent(compIndex)) {
    const auto parentIndex = componentInfo.parent(compIndex);
    rotation = componentInfo.rotation(parentIndex) * newRelRot;
  }
  componentInfo.setRotation(compIndex, rotation);
}

void adjustPositionsFromScaleFactor(ComponentInfo &componentInfo, const IComponent *component,
                                    const std::string &paramName, double factor) {
  double ScaleX = 1.0;
  double ScaleY = 1.0;
  if (paramName == "scalex")
    ScaleX = factor;
  else
    ScaleY = factor;
  applyRectangularDetectorScaleToComponentInfo(componentInfo, component->getComponentID(), ScaleX, ScaleY);
}
} // namespace

/** Add parameters to the instrument parameter map that are defined in
 * instrument
 *   definition file or parameter file, which may contain parameters that
 * require logfile data to be available. Logs must be loaded before running this
 * method.
 */
void ExperimentInfo::populateInstrumentParameters() {
  populateIfNotLoaded();

  // Reference to the run
  const auto &runData = run();

  // Get pointer to parameter map that we may add parameters to and information
  // about
  // the parameters that my be specified in the instrument definition file (IDF)
  Geometry::ParameterMap &paramMap = instrumentParameters();
  Geometry::ParameterMap paramMapForPosAndRot;

  // Get instrument and sample
  auto &compInfo = mutableComponentInfo();
  const auto parInstrument = getInstrument();
  const auto instrument = parInstrument->baseInstrument();
  const auto &paramInfoFromIDF = instrument->getLogfileCache();

  std::map<const IComponent *, RTP> rtpParams;

  // In this loop position and rotation parameters are inserted into the
  // temporary map paramMapForPosAndRot. In the subsequent loop, after all
  // parameters have been parsed, we update positions and rotations in
  // DetectorInfo and the temporary map goes out of scope. The main reason for
  // this is that ParameterMap will then take care of assembling parameters for
  // individual position or rotation components into a vector or quaternion. In
  // particular, we cannot directly change DetectorInfo since the order of
  // rotation components is not guaranteed.
  for (const auto &item : paramInfoFromIDF) {
    const auto &nameComp = item.first;
    const auto &paramInfo = item.second;
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
          rtpValues.theta = value;
        else if (paramN.compare(0, 1, "p") == 0)
          rtpValues.phi = value;
        if (rtpValues.haveRadius) {
          V3D pos;
          pos.spherical(rtpValues.radius, rtpValues.theta, rtpValues.phi);
          paramMapForPosAndRot.addV3D(paramInfo->m_component, ParameterMap::pos(), pos);
        }
      } else {
        populateWithParameter(paramMap, paramMapForPosAndRot, paramN, *paramInfo, runData);
      }
    } catch (std::exception &exc) {
      g_log.information() << "Unable to add component parameter '" << nameComp.first << "'. Error: " << exc.what();
      continue;
    }
  }
  for (const auto &item : paramMapForPosAndRot) {
    if (isPositionParameter(item.second->name())) {
      const auto newRelPos = item.second->value<V3D>();
      updatePosition(compInfo, item.first, newRelPos);
    } else if (isRotationParameter(item.second->name())) {
      const auto newRelRot = item.second->value<Quat>();
      updateRotation(compInfo, item.first, newRelRot);
    }
    // Parameters for individual components (x,y,z) are ignored. ParameterMap
    // did compute the correct compound positions and rotations internally.
  }
  // Special case RectangularDetector: Parameters scalex and scaley affect pixel
  // positions.
  for (const auto &item : paramMap) {
    if (isScaleParameter(item.second->name()))
      adjustPositionsFromScaleFactor(compInfo, item.first, item.second->name(), item.second->value<double>());
  }
  // paramMapForPosAndRot goes out of scope, dropping all position and rotation
  // parameters of detectors (parameters for non-detector components have been
  // inserted into paramMap via DetectorInfo::setPosition(IComponent *)).
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
  m_spectrumInfo = std::make_unique<Beamline::SpectrumInfo>(count);
  m_spectrumInfoWrapper = nullptr;
}

/** Returns the number of detector groups.
 *
 * For MatrixWorkspace this is equal to getNumberHistograms() (after
 *initialization). */
size_t ExperimentInfo::numberOfDetectorGroups() const { return m_spectrumDefinitionNeedsUpdate.size(); }

/** Sets the detector grouping for the spectrum with the given `index`.
 *
 * This method should not need to be called explicitly. Groupings are updated
 * automatically when modifying detector IDs in a workspace (via ISpectrum). */
void ExperimentInfo::setDetectorGrouping(const size_t index, const std::set<detid_t> &detIDs) const {
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
void ExperimentInfo::updateCachedDetectorGrouping(const size_t /*unused*/) const {
  throw std::runtime_error("ExperimentInfo::updateCachedDetectorGrouping: "
                           "Cannot update -- grouping information not "
                           "available");
}

/** Get a constant reference to the Sample associated with this workspace.
 * @return const reference to Sample object
 */
const Sample &ExperimentInfo::sample() const {
  populateIfNotLoaded();
  return *m_sample;
}

/** Get a reference to the Sample associated with this workspace.
 *  This non-const method will copy the sample if it is shared between
 *  more than one workspace, and the reference returned will be to the copy.
 * @return reference to sample object
 */
Sample &ExperimentInfo::mutableSample() {
  populateIfNotLoaded();
  return m_sample.access();
}

/** Get a constant reference to the Run object associated with this workspace.
 * @return const reference to run object
 */
const Run &ExperimentInfo::run() const {
  populateIfNotLoaded();
  return *m_run;
}

/** Get a reference to the Run object associated with this workspace.
 *  This non-const method will copy the Run object if it is shared between
 *  more than one workspace, and the reference returned will be to the copy.
 * @return reference to Run object
 */
Run &ExperimentInfo::mutableRun() {
  populateIfNotLoaded();
  return m_run.access();
}

/// Set the run object. Use in particular to clear run without copying old run.
void ExperimentInfo::setSharedRun(Kernel::cow_ptr<Run> run) { m_run = std::move(run); }

/// Return the cow ptr of the run
Kernel::cow_ptr<Run> ExperimentInfo::sharedRun() { return m_run; }

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
  const std::string logName = constInstrumentParameters().getString(sptr_instrument.get(), log);
  if (logName.empty()) {
    throw std::invalid_argument("ExperimentInfo::getLog - No instrument parameter named \"" + log +
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
  const std::string logName = constInstrumentParameters().getString(sptr_instrument.get(), log);
  if (logName.empty()) {
    throw std::invalid_argument("ExperimentInfo::getLog - No instrument parameter named \"" + log +
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
  } else if (sptr_instrument && constInstrumentParameters().contains(sptr_instrument.get(), emodeTag)) {
    Geometry::Parameter_sptr param = constInstrumentParameters().get(sptr_instrument.get(), emodeTag);
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
double ExperimentInfo::getEFixed(const std::shared_ptr<const Geometry::IDetector> &detector) const {
  populateIfNotLoaded();
  Kernel::DeltaEMode::Type emode = getEMode();
  return getEFixedGivenEMode(detector, emode);
}

double ExperimentInfo::getEFixedForIndirect(const std::shared_ptr<const Geometry::IDetector> &detector,
                                            const std::vector<std::string> &parameterNames) const {
  double efixed = 0.;
  for (auto &parameterName : parameterNames) {
    Parameter_sptr par = constInstrumentParameters().getRecursive(detector.get(), parameterName);
    if (par) {
      efixed = par->value<double>();
    } else {
      std::vector<double> efixedVec = detector->getNumberParameter(parameterName);
      if (efixedVec.empty()) {
        int detid = detector->getID();
        IDetector_const_sptr detectorSingle = getInstrument()->getDetector(detid);
        efixedVec = detectorSingle->getNumberParameter(parameterName);
      }
      if (!efixedVec.empty()) {
        efixed = efixedVec.at(0);
      }
    }
  }
  if (efixed == 0.) {
    std::ostringstream os;
    os << "ExperimentInfo::getEFixed - Indirect mode efixed requested but "
          "detector has no Efixed parameter attached. ID="
       << detector->getID();
    throw std::runtime_error(os.str());
  }
  return efixed;
}

/**
 * Easy access to the efixed value for this run & detector
 * @param detector :: The detector object to ask for the efixed mode. Only
 * required for Indirect mode
 * @param emode :: enum value indicating whether elastic, direct or indirect
 * @return The current efixed value
 */
double ExperimentInfo::getEFixedGivenEMode(const std::shared_ptr<const Geometry::IDetector> &detector,
                                           const Kernel::DeltaEMode::Type emode) const {
  if (emode == Kernel::DeltaEMode::Direct) {
    double efixed = 0.;
    for (auto &parameterName : {"Ei", "EnergyRequested", "EnergyEstimate"}) {
      if (run().hasProperty(parameterName)) {
        efixed = run().getPropertyValueAsType<double>(parameterName);
        break;
      }
    }
    if (efixed == 0.) {
      throw std::runtime_error("Experiment logs do not contain an Ei "
                               "value. Have you run GetEi?");
    }
    return efixed;
  } else if (emode == Kernel::DeltaEMode::Indirect) {
    if (!detector)
      throw std::runtime_error("ExperimentInfo::getEFixed - Indirect mode "
                               "efixed requested without a valid detector.");
    return getEFixedForIndirect(detector, {"Efixed", "EFixed-val"});
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
    date = Types::Core::DateAndTime::getCurrentTime().toISO8601String();
  }
  return date;
}

/** Return workspace start date as a formatted string (strftime, as
 *  returned by Types::Core::DateAndTime) string, if available. If
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

//-----------------------------------------------------------------------------------------------------------------------

/** Return a const reference to the DetectorInfo object.
 *
 * Setting a new instrument via ExperimentInfo::setInstrument will invalidate
 * this reference.
 */
const Geometry::DetectorInfo &ExperimentInfo::detectorInfo() const {
  populateIfNotLoaded();
  return m_parmap->detectorInfo();
}

/** Return a non-const reference to the DetectorInfo object. */
Geometry::DetectorInfo &ExperimentInfo::mutableDetectorInfo() {
  populateIfNotLoaded();
  return m_parmap->mutableDetectorInfo();
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
    if (!m_spectrumInfoWrapper) {
      static_cast<void>(detectorInfo());
      m_spectrumInfoWrapper = std::make_unique<SpectrumInfo>(*m_spectrumInfo, *this, m_parmap->mutableDetectorInfo());
    }
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
  if (std::any_of(m_spectrumDefinitionNeedsUpdate.cbegin(), m_spectrumDefinitionNeedsUpdate.cend(),
                  [](char i) { return i == 1; })) {
    std::lock_guard<std::mutex> lock{m_spectrumInfoMutex};
    if (std::any_of(m_spectrumDefinitionNeedsUpdate.cbegin(), m_spectrumDefinitionNeedsUpdate.cend(),
                    [](char i) { return i == 1; })) {
      auto size = static_cast<int64_t>(m_spectrumInfoWrapper->size());
#pragma omp parallel for
      for (int64_t i = 0; i < size; ++i) {
        updateSpectrumDefinitionIfNecessary(i);
      }
    }
  }
  return *m_spectrumInfoWrapper;
}

/** Return a non-const reference to the SpectrumInfo object. Not thread safe.
 */
SpectrumInfo &ExperimentInfo::mutableSpectrumInfo() {
  return const_cast<SpectrumInfo &>(static_cast<const ExperimentInfo &>(*this).spectrumInfo());
}

const Geometry::ComponentInfo &ExperimentInfo::componentInfo() const { return m_parmap->componentInfo(); }

ComponentInfo &ExperimentInfo::mutableComponentInfo() { return m_parmap->mutableComponentInfo(); }

/// Sets the SpectrumDefinition for all spectra.
void ExperimentInfo::setSpectrumDefinitions(Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinitions) {
  if (spectrumDefinitions) {
    m_spectrumInfo = std::make_unique<Beamline::SpectrumInfo>(std::move(spectrumDefinitions));
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

void ExperimentInfo::updateSpectrumDefinitionIfNecessary(const size_t index) const {
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

/// Sets flags for all spectrum definitions indicating that they need to be
/// updated.
void ExperimentInfo::invalidateAllSpectrumDefinitions() {
  std::fill(m_spectrumDefinitionNeedsUpdate.begin(), m_spectrumDefinitionNeedsUpdate.end(), 1);
}

/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param saveLegacyInstrument : defaults to true, otherwise not in file output
 */
void ExperimentInfo::saveExperimentInfoNexus(::NeXus::File *file, bool saveLegacyInstrument) const {
  Instrument_const_sptr instrument = getInstrument();
  if (saveLegacyInstrument) {
    instrument->saveNexus(file, "instrument");
  }
  sample().saveNexus(file, "sample");
  run().saveNexus(file, "logs");
}

/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param saveInstrument :: option to save Instrument
 * @param saveSample :: option to save Sample
 * @param saveLogs :: option to save Logs
 */
void ExperimentInfo::saveExperimentInfoNexus(::NeXus::File *file, bool saveInstrument, bool saveSample,
                                             bool saveLogs) const {
  Instrument_const_sptr instrument = getInstrument();

  if (saveInstrument)
    instrument->saveNexus(file, "instrument");
  if (saveSample)
    sample().saveNexus(file, "sample");
  if (saveLogs)
    run().saveNexus(file, "logs");
}

/** Load the sample and log info from an open NeXus file.
 * @param file :: open NeXus file object
 * @param fileInfo :: The file info descriptor corresponding to the provided file
 * @param prefix :: The prefix of the file
 */
void ExperimentInfo::loadSampleAndLogInfoNexus(::NeXus::File *file, const Mantid::Kernel::NexusDescriptor &fileInfo,
                                               const std::string &prefix) {
  // First, the sample and then the logs
  int sampleVersion = mutableSample().loadNexus(file, "sample");
  if (sampleVersion == 0) {
    // Old-style (before Sep-9-2011) NXS processed
    // sample field contains both the logs and the sample details
    file->openGroup("sample", "NXsample");
    this->mutableRun().loadNexus(file, "", fileInfo, prefix);
    file->closeGroup();
  } else {
    // Newer style: separate "logs" field for the Run object
    this->mutableRun().loadNexus(file, "logs", fileInfo, prefix);
  }
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

void ExperimentInfo::loadExperimentInfoNexus(const std::string &nxFilename, ::NeXus::File *file,
                                             std::string &parameterStr, const Mantid::Kernel::NexusDescriptor &fileInfo,
                                             const std::string &prefix) {
  // TODO
  // load sample and log info
  loadSampleAndLogInfoNexus(file, fileInfo, prefix);

  loadInstrumentInfoNexus(nxFilename, file, parameterStr);
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
void ExperimentInfo::loadExperimentInfoNexus(const std::string &nxFilename, ::NeXus::File *file,
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
void ExperimentInfo::loadInstrumentInfoNexus(const std::string &nxFilename, ::NeXus::File *file,
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
void ExperimentInfo::loadInstrumentInfoNexus(const std::string &nxFilename, ::NeXus::File *file) {

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
void ExperimentInfo::loadEmbeddedInstrumentInfoNexus(::NeXus::File *file, std::string &instrumentName,
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
void ExperimentInfo::setInstumentFromXML(const std::string &nxFilename, std::string &instrumentName,
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
    instrumentFilename = InstrumentFileFinder::getInstrumentFilename(instrumentName, getWorkspaceStartDate());
    // And now load the contents
    instrumentXml = loadInstrumentXML(instrumentFilename);
  }

  // ---------- Now parse that XML to make the instrument -------------------
  if (!instrumentXml.empty() && !instrumentName.empty()) {
    InstrumentDefinitionParser parser(instrumentFilename, instrumentName, instrumentXml);

    std::string instrumentNameMangled = parser.getMangledName();
    Instrument_sptr instr;
    // Check whether the instrument is already in the InstrumentDataService
    if (InstrumentDataService::Instance().doesExist(instrumentNameMangled)) {
      // If it does, just use the one from the one stored there
      instr = InstrumentDataService::Instance().retrieve(instrumentNameMangled);
    } else {
      // Really create the instrument
      instr = parser.parseXML(nullptr);
      // Parse the instrument tree (internally create ComponentInfo and
      // DetectorInfo). This is an optimization that avoids duplicate parsing
      // of the instrument tree when loading multiple workspaces with the same
      // instrument. As a consequence less time is spent and less memory is
      // used. Note that this is only possible since the tree in `instrument`
      // will not be modified once we add it to the IDS.
      instr->parseTreeAndCacheBeamline();

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
void ExperimentInfo::loadInstrumentParametersNexus(::NeXus::File *file, std::string &parameterStr) {
  try {
    file->openGroup("instrument_parameter_map", "NXnote");
    file->readData("data", parameterStr);
    file->closeGroup();
  } catch (NeXus::Exception &ex) {
    g_log.debug(std::string("Unable to load instrument_parameter_map: ") + ex.what());
    g_log.information("Parameter map entry missing from NeXus file. Continuing without it.");
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
  auto &compInfo = mutableComponentInfo();
  auto &detInfo = mutableDetectorInfo();
  const auto parInstrument = getInstrument();
  const auto instr = parInstrument->baseInstrument();

  int options = Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY;
  options += Mantid::Kernel::StringTokenizer::TOK_TRIM;
  Mantid::Kernel::StringTokenizer splitter(parameterStr, "|", options);

  auto iend = splitter.end();

  Mantid::Kernel::StringTokenizer tokens;
  const std::string visibilityKey = "visible:"; // if visibility is defined, the value will follow this key
  // std::string prev_name;
  for (auto itr = splitter.begin(); itr != iend; ++itr) {
    tokens = Mantid::Kernel::StringTokenizer(*itr, ";");
    if (tokens.count() < 4)
      continue;
    std::string comp_name = tokens[0];
    // if( comp_name == prev_name ) continue; this blocks reading in different
    // parameters of the same component. RNT
    // prev_name = comp_name;
    const Geometry::IComponent *comp = nullptr;
    if (comp_name.find("detID:") != std::string::npos) {
      int detID = std::stoi(comp_name.substr(6));
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
    auto size = static_cast<int>(tokens.count());
    for (int i = 4; i < size; i++)
      paramValue += ";" + tokens[i];
    const auto &paramType = tokens[1];
    const auto &paramName = tokens[2];
    auto &paramVisibility = tokens[size - 1]; // parameter visibility, if defined, is the last token
    if (paramVisibility.find(visibilityKey) > paramVisibility.size())
      paramVisibility = "true"; // visibility not defined: default to visible
    else {                      // defined, the paramValue has one too many entries, -1 to remove also the semicolon
      paramVisibility =
          paramVisibility.substr(paramVisibility.find(visibilityKey) + visibilityKey.size(), paramVisibility.size());
      paramValue.erase(paramValue.find(visibilityKey) - 1, paramValue.size());
    }
    const auto paramDescr = std::string("");
    if (paramName == "masked") {
      auto value = getParam<bool>(paramType, paramValue);
      if (value) {
        // Do not add masking to ParameterMap, it is stored in DetectorInfo
        const auto componentIndex = compInfo.indexOf(comp->getComponentID());
        if (!compInfo.isDetector(componentIndex)) {
          throw std::runtime_error("Found masking for a non-detector "
                                   "component. This is not possible");
        } else
          detInfo.setMasked(componentIndex, value); // all detector indexes
                                                    // have same component
                                                    // index (guarantee)
      }
    } else if (isPositionParameter(paramName)) {
      // We are parsing a string obtained from a ParameterMap. The map may
      // contain posx, posy, and posz (in addition to pos). However, when these
      // component wise positions are set, 'pos' is updated accordingly. We are
      // thus ignoring position components below.
      const auto newRelPos = getParam<V3D>(paramType, paramValue);
      updatePosition(compInfo, comp, newRelPos);
    } else if (isRotationParameter(paramName)) {
      // We are parsing a string obtained from a ParameterMap. The map may
      // contain rotx, roty, and rotz (in addition to rot). However, when these
      // component wise rotations are set, 'rot' is updated accordingly. We are
      // thus ignoring rotation components below.
      const auto newRelRot = getParam<Quat>(paramType, paramValue);
      updateRotation(compInfo, comp, newRelRot);
    } else if (!isRedundantPosOrRot(paramName)) {
      // Special case RectangularDetector: Parameters scalex and scaley affect
      // pixel positions, but we must also add the parameter below.
      if (isScaleParameter(paramName))
        adjustPositionsFromScaleFactor(compInfo, comp, paramName, getParam<double>(paramType, paramValue));
      pmap.add(paramType, comp, paramName, paramValue, &paramDescr, paramVisibility);
    }
  }
}

/**
 * Fill map with instrument parameter first set in xml file
 * Where this is appropriate a parameter value is dependent on values in a log
 * entry
 * @param paramMap Map to populate (except for position and rotation parameters)
 * @param paramMapForPosAndRot Map to populate with positions and rotations
 * @param name The name of the parameter
 * @param paramInfo A reference to the object describing this parameter
 * @param runData A reference to the run object, which stores log value entries
 */
void ExperimentInfo::populateWithParameter(Geometry::ParameterMap &paramMap,
                                           Geometry::ParameterMap &paramMapForPosAndRot, const std::string &name,
                                           const Geometry::XMLInstrumentParameter &paramInfo, const Run &runData) {
  const std::string &category = paramInfo.m_type;
  ParameterValue paramValue(paramInfo,
                            runData); // Defines implicit conversion operator

  const std::string *pDescription = nullptr;
  if (!paramInfo.m_description.empty())
    pDescription = &paramInfo.m_description;
  std::string pVisible = "true";
  if (!paramInfo.m_visible.empty())
    pVisible = paramInfo.m_visible;

  // Some names are special. Values should be convertible to double
  if (name == "masked") {
    bool value(paramValue);
    if (value) {
      // Do not add masking to ParameterMap, it is stored in DetectorInfo

      const auto componentIndex = componentInfo().indexOf(paramInfo.m_component->getComponentID());
      if (!componentInfo().isDetector(componentIndex))
        throw std::runtime_error("Found masking for a non-detector component. This is not possible");
      mutableDetectorInfo().setMasked(componentIndex,
                                      paramValue); // all detector indexes have
                                                   // same component index
                                                   // (guarantee)
    }
  } else if (name == "x" || name == "y" || name == "z") {
    paramMapForPosAndRot.addPositionCoordinate(paramInfo.m_component, name, paramValue);
  } else if (name == "rot" || name == "rotx" || name == "roty" || name == "rotz") {
    // Effectively this is dropping any parameters named 'rot'.
    paramMapForPosAndRot.addRotationParam(paramInfo.m_component, name, paramValue, pDescription);
  } else if (category == "fitting") {
    std::ostringstream str;
    str << paramInfo.m_value << " , " << paramInfo.m_fittingFunction << " , " << name << " , "
        << paramInfo.m_constraint[0] << " , " << paramInfo.m_constraint[1] << " , " << paramInfo.m_penaltyFactor
        << " , " << paramInfo.m_tie << " , " << paramInfo.m_formula << " , " << paramInfo.m_formulaUnit << " , "
        << paramInfo.m_resultUnit << " , " << (*(paramInfo.m_interpolation));
    paramMap.add("fitting", paramInfo.m_component, name, str.str(), pDescription, pVisible);
  } else if (category == "string") {
    paramMap.addString(paramInfo.m_component, name, paramInfo.m_value, pDescription, pVisible);
  } else if (category == "bool") {
    paramMap.addBool(paramInfo.m_component, name, paramValue, pDescription, pVisible);
  } else if (category == "int") {
    paramMap.addInt(paramInfo.m_component, name, paramValue, pDescription, pVisible);
  } else { // assume double
    paramMap.addDouble(paramInfo.m_component, name, paramValue, pDescription, pVisible);
  }
}

void ExperimentInfo::populateIfNotLoaded() const {
  // The default implementation does nothing. Used by subclasses
  // (FileBackedExperimentInfo) to load content from files upon access.
}

} // namespace Mantid::API

namespace Mantid::Kernel {

template <>
MANTID_API_DLL Mantid::API::ExperimentInfo_sptr
IPropertyManager::getValue<Mantid::API::ExperimentInfo_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<Mantid::API::ExperimentInfo_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<ExperimentInfo>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL Mantid::API::ExperimentInfo_const_sptr
IPropertyManager::getValue<Mantid::API::ExperimentInfo_const_sptr>(const std::string &name) const {
  auto const *prop = dynamic_cast<PropertyWithValue<Mantid::API::ExperimentInfo_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<ExperimentInfo>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel
