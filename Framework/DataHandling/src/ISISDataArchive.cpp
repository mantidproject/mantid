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
 * Query the ISIS archive for a set of filenames & extensions. The method goes
 * through each extension
 * and checks whether it can find a match with each filename in the filenames
 * list. The first match is returned.
 * @param filenames :: A set of filenames without extensions
 * @param exts :: A list of extensions to try in order with each filename
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

  for (const auto &ext : exts) {
    for (const auto &filename : filenames) {
      const std::string fullPath = getPath(filename + ext);
      if (!fullPath.empty())
        return fullPath;
    } // it
  }   // ext
  return "";
}

/**
 * Calls a web service to get a full path to a file.
 * Only returns a full path string if the file exists
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
    try {
      const std::string expectedPath = os.str();
      if (Poco::File(expectedPath).exists())
        return expectedPath;
    } catch (Poco::Exception &) {
    }
  } catch (Kernel::Exception::InternetError &ie) {
    g_log.warning() << "Could not access archive index " << ie.what();
  }

  return "";
}

} // namespace DataHandling
} // namespace Mantid
