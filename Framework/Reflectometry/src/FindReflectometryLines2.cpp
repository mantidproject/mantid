// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/FindReflectometryLines2.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DynamicPointerCastHelper.h"
#include "MantidKernel/Statistics.h"

namespace {
/// String constants for the algorithm's property names
namespace Prop {
std::string const END_INDEX{"EndWorkspaceIndex"};
std::string const INPUT_WS{"InputWorkspace"};
std::string const LINE_CENTRE{"LineCentre"};
std::string const OUTPUT_WS{"OutputWorkspace"};
std::string const RANGE_LOWER{"RangeLower"};
std::string const RANGE_UPPER{"RangeUpper"};
std::string const START_INDEX{"StartWorkspaceIndex"};
} // namespace Prop

/** Set the first bin edge to 0 and last to 1.
 *  @param ws a preferably single bin workspace
 */
void clearIntegrationLimits(Mantid::API::MatrixWorkspace &ws) {
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    auto &Xs = ws.mutableX(i);
    Xs.front() = 0.;
    Xs.back() = 1.;
  }
}

/** Fill the X values of the first histogram of ws with workspace indices.
 *  @param ws a workspace to modify
 */
void convertXToWorkspaceIndex(Mantid::API::MatrixWorkspace &ws) {
  auto &xs = ws.mutableX(0);
  std::iota(xs.begin(), xs.end(), 0.);
}

/** Calculate the median over the first histogram.
 *  @param ws a workspace
 *  @return the median Y over the first histogram
 */
double median(const Mantid::API::MatrixWorkspace &ws) {
  using namespace Mantid::Kernel;
  const auto statistics = getStatistics(ws.y(0).rawData(), StatOptions::Median);
  return statistics.median;
}

/** Create a single value workspace from the input value.
 *  @param x a value to store in the returned workspace
 *  @return a single value workspace
 */
Mantid::API::MatrixWorkspace_sptr makeOutput(double const x) {
  auto ws = std::make_shared<Mantid::DataObjects::WorkspaceSingleValue>(x);
  return std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
}
} // namespace

namespace Mantid::Reflectometry {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindReflectometryLines2)

/// Algorithms name for identification. @see Algorithm::name
const std::string FindReflectometryLines2::name() const { return "FindReflectometryLines"; }

/// Algorithm's version for identification. @see Algorithm::version
int FindReflectometryLines2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FindReflectometryLines2::category() const { return "Reflectometry;ILL\\Reflectometry"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string FindReflectometryLines2::summary() const {
  return "Finds fractional workspace index corresponding to reflected or "
         "direct line in a line detector workspace.";
}

/// Initialize the algorithm's properties.
void FindReflectometryLines2::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::INPUT_WS, "", Kernel::Direction::Input),
      "A reflectometry workspace.");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      Prop::OUTPUT_WS, "", Kernel::Direction::Output, API::PropertyMode::Optional),
                  "A workspace containing the fractional workspace index of "
                  "the line centre.");
  declareProperty(Prop::LINE_CENTRE, EMPTY_DBL(), "The fractional workspace index of the line centre",
                  Kernel::Direction::Output);
  declareProperty(Prop::RANGE_LOWER, EMPTY_DBL(), "The lower peak search limit (an X value).");
  declareProperty(Prop::RANGE_UPPER, EMPTY_DBL(), "The upper peak search limit (an X value).");
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(Prop::START_INDEX, 0, mustBePositive, "Index of the first histogram to include in the peak search.");
  declareProperty(Prop::END_INDEX, EMPTY_INT(), mustBePositive,
                  "Index of the last histogram to include in the peak search.");
}

/// Validate the algorithm's input properties.
std::map<std::string, std::string> FindReflectometryLines2::validateInputs() {
  std::map<std::string, std::string> issues;
  if (!isDefault(Prop::RANGE_LOWER) && !isDefault(Prop::RANGE_UPPER)) {
    double const lower = getProperty(Prop::RANGE_LOWER);
    double const upper = getProperty(Prop::RANGE_UPPER);
    if (lower >= upper) {
      issues[Prop::RANGE_UPPER] = "The upper limit is smaller than the lower.";
    }
  }
  if (!isDefault(Prop::END_INDEX)) {
    int const start = getProperty(Prop::START_INDEX);
    int const end = getProperty(Prop::END_INDEX);
    if (start > end) {
      issues[Prop::END_INDEX] = "The index is smaller than the start.";
    }
  }
  return issues;
}

/// Execute the algorithm.
void FindReflectometryLines2::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty(Prop::INPUT_WS);
  double const peakWSIndex = findPeak(inputWS);
  setProperty(Prop::LINE_CENTRE, peakWSIndex);
  if (!isDefault(Prop::OUTPUT_WS)) {
    auto outputWS = makeOutput(peakWSIndex);
    setProperty(Prop::OUTPUT_WS, std::move(outputWS));
  }
}

/** Gaussian + linear background fit to determine peak position.
 *  @param ws a workspace to fit to
 *  @return fractional workspace index of the peak: Gaussian fit and position
 *  of the maximum
 */
double FindReflectometryLines2::findPeak(const API::MatrixWorkspace_sptr &ws) {
  auto integralWS = integrate(ws);
  // integralWS may be ragged due to different integration limits for each
  // histogram. We don't really care but Transpose does.
  clearIntegrationLimits(*integralWS);
  auto transposedWS = transpose(integralWS);
  // Use median as an initial guess for background
  auto const medianY = median(*transposedWS);
  convertXToWorkspaceIndex(*transposedWS);
  // determine initial height: maximum value
  auto const &Ys = transposedWS->y(0);
  auto const maxValueIt = std::max_element(Ys.cbegin(), Ys.cend());
  double const height = *maxValueIt;
  // determine initial centre: index of the maximum value
  size_t const maxIndex = std::distance(Ys.cbegin(), maxValueIt);
  auto const centreIndex = static_cast<double>(maxIndex);
  int const startIndex = getProperty(Prop::START_INDEX);
  double const centreByMax = static_cast<double>(startIndex) + centreIndex;
  g_log.debug() << "Line maximum position: " << centreByMax << '\n';
  // determine sigma
  auto lessThanHalfMax = [height, medianY](double const x) { return x - medianY < 0.5 * (height - medianY); };
  using IterType = HistogramData::HistogramY::const_iterator;
  std::reverse_iterator<IterType> revMaxValueIt{maxValueIt};
  auto revMinFwhmIt = std::find_if(revMaxValueIt, Ys.crend(), lessThanHalfMax);
  auto maxFwhmIt = std::find_if(maxValueIt, Ys.cend(), lessThanHalfMax);
  std::reverse_iterator<IterType> revMaxFwhmIt{maxFwhmIt};
  if (revMinFwhmIt == Ys.crend() || maxFwhmIt == Ys.cend()) {
    g_log.warning() << "Couldn't determine fwhm of line, using position of max "
                       "value as line center.\n";
    return centreByMax;
  }
  auto const fwhm = static_cast<double>(std::distance(revMaxFwhmIt, revMinFwhmIt) + 1);
  g_log.debug() << "Initial fwhm (full width at half maximum): " << fwhm << '\n';
  auto func = API::FunctionFactory::Instance().createFunction("CompositeFunction");
  auto sum =
      Kernel::DynamicPointerCastHelper::dynamicPointerCastWithCheck<API::CompositeFunction, API::IFunction>(func);
  func = API::FunctionFactory::Instance().createFunction("Gaussian");
  auto gaussian =
      Kernel::DynamicPointerCastHelper::dynamicPointerCastWithCheck<API::IPeakFunction, API::IFunction>(func);
  gaussian->setHeight(height);
  gaussian->setCentre(centreIndex);
  gaussian->setFwhm(fwhm);
  sum->addFunction(gaussian);
  func = API::FunctionFactory::Instance().createFunction("LinearBackground");
  func->setParameter("A0", medianY);
  func->setParameter("A1", 0.);
  sum->addFunction(std::move(func));
  // call Fit child algorithm
  auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(sum));
  fit->setProperty("InputWorkspace", transposedWS);
  fit->setProperty("StartX", centreIndex - 3 * fwhm);
  fit->setProperty("EndX", centreIndex + 3 * fwhm);
  fit->execute();
  std::string const fitStatus = fit->getProperty("OutputStatus");
  if (fitStatus != "success") {
    g_log.warning("Fit not successful, using position of max value.\n");
    return centreByMax;
  }
  auto const centreByFit = gaussian->centre() + static_cast<double>(startIndex);
  g_log.debug() << "Sigma: " << gaussian->fwhm() << '\n';
  g_log.debug() << "Estimated line position: " << centreByFit << '\n';
  return centreByFit;
}

/** Integrate a workspace.
 *  @param ws a workspace to integrate
 *  @return a workspace containing the integrals
 */
API::MatrixWorkspace_sptr FindReflectometryLines2::integrate(const API::MatrixWorkspace_sptr &ws) {
  int const startIndex = getProperty(Prop::START_INDEX);
  int const endIndex = getProperty(Prop::END_INDEX);
  double const startX = getProperty(Prop::RANGE_LOWER);
  double const endX = getProperty(Prop::RANGE_UPPER);
  auto integration = createChildAlgorithm("Integration");
  integration->initialize();
  integration->setProperty("InputWorkspace", ws);
  integration->setProperty("OutputWorkspace", "__unused_for_child");
  integration->setProperty("RangeLower", startX);
  integration->setProperty("RangeUpper", endX);
  integration->setProperty("StartWorkspaceIndex", startIndex);
  integration->setProperty("EndWorkspaceIndex", endIndex);
  integration->execute();
  API::MatrixWorkspace_sptr integralWS = integration->getProperty("OutputWorkspace");
  return integralWS;
}

/** Transpose a workspace.
 *  @param ws a workspace to transpos
 *  @return a transposed workspace
 */
API::MatrixWorkspace_sptr FindReflectometryLines2::transpose(const API::MatrixWorkspace_sptr &ws) {
  auto transpose = createChildAlgorithm("Transpose");
  transpose->initialize();
  transpose->setProperty("InputWorkspace", ws);
  transpose->setProperty("OutputWorkspace", "__unused_for_child");
  transpose->execute();
  API::MatrixWorkspace_sptr transposedWS = transpose->getProperty("OutputWorkspace");
  return transposedWS;
}

} // namespace Mantid::Reflectometry
