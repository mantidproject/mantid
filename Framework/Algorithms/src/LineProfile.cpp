// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/LineProfile.h"

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Unit.h"

#include "boost/make_shared.hpp"
#include <algorithm>

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
using Mantid::DataObjects::Workspace2D;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::DataObjects::create;
using Mantid::HistogramData::HistogramBuilder;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::ListValidator;
using Mantid::Kernel::MandatoryValidator;

namespace {
/// An enum specifying a line profile orientation.
enum class LineDirection { horizontal, vertical };

/// A private namespace for the options for the Direction property.
namespace DirectionChoices {
const static std::string HORIZONTAL{"Horizontal"};
const static std::string VERTICAL{"Vertical"};
} // namespace DirectionChoices

/// A private namespace for the mode options.
namespace ModeChoices {
const static std::string AVERAGE{"Average"};
const static std::string SUM{"Sum"};
} // namespace ModeChoices

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
} // namespace PropertyNames

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
 * Create the profile workspace.
 * @param ws The parent workspace.
 * @param Xs Profile's X values.
 * @param Ys Profile's Y values.
 * @param Es Profile's E values.
 * @return A single histogram profile workspace.
 */
Workspace2D_sptr makeOutput(const MatrixWorkspace &parent,
                            const LineDirection direction,
                            std::vector<double> &&Xs, std::vector<double> &&Ys,
                            std::vector<double> &&Es) {
  HistogramBuilder builder;
  builder.setX(std::move(Xs));
  builder.setY(std::move(Ys));
  builder.setE(std::move(Es));
  builder.setDistribution(direction == LineDirection::horizontal &&
                          parent.isDistribution());
  return create<Workspace2D>(parent, 1, builder.build());
}

/**
 * Set correct units and vertical axis binning.
 * @param outWS A single-histogram workspace whose axes to modify.
 * @param ws A workspace to copy units from.
 * @param box Line profile constraints.
 * @param dir Line profile orientation.
 */
void setAxesAndUnits(Workspace2D &outWS, const MatrixWorkspace &ws,
                     const Box &box, const LineDirection dir) {
  // Y units.
  outWS.setYUnit(ws.YUnit());
  outWS.setYUnitLabel(ws.YUnitLabel());
  // Horizontal axis.
  auto axisIndex = dir == LineDirection::horizontal ? 0 : 1;
  if (ws.getAxis(axisIndex)->isSpectra()) {
    outWS.getAxis(axisIndex)->setUnit("Empty");
  } else {
    outWS.getAxis(0)->setUnit(ws.getAxis(axisIndex)->unit()->unitID());
  }
  // Vertical axis. We'll use bin edges set to Centre +/- HalfWidth.
  std::vector<double> vertBins(2);
  vertBins.front() = dir == LineDirection::horizontal ? box.top : box.left;
  vertBins.back() = dir == LineDirection::horizontal ? box.bottom : box.right;
  auto outVertAxis = std::make_unique<BinEdgeAxis>(vertBins);
  axisIndex = dir == LineDirection::horizontal ? 1 : 0;
  if (ws.getAxis(axisIndex)->isSpectra()) {
    outVertAxis->setUnit("Empty");
  } else {
    outVertAxis->setUnit(ws.getAxis(axisIndex)->unit()->unitID());
  }
  outWS.replaceAxis(1, std::move(outVertAxis));
}

/**
 * Find the start and end indices for a line profile.
 * @param bins Binning in a std::vector like container.
 * @param isBinEdges Whether bins contains edges or points.
 * @param lowerLimit A lower constraint.
 * @param upperLImit An upper constraint.
 * @return The interval as pair.
 * @throw std::runtime_error if given constraints don't make sense.
 */
template <typename Container>
std::pair<size_t, size_t>
startAndEnd(const Container &bins, const bool isBinEdges,
            const double lowerLimit, const double upperLimit) {
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
  const auto start = std::distance(bins.cbegin(), lowerIt);
  const auto end = std::distance(bins.cbegin(), upperIt);
  return std::pair<size_t, size_t>{start, end};
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
             std::vector<double> &Es, const MatrixWorkspace &ws,
             const LineDirection dir, const IndexLimits &limits,
             const Container &lineBins, const bool isBinEdges,
             Function modeFunction, const bool ignoreNans,
             const bool ignoreInfs) {
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
      auto histogram = ws.histogram(iVert);
      auto iter = histogram.begin();
      std::advance(iter, iHor);
      const double y = iter->counts();
      if ((ignoreNans && std::isnan(y)) || (ignoreInfs && std::isinf(y))) {
        continue;
      }
      const double e = iter->countStandardDeviation();
      ySum += y;
      eSqSum += e * e;
      ++n;
    }
    const size_t nTotal = limits.widthEnd - limits.widthStart;
    Ys[i - limits.lineStart] =
        n == 0 ? std::nan("") : modeFunction(ySum, n, nTotal);
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
double averageMode(const double sum, const size_t n,
                   const size_t /*unused*/) noexcept {
  return sum / static_cast<double>(n);
}

/**
 * A mode function for weigthed summing. The weight is inversely proportional
 * to the number of data points in the sum.
 * @param sum A sum of data points.
 * @param n Number of summed points.
 * @param nTot Total number of possible points, including NaNs and infs.
 * @return The weigthed sum.
 */
double sumMode(const double sum, const size_t n, const size_t nTot) noexcept {
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
  return sumMode;
}

void divideByBinHeight(MatrixWorkspace &ws) {
  const BinEdgeAxis &axis = *static_cast<BinEdgeAxis *>(ws.getAxis(1));
  const auto height = axis.getMax() - axis.getMin();
  ws.mutableY(0) /= height;
  ws.mutableE(0) /= height;
}
} // namespace

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
  positiveDouble->setLower(0.0);
  positiveDouble->setLowerExclusive(true);
  const auto mandatoryPositiveDouble = boost::make_shared<CompositeValidator>();
  mandatoryPositiveDouble->add(mandatoryDouble);
  mandatoryPositiveDouble->add(positiveDouble);
  const auto inputWorkspaceValidator = boost::make_shared<CompositeValidator>();
  inputWorkspaceValidator->add(boost::make_shared<CommonBinsValidator>());
  inputWorkspaceValidator->add(boost::make_shared<IncreasingAxisValidator>());
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      PropertyNames::INPUT_WORKSPACE, "", Direction::Input,
                      inputWorkspaceValidator),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
                  "A single histogram workspace containing the profile.");
  declareProperty(PropertyNames::CENTRE, EMPTY_DBL(), mandatoryDouble,
                  "Centre of the line.");
  declareProperty(PropertyNames::HALF_WIDTH, EMPTY_DBL(),
                  mandatoryPositiveDouble,
                  "Half of the width over which to calcualte the profile.");
  const std::array<std::string, 2> directions = {
      {DirectionChoices::HORIZONTAL, DirectionChoices::VERTICAL}};
  declareProperty(PropertyNames::DIRECTION, DirectionChoices::HORIZONTAL,
                  boost::make_shared<ListValidator<std::string>>(directions),
                  "Orientation of the profile line.");
  declareProperty(PropertyNames::START, EMPTY_DBL(),
                  "Starting point of the line.");
  declareProperty(PropertyNames::END, EMPTY_DBL(), "End point of the line.");
  const std::array<std::string, 2> modes = {
      {ModeChoices::AVERAGE, ModeChoices::SUM}};
  declareProperty(PropertyNames::MODE, ModeChoices::AVERAGE,
                  boost::make_shared<ListValidator<std::string>>(modes),
                  "How the profile is calculated over the line width.");
  declareProperty(PropertyNames::IGNORE_INFS, false,
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
  const auto vertInterval =
      startAndEnd(verticalBins, verticalIsBinEdges, bounds.top, bounds.bottom);
  const auto horInterval = startAndEnd(horizontalBins, horizontalIsBinEdges,
                                       bounds.left, bounds.right);
  // Choose mode.
  auto mode = createMode(getProperty(PropertyNames::MODE));
  // Build the actual profile.
  std::vector<double> profileYs;
  std::vector<double> profileEs;
  std::vector<double> Xs;
  if (dir == LineDirection::horizontal) {
    IndexLimits limits;
    limits.lineStart = horInterval.first;
    limits.lineEnd = horInterval.second;
    limits.widthStart = vertInterval.first;
    limits.widthEnd = vertInterval.second;
    profile(Xs, profileYs, profileEs, *ws, dir, limits, horizontalBins,
            horizontalIsBinEdges, mode, ignoreNans, ignoreInfs);
  } else {
    IndexLimits limits;
    limits.lineStart = vertInterval.first;
    limits.lineEnd = vertInterval.second;
    limits.widthStart = horInterval.first;
    limits.widthEnd = horInterval.second;
    profile(Xs, profileYs, profileEs, *ws, dir, limits, verticalBins,
            verticalIsBinEdges, mode, ignoreNans, ignoreInfs);
  }
  // Prepare and set output.
  auto outWS = makeOutput(*ws, dir, std::move(Xs), std::move(profileYs),
                          std::move(profileEs));
  // The actual profile might be of different size than what user
  // specified.
  Box actualBounds;
  actualBounds.top = verticalBins[vertInterval.first];
  actualBounds.bottom = vertInterval.second < verticalBins.size()
                            ? verticalBins[vertInterval.second]
                            : verticalBins.back();
  actualBounds.left = horizontalBins[horInterval.first];
  actualBounds.right = horInterval.second < horizontalBins.size()
                           ? horizontalBins[horInterval.second]
                           : horizontalBins.back();
  setAxesAndUnits(*outWS, *ws, actualBounds, dir);
  if (dir == LineDirection::vertical && ws->isDistribution()) {
    divideByBinHeight(*outWS);
  }
  setProperty(PropertyNames::OUTPUT_WORKSPACE, outWS);
}

/** Validate the algorithm's inputs.
 */
std::map<std::string, std::string> LineProfile::validateInputs() {
  std::map<std::string, std::string> issues;
  const double start = getProperty(PropertyNames::START);
  const double end = getProperty(PropertyNames::END);
  if (start > end) {
    issues[PropertyNames::START] =
        PropertyNames::START + " greater than " + PropertyNames::END + ".";
  }
  MatrixWorkspace_const_sptr ws = getProperty(PropertyNames::INPUT_WORKSPACE);
  if (ws->getAxis(1)->isText()) {
    issues[PropertyNames::INPUT_WORKSPACE] =
        "The vertical axis in " + PropertyNames::INPUT_WORKSPACE + " is text.";
  }
  return issues;
}

} // namespace Algorithms
} // namespace Mantid
