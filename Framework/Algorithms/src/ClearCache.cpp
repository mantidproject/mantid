#include "MantidAlgorithms/ClearCache.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidKernel/UsageService.h"
#include <Poco/File.h>
#include <Poco/Glob.h>
#include <Poco/Path.h>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearCache)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ClearCache::name() const { return "ClearCache"; }

/// Algorithm's version for identification. @see Algorithm::version
int ClearCache::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearCache::category() const { return "Utility"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ClearCache::summary() const {
  return "Clears out selected cached information held by Mantidplot.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ClearCache::init() {
  declareProperty(
      "AlgorithmCache", false,
      "Clears the memory cache of the last used algorithm parameters.");
  declareProperty(
      "InstrumentCache", false,
      "Clears the memory cache of the loaded instrument definitions.");
  declareProperty("DownloadedInstrumentFileCache", false,
                  "Clears the file cache of the downloaded instrument "
                  "definitions.  This can be repopulated using "
                  "DownloadInstrument.");
  declareProperty(
      "GeometryFileCache", false,
      "Clears the file cache of the triangulated detector geometries.");
  declareProperty("WorkspaceCache", false,
                  "Clears the memory cache of any workspaces.");
  declareProperty("UsageServiceCache", false,
                  "Clears the memory cache of usage data.");
  declareProperty(
      "FilesRemoved", 0,
      "The number of files removed. Memory clearance do not add to this.",
      Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ClearCache::exec() {
  int filesRemoved = 0;
  bool clearAlgCache = getProperty("AlgorithmCache");
  bool clearInstService = getProperty("InstrumentCache");
  bool clearInstFileCache = getProperty("DownloadedInstrumentFileCache");
  bool clearGeometryFileCache = getProperty("GeometryFileCache");
  bool clearUsageService = getProperty("UsageServiceCache");
  bool clearAnalysisService = getProperty("WorkspaceCache");

  bool isAnythingSelected = clearAlgCache || clearInstService ||
                            clearInstFileCache || clearGeometryFileCache ||
                            clearUsageService || clearAnalysisService;
  if (!isAnythingSelected) {
    g_log.warning("Nothing caches to clear.  Nothing done.");
    return;
  }

  // get the instrument directories
  auto instrumentDirs =
      Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories();
  Poco::Path localPath(instrumentDirs[0]);
  localPath.makeDirectory();

  if (clearAlgCache) {
    g_log.debug("Emptying the Algorithm cache (AlgorithmCache).");
    AlgorithmManager::Instance().clear();
  }
  if (clearAnalysisService) {
    g_log.debug("Emptying the Analysis data service (WorkspaceCache).");
    AnalysisDataService::Instance().clear();
  }
  if (clearInstService) {
    g_log.debug("Emptying the Instrument data service (InstrumentCache).");
    InstrumentDataService::Instance().clear();
  }
  if (clearInstFileCache) {
    g_log.debug("Removing files from the Downloaded Instrument file cache "
                "(DownloadedInstrumentFileCache).");
    int filecount = deleteFiles(localPath.toString(), "*.xml");
    filecount += deleteFiles(localPath.toString(), "github.json");
    g_log.information() << filecount << " files deleted\n";
    filesRemoved += filecount;
  }
  if (clearGeometryFileCache) {
    g_log.debug("Removing files from the triangulated detector geometry file "
                "cache (GeometryFileCache).");
    Poco::Path GeomPath(localPath);
    GeomPath.append("geometryCache").makeDirectory();
    int filecount = deleteFiles(GeomPath.toString(), "*.vtp");
    g_log.information() << filecount << " files deleted\n";
    filesRemoved += filecount;
  }
  if (clearUsageService) {
    g_log.debug("Emptying the Usage data service (UsageServiceCache).");
    UsageService::Instance().clear();
  }
  setProperty("FilesRemoved", filesRemoved);
}

/** Deletes files on the path that match the pattern provided
 *  @param path The path for the deletion
 *  @param pattern The pattern to match filenames * and ? are wildcards
 *  @returns The number of files deleted
 */
int ClearCache::deleteFiles(const std::string &path,
                            const std::string &pattern) const {
  int filesDeleted = 0;

  Poco::Path pathPattern(path);
  pathPattern.makeDirectory();
  pathPattern.append(pattern);
  std::set<std::string> files;
  Poco::Glob::glob(pathPattern, files, Poco::Glob::GLOB_CASELESS);

  for (const auto &filepath : files) {
    Poco::File file(filepath);
    g_log.debug("Deleting file " + filepath);
    try {
      file.remove();
      filesDeleted++;
    } catch (Poco::FileException &ex) {
      g_log.warning("Cannot delete file " + filepath + ": " + ex.what());
    }
  }
  return filesDeleted;
}

} // namespace Algorithms
} // namespace Mantid
