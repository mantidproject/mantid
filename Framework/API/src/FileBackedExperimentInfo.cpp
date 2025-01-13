// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FileBackedExperimentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <sstream>
#include <utility>

namespace Mantid::API {

namespace {
/// static logger object
Kernel::Logger g_log("FileBackedExperimentInfo");
} // namespace

/**
 * Create an object based on a NeXus file and path
 * @param filename The full path to the file
 * @param nxpath Path to the location of the experiment information
 */
FileBackedExperimentInfo::FileBackedExperimentInfo(std::string filename, std::string nxpath)
    : ExperimentInfo(), m_loaded(false), m_filename(std::move(filename)), m_nxpath(std::move(nxpath)) {}

/**
 * This clones the FileBackedExperimentInfo and will not cause a load
 * of the information.
 * @return A clone of the object.
 */
ExperimentInfo *FileBackedExperimentInfo::cloneExperimentInfo() const { return new FileBackedExperimentInfo(*this); }

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
    const_cast<FileBackedExperimentInfo *>(this)->loadExperimentInfoNexus(m_filename, &nxFile, parameterStr);
    const_cast<FileBackedExperimentInfo *>(this)->readParameterMap(parameterStr);
  } catch (::NeXus::Exception &exc) {
    std::ostringstream os;
    os << "Unable to load experiment information from NeXus file: " << exc.what() << "\n";
    throw std::runtime_error(os.str());
  }
}

} // namespace Mantid::API
