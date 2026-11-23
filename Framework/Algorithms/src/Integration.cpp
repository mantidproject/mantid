// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Integration.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/SpectrumUnitConversion.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Integration)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;

namespace {
/// Stands in for a lower integration limit that was not given
constexpr double NO_LOWER_LIMIT = std::numeric_limits<double>::lowest();

bool canConvertQuickly(const Unit &from, const Unit &to) {
  double factor{0.0}, power{0.0};
  return from.quickConversion(to, factor, power);
}

/// True if RangeUnit was given and is not the X unit of the workspace
bool rangeUnitDiffersFromXUnit(const std::string &rangeUnit, const MatrixWorkspace &workspace) {
  return !rangeUnit.empty() && rangeUnit != workspace.getAxis(0)->unit()->unitID();
}

/// The first and last workspace indices to integrate, replacing any that are out of range
std::pair<int, int> workspaceIndexRange(int first, int last, const int numberOfSpectra) {
  if (first >= numberOfSpectra)
    first = 0;
  if (last == EMPTY_INT() || last > numberOfSpectra - 1 || last < first)
    last = numberOfSpectra - 1;
  return {first, last};
}

/**
 * The stricter of the single and list limits for a spectrum
 * @param listIndex :: The position of the spectrum among those being integrated
 * @return The lower limit, or NO_LOWER_LIMIT, and the upper limit, or EMPTY_DBL()
 */
std::pair<double, double> spectrumLimits(const std::size_t listIndex, const double minRange, const double maxRange,
                                         const std::vector<double> &minRanges, const std::vector<double> &maxRanges) {
  const double lower = minRange == EMPTY_DBL() ? NO_LOWER_LIMIT : minRange;
  return {minRanges.empty() ? lower : std::max(lower, minRanges[listIndex]),
          maxRanges.empty() ? maxRange : std::min(maxRange, maxRanges[listIndex])};
}

/// Converts the integration limits of a spectrum from RangeUnit to the X unit of the workspace
class LimitConverter {
public:
  LimitConverter(const MatrixWorkspace &workspace, const std::string &rangeUnit)
      : m_rangeUnit(UnitFactory::Instance().create(rangeUnit)), m_xUnit(workspace.getAxis(0)->unit()),
        m_isQuick(canConvertQuickly(*m_rangeUnit, *m_xUnit) && canConvertQuickly(*m_xUnit, *m_rangeUnit)),
        m_l1(m_isQuick ? 0.0 : workspace.spectrumInfo().l1()),
        m_setup{workspace.spectrumInfo(),
                *m_rangeUnit,
                *m_xUnit,
                m_isQuick ? DeltaEMode::Elastic : workspace.getEMode(),
                false,
                {}} {}

  /**
   * @param index :: The workspace index of the spectrum
   * @param x :: The X values of the spectrum
   * @param lower :: The lower limit in RangeUnit, or NO_LOWER_LIMIT
   * @param upper :: The upper limit in RangeUnit, or EMPTY_DBL()
   * @return The limits in the X unit
   * @throw std::runtime_error if the limits cannot be converted
   */
  std::pair<double, double> convert(const std::size_t index, const HistogramX &x, const double lower,
                                    const double upper) const {
    const bool hasLower = lower != NO_LOWER_LIMIT;
    const bool hasUpper = upper != EMPTY_DBL();
    std::pair<double, double> limits{NO_LOWER_LIMIT, EMPTY_DBL()};
    SpectrumUnitConversion conversion(m_setup);
    try {
      if (!m_isQuick) {
        if (!m_setup.spectrumInfo.hasDetectors(index))
          throw std::runtime_error("the spectrum has no detectors");
        conversion.readDetectorValues(index);
      }
      const auto toX = [&](const double value) {
        return UnitConversion::run(*conversion.from, *conversion.to, value, m_l1, m_setup.emode, conversion.params);
      };
      const auto toRangeUnit = [&](const double value) {
        return UnitConversion::run(*conversion.to, *conversion.from, value, m_l1, m_setup.emode, conversion.params);
      };

      if (hasLower && hasUpper) {
        const double convertedLower = toX(lower);
        const double convertedUpper = toX(upper);
        // Some spectra convert every value to the same result instead of failing, e.g. a monitor has no d-spacing
        if (lower < upper && convertedLower == convertedUpper)
          throw std::runtime_error("the spectrum does not support this conversion, as both limits convert to " +
                                   std::to_string(convertedLower));
        // The units may run in opposite directions, e.g. energy falls as time-of-flight rises
        limits = std::minmax({convertedLower, convertedUpper});
      } else if (hasLower || hasUpper) {
        const double first = toRangeUnit(x.front());
        const double last = toRangeUnit(x.back());
        if (std::isnan(first) || std::isnan(last))
          throw std::runtime_error("the X values cannot be converted to " + m_rangeUnit->unitID());
        const bool reversed = first > last;
        const double limit = toX(hasLower ? lower : upper);
        if (hasLower != reversed)
          limits.first = limit;
        else
          limits.second = limit;
      }
    } catch (const std::exception &error) {
      throw std::runtime_error("Unable to convert the integration limits from " + m_rangeUnit->unitID() + " to " +
                               m_xUnit->unitID() + " for workspace index " + std::to_string(index) + ": " +
                               error.what());
    }

    if (std::isnan(limits.first) || std::isnan(limits.second))
      throw std::runtime_error("The integration limits for workspace index " + std::to_string(index) +
                               " are not valid values of " + m_rangeUnit->unitID() + ".");
    return limits;
  }

private:
  const Unit_const_sptr m_rangeUnit;
  const Unit_const_sptr m_xUnit;
  const bool m_isQuick;
  const double m_l1;
  const UnitConversionSetup m_setup;
};
} // namespace

/** Initialisation method.
 *
 */
void Integration::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "The input workspace to integrate.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace with the results of the integration.");

  declareProperty("RangeLower", EMPTY_DBL(), "The lower integration limit (an X value).");
  declareProperty("RangeUpper", EMPTY_DBL(), "The upper integration limit (an X value).");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive, "Index of the first spectrum to integrate.");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive, "Index of the last spectrum to integrate.");
  declareProperty("IncludePartialBins", false,
                  "If true then partial bins from the beginning and end of the "
                  "input range are also included in the integration.");
  declareProperty(std::make_unique<ArrayProperty<double>>("RangeLowerList"),
                  "A list of lower integration limits (as X values).");
  declareProperty(std::make_unique<ArrayProperty<double>>("RangeUpperList"),
                  "A list of upper integration limits (as X values).");

  // An empty value means that the limits are in the X unit of the input workspace
  std::vector<std::string> rangeUnits{""};
  const auto convertibleUnits = UnitFactory::Instance().getConvertibleUnits();
  rangeUnits.insert(rangeUnits.end(), convertibleUnits.cbegin(), convertibleUnits.cend());
  declareProperty("RangeUnit", "", std::make_shared<StringListValidator>(rangeUnits),
                  "The unit of the integration limits. If not given, the limits are in the X unit of the input "
                  "workspace. At least one limit is required if this differs from the X unit. Only the limits are "
                  "converted, separately for each spectrum, and this must be possible for every spectrum integrated. "
                  "The output X values remain in the X unit of the input workspace.");
}

/**
 * Std-style comparision function object (satisfies the requirements of Compare)
 * @return true if first argument < second argument (with some
 * tolerance/epsilon)
 */
struct tolerant_less {
public:
  bool operator()(const double &left, const double &right) const {
    // soft equal, if the diff left-right is below a numerical error
    // (uncertainty) threshold, we cannot say
    // NOTE this could perhaps use FloatingPointComparison::equal operation
    return (left < right) && (std::abs(left - right) > 1 * std::numeric_limits<double>::epsilon());
  }
};

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Integration::exec() {
  // Try and retrieve the optional properties

  /// The value in X to start the integration from
  const double minRange = getProperty("RangeLower");
  /// The value in X to finish the integration at
  const double maxRange = getProperty("RangeUpper");
  /// The spectrum requested to start the integration from
  const int requestedMinWsIndex = getProperty("StartWorkspaceIndex");
  /// The spectrum requested to finish the integration at
  const int requestedMaxWsIndex = getProperty("EndWorkspaceIndex");
  /// Flag for including partial bins
  const bool incPartBins = getProperty("IncludePartialBins");
  /// List of X values to start the integration from
  const std::vector<double> minRanges = getProperty("RangeLowerList");
  /// List of X values to finish the integration at
  const std::vector<double> maxRanges = getProperty("RangeUpperList");

  // Get the input workspace
  MatrixWorkspace_sptr localworkspace = this->getInputWorkspace();

  const auto numberOfSpectra = static_cast<int>(localworkspace->getNumberHistograms());

  // Check the workspace indices are in range 0-numberOfSpectra
  int minWsIndex{0}, maxWsIndex{0};
  std::tie(minWsIndex, maxWsIndex) = workspaceIndexRange(requestedMinWsIndex, requestedMaxWsIndex, numberOfSpectra);
  if (minWsIndex != requestedMinWsIndex)
    g_log.warning("StartWorkspaceIndex out of range! Set to 0.");
  if (!isEmpty(requestedMaxWsIndex) && maxWsIndex != requestedMaxWsIndex)
    g_log.warning("EndWorkspaceIndex out of range! Set to max workspace index.");

  // validateInputs has checked that the limits of every spectrum can be converted
  const std::string rangeUnit = getProperty("RangeUnit");
  std::optional<LimitConverter> limitConverter;
  if (rangeUnitDiffersFromXUnit(rangeUnit, *localworkspace))
    limitConverter.emplace(*localworkspace, rangeUnit);

  double progressStart = 0.0;
  //---------------------------------------------------------------------------------
  // Now, determine if the input workspace is actually an EventWorkspace
  EventWorkspace_sptr eventInputWS = std::dynamic_pointer_cast<EventWorkspace>(localworkspace);

  // Converted limits differ between spectra, which a single rebin cannot represent, so in that case the existing bins
  // of the EventWorkspace are integrated
  if (eventInputWS != nullptr && !limitConverter) {
    //------- EventWorkspace as input -------------------------------------
    // Get the eventworkspace rebinned to apply the upper and lowerrange
    double evntMinRange = isEmpty(minRange) ? eventInputWS->getEventXMin() : minRange;
    double evntMaxRange = isEmpty(maxRange) ? eventInputWS->getEventXMax() : maxRange;
    localworkspace = rangeFilterEventWorkspace(eventInputWS, evntMinRange, evntMaxRange);

    progressStart = 0.5;
  }

  // Create the 2D workspace (with 1 bin) for the output

  MatrixWorkspace_sptr outputWorkspace = create<Workspace2D>(*localworkspace, maxWsIndex - minWsIndex + 1, BinEdges(2));
  auto rebinned_input = std::dynamic_pointer_cast<const RebinnedOutput>(localworkspace);
  auto rebinned_output = std::dynamic_pointer_cast<RebinnedOutput>(outputWorkspace);

  Progress progress(this, progressStart, 1.0, maxWsIndex - minWsIndex + 1);

  const bool axisIsText = localworkspace->getAxis(1)->isText();
  const bool axisIsNumeric = localworkspace->getAxis(1)->isNumeric();

  // Loop over spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*localworkspace, *outputWorkspace))
  for (int i = minWsIndex; i <= maxWsIndex; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    // Workspace index on the output
    const int outWI = i - minWsIndex;

    // Copy Axis values from previous workspace
    if (axisIsText) {
      TextAxis *newAxis = dynamic_cast<TextAxis *>(outputWorkspace->getAxis(1));
      if (newAxis)
        newAxis->setLabel(outWI, localworkspace->getAxis(1)->label(i));
    } else if (axisIsNumeric) {
      NumericAxis *newAxis = dynamic_cast<NumericAxis *>(outputWorkspace->getAxis(1));
      if (newAxis)
        newAxis->setValue(outWI, (*(localworkspace->getAxis(1)))(i));
    }

    // This is the output
    auto &outSpec = outputWorkspace->getSpectrum(outWI);
    // This is the input
    const auto &inSpec = localworkspace->getSpectrum(i);

    // Copy spectrum number, detector IDs
    outSpec.copyInfoFrom(inSpec);

    const MantidVec *Fin(nullptr);
    MantidVec *Fout(nullptr);
    if (rebinned_input)
      Fin = &rebinned_input->readF(i);
    if (rebinned_output)
      Fout = &rebinned_output->dataF(outWI);

    double lowerLimit{0.0}, upperLimit{0.0};
    std::tie(lowerLimit, upperLimit) =
        spectrumLimits(static_cast<std::size_t>(outWI), minRange, maxRange, minRanges, maxRanges);

    if (upperLimit < lowerLimit) {
      std::ostringstream sout;
      sout << "Upper integration limit " << upperLimit << " for workspace index " << i
           << " smaller than the lower limit " << lowerLimit << ". Setting integral to zero.\n";
      g_log.warning() << sout.str();
      progress.report();
      continue;
    }

    if (limitConverter) {
      std::tie(lowerLimit, upperLimit) =
          limitConverter->convert(static_cast<std::size_t>(i), inSpec.x(), lowerLimit, upperLimit);
    }
    // A side without a limit is integrated to the end of the spectrum, which is also what the output X records
    if (lowerLimit == NO_LOWER_LIMIT)
      lowerLimit = inSpec.x().front();
    if (upperLimit == EMPTY_DBL())
      upperLimit = inSpec.x().back();

    integrateSpectrum(inSpec, outSpec, Fin, Fout, lowerLimit, upperLimit, incPartBins);

    progress.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  if (rebinned_output) {
    rebinned_output->finalize(false);
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWorkspace);
}

/**
 * Integrate a single spectrum between the supplied limits
 */
void Integration::integrateSpectrum(const API::ISpectrum &inSpec, API::ISpectrum &outSpec,
                                    const std::vector<double> *Fin, std::vector<double> *Fout, const double lowerLimit,
                                    const double upperLimit, const bool incPartBins) {
  // Retrieve the spectrum into a vector (Histogram)
  const auto &X = inSpec.x();
  const auto &Y = inSpec.y();
  const auto &E = inSpec.e();

  // Find the range [min,max]
  MantidVec::const_iterator lowit, highit;

  // If doing partial bins, we want to set the bin boundaries to the specified
  // values regardless of whether they're 'in range' for this spectrum
  // Have to do this here, ahead of the 'continue' a bit down from here.
  if (incPartBins) {
    outSpec.mutableX()[0] = lowerLimit;
    outSpec.mutableX()[1] = upperLimit;
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
    return;

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
  double Fnor = 0.0;
  auto is_distrib = inSpec.yMode() == HistogramData::Histogram::YMode::Frequencies;
  if (distmax <= distmin) {
    sumY = 0.;
    sumE = 0.;
  } else {
    if (Fin) {
      // Workspace has fractional area information, need to take into account
      sumF = std::accumulate(Fin->cbegin() + distmin, Fin->cbegin() + distmax, 0.0);
      // Need to normalise by the number of non-NaN bins - see issue #33407 for details
      Fnor = static_cast<double>(
          std::count_if(Fin->begin() + distmin, Fin->begin() + distmax, [](double f) { return f != 0.; }));
      if (distmin > 0)
        Fmin = (*Fin)[distmin - 1];
      Fmax = (*Fin)[static_cast<std::size_t>(distmax) < Fin->size() ? distmax : Fin->size() - 1];
    }
    if (!is_distrib) {
      // Sum the Y, and sum the E in quadrature
      {
        sumY = std::accumulate(Y.cbegin() + distmin, Y.cbegin() + distmax, 0.0);
        sumE = std::accumulate(E.cbegin() + distmin, E.cbegin() + distmax, 0.0, VectorHelper::SumSquares<double>());
      }
    } else {
      // Sum Y*binwidth and Sum the (E*binwidth)^2.
      std::vector<double> widths(X.size());
      // highit+1 is safe while input workspace guaranteed to be histogram
      std::adjacent_difference(lowit, highit + 1, widths.begin());
      sumY = std::inner_product(Y.begin() + distmin, Y.begin() + distmax, widths.begin() + 1, 0.0);
      sumE = std::inner_product(E.begin() + distmin, E.begin() + distmax, widths.begin() + 1, 0.0, std::plus<double>(),
                                VectorHelper::TimesSquares<double>());
    }
  }
  // If partial bins are included, set integration range to exact range
  // given and add on contributions from partial bins either side of range.
  if (incPartBins) {
    if ((distmax <= distmin) && (distmin > 0) && (highit < X.end() - 1)) {
      // handle special case where lower and upper limit are in the same bin
      const double lower_bin = *lowit;
      const double prev_bin = *(lowit - 1);
      double fraction = (upperLimit - lowerLimit);
      if (!is_distrib) {
        fraction /= (lower_bin - prev_bin);
      }
      const MantidVec::size_type val_index = distmin - 1;
      sumY += Y[val_index] * fraction;
      const double eval = E[val_index];
      sumE += eval * eval * fraction * fraction;
      if (Fin) {
        sumF += Fmin * fraction;
        if (Fmin != 0.0)
          Fnor += fraction;
      }
    } else {
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
        if (Fin) {
          sumF += Fmin * fraction;
          if (Fmin != 0.0)
            Fnor += fraction;
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
        if (Fin) {
          sumF += Fmax * fraction;
          if (Fmax != 0.0)
            Fnor += fraction;
        }
      }
    }
  } else {
    outSpec.mutableX()[0] = lowit == X.end() ? *(lowit - 1) : *(lowit);
    outSpec.mutableX()[1] = *highit;
  }

  outSpec.mutableY()[0] = sumY;
  outSpec.mutableE()[0] = sqrt(sumE); // Propagate Gaussian error

  if (Fout) {
    (*Fout)[0] = sumF;
    if (Fnor != 0)
      (*Fout)[0] /= Fnor;
  }
}

/**
 * Uses rebin to reduce event workspaces to a single bin histogram
 */
API::MatrixWorkspace_sptr Integration::rangeFilterEventWorkspace(const API::MatrixWorkspace_sptr &workspace,
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
    auto alg = createChildAlgorithm("ReplaceSpecialValues");
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
    std::dynamic_pointer_cast<RebinnedOutput>(temp)->unfinalize();
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
  if (!minRanges.empty() && !maxRanges.empty() && minRanges.size() != maxRanges.size()) {
    issues["RangeLowerList"] = "RangeLowerList has different number of values as RangeUpperList.";
    return issues;
  }
  for (size_t i = 0; i < minRanges.size(); ++i) {
    const auto x = minRanges[i];
    if (!isEmpty(maxRange) && x > maxRange) {
      issues["RangeLowerList"] = "RangeLowerList has a value greater than RangeUpper.";
      break;
    } else if (!maxRanges.empty() && x > maxRanges[i]) {
      issues["RangeLowerList"] = "RangeLowerList has a value greater than the "
                                 "corresponding one in RangeUpperList.";
      break;
    }
  }
  if (!isEmpty(minRange)) {
    if (std::any_of(maxRanges.cbegin(), maxRanges.cend(), [minRange](const auto x) { return x < minRange; })) {
      issues["RangeUpperList"] = "RangeUpperList has a value lower than RangeLower.";
    }
  }

  const MatrixWorkspace_const_sptr workspace = getProperty("InputWorkspace");
  if (!workspace)
    return issues;

  const int startIndex = getProperty("StartWorkspaceIndex");
  const int endIndex = getProperty("EndWorkspaceIndex");
  const auto [firstIndex, lastIndex] =
      workspaceIndexRange(startIndex, endIndex, static_cast<int>(workspace->getNumberHistograms()));
  const auto numberIntegrated = static_cast<std::size_t>(lastIndex - firstIndex + 1);
  for (const auto &[rangeName, list] :
       {std::pair<std::string, const std::vector<double> &>{"RangeLowerList", minRanges},
        std::pair<std::string, const std::vector<double> &>{"RangeUpperList", maxRanges}}) {
    if (!list.empty() && list.size() != numberIntegrated) {
      issues[rangeName] = rangeName + " has " + std::to_string(list.size()) + " values but it should contain " +
                          std::to_string(numberIntegrated) + ".";
    }
  }

  const std::string rangeUnit = getProperty("RangeUnit");
  const bool convertLimits = rangeUnitDiffersFromXUnit(rangeUnit, *workspace);
  if (!convertLimits && std::dynamic_pointer_cast<const EventWorkspace>(workspace) &&
      (!minRanges.empty() || !maxRanges.empty())) {
    issues[minRanges.empty() ? "RangeUpperList" : "RangeLowerList"] =
        "Range lists are not supported for EventWorkspaces unless the limits are converted using RangeUnit.";
  }
  if (!convertLimits)
    return issues;

  const Unit_const_sptr xUnit = workspace->getAxis(0)->unit();
  const Unit_const_sptr limitUnit = UnitFactory::Instance().create(rangeUnit);
  if (!xUnit->isConvertible()) {
    issues["RangeUnit"] = "RangeUnit requires an input workspace with a convertible X unit.";
    return issues;
  }
  if (isEmpty(minRange) && isEmpty(maxRange) && minRanges.empty() && maxRanges.empty()) {
    issues["RangeUnit"] = "RangeUnit was given but there are no integration limits to convert. Give at least one limit "
                          "or leave RangeUnit empty.";
    return issues;
  }
  if (!canConvertQuickly(*limitUnit, *xUnit) || !canConvertQuickly(*xUnit, *limitUnit)) {
    const auto instrument = workspace->getInstrument();
    if (!instrument->getSource() || !instrument->getSample()) {
      issues["InputWorkspace"] = "Converting the integration limits from " + rangeUnit + " to " + xUnit->unitID() +
                                 " requires an instrument with a source and a sample.";
      return issues;
    }
  }
  if (!issues.empty())
    return issues;

  // Every spectrum being integrated must have limits that can be converted
  try {
    const LimitConverter converter(*workspace, rangeUnit);
    for (int i = firstIndex; i <= lastIndex; ++i) {
      const auto [lower, upper] =
          spectrumLimits(static_cast<std::size_t>(i - firstIndex), minRange, maxRange, minRanges, maxRanges);
      // exec sets the integral to zero for these spectra without using the limits
      if (upper < lower)
        continue;
      converter.convert(static_cast<std::size_t>(i), workspace->x(i), lower, upper);
    }
  } catch (const std::exception &error) {
    issues["RangeUnit"] = error.what();
  }
  return issues;
}

} // namespace Mantid::Algorithms
