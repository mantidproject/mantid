#include "MantidAlgorithms/LineProfile.h"

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Unit.h"

#include <algorithm>
#include "boost/make_shared.hpp"

namespace Mantid {
namespace Algorithms {

using Mantid::API::Axis;
using Mantid::API::BinEdgeAxis;
using Mantid::API::CommonBinsValidator;
using Mantid::API::IncreasingAxisValidator;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::create;
using Mantid::DataObjects::Workspace2D;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::Points;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::ListValidator;
using Mantid::Kernel::make_unique;
using Mantid::Kernel::MandatoryValidator;
using Mantid::Kernel::Unit;

namespace {
/// An enum specifying a line profile orientation.
enum class LineDirection { horizontal, vertical };

/// A private namespace for the options for the Direction property.
namespace DirectionChoices {
const static std::string HORIZONTAL{"Horizontal"};
const static std::string VERTICAL{"Vertical"};
}

/// A private namespace for the mode options.
namespace ModeChoices {
const static std::string AVERAGE{"Average"};
const static std::string SUM{"Sum"};
const static std::string WEIGHED_SUM{"Weighed Sum"};
}

/// A private namespace for property names.
namespace PropertyNames {
const static std::string CENTRE{"Centre"};
const static std::string DIRECTION{"Direction"};
const static std::string END{"End"};
const static std::string INPUT_WORKSPACE{"InputWorkspace"};
const static std::string HALF_WIDTH{"HalfWidth"};
const static std::string IGNORE_INFS{"IgnoreInfs"};
const static std::string IGNORE_NANS{"IgnoreNans"};
const static std::string MODE{"Mode"};
const static std::string OUTPUT_WORKSPACE{"OutputWorkspace"};
const static std::string START{"Start"};
}

/// A convenience struct for rectangular constraints.
struct Box {
  double top;
  double bottom;
  double left;
  double right;
};

/// Profile constraints as array indices.
struct IndexLimits {
  size_t lineStart;
  size_t lineEnd;
  size_t widthStart;
  size_t widthEnd;
};

/**
 * Set correct units and vertical axis binning.
 * @param outWS A single-histogram workspace whose axes to modify.
 * @param ws A workspace to copy units from.
 * @param box Line profile constraints.
 * @param dir Line profile orientation.
 */
void setAxesAndUnits(const Workspace2D_sptr &outWS,
                     const MatrixWorkspace_const_sptr &ws, const Box &box,
                     const LineDirection dir) {
  // Y units.
  outWS->setYUnit(ws->YUnit());
  outWS->setYUnitLabel(ws->YUnitLabel());
  // Horizontal axis.
  auto axisIndex = dir == LineDirection::horizontal ? 0 : 1;
  if (ws->getAxis(axisIndex)->isSpectra()) {
    outWS->getAxis(axisIndex)->setUnit("Empty");
  } else {
    outWS->getAxis(0)->setUnit(ws->getAxis(axisIndex)->unit()->unitID());
  }
  // Vertical axis. We'll use bin edges set to Centre +/- HalfWidth.
  std::vector<double> vertBins(2);
  vertBins.front() = dir == LineDirection::horizontal ? box.top : box.left;
  vertBins.back() = dir == LineDirection::horizontal ? box.bottom : box.right;
  auto outVertAxis = make_unique<BinEdgeAxis>(vertBins);
  axisIndex = dir == LineDirection::horizontal ? 1 : 0;
  if (ws->getAxis(axisIndex)->isSpectra()) {
    outVertAxis->setUnit("Empty");
  } else {
    outVertAxis->setUnit(ws->getAxis(axisIndex)->unit()->unitID());
  }
  outWS->replaceAxis(1, outVertAxis.release());
}

/**
 * Find the start and end indices for a line profile.
 * @param start An output parameter for the start index.
 * @param end An output parameter for the end index.
 * @param bins Binning in a std::vector like container.
 * @param isBinEdges Whether bins contains edges or points.
 * @param lowerLimit A lower constraint.
 * @param upperLImit An upper constraint.
 * @throw std::runtime_error if given constraints don't make sense.
 */
template <typename Container>
void startAndEnd(size_t &start, size_t &end, const Container &bins,
                 const bool isBinEdges, const double lowerLimit,
                 const double upperLimit) {
  auto lowerIt = std::upper_bound(bins.cbegin(), bins.cend(), lowerLimit);
  if (lowerIt == bins.cend()) {
    throw std::runtime_error("Profile completely outside input workspace.");
  }
  if (lowerIt != bins.cbegin()) {
    --lowerIt;
  }
  auto upperIt = std::upper_bound(lowerIt, bins.cend(), upperLimit);
  if (upperIt == bins.cbegin()) {
    throw std::runtime_error("Profile completely outside input workspace.");
  }
  if (isBinEdges && upperIt == bins.cend()) {
    --upperIt;
  }
  start = std::distance(bins.cbegin(), lowerIt);
  end = std::distance(bins.cbegin(), upperIt);
}

/**
 * Extract values (binning) from (vertical) axis as vector. For
 * spectrum axis, spectrum numbers are returned.
 * @param axis An axis.
 * @param numberHistograms The actual number of histograms.
 * @return Axis bins.
 */
std::vector<double> extractVerticalBins(const Axis &axis,
                                        const size_t numberHistograms) {
  if (axis.isSpectra()) {
    std::vector<double> spectrumNumbers(numberHistograms);
    std::iota(spectrumNumbers.begin(), spectrumNumbers.end(), 1.0);
    return spectrumNumbers;
  }
  std::vector<double> bins(axis.length());
  for (size_t i = 0; i < bins.size(); ++i) {
    bins[i] = axis.getValue(i);
  }
  return bins;
}

/**
 * Calculate a line profile.
 * @param Xs Output for line profile histogram's X data.
 * @param Ys Output for line profile histogram's Y data.
 * @param Es Output for line profile histogram's E data.
 * @param ws A workspace where to extract a profile from.
 * @param dir Line orientation.
 * @param limits Line dimensions.
 * @param lineBins Bins in line's direction.
 * @param isBinEdges Whether lineBins represent edges or points.
 * @param modeFunction A function performing the final calculation.
 * @param ignoreNans Whether NaN values should be ignored or not.
 * @param ignoreInfs Whether infinities should be ignored or not.
 */
template <typename Container, typename Function>
void profile(std::vector<double> &Xs, std::vector<double> &Ys,
             std::vector<double> &Es, const MatrixWorkspace_const_sptr &ws,
             const LineDirection dir, const IndexLimits &limits,
             const Container &lineBins, const bool isBinEdges, Function modeFunction,
             const bool ignoreNans, const bool ignoreInfs) {
  const auto lineSize = limits.lineEnd - limits.lineStart;
  Xs.resize(lineSize + (isBinEdges ? 1 : 0));
  Ys.resize(lineSize);
  Es.resize(lineSize);
  for (size_t i = limits.lineStart; i < limits.lineEnd; ++i) {
    Xs[i - limits.lineStart] = lineBins[i];
    double ySum = 0;
    double eSqSum = 0;
    int n = 0;
    for (size_t j = limits.widthStart; j < limits.widthEnd; ++j) {
      const size_t iHor = dir == LineDirection::horizontal ? i : j;
      const size_t iVert = dir == LineDirection::horizontal ? j : i;
      const double y = ws->y(iVert)[iHor];
      if ((ignoreNans && std::isnan(y)) || (ignoreInfs && std::isinf(y))) {
        continue;
      }
      ySum += y;
      eSqSum += ws->e(iVert)[iHor] * ws->e(iVert)[iHor];
      ++n;
    }
    const int nTotal = static_cast<int>(limits.widthEnd) - static_cast<int>(limits.widthStart);
    Ys[i - limits.lineStart] = n == 0 ? std::nan("") : modeFunction(ySum, n, nTotal);
    const double e = modeFunction(std::sqrt(eSqSum), n, nTotal);
    Es[i - limits.lineStart] = std::isnan(e) ? 0 : e;
  }
  if (isBinEdges) {
    Xs.back() = lineBins[limits.lineEnd];
  }
}

/**
 * A mode function for averaging.
 * @param sum A sum of data point.
 * @param n Number of summed points.
 * @return The average.
 */
double averageMode(const double sum, const int n, const int) noexcept {
  return sum / n;
}

/**
 * A mode function for summing.
 * @param sum A sum of data points.
 * @return The sum.
 */
double sumMode(const double sum, const int, const int) noexcept {
  return sum;
}

/**
 * A mode function for weighed summing. The weight is inversely proportional
 * to the number of data points in the sum.
 * @param sum A sum of data points.
 * @param n Number of summed points.
 * @param nTot Total number of possible points, including NaNs and infs.
 * @return The weighed sum.
 */
double weighedSumMode(const double sum, const int n, const int nTot) noexcept {
  return static_cast<double>(nTot) / static_cast<double>(n) * sum;
}

/**
 * Return a suitable function to calculate the profile over its width.
 * @param modeName The name of the calculation mode.
 */
auto createMode(const std::string &modeName) noexcept {
  if (modeName == ModeChoices::AVERAGE) {
    return averageMode;
  }
  if (modeName == ModeChoices::SUM) {
    return sumMode;
  }
  return weighedSumMode;
}
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LineProfile)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LineProfile::name() const { return "LineProfile"; }

/// Algorithm's version for identification. @see Algorithm::version
int LineProfile::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LineProfile::category() const { return "Utility"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LineProfile::summary() const {
  return "Calculates a line profile over a MatrixWorkspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LineProfile::init() {
  const auto mandatoryDouble = boost::make_shared<MandatoryValidator<double>>();
  const auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLowerExclusive(0.0);
  const auto mandatoryPositiveDouble = boost::make_shared<CompositeValidator>();
  mandatoryPositiveDouble->add(mandatoryDouble);
  mandatoryPositiveDouble->add(positiveDouble);
  const auto inputWorkspaceValidator = boost::make_shared<CompositeValidator>();
  inputWorkspaceValidator->add(boost::make_shared<CommonBinsValidator>());
  inputWorkspaceValidator->add(boost::make_shared<IncreasingAxisValidator>());
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      PropertyNames::INPUT_WORKSPACE, "", Direction::Input,
                      inputWorkspaceValidator),
                  "An input workspace.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<Workspace2D>>(
                      PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
                  "A single histogram workspace containing the profile.");
  declareProperty(PropertyNames::CENTRE, EMPTY_DBL(), mandatoryDouble,
                  "Centre of the line.");
  declareProperty(PropertyNames::HALF_WIDTH, EMPTY_DBL(),
                  mandatoryPositiveDouble,
                  "Half of the width over which to calcualte the profile.");
  const std::set<std::string> directions{DirectionChoices::HORIZONTAL,
                                         DirectionChoices::VERTICAL};
  declareProperty(PropertyNames::DIRECTION, DirectionChoices::HORIZONTAL,
                  boost::make_shared<ListValidator<std::string>>(directions),
                  "Orientation of the profile line.");
  declareProperty(PropertyNames::START, EMPTY_DBL(),
                  "Starting point of the line.");
  declareProperty(PropertyNames::END, EMPTY_DBL(),
                  "End point of the line.");
  const std::set<std::string> modes{ModeChoices::AVERAGE, ModeChoices::SUM, ModeChoices::WEIGHED_SUM};
  declareProperty(PropertyNames::MODE, ModeChoices::AVERAGE, boost::make_shared<ListValidator<std::string>>(modes), "How the profile is calculated over the line width.");
  declareProperty(
      PropertyNames::IGNORE_INFS, false,
      "If true, ignore infinities when calculating the profile.");
  declareProperty(
      PropertyNames::IGNORE_NANS, true,
      "If true, ignore not-a-numbers when calculating the profile.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LineProfile::exec() {
  // Extract properties.
  MatrixWorkspace_const_sptr ws = getProperty(PropertyNames::INPUT_WORKSPACE);
  const bool ignoreNans = getProperty(PropertyNames::IGNORE_NANS);
  const bool ignoreInfs = getProperty(PropertyNames::IGNORE_INFS);
  const auto &horizontalBins = ws->x(0);
  const auto horizontalIsBinEdges = ws->isHistogramData();
  const auto vertAxis = ws->getAxis(1);
  // It is easier if the vertical axis values are in a vector.
  const auto verticalBins =
      extractVerticalBins(*vertAxis, ws->getNumberHistograms());
  const auto verticalIsBinEdges =
      verticalBins.size() > ws->getNumberHistograms();
  const std::string directionString = getProperty(PropertyNames::DIRECTION);
  LineDirection dir{LineDirection::horizontal};
  if (directionString == DirectionChoices::VERTICAL) {
    dir = LineDirection::vertical;
  }
  const double centre = getProperty(PropertyNames::CENTRE);
  const double halfWidth = getProperty(PropertyNames::HALF_WIDTH);
  double start = getProperty(PropertyNames::START);
  if (start == EMPTY_DBL()) {
    start = std::numeric_limits<double>::lowest();
  }
  double end = getProperty(PropertyNames::END);
  if (end == EMPTY_DBL()) {
    end = std::numeric_limits<double>::max();
  }
  // Define a box in workspace's units to have a standard representation
  // of the profile's dimensions.
  Box bounds;
  if (dir == LineDirection::horizontal) {
    bounds.top = centre - halfWidth;
    bounds.bottom = centre + halfWidth;
    bounds.left = start;
    bounds.right = end;
  } else {
    bounds.top = start;
    bounds.bottom = end;
    bounds.left = centre - halfWidth;
    bounds.right = centre + halfWidth;
  }
  // Convert the bounds from workspace units to indices.
  size_t vertStart;
  size_t vertEnd;
  startAndEnd(vertStart, vertEnd, verticalBins, verticalIsBinEdges, bounds.top,
              bounds.bottom);
  size_t horStart;
  size_t horEnd;
  startAndEnd(horStart, horEnd, horizontalBins, horizontalIsBinEdges,
              bounds.left, bounds.right);
  // Choose mode.
  auto mode = createMode(getProperty(PropertyNames::MODE));
  // Build the actual profile.
  std::vector<double> profileYs;
  std::vector<double> profileEs;
  std::vector<double> Xs;
  if (dir == LineDirection::horizontal) {
    IndexLimits limits;
    limits.lineStart = horStart;
    limits.lineEnd = horEnd;
    limits.widthStart = vertStart;
    limits.widthEnd = vertEnd;
    profile(Xs, profileYs, profileEs, ws, dir, limits, horizontalBins,
            horizontalIsBinEdges, mode, ignoreNans, ignoreInfs);
  } else {
    IndexLimits limits;
    limits.lineStart = vertStart;
    limits.lineEnd = vertEnd;
    limits.widthStart = horStart;
    limits.widthEnd = horEnd;
    profile(Xs, profileYs, profileEs, ws, dir, limits, verticalBins,
            verticalIsBinEdges, mode, ignoreNans, ignoreInfs);
  }
  // Prepare and set output.
  Workspace2D_sptr outWS;
  if (Xs.size() > profileYs.size()) {
    outWS =
        create<Workspace2D>(1, Histogram(BinEdges(Xs), Counts(profileYs),
                                         CountStandardDeviations(profileEs)));
  } else {
    outWS =
        create<Workspace2D>(1, Histogram(Points(Xs), Counts(profileYs),
                                         CountStandardDeviations(profileEs)));
  }
  // The actual profile might be of different size than what user
  // specified.
  Box actualBounds;
  actualBounds.top = verticalBins[vertStart];
  actualBounds.bottom = verticalBins[vertEnd];
  actualBounds.left = horizontalBins[horStart];
  actualBounds.right = horizontalBins[horEnd];
  setAxesAndUnits(outWS, ws, actualBounds, dir);
  setProperty(PropertyNames::OUTPUT_WORKSPACE, outWS);
}

/** Validate the algorithm's inputs.
 */
std::map<std::string, std::string> LineProfile::validateInputs() {
  std::map<std::string, std::string> issues;
  MatrixWorkspace_const_sptr ws = getProperty(PropertyNames::INPUT_WORKSPACE);
  if (ws->getAxis(1)->isText()) {
    issues[PropertyNames::INPUT_WORKSPACE] =
        "The vertical axis in " + PropertyNames::INPUT_WORKSPACE + " is text.";
  }
  return issues;
}

} // namespace Algorithms
} // namespace Mantid
