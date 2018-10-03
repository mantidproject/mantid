// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Integration.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VectorHelper.h"

#include <cmath>
#include <numeric>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Integration)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/** Initialisation method.
 *
 */
void Integration::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The input workspace to integrate.");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The output workspace with the results of the integration.");

  declareProperty("RangeLower", EMPTY_DBL(),
                  "The lower integration limit (an X value).");
  declareProperty("RangeUpper", EMPTY_DBL(),
                  "The upper integration limit (an X value).");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "Index of the first spectrum to integrate.");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "Index of the last spectrum to integrate.");
  declareProperty("IncludePartialBins", false,
                  "If true then partial bins from the beginning and end of the "
                  "input range are also included in the integration.");
  declareProperty(make_unique<ArrayProperty<double>>("RangeLowerList"),
                  "A list of lower integration limits (as X values).");
  declareProperty(make_unique<ArrayProperty<double>>("RangeUpperList"),
                  "A list of upper integration limits (as X values).");
}

/**
 * Std-style comparision function object (satisfies the requirements of Compare)
 * @return true if first argument < second argument (with some
 * tolerance/epsilon)
 */
struct tolerant_less : public std::binary_function<double, double, bool> {
public:
  bool operator()(const double &left, const double &right) const {
    // soft equal, if the diff left-right is below a numerical error
    // (uncertainty) threshold, we cannot say
    return (left < right) && (std::abs(left - right) >
                              1 * std::numeric_limits<double>::epsilon());
  }
};

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Integration::exec() {
  // Try and retrieve the optional properties

  /// The value in X to start the integration from
  double minRange = getProperty("RangeLower");
  /// The value in X to finish the integration at
  double maxRange = getProperty("RangeUpper");
  /// The spectrum to start the integration from
  int minWsIndex = getProperty("StartWorkspaceIndex");
  /// The spectrum to finish the integration at
  int maxWsIndex = getProperty("EndWorkspaceIndex");
  /// Flag for including partial bins
  const bool incPartBins = getProperty("IncludePartialBins");
  /// List of X values to start the integration from
  const std::vector<double> minRanges = getProperty("RangeLowerList");
  /// List of X values to finish the integration at
  const std::vector<double> maxRanges = getProperty("RangeUpperList");

  // Get the input workspace
  MatrixWorkspace_sptr localworkspace = this->getInputWorkspace();

  const int numberOfSpectra =
      static_cast<int>(localworkspace->getNumberHistograms());

  // Check 'StartWorkspaceIndex' is in range 0-numberOfSpectra
  if (minWsIndex >= numberOfSpectra) {
    g_log.warning("StartWorkspaceIndex out of range! Set to 0.");
    minWsIndex = 0;
  }
  if (isEmpty(maxWsIndex))
    maxWsIndex = numberOfSpectra - 1;
  if (maxWsIndex > numberOfSpectra - 1 || maxWsIndex < minWsIndex) {
    g_log.warning(
        "EndWorkspaceIndex out of range! Set to max workspace index.");
    maxWsIndex = numberOfSpectra - 1;
  }
  auto rangeListCheck = [minWsIndex, maxWsIndex](
                            const std::vector<double> &list, const char *name) {
    if (!list.empty() &&
        list.size() != static_cast<size_t>(maxWsIndex - minWsIndex) + 1) {
      std::ostringstream sout;
      sout << name << " has " << list.size() << " values but it should contain "
           << maxWsIndex - minWsIndex + 1 << '\n';
      throw std::runtime_error(sout.str());
    }
  };
  rangeListCheck(minRanges, "RangeLowerList");
  rangeListCheck(maxRanges, "RangeUpperList");

  double progressStart = 0.0;
  //---------------------------------------------------------------------------------
  // Now, determine if the input workspace is actually an EventWorkspace
  EventWorkspace_sptr eventInputWS =
      boost::dynamic_pointer_cast<EventWorkspace>(localworkspace);

  if (eventInputWS != nullptr) {
    //------- EventWorkspace as input -------------------------------------
    if (!minRanges.empty() || !maxRanges.empty()) {
      throw std::runtime_error(
          "Range lists not supported for EventWorkspaces.");
    }
    // Get the eventworkspace rebinned to apply the upper and lowerrange
    double evntMinRange =
        isEmpty(minRange) ? eventInputWS->getEventXMin() : minRange;
    double evntMaxRange =
        isEmpty(maxRange) ? eventInputWS->getEventXMax() : maxRange;
    localworkspace =
        rangeFilterEventWorkspace(eventInputWS, evntMinRange, evntMaxRange);

    progressStart = 0.5;
  }
  if (isEmpty(minRange)) {
    minRange = std::numeric_limits<double>::lowest();
  }

  // Create the 2D workspace (with 1 bin) for the output
  MatrixWorkspace_sptr outputWorkspace =
      API::WorkspaceFactory::Instance().create(
          localworkspace, maxWsIndex - minWsIndex + 1, 2, 1);
  auto rebinned_input =
      boost::dynamic_pointer_cast<const RebinnedOutput>(localworkspace);
  auto rebinned_output =
      boost::dynamic_pointer_cast<RebinnedOutput>(outputWorkspace);

  bool is_distrib = outputWorkspace->isDistribution();
  Progress progress(this, progressStart, 1.0, maxWsIndex - minWsIndex + 1);

  const bool axisIsText = localworkspace->getAxis(1)->isText();
  const bool axisIsNumeric = localworkspace->getAxis(1)->isNumeric();

  // Loop over spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*localworkspace, *outputWorkspace))
  for (int i = minWsIndex; i <= maxWsIndex; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Workspace index on the output
    const int outWI = i - minWsIndex;

    // Copy Axis values from previous workspace
    if (axisIsText) {
      TextAxis *newAxis = dynamic_cast<TextAxis *>(outputWorkspace->getAxis(1));
      if (newAxis)
        newAxis->setLabel(outWI, localworkspace->getAxis(1)->label(i));
    } else if (axisIsNumeric) {
      NumericAxis *newAxis =
          dynamic_cast<NumericAxis *>(outputWorkspace->getAxis(1));
      if (newAxis)
        newAxis->setValue(outWI, (*(localworkspace->getAxis(1)))(i));
    }

    // This is the output
    auto &outSpec = outputWorkspace->getSpectrum(outWI);
    // This is the input
    const auto &inSpec = localworkspace->getSpectrum(i);

    // Copy spectrum number, detector IDs
    outSpec.copyInfoFrom(inSpec);

    // Retrieve the spectrum into a vector (Histogram)
    const auto &X = inSpec.x();
    const auto &Y = inSpec.y();
    const auto &E = inSpec.e();

    // Find the range [min,max]
    MantidVec::const_iterator lowit, highit;
    const double lowerLimit =
        minRanges.empty() ? minRange : std::max(minRange, minRanges[outWI]);
    const double upperLimit =
        maxRanges.empty() ? maxRange : std::min(maxRange, maxRanges[outWI]);

    // If doing partial bins, we want to set the bin boundaries to the specified
    // values regardless of whether they're 'in range' for this spectrum
    // Have to do this here, ahead of the 'continue' a bit down from here.
    if (incPartBins) {
      outSpec.dataX()[0] = lowerLimit;
      outSpec.dataX()[1] = upperLimit;
    }

    if (upperLimit < lowerLimit) {
      std::ostringstream sout;
      sout << "Upper integration limit " << upperLimit
           << " for workspace index " << i << " smaller than the lower limit "
           << lowerLimit << ". Setting integral to zero.\n";
      g_log.warning() << sout.str();
      progress.report();
      continue;
    }
    if (lowerLimit == EMPTY_DBL()) {
      lowit = X.begin();
    } else {
      lowit = std::lower_bound(X.begin(), X.end(), lowerLimit, tolerant_less());
    }

    if (upperLimit == EMPTY_DBL()) {
      highit = X.end();
    } else {
      highit = std::upper_bound(lowit, X.end(), upperLimit, tolerant_less());
    }

    // If range specified doesn't overlap with this spectrum then bail out
    if (lowit == X.end() || highit == X.begin())
      continue;

    // Upper limit is the bin before, i.e. the last value smaller than MaxRange
    --highit; // (note: decrementing 'end()' is safe for vectors, at least
              // according to the C++ standard)

    auto distmin = std::distance(X.begin(), lowit);
    auto distmax = std::distance(X.begin(), highit);

    double sumY = 0.0;
    double sumE = 0.0;
    double sumF = 0.0;
    double Fmin = 0.0;
    double Fmax = 0.0;
    if (distmax <= distmin) {
      sumY = 0.;
      sumE = 0.;
    } else {
      if (rebinned_input) {
        // Workspace has fractional area information, need to take into account
        const MantidVec &F = rebinned_input->readF(i);
        sumF = std::accumulate(F.begin() + distmin, F.begin() + distmax, 0.0);
        if (distmin > 0)
          Fmin = F[distmin - 1];
        Fmax = F[distmax];
      }
      if (!is_distrib) {
        // Sum the Y, and sum the E in quadrature
        {
          sumY = std::accumulate(Y.begin() + distmin, Y.begin() + distmax, 0.0);
          sumE = std::accumulate(E.begin() + distmin, E.begin() + distmax, 0.0,
                                 VectorHelper::SumSquares<double>());
        }
      } else {
        // Sum Y*binwidth and Sum the (E*binwidth)^2.
        std::vector<double> widths(X.size());
        // highit+1 is safe while input workspace guaranteed to be histogram
        std::adjacent_difference(lowit, highit + 1, widths.begin());
        sumY = std::inner_product(Y.begin() + distmin, Y.begin() + distmax,
                                  widths.begin() + 1, 0.0);
        sumE = std::inner_product(E.begin() + distmin, E.begin() + distmax,
                                  widths.begin() + 1, 0.0, std::plus<double>(),
                                  VectorHelper::TimesSquares<double>());
      }
    }
    // If partial bins are included, set integration range to exact range
    // given and add on contributions from partial bins either side of range.
    if (incPartBins) {
      if (distmin > 0) {
        const double lower_bin = *lowit;
        const double prev_bin = *(lowit - 1);
        double fraction = (lower_bin - lowerLimit);
        if (!is_distrib) {
          fraction /= (lower_bin - prev_bin);
        }
        const MantidVec::size_type val_index = distmin - 1;
        sumY += Y[val_index] * fraction;
        const double eval = E[val_index];
        sumE += eval * eval * fraction * fraction;
        if (rebinned_input) {
          sumF += Fmin * fraction;
        }
      }
      if (highit < X.end() - 1) {
        const double upper_bin = *highit;
        const double next_bin = *(highit + 1);
        double fraction = (upperLimit - upper_bin);
        if (!is_distrib) {
          fraction /= (next_bin - upper_bin);
        }
        sumY += Y[distmax] * fraction;
        const double eval = E[distmax];
        sumE += eval * eval * fraction * fraction;
        if (rebinned_input) {
          sumF += Fmax * fraction;
        }
      }
    } else {
      outSpec.mutableX()[0] = lowit == X.end() ? *(lowit - 1) : *(lowit);
      outSpec.mutableX()[1] = *highit;
    }

    outSpec.mutableY()[0] = sumY;
    outSpec.mutableE()[0] = sqrt(sumE); // Propagate Gaussian error
    if (rebinned_output) {
      rebinned_output->dataF(outWI)[0] = sumF;
    }

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (rebinned_output) {
    rebinned_output->finalize(false);
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWorkspace);
}

/**
 * Uses rebin to reduce event workspaces to a single bin histogram
 */
API::MatrixWorkspace_sptr
Integration::rangeFilterEventWorkspace(API::MatrixWorkspace_sptr workspace,
                                       double minRange, double maxRange) {
  bool childLog = g_log.is(Logger::Priority::PRIO_DEBUG);
  auto childAlg = createChildAlgorithm("Rebin", 0, 0.5, childLog);
  childAlg->setProperty("InputWorkspace", workspace);
  std::ostringstream binParams;
  binParams << minRange << "," << maxRange - minRange << "," << maxRange;
  childAlg->setPropertyValue("Params", binParams.str());
  childAlg->setProperty("PreserveEvents", false);
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

/**
 * This function gets the input workspace. In the case for a RebinnedOutput
 * workspace, it must be cleaned before proceeding. Other workspaces are
 * untouched.
 * @return the input workspace, cleaned if necessary
 */
MatrixWorkspace_sptr Integration::getInputWorkspace() {
  MatrixWorkspace_sptr temp = getProperty("InputWorkspace");

  if (temp->id() == "RebinnedOutput") {
    // Clean the input workspace in the RebinnedOutput case for nan's and
    // inf's in order to treat the data correctly later.
    IAlgorithm_sptr alg = this->createChildAlgorithm("ReplaceSpecialValues");
    alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", temp);
    std::string outName = "_" + temp->getName() + "_clean";
    alg->setProperty("OutputWorkspace", outName);
    alg->setProperty("NaNValue", 0.0);
    alg->setProperty("NaNError", 0.0);
    alg->setProperty("InfinityValue", 0.0);
    alg->setProperty("InfinityError", 0.0);
    alg->executeAsChildAlg();
    temp = alg->getProperty("OutputWorkspace");
    // Now if the workspace is "finalized" need to undo this before integrating
    boost::dynamic_pointer_cast<RebinnedOutput>(temp)->unfinalize();
  }

  // To integrate point data it will be converted to histograms
  if (!temp->isHistogramData()) {
    auto alg = this->createChildAlgorithm("ConvertToHistogram");
    alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", temp);
    std::string outName = "_" + temp->getName() + "_histogram";
    alg->setProperty("OutputWorkspace", outName);
    alg->executeAsChildAlg();
    temp = alg->getProperty("OutputWorkspace");
    temp->setDistribution(true);
  }

  return temp;
}

std::map<std::string, std::string> Integration::validateInputs() {
  std::map<std::string, std::string> issues;
  const double minRange = getProperty("RangeLower");
  const double maxRange = getProperty("RangeUpper");
  const std::vector<double> minRanges = getProperty("RangeLowerList");
  const std::vector<double> maxRanges = getProperty("RangeUpperList");
  if (!minRanges.empty() && !maxRanges.empty() &&
      minRanges.size() != maxRanges.size()) {
    issues["RangeLowerList"] =
        "RangeLowerList has different number of values as RangeUpperList.";
    return issues;
  }
  for (size_t i = 0; i < minRanges.size(); ++i) {
    const auto x = minRanges[i];
    if (!isEmpty(maxRange) && x > maxRange) {
      issues["RangeLowerList"] =
          "RangeLowerList has a value greater than RangeUpper.";
      break;
    } else if (!maxRanges.empty() && x > maxRanges[i]) {
      issues["RangeLowerList"] = "RangeLowerList has a value greater than the "
                                 "corresponding one in RangeUpperList.";
      break;
    }
  }
  if (!isEmpty(minRange)) {
    for (const auto x : maxRanges) {
      if (x < minRange) {
        issues["RangeUpperList"] =
            "RangeUpperList has a value lower than RangeLower.";
        break;
      }
    }
  }
  return issues;
}

} // namespace Algorithms
} // namespace Mantid
