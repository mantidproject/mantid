// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MaxMin.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MaxMin)

using namespace Kernel;
using namespace API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

/** Initialisation method.
 *
 */
void MaxMin::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<HistogramValidator>()),
                  "The name of the Workspace2D to take as input");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name of the workspace in which to store the result");

  declareProperty("ShowMin", false,
                  "Flag to show minimum instead of maximum (default=false)");
  declareProperty("RangeLower", EMPTY_DBL(),
                  "The X value to search from (default min)");
  declareProperty("RangeUpper", EMPTY_DBL(),
                  "The X value to search to (default max)");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "Start spectrum number (default 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "End spectrum number  (default max)");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void MaxMin::exec() {
  // Try and retrieve the optional properties

  /// The value in X to start the search from
  double MinRange = getProperty("RangeLower");
  /// The value in X to finish the search at
  double MaxRange = getProperty("RangeUpper");
  /// The spectrum to start the integration from
  int MinSpec = getProperty("StartWorkspaceIndex");
  /// The spectrum to finish the integration at
  int MaxSpec = getProperty("EndWorkspaceIndex");
  /// The flag to show minimum
  bool showMin = getProperty("ShowMin");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  const auto numberOfSpectra =
      static_cast<int>(localworkspace->getNumberHistograms());

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if (MinSpec > numberOfSpectra) {
    g_log.warning("StartSpectrum out of range! Set to 0.");
    MinSpec = 0;
  }
  if (isEmpty(MaxSpec))
    MaxSpec = numberOfSpectra - 1;
  if (MaxSpec > numberOfSpectra - 1 || MaxSpec < MinSpec) {
    g_log.warning("EndSpectrum out of range! Set to max detector number");
    MaxSpec = numberOfSpectra;
  }
  if (MinRange > MaxRange) {
    g_log.warning("Range_upper is less than Range_lower. Will integrate up to "
                  "frame maximum.");
    MaxRange = 0.0;
  }

  // Create the 1D workspace for the output
  MatrixWorkspace_sptr outputWorkspace;

  outputWorkspace = create<HistoWorkspace>(*localworkspace,
                                           MaxSpec - MinSpec + 1, BinEdges(2));

  Progress progress(this, 0.0, 1.0, (MaxSpec - MinSpec + 1));
  PARALLEL_FOR_IF(Kernel::threadSafe(*localworkspace, *outputWorkspace))
  // Loop over spectra
  for (int i = MinSpec; i <= MaxSpec; ++i) {
    PARALLEL_START_INTERUPT_REGION
    int newindex = i - MinSpec;
    // Copy over spectrum and detector number info
    outputWorkspace->getSpectrum(newindex).copyInfoFrom(
        localworkspace->getSpectrum(i));

    // Retrieve the spectrum into a vector
    auto &X = localworkspace->x(i);
    auto &Y = localworkspace->y(i);

    // Find the range [min,max]
    MantidVec::const_iterator lowit, highit;
    if (MinRange == EMPTY_DBL())
      lowit = X.begin();
    else
      lowit = std::lower_bound(X.begin(), X.end(), MinRange);

    if (MaxRange == EMPTY_DBL())
      highit = X.end();
    else
      highit = std::find_if(lowit, X.end(),
                            std::bind2nd(std::greater<double>(), MaxRange));

    // If range specified doesn't overlap with this spectrum then bail out
    if (lowit == X.end() || highit == X.begin() || lowit == highit)
      continue;

    --highit; // Upper limit is the bin before, i.e. the last value smaller than
              // MaxRange

    MantidVec::difference_type distmin = std::distance(X.begin(), lowit);
    MantidVec::difference_type distmax = std::distance(X.begin(), highit);

    MantidVec::const_iterator maxY;
    // Find the max/min element
    if (showMin) {
      maxY = std::min_element(Y.begin() + distmin, Y.begin() + distmax);
    } else {
      maxY = std::max_element(Y.begin() + distmin, Y.begin() + distmax);
    }
    MantidVec::difference_type d = std::distance(Y.begin(), maxY);
    // X boundaries for the max/min element
    outputWorkspace->mutableX(newindex)[0] = *(X.begin() + d);
    outputWorkspace->mutableX(newindex)[1] =
        *(X.begin() + d + 1); // This is safe since X is of dimension Y+1
    outputWorkspace->mutableY(newindex)[0] = *maxY;
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
