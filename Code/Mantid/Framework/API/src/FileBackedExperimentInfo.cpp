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
Geometry::Instrument_const_sptr FileBackedExperimentInfo::getInstrument() const
{
  populateIfNotLoaded();
  return ExperimentInfo::getInstrument();
}

/**
 * @return A reference to a const version of the parameter map
 */
const Geometry::ParameterMap & FileBackedExperimentInfo::instrumentParameters() const
{
  populateIfNotLoaded();
  return ExperimentInfo::instrumentParameters();
}

/**
 * @return A reference to a non-const version of the parameter map
 */
Geometry::ParameterMap &FileBackedExperimentInfo::instrumentParameters()
{
  populateIfNotLoaded();
  return ExperimentInfo::instrumentParameters();
  
}

/**
 * @return A reference to a const version of the parameter map
 */
const Geometry::ParameterMap &FileBackedExperimentInfo::constInstrumentParameters() const
{
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
void FileBackedExperimentInfo::replaceInstrumentParameters(const Geometry::ParameterMap &pmap) {
  populateIfNotLoaded();
  ExperimentInfo::replaceInstrumentParameters(pmap);
}

/**
 * Populate object and then swap parameter map
 * @param pmap The new parameter map
 */
void FileBackedExperimentInfo::swapInstrumentParameters(Geometry::ParameterMap &pmap) {
  populateIfNotLoaded();
  ExperimentInfo::swapInstrumentParameters(pmap);
}

/**
 * Populate the object and cache the groupings
 * @param mapping A set of the detector mappings
 */
void FileBackedExperimentInfo::cacheDetectorGroupings(const det2group_map &mapping) {
  populateIfNotLoaded();
  ExperimentInfo::cacheDetectorGroupings(mapping);
}

/**
 * Populate the object and returns the members of the group for a given ID
 * @param detID A detector ID to lookup
 */
const std::vector<detid_t> & FileBackedExperimentInfo::getGroupMembers(const detid_t detID) const {
  populateIfNotLoaded();
  return ExperimentInfo::getGroupMembers(detID);
}

/**
 * Populate the object and return a detector by ID
 */
Geometry::IDetector_const_sptr FileBackedExperimentInfo::getDetectorByID(const detid_t detID) const {
  populateIfNotLoaded();
  return ExperimentInfo::getDetectorByID(detID);
}



//------------------------------------------------------------------------------------------------------
// Private members
//------------------------------------------------------------------------------------------------------
/**
 * Check if the object has been populated and load the information if
 * it has not
 */
void FileBackedExperimentInfo::populateIfNotLoaded() const {
  if(m_loaded) return;
  populateFromFile();
}

/**
 * Populate this object with the data from the file
 */
void FileBackedExperimentInfo::populateFromFile() const {
  try {
    m_file->openPath(m_path);
    // Get the sample, logs, instrument
    std::string parameterStr;
    const_cast<FileBackedExperimentInfo*>(this)->loadExperimentInfoNexus(m_file, parameterStr);
    // readParameterMap calls getInstrument() & instrumentParameters() so make sure we
    // have marked the read as done by this point or it will keep entering this method
    m_loaded = true;
    const_cast<FileBackedExperimentInfo*>(this)->readParameterMap(parameterStr);
  } catch (::NeXus::Exception &exc) {
    std::ostringstream os;
    os << "Unable to load experiment information from NeXus file: "
       << exc.what() << "\n";
    throw std::runtime_error(os.str());
  }
}

} // namespace API
} // namespace Mantid
