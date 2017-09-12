#include "MantidAlgorithms/CalculatePolynomialBackground.h"

#include "MantidAPI/BasicJacobian.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayOrderedPairsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <utility>

namespace {
/// String constants for algorithm's properties.
namespace Prop {
constexpr char *INPUT_WS = "InputWorkspace";
constexpr char *OUTPUT_WS = "OutputWorkspace";
constexpr char *POLY_DEGREE = "Degree";
constexpr char *XRANGES = "XRanges";
}

/** Filters ranges completely outside the histogram X values.
 *  @param ranges a vector of start-end pairs to filter
 *  @param ws a workspace
 *  @param wsIndex a workspace index to specify a histogram in ws
 *  @return a ranges-like vector with filtered pairs removed
 */
std::vector<double> filterRangesOutsideX(const std::vector<double> &ranges, const Mantid::API::MatrixWorkspace &ws, const size_t wsIndex) {
  const auto minX = ws.x(wsIndex).front();
  const auto maxX = ws.x(wsIndex).back();
  std::vector<double> filtered;
  filtered.reserve(ranges.size());
  for (size_t i = 0; i < ranges.size() / 2; ++i) {
    const auto first = ranges[2 * i];
    const auto second = ranges[2 * i + 1];
    if (!(first > maxX || second < minX)) {
      filtered.emplace_back(first);
      filtered.emplace_back(second);
    }
  }
  return filtered;
}

/** Merges, sorts and limits ranges within totalRange.
 *  @param ranges a vector of start-end pairs to process
 *  @param totalRange a pair of start-end values to limit the output ranges
 *  @return a ranges-like vector of processed ranges
 */
std::vector<double> includedRanges(const std::vector<double> &ranges, const std::pair<double, double> &totalRange) {
  if (ranges.empty()) {
    return {totalRange.first, totalRange.second};
  }
  // Sort the range edges keeping the information whether the edge
  // 'starts' or 'ends' a range.
  enum class Edge { start, end };
  std::vector<std::pair<double, Edge>> edges(ranges.size());
  for (size_t i = 0; i < ranges.size(); ++i) {
    edges[i].first = ranges[i];
    edges[i].second = i % 2 == 0 ? Edge::start : Edge::end;
  }
  std::sort(edges.begin(), edges.end(), [](const std::pair<double, Edge> &p1, const std::pair<double, Edge> &p2) {
    if (p1.first == p2.first)
      return p1.second == Edge::start;
    return p1.first < p2.first;
  });
  // If an 'end' edge is followed by a 'start', we have a new range. Everything else
  // can be merged.
  std::vector<double> mergedRanges;
  mergedRanges.reserve(ranges.size());
  auto edgeIt = edges.begin();
  mergedRanges.emplace_back(std::max(edges.front().first, totalRange.first));
  while (edgeIt != edges.end()) {
    auto endEdgeIt = edgeIt + 1;
    while (endEdgeIt != edges.end()) {
      const auto val = *endEdgeIt;
      const auto prevVal = *(endEdgeIt - 1);
      if (val.second == Edge::start && prevVal.second == Edge::end) {
        mergedRanges.emplace_back(prevVal.first);
        mergedRanges.emplace_back(val.first);
        edgeIt = endEdgeIt;
        break;
      }
      ++endEdgeIt;
    }
    ++edgeIt;
  }
  mergedRanges.emplace_back(std::min(edges.back().first, totalRange.second));
  return mergedRanges;
}

/** Return the gaps between ranges, if any.
 *  @param ranges a vector of start-end pairs to invert
 *  @return a ranges-like vector of gaps between the given ranges.
 */
std::vector<double> invertRanges(const std::vector<double> &ranges) {
  std::vector<double> inversion(ranges.size() - 2);
  for (size_t i = 1; i < ranges.size() - 1; ++i) {
    inversion[i - 1] = ranges[i];
  }
  return inversion;
}

/** Return a Fit algorithm compatible string representing a polynomial.
 *  @param parameters a vector containing the polynomial coefficients
 *  @return a function string
 */
std::string makeFunctionString(const std::vector<double> &parameters) {
  const auto degree = parameters.size() - 1;
  std::ostringstream s;
  switch (degree) {
  case 0:
    s << "name=FlatBackground";
    break;
  case 1:
    s << "name=LinearBackground";
    break;
  case 2:
    s << "name=Quadratic";
    break;
  default:
    s << "name=Polynomial,n=" << degree;
  }
  for (size_t d = 0; d <= degree; ++d) {
    s << ',' << 'A' << d << '=' << parameters[d];
  }
  return s.str();
}

/** Construct the largest range spanning histogram's X values and ranges.
 *  @param ranges a vector of start-end pairs
 *  @param ws a workspace
 *  @param wsIndex a workspace index identifying a histogram
 *  @return a pair of values spanning a range
 */
std::pair<double, double> totalRange(const std::vector<double> &ranges, const Mantid::API::MatrixWorkspace &ws, const size_t wsIndex) {
  const auto minX = ws.x(wsIndex).front();
  const auto maxX = ws.x(wsIndex).back();
  if (ranges.empty()) {
    return std::pair<double, double>(minX, maxX);
  }
  const auto minmaxIt = std::minmax_element(ranges.cbegin(), ranges.cend());
  const auto minEdge = *minmaxIt.first;
  const auto maxEdge = *minmaxIt.second;
  return std::pair<double, double>(std::min(minEdge, minX), std::max(maxEdge, maxX));
}
}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculatePolynomialBackground)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculatePolynomialBackground::name() const { return "CalculatePolynomialBackground"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculatePolynomialBackground::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculatePolynomialBackground::category() const {
  return "CorrectionFunctions\\BackgroundCorrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculatePolynomialBackground::summary() const {
  return "Fits a polynomial background to a workspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculatePolynomialBackground::init() {
  auto increasingAxis = boost::make_shared<API::IncreasingAxisValidator>();
  auto nonnegativeInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  nonnegativeInt->setLower(0);
  auto orderedPairs = boost::make_shared<Kernel::ArrayOrderedPairsValidator<double>>();
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::INPUT_WS, "",
                                                             Kernel::Direction::Input, increasingAxis),
      "An input workspace.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::OUTPUT_WS, "",
                                                             Kernel::Direction::Output),
      "A workspace containing the fitted background.");
  declareProperty(Prop::POLY_DEGREE, 0, nonnegativeInt, "Degree of the fitted polynomial.");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(Prop::XRANGES, std::vector<double>(), orderedPairs), "A list of fitting ranges given as pairs of X values.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePolynomialBackground::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty(Prop::INPUT_WS);

  API::MatrixWorkspace_sptr outWS{DataObjects::create<DataObjects::Workspace2D>(*inWS)};
  const std::vector<double> ranges = getProperty(Prop::XRANGES);
  const auto polyDegree = static_cast<size_t>(static_cast<int>(getProperty(Prop::POLY_DEGREE)));
  const std::vector<double> initialParams(polyDegree + 1, 0.1);
  const auto fitFunction = makeFunctionString(initialParams);
  const auto nHistograms = static_cast<int64_t>(inWS->getNumberHistograms());
  const auto nBins = inWS->blocksize();
  API::Progress progress(this, 0, 1.0, nHistograms);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *outWS))
  for (int64_t i = 0; i < nHistograms; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const auto filteredRanges = filterRangesOutsideX(ranges, *inWS, i);
    if (!ranges.empty() && filteredRanges.empty()) {
      throw std::runtime_error("The given XRanges mismatch with the histogram at workspace index " + std::to_string(i));
    }
    const auto fullRange = totalRange(filteredRanges, *inWS, i);
    const auto includedR = includedRanges(filteredRanges, fullRange);
    const bool logging{false};
    auto fit = createChildAlgorithm("Fit", 0, 0, logging);
    fit->setProperty("Function", fitFunction);
    fit->setProperty("InputWorkspace", inWS);
    fit->setProperty("WorkspaceIndex", static_cast<int>(i));
    fit->setProperty("StartX", includedR.front());
    fit->setProperty("EndX", includedR.back());
    fit->setProperty("Exclude", invertRanges(includedR));
    fit->setProperty("CreateOutput", true);
    fit->executeAsChildAlg();
    API::ITableWorkspace_sptr fitResult = fit->getProperty("OutputParameters");
    std::vector<double> parameters(polyDegree + 1);
    std::vector<double> paramErrors(polyDegree + 1);
    for (size_t row = 0; row < parameters.size(); ++row) {
      parameters[row] = fitResult->cell<double>(row, 1);
      paramErrors[row] = fitResult->cell<double>(row, 2);
    }
    const auto bkgFunction = makeFunctionString(parameters);
    auto bkg = boost::dynamic_pointer_cast<API::IFunction1D>(API::FunctionFactory::Instance().createInitialized(bkgFunction));
    // We want bkg to directly write to the output workspace.
    double *bkgY = const_cast<double *>(outWS->mutableY(i).rawData().data());
    bkg->function1D(bkgY, outWS->points(i).rawData().data(), nBins);
    // Calculate the errors using partial derivatives.
    API::BasicJacobian jacobian{nBins, polyDegree + 1};
    bkg->functionDeriv1D(&jacobian, outWS->points(i).rawData().data(), nBins);
    for (size_t j = 0; j < nBins; ++j) {
      double uncertainty{0.0};
      for (size_t k = 0; k < paramErrors.size(); ++k) {
        uncertainty += std::abs(jacobian.get(j, k)) * paramErrors[k];
      }
      outWS->mutableE(i)[j] = uncertainty;
    }
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty(Prop::OUTPUT_WS, outWS);
}

} // namespace Algorithms
} // namespace Mantid
