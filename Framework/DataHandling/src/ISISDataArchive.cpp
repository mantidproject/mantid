// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/ISISDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"

#include <MantidKernel/StringTokenizer.h>
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include <sstream>

namespace Mantid {
namespace DataHandling {
namespace {
/// static logger
Kernel::Logger g_log("ISISDataArchive");
} // namespace

DECLARE_ARCHIVESEARCH(ISISDataArchive, ISISDataSearch)

namespace {
#ifdef _WIN32
const char *URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/windir?name=";
#else
const char *URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/unixdir?name=";
#endif
}

/**
 * Query the ISIS archive for a set of filenames and vector of extensions. The
 * method gets a path to each of the filenames, and then loops over the
 * extensions to find the correct file.
 * @param filenames :: A set of filenames without extensions
 * @param exts :: A vector of file extensions to search over.
 * @returns The full path to the first found
 */
std::string
ISISDataArchive::getArchivePath(const std::set<std::string> &filenames,
                                const std::vector<std::string> &exts) const {
  for (const auto &filename : filenames) {
    g_log.debug() << filename << ")\n";
  }
  for (const auto &ext : exts) {
    g_log.debug() << ext << ")\n";
  }

  for (const auto &filename : filenames) {
    const std::string path_without_extension = getPath(filename);
    if (!path_without_extension.empty()) {
      std::string fullPath = getCorrectExtension(path_without_extension, exts);
      if (!fullPath.empty())
        return fullPath;
    }
  }
  return "";
}

/**
 * Calls a web service to get a path to a file.
 * Only returns a path string if the file exists.
 * The ISIS web service return a path independent of the file extension of the
 * file provided. Thus the call to web service uses a file WITHOUT an extension.
 * @param fName :: The file name.
 * @return The path to the file or an empty string in case of error/non-existing
 * file.
 */
std::string ISISDataArchive::getPath(const std::string &fName) const {
  g_log.debug() << "ISISDataArchive::getPath() - fName=" << fName << "\n";
  if (fName.empty())
    return ""; // Avoid pointless call to service

  Kernel::InternetHelper inetHelper;
  std::ostringstream os;
  try {
    inetHelper.sendRequest(URL_PREFIX + fName, os);

    os << Poco::Path::separator() << fName;
    const std::string expectedPath = os.str();
    return expectedPath;
  } catch (Kernel::Exception::InternetError &ie) {
    g_log.warning() << "Could not access archive index " << ie.what();
  }

  return "";
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
std::string ISISDataArchive::getCorrectExtension(
    const std::string &path, const std::vector<std::string> &exts) const {
  for (auto ext : exts) {
    std::string temp_path = path + ext;
    try {
      if (Poco::File(temp_path).exists())
        return temp_path;
    } catch (Poco::Exception &) {
    }
  }
  return "";
}

} // namespace DataHandling
} // namespace Mantid
