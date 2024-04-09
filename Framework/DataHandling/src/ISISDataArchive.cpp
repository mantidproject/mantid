// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/ISISDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"

#include "MantidKernel/StringTokenizer.h"
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid::DataHandling {
namespace {
/// static logger
Kernel::Logger g_log("ISISDataArchive");
} // namespace

DECLARE_ARCHIVESEARCH(ISISDataArchive, ISISDataSearch)

namespace {
#ifdef _WIN32
constexpr std::string_view URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/windir?name=";
#else
constexpr std::string_view URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/unixdir?name=";
#endif
} // namespace

/**
 * Query the ISIS archive for a set of filenames and vector of extensions. The
 * method gets a path to each of the filenames, and then loops over the
 * extensions to find the correct file.
 * @param filenames :: A set of filenames without extensions
 * @param exts :: A vector of file extensions to search over.
 * @returns The full path to the first found
 */
const API::Result<std::string> ISISDataArchive::getArchivePath(const std::set<std::string> &filenames,
                                                               const std::vector<std::string> &exts) const {
  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    for (const auto &filename : filenames) {
      g_log.debug() << filename << ")\n";
    }
    for (const auto &ext : exts) {
      g_log.debug() << ext << ")\n";
    }
  }

  std::string errors = "";
  for (const auto &filename : filenames) {
    std::string path_without_extension = getPath(filename);
    if (!path_without_extension.empty()) {
#ifdef __APPLE__
      // Replace Linux path with macOS path if we're on a Mac.
      constexpr std::string_view unixString = "/archive/";
      constexpr std::string_view macOSString = "/Volumes/inst$/";
      path_without_extension.replace(0, unixString.length(), macOSString);
#endif
      std::string fullPath = getCorrectExtension(path_without_extension, exts);
      if (!fullPath.empty())
        return API::Result<std::string>(fullPath);
      errors += "No file found. ";
#ifdef __linux__
      errors +=
          "If you are an IDAaaS user, and your file is in the ISIS archive, then check if the archive is mounted. If it is not mounted, \
                \nthen click Applications->Data->Experiment Archive (Staff Only), and enter your federal ID credentials with 'clrc' as the domain.";
#endif // __linux__
    }
  }
  return API::Result<std::string>("", errors);
}

/**
 * Gets the path to the file, or most recent set of files.
 * @param fName :: The file name.
 * @return The path to the file or an empty string in case of empty filename
 */
std::string ISISDataArchive::getPath(const std::string &fName) const {
  g_log.debug() << "ISISDataArchive::getPath() - fName=" << fName << "\n";
  if (fName.empty())
    return ""; // Avoid pointless call to service

  std::ostringstream os = sendRequest(fName);
  os << Poco::Path::separator() << fName;
  std::string expectedPath = os.str();
  return expectedPath;
}

/** Calls a web service to get a path to a file.
 * If the file does not exist, returns path to most recent run.
 * The ISIS web service return a path independent of the file extension of the
 * file provided. Thus the call to web service uses a file WITHOUT an extension.
 * @param fName :: The file name.
 * @return ostringstream object containing path to file (without filename
 * itself)
 */
std::ostringstream ISISDataArchive::sendRequest(const std::string &fName) const {
  Kernel::InternetHelper inetHelper;
  std::ostringstream os;
  try {
    inetHelper.sendRequest(URL_PREFIX.data() + fName, os);
  } catch (Kernel::Exception::InternetError &ie) {
    g_log.warning() << "Could not access archive index." << ie.what();
  }
  return os;
}

/**
 * Given a path to a file, this searches over possible
 * extensions to find the full path.
 * Only returns a full path string if the file exists.
 * @param path :: The path to the file without an extension.
 * @param exts :: vector of possible file extensions to search over.
 * @return The full path to the file or an empty string in case of
 * error/non-existing file.
 */
std::string ISISDataArchive::getCorrectExtension(const std::string &path, const std::vector<std::string> &exts) const {
  for (const auto &ext : exts) {
    std::string temp_path = path + ext;
    if (fileExists(temp_path))
      return temp_path;
  }
  return "";
}

/**
 * Checks if the given file path exists or not.
 * This code is in a separate function so it
 * can be mocked out in testing.
 * @param path :: The path to the file (including extension)
 * @return A bool. Whether or not the file exists.
 */
bool ISISDataArchive::fileExists(const std::string &path) const {
  try {
    if (Poco::File(path).exists())
      return true;
  } catch (Poco::Exception &) {
  }
  return false;
}

} // namespace Mantid::DataHandling
