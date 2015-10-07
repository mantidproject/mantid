//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadRaw2)

using namespace Kernel;
using namespace API;

/// Empty default constructor
LoadRaw2::LoadRaw2() : Algorithm(), DeprecatedAlgorithm() {
  useAlgorithm("LoadRaw", 3);
}

/** Initialises the algorithm with the properties as they were when this
 * algorithm was removed from Mantid,
 *  though all validators have been removed
 */
void LoadRaw2::init() {
  declareProperty(
      "Filename", "",
      "The name of the RAW file to read, including its full or relative\n"
      "path. (N.B. case sensitive if running on Linux).");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "None", Direction::Output),
      "The name of the workspace that will be created, filled with the\n"
      "read-in data and stored in the Analysis Data Service.  If the input\n"
      "RAW file contains multiple periods higher periods will be stored in\n"
      "separate workspaces called OutputWorkspace_PeriodNo.");

  declareProperty(
      "SpectrumMin", 1,
      "The index number of the first spectrum to read.  Only used if\n"
      "spectrum_max is set.");
  declareProperty(
      "SpectrumMax", Mantid::EMPTY_INT(),
      "The number of the last spectrum to read. Only used if explicitly\n"
      "set.");

  declareProperty(
      new ArrayProperty<specid_t>("SpectrumList"),
      "A comma-separated list of individual spectra to read.  Only used if\n"
      "explicitly set.");
  std::vector<std::string> m_cache_options;
  m_cache_options.push_back("If Slow");
  m_cache_options.push_back("Always");
  m_cache_options.push_back("Never");
  declareProperty("Cache", "If Slow",
                  boost::make_shared<StringListValidator>(m_cache_options));
}

/** Prints a message indicating that this algorithm has been removed.
 *  @throw Exception::NotImplementedError Always
 */
void LoadRaw2::exec() {
  throw Kernel::Exception::NotImplementedError(
      "This version of LoadRaw has been removed from Mantid. "
      "You should use the current version of this algorithm or try an earlier "
      "release of Mantid.");
}

} // namespace DataHandling
} // namespace Mantid
