//----------------------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------------------
#include "MantidAPI/FileBackedExperimentInfo.h"
#include <nexus/NeXusException.hpp>

#include <sstream>

namespace Mantid {
namespace API {

namespace {

/// static logger object
Kernel::Logger g_log("FileBackedExperimentInfo");
}

/**
  * Create an object based on a NeXus file and path
  * @param file A pointer to an open NeXus file object
  * @param path Path to the location of the data
  */
FileBackedExperimentInfo::FileBackedExperimentInfo(::NeXus::File *file,
                                                   const std::string &path)
    : ExperimentInfo(), m_loaded(false), m_file(file), m_path(path) {}

/**
 * @return A clone of the object
 */
ExperimentInfo *FileBackedExperimentInfo::cloneExperimentInfo() const {
  populateIfNotLoaded();
  return ExperimentInfo::cloneExperimentInfo();
}

/// @returns A human-readable description of the object
const std::string FileBackedExperimentInfo::toString() const {
  populateIfNotLoaded();
  return ExperimentInfo::toString();
}

/// @return A pointer to the parametrized instrument
Geometry::Instrument_const_sptr
FileBackedExperimentInfo::getInstrument() const {
  populateIfNotLoaded();
  return ExperimentInfo::getInstrument();
}

/**
 * @return A reference to a const version of the parameter map
 */
const Geometry::ParameterMap &
FileBackedExperimentInfo::instrumentParameters() const {
  populateIfNotLoaded();
  return ExperimentInfo::instrumentParameters();
}

/**
 * @return A reference to a non-const version of the parameter map
 */
Geometry::ParameterMap &FileBackedExperimentInfo::instrumentParameters() {
  populateIfNotLoaded();
  return ExperimentInfo::instrumentParameters();
}

/**
 * @return A reference to a const version of the parameter map
 */
const Geometry::ParameterMap &
FileBackedExperimentInfo::constInstrumentParameters() const {
  populateIfNotLoaded();
  return ExperimentInfo::constInstrumentParameters();
}

/**
 * Populate object with instrument parameters
 */
void FileBackedExperimentInfo::populateInstrumentParameters() {
  populateIfNotLoaded();
  return ExperimentInfo::populateInstrumentParameters();
}

/**
 * Populate object and then replace parameter map
 * @param pmap The new parameter map
 */
void FileBackedExperimentInfo::replaceInstrumentParameters(
    const Geometry::ParameterMap &pmap) {
  populateIfNotLoaded();
  ExperimentInfo::replaceInstrumentParameters(pmap);
}

/**
 * Populate object and then swap parameter map
 * @param pmap The new parameter map
 */
void FileBackedExperimentInfo::swapInstrumentParameters(
    Geometry::ParameterMap &pmap) {
  populateIfNotLoaded();
  ExperimentInfo::swapInstrumentParameters(pmap);
}

/**
 * Populate the object and cache the groupings
 * @param mapping A set of the detector mappings
 */
void
FileBackedExperimentInfo::cacheDetectorGroupings(const det2group_map &mapping) {
  populateIfNotLoaded();
  ExperimentInfo::cacheDetectorGroupings(mapping);
}

/**
 * Populate the object and returns the members of the group for a given ID
 * @param detID A detector ID to lookup
 */
const std::vector<detid_t> &
FileBackedExperimentInfo::getGroupMembers(const detid_t detID) const {
  populateIfNotLoaded();
  return ExperimentInfo::getGroupMembers(detID);
}

/**
 * Populate the object and return a detector by ID
 * @param detID A detector ID to lookup
 */
Geometry::IDetector_const_sptr
FileBackedExperimentInfo::getDetectorByID(const detid_t detID) const {
  populateIfNotLoaded();
  return ExperimentInfo::getDetectorByID(detID);
}

/**
 * Populate the object and set the moderator model
 * @param source A pointer to the model of the moderator
 */
void FileBackedExperimentInfo::setModeratorModel(ModeratorModel *source) {
  populateIfNotLoaded();
  ExperimentInfo::setModeratorModel(source);
}

/**
 * @return The object governing the moderator model
 */
ModeratorModel &FileBackedExperimentInfo::moderatorModel() const {
  populateIfNotLoaded();
  return ExperimentInfo::moderatorModel();
}

/**
 * Populate the object & set the model governing the chopper
 * @param chopper The model governing the chopper
 * @param index The index of the chopper
 */
void FileBackedExperimentInfo::setChopperModel(ChopperModel *chopper,
                                               const size_t index) {
  populateIfNotLoaded();
  ExperimentInfo::setChopperModel(chopper, index);
}

/**
 * Populate the object & return the model of the chopper
 * @param index The index of the chopper
 */
ChopperModel &FileBackedExperimentInfo::chopperModel(const size_t index) const {
  populateIfNotLoaded();
  return ExperimentInfo::chopperModel(index);
}

/**
 * Populate object and return the Sample
 * @return A const reference to the Sample
 */
const Sample &FileBackedExperimentInfo::sample() const {
  populateIfNotLoaded();
  return ExperimentInfo::sample();
}

/**
 * Populate object and return a non-const reference to the sample
 * @return A non-const reference to the Sample
 */
Sample &FileBackedExperimentInfo::mutableSample() {
  populateIfNotLoaded();
  return ExperimentInfo::mutableSample();
}

/**
 * Populate object and return a const reference to the run
 * @return A const reference to the Run
 */
const Run &FileBackedExperimentInfo::run() const {
  populateIfNotLoaded();
  return ExperimentInfo::run();
}

/**
 * Populate object and return a non-const reference to the run
 * @return A non-const reference to the Run
 */
Run &FileBackedExperimentInfo::mutableRun() {
  populateIfNotLoaded();
  return ExperimentInfo::mutableRun();
}

/**
 * Return a pointer to a log entry
 * @param log A string name of the log
 * @return A pointer to the log entry
 */
Kernel::Property *
FileBackedExperimentInfo::getLog(const std::string &log) const {
  populateIfNotLoaded();
  return ExperimentInfo::getLog(log);
}

/**
 * Return a pointer to a log entry
 * @param log A string name of the log
 * @return A pointer to the log entry
 */
double
FileBackedExperimentInfo::getLogAsSingleValue(const std::string &log) const {
  populateIfNotLoaded();
  return ExperimentInfo::getLogAsSingleValue(log);
}

/**
 * @return The run number
 */
int FileBackedExperimentInfo::getRunNumber() const {
  populateIfNotLoaded();
  return ExperimentInfo::getRunNumber();
}

/**
 * @return The inelastic energy mode
 */
Kernel::DeltaEMode::Type FileBackedExperimentInfo::getEMode() const {
  populateIfNotLoaded();
  return ExperimentInfo::getEMode();
}

/**
 * @return The efixed for a given detector
 * @param detID The ID of the detector
 */
double FileBackedExperimentInfo::getEFixed(const detid_t detID) const {
  populateIfNotLoaded();
  return ExperimentInfo::getEFixed(detID);
}

/**
 * Return the efixed value for a given detector
 * @param detector The detector object
 */
double FileBackedExperimentInfo::getEFixed(
    const Geometry::IDetector_const_sptr detector) const {
  populateIfNotLoaded();
  return ExperimentInfo::getEFixed(detector);
}

/**
 * Set the efixed value for a given detector
 * @param detID The ID of the detector
 * @param value The value of EFixed
 */
void FileBackedExperimentInfo::setEFixed(const detid_t detID,
                                         const double value) {
  populateIfNotLoaded();
  ExperimentInfo::setEFixed(detID, value);
}

//------------------------------------------------------------------------------------------------------
// Private members
//------------------------------------------------------------------------------------------------------
/**
 * Check if the object has been populated and load the information if
 * it has not
 */
void FileBackedExperimentInfo::populateIfNotLoaded() const {
  if (m_loaded)
    return;
  populateFromFile();
}

/**
 * Populate this object with the data from the file
 */
void FileBackedExperimentInfo::populateFromFile() const {
  try {
    m_file->openPath(m_path);
    // The loadExperimentInfo calls things such as mutableSample()
    // and if m_loaded is not true then this function is
    // will be called recursively.
    m_loaded = true;

    std::string parameterStr;
    const_cast<FileBackedExperimentInfo *>(this)
        ->loadExperimentInfoNexus(m_file, parameterStr);
    const_cast<FileBackedExperimentInfo *>(this)
        ->readParameterMap(parameterStr);
  } catch (::NeXus::Exception &exc) {
    std::ostringstream os;
    os << "Unable to load experiment information from NeXus file: "
       << exc.what() << "\n";
    throw std::runtime_error(os.str());
  }
}

} // namespace API
} // namespace Mantid
