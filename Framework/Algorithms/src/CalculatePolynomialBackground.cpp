// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculatePolynomialBackground.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayOrderedPairsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include <utility>

namespace {
/// String constants for algorithm's properties.
namespace Prop {
const static std::string COST_FUNCTION = "CostFunction";
const static std::string INPUT_WS = "InputWorkspace";
const static std::string OUTPUT_WS = "OutputWorkspace";
const static std::string POLY_DEGREE = "Degree";
const static std::string XRANGES = "XRanges";
} // namespace Prop

/// String constants for cost function options.
namespace CostFunc {
const static std::string UNWEIGHTED_LEAST_SQUARES = "Unweighted least squares";
const static std::string WEIGHTED_LEAST_SQUARES = "Least squares";
} // namespace CostFunc

/** Filters ranges completely outside the histogram X values.
 *  @param ranges a vector of start-end pairs to filter
 *  @param ws a workspace
 *  @param wsIndex a workspace index to specify a histogram in ws
 *  @return a ranges-like vector with filtered pairs removed
 */
std::vector<double> filterRangesOutsideX(const std::vector<double> &ranges,
                                         const Mantid::API::MatrixWorkspace &ws,
                                         const size_t wsIndex) {
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

/** Construct the largest range spanning histogram's X values and ranges.
 *  @param ranges a vector of start-end pairs
 *  @param ws a workspace
 *  @param wsIndex a workspace index identifying a histogram
 *  @return a pair of values spanning a range
 */
std::pair<double, double> totalRange(const std::vector<double> &ranges,
                                     const Mantid::API::MatrixWorkspace &ws,
                                     const size_t wsIndex) {
  const auto minX = ws.x(wsIndex).front();
  const auto maxX = ws.x(wsIndex).back();
  if (ranges.empty()) {
    return std::pair<double, double>(minX, maxX);
  }
  const auto minmaxIt = std::minmax_element(ranges.cbegin(), ranges.cend());
  const auto minEdge = *minmaxIt.first;
  const auto maxEdge = *minmaxIt.second;
  return std::pair<double, double>(std::min(minEdge, minX),
                                   std::max(maxEdge, maxX));
}

/** Merges, sorts and limits ranges within totalRange.
 *  @param ranges a vector of start-end pairs to process
 *  @param totalRange a pair of start-end values to limit the output ranges
 *  @return a ranges-like vector of processed ranges
 */
std::vector<double>
includedRanges(const std::vector<double> &ranges,
               const std::pair<double, double> &totalRange) {
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
  std::sort(
      edges.begin(), edges.end(),
      [](const std::pair<double, Edge> &p1, const std::pair<double, Edge> &p2) {
        if (p1.first == p2.first)
          return p1.second == Edge::start;
        return p1.first < p2.first;
      });
  // If an 'end' edge is followed by a 'start', we have a new range. Everything
  // else can be merged.
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

/** Constrains given ranges within a histogram.
 *  @param ranges a vector of start-end pairs to process
 *  @param ws a workspace
 *  @param wsIndex a workspace index identifying a histogram in ws
 *  @return a ranges-like vector of processed ranges
 */
std::vector<double> histogramRanges(const std::vector<double> &ranges,
                                    const Mantid::API::MatrixWorkspace &ws,
                                    const size_t wsIndex) {
  const auto filteredRanges = filterRangesOutsideX(ranges, ws, wsIndex);
  if (!ranges.empty() && filteredRanges.empty()) {
    throw std::runtime_error(
        "The given XRanges mismatch with the histogram at workspace index " +
        std::to_string(wsIndex));
  }
  const auto fullRange = totalRange(filteredRanges, ws, wsIndex);
  return includedRanges(filteredRanges, fullRange);
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

/** Executes the given algorithm returning the fitted parameters.
 *  @param fit the Fit algorithm
 *  @param function a string representing the function to fit
 *  @param ws a workspace to fit to
 *  @param wsIndex a workspace index identifying the histogram to fit to
 *  @param ranges a vector defining the fitting intervals
 *  @return a vector of final fitted parameters
 */
std::vector<double>
executeFit(Mantid::API::Algorithm &fit, const std::string &function,
           Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex,
           const std::vector<double> &ranges, const std::string &costFunction) {
  const auto fitRanges = histogramRanges(ranges, *ws, wsIndex);
  const auto excludedRanges = invertRanges(fitRanges);
  fit.setProperty("Function", function);
  fit.setProperty("InputWorkspace", ws);
  fit.setProperty("WorkspaceIndex", static_cast<int>(wsIndex));
  fit.setProperty("StartX", fitRanges.front());
  fit.setProperty("EndX", fitRanges.back());
  fit.setProperty("Exclude", excludedRanges);
  fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
  fit.setProperty(Prop::COST_FUNCTION, costFunction);
  fit.setProperty("CreateOutput", true);
  fit.executeAsChildAlg();
  Mantid::API::ITableWorkspace_sptr fitResult =
      fit.getProperty("OutputParameters");
  std::vector<double> parameters(fitResult->rowCount() - 1);
  for (size_t row = 0; row < parameters.size(); ++row) {
    parameters[row] = fitResult->cell<double>(row, 1);
  }
  return parameters;
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

/** Evaluates the given function directly on a histogram
 *  @param function a string representing function to evaluate
 *  @param ws an output workspace
 *  @param wsIndex a workspace index identifying a histogram
 */
void evaluateInPlace(const std::string &function,
                     Mantid::API::MatrixWorkspace &ws, const size_t wsIndex) {
  auto bkg = boost::dynamic_pointer_cast<Mantid::API::IFunction1D>(
      Mantid::API::FunctionFactory::Instance().createInitialized(function));
  // We want to write directly to the workspace.
  double *y = const_cast<double *>(ws.mutableY(wsIndex).rawData().data());
  bkg->function1D(y, ws.points(wsIndex).rawData().data(), ws.y(wsIndex).size());
}
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculatePolynomialBackground)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculatePolynomialBackground::name() const {
  return "CalculatePolynomialBackground";
}

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
  auto orderedPairs =
      boost::make_shared<Kernel::ArrayOrderedPairsValidator<double>>();
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::INPUT_WS, "", Kernel::Direction::Input, increasingAxis),
      "An input workspace.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::OUTPUT_WS, "", Kernel::Direction::Output),
      "A workspace containing the fitted background.");
  declareProperty(Prop::POLY_DEGREE, 0, nonnegativeInt,
                  "Degree of the fitted polynomial.");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      Prop::XRANGES, std::vector<double>(), orderedPairs),
                  "A list of fitting ranges given as pairs of X values.");
  std::array<std::string, 2> costFuncOpts{
      {CostFunc::WEIGHTED_LEAST_SQUARES, CostFunc::UNWEIGHTED_LEAST_SQUARES}};
  declareProperty(
      Prop::COST_FUNCTION, CostFunc::WEIGHTED_LEAST_SQUARES.c_str(),
      boost::make_shared<Kernel::ListValidator<std::string>>(costFuncOpts),
      "The cost function to be passed to the Fit algorithm.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePolynomialBackground::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty(Prop::INPUT_WS);

  API::MatrixWorkspace_sptr outWS{
      DataObjects::create<DataObjects::Workspace2D>(*inWS)};
  const std::vector<double> inputRanges = getProperty(Prop::XRANGES);
  const std::string costFunction = getProperty(Prop::COST_FUNCTION);
  const auto polyDegree =
      static_cast<size_t>(static_cast<int>(getProperty(Prop::POLY_DEGREE)));
  const std::vector<double> initialParams(polyDegree + 1, 0.1);
  const auto fitFunction = makeFunctionString(initialParams);
  const auto nHistograms = static_cast<int64_t>(inWS->getNumberHistograms());
  API::Progress progress(this, 0, 1.0, nHistograms);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *outWS))
  for (int64_t i = 0; i < nHistograms; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const bool logging{false};
    auto fit = createChildAlgorithm("Fit", 0, 0, logging);
    const auto parameters =
        executeFit(*fit, fitFunction, inWS, i, inputRanges, costFunction);
    const auto bkgFunction = makeFunctionString(parameters);
    evaluateInPlace(bkgFunction, *outWS, i);
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty(Prop::OUTPUT_WS, outWS);
}

} // namespace Algorithms
} // namespace Mantid
