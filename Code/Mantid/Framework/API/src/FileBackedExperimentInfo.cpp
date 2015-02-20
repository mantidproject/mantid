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

//----------------------------------------------------------------------------------------------
/**
  * Create an object based on a NeXus file and path
  * @param file A pointer to an open NeXus file object
  * @param path Path to the location of the data
  */
FileBackedExperimentInfo::FileBackedExperimentInfo(::NeXus::File *file,
                                                   const std::string & path)
    : ExperimentInfo(), m_loaded(false), m_file(file), m_path(path) {
}

//----------------------------------------------------------------------------------------------
/// @returns A human-readable description of the object
const std::string FileBackedExperimentInfo::toString() {
  checkAndPopulate();
  return ExperimentInfo::toString();
}


//------------------------------------------------------------------------------------------------------
// Private members
//------------------------------------------------------------------------------------------------------
/**
 * Check if the object has been populated and load the information if
 * it has not
 */
void FileBackedExperimentInfo::checkAndPopulate()
{
  if (!m_loaded) {
    populateFromFile();
  }
}

/**
 * Populate this object with the data from the file
 */
void FileBackedExperimentInfo::populateFromFile() {
  try {
    m_file->openPath(m_path);
    // Get the sample, logs, instrument
    std::string parameterStr;
    this->loadExperimentInfoNexus(m_file, parameterStr);
    // Now do the parameter map
    this->readParameterMap(parameterStr);
  } catch (::NeXus::Exception &exc) {
    std::ostringstream os;
    os << "Unable to load experiment information from NeXus file: " << exc.what() << "\n";
    throw std::runtime_error(os.str());
  }
  m_loaded = true;
}

} // namespace API
} // namespace Mantid
