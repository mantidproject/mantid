#include "MantidKernel/Logger.h"
#include "MantidAPI/FileBackedExperimentInfo.h"

#include <sstream>

// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

namespace Mantid {
namespace API {

namespace {
/// static logger object
Kernel::Logger g_log("FileBackedExperimentInfo");
}

/**
  * Create an object based on a NeXus file and path
  * @param filename The full path to the file
  * @param nxpath Path to the location of the experiment information
  */
FileBackedExperimentInfo::FileBackedExperimentInfo(const std::string &filename,
                                                   const std::string &nxpath)
    : ExperimentInfo(), m_loaded(false), m_filename(filename),
      m_nxpath(nxpath) {}

/**
 * This clones the FileBackedExperimentInfo and will not cause a load
 * of the information.
 * @return A clone of the object.
 */
ExperimentInfo *FileBackedExperimentInfo::cloneExperimentInfo() const {
  return new FileBackedExperimentInfo(*this);
}

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
    ::NeXus::File nxFile(m_filename);
    nxFile.openPath(m_nxpath);
    // The loadExperimentInfo calls things such as mutableSample()
    // and if m_loaded is not true then this function is
    // will be called recursively.
    m_loaded = true;

    std::string parameterStr;
    const_cast<FileBackedExperimentInfo *>(this)
        ->loadExperimentInfoNexus(m_filename, &nxFile, parameterStr);
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
