// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
const static std::string MINIMIZER = "Minimizer";
} // namespace Prop

/// String constants for cost function options.
namespace CostFunc {
const static std::string UNWEIGHTED_LEAST_SQUARES = "Unweighted least squares";
const static std::string WEIGHTED_LEAST_SQUARES = "Least squares";
} // namespace CostFunc

/// String constants for minimizer options.
namespace Minimizer {
const static std::string LEVENBERG_MARQUARDT_MD = "Levenberg-MarquardtMD";
const static std::string LEVENBERG_MARQUARDT = "Levenberg-Marquardt";
} // namespace Minimizer

/** Filters ranges completely outside the histogram X values.
 *  @param ranges a vector of start-end pairs to filter
 *  @param ws a workspace
 *  @param wsIndex a workspace index to specify a histogram in ws
 *  @return a ranges-like vector with filtered pairs removed
 */
std::vector<double> filterRangesOutsideX(const std::vector<double> &ranges, const Mantid::API::MatrixWorkspace &ws,
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
std::pair<double, double> totalRange(const std::vector<double> &ranges, const Mantid::API::MatrixWorkspace &ws,
                                     const size_t wsIndex) {
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
  enum class Edge { start = -1, end = 1 };
  std::vector<std::pair<double, Edge>> edges(ranges.size());
  for (size_t i = 0; i < ranges.size(); ++i) {
    edges[i].first = ranges[i];
    edges[i].second = i % 2 == 0 ? Edge::start : Edge::end;
  }
  std::sort(edges.begin(), edges.end(), [](const std::pair<double, Edge> &p1, const std::pair<double, Edge> &p2) {
    if (p1.first == p2.first)
      return p1.second < p2.second;
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
std::vector<double> histogramRanges(const std::vector<double> &ranges, const Mantid::API::MatrixWorkspace &ws,
                                    const size_t wsIndex) {
  const auto filteredRanges = filterRangesOutsideX(ranges, ws, wsIndex);
  if (!ranges.empty() && filteredRanges.empty()) {
    throw std::runtime_error("The given XRanges mismatch with the histogram at workspace index " +
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
 *  @param costFunction a string representing the cost function used for the fit
 *  @param minimizer a string representing the minimizer used for the fitting
 *  @return a vector of final fitted parameters
 */
std::vector<double> executeFit(Mantid::API::Algorithm &fit, const std::string &function,
                               Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex,
                               const std::vector<double> &ranges, const std::string &costFunction,
                               const std::string &minimizer) {
  const auto fitRanges = histogramRanges(ranges, *ws, wsIndex);
  const auto excludedRanges = invertRanges(fitRanges);
  fit.setProperty("Function", function);
  fit.setProperty("InputWorkspace", ws);
  fit.setProperty("WorkspaceIndex", static_cast<int>(wsIndex));
  fit.setProperty("StartX", fitRanges.front());
  fit.setProperty("EndX", fitRanges.back());
  fit.setProperty("Exclude", excludedRanges);
  fit.setProperty("Minimizer", minimizer);
  fit.setProperty(Prop::COST_FUNCTION, costFunction);
  fit.setProperty("CreateOutput", true);
  fit.executeAsChildAlg();
  Mantid::API::ITableWorkspace_sptr fitResult = fit.getProperty("OutputParameters");
  std::vector<double> parameters(fitResult->rowCount() - 1);
  for (size_t row = 0; row < parameters.size(); ++row) {
    parameters[row] = fitResult->cell<double>(row, 1);
  }
  return parameters;
}

/** Return a Fit algorithm compatible string representing a polynomial.
 *  @param name a string respresenting the name of the polynomial
 *  @param parameters a vector containing the polynomial coefficients
 *  @return a function string
 */
std::string makeFunctionString(const std::string &name, const std::vector<double> &parameters) {
  const auto degree = parameters.size() - 1;
  std::ostringstream s;
  s << "name=" << name;
  if (degree > 2)
    s << ",n=" << degree;
  for (size_t d = 0; d <= degree; ++d) {
    s << ',' << 'A' << d << '=' << parameters[d];
  }
  return s.str();
}

/** Return a name of the function used in the fit.
 *  @param degree an integer representing the degree of the polynomial
 *  @return a string containing the name of the polynomial
 */
std::string makeNameString(const size_t degree) {
  std::ostringstream name;
  switch (degree) {
  case 0:
    name << "FlatBackground";
    break;
  case 1:
    name << "LinearBackground";
    break;
  case 2:
    name << "Quadratic";
    break;
  default:
    name << "Polynomial";
  }
  return name.str();
}

/** Evaluates the given function directly on a histogram
 *  @param name a string representing the name of the polynomial to evaluate
 *  @param parameters a vector containing the coefficients of the polynomial
 *  @param ws an output workspace
 *  @param wsIndex a workspace index identifying a histogram
 */
void evaluateInPlace(const std::string &name, const std::vector<double> &parameters, Mantid::API::MatrixWorkspace &ws,
                     const size_t wsIndex) {
  const auto degree = parameters.size() - 1;
  auto bkg = std::dynamic_pointer_cast<Mantid::API::IFunction1D>(
      Mantid::API::FunctionFactory::Instance().createFunction(name));
  if (degree > 2) {
    Mantid::API::IFunction1D::Attribute att = bkg->getAttribute("n");
    att.fromString(std::to_string(degree));
    bkg->setAttribute("n", att);
  }
  for (size_t d = 0; d <= degree; ++d) {
    std::string param = 'A' + std::to_string(d);
    bkg->setParameter(param, parameters[d]);
  }
  auto *y = const_cast<double *>(ws.mutableY(wsIndex).rawData().data());
  bkg->function1D(y, ws.points(wsIndex).rawData().data(), ws.y(wsIndex).size());
}
} // namespace

namespace Mantid::Algorithms {

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
  auto increasingAxis = std::make_shared<API::IncreasingAxisValidator>();
  auto nonnegativeInt = std::make_shared<Kernel::BoundedValidator<int>>();
  nonnegativeInt->setLower(0);
  auto orderedPairs = std::make_shared<Kernel::ArrayOrderedPairsValidator<double>>();
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      Prop::INPUT_WS, "", Kernel::Direction::Input, increasingAxis),
                  "An input workspace.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::OUTPUT_WS, "", Kernel::Direction::Output),
      "A workspace containing the fitted background.");
  declareProperty(Prop::POLY_DEGREE, 0, nonnegativeInt, "Degree of the fitted polynomial.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>(Prop::XRANGES, std::vector<double>(), orderedPairs),
                  "A list of fitting ranges given as pairs of X values.");
  std::array<std::string, 2> costFuncOpts{{CostFunc::WEIGHTED_LEAST_SQUARES, CostFunc::UNWEIGHTED_LEAST_SQUARES}};
  declareProperty(Prop::COST_FUNCTION, CostFunc::WEIGHTED_LEAST_SQUARES,
                  std::make_shared<Kernel::ListValidator<std::string>>(costFuncOpts),
                  "The cost function to be passed to the Fit algorithm.");
  std::array<std::string, 2> minimizerOpts{{Minimizer::LEVENBERG_MARQUARDT_MD, Minimizer::LEVENBERG_MARQUARDT}};
  declareProperty(Prop::MINIMIZER, Minimizer::LEVENBERG_MARQUARDT_MD,
                  std::make_shared<Kernel::ListValidator<std::string>>(minimizerOpts),
                  "The minimizer to be passed to the Fit algorithm.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePolynomialBackground::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty(Prop::INPUT_WS);

  API::MatrixWorkspace_sptr outWS{DataObjects::create<DataObjects::Workspace2D>(*inWS)};
  const std::vector<double> inputRanges = getProperty(Prop::XRANGES);
  const std::string costFunction = getProperty(Prop::COST_FUNCTION);
  const std::string minimizer = getProperty(Prop::MINIMIZER);
  const auto polyDegree = static_cast<size_t>(static_cast<int>(getProperty(Prop::POLY_DEGREE)));
  const std::vector<double> initialParams(polyDegree + 1, 0.1);
  const auto polyDegreeStr = makeNameString(polyDegree);
  const auto fitFunction = makeFunctionString(polyDegreeStr, initialParams);
  const auto nHistograms = static_cast<int64_t>(inWS->getNumberHistograms());
  API::Progress progress(this, 0, 1.0, nHistograms);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *outWS))
  for (int64_t i = 0; i < nHistograms; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    const bool logging{false};
    auto fit = createChildAlgorithm("Fit", 0, 0, logging);
    const auto parameters = executeFit(*fit, fitFunction, inWS, i, inputRanges, costFunction, minimizer);
    evaluateInPlace(polyDegreeStr, parameters, *outWS, i);
    progress.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  setProperty(Prop::OUTPUT_WS, outWS);
}

} // namespace Mantid::Algorithms
