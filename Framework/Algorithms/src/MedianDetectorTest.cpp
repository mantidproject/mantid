// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MedianDetectorTest.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/BoundedValidator.h"

#include <cmath>

namespace Mantid::Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(MedianDetectorTest)

using namespace Kernel;
using namespace API;
using DataObjects::MaskWorkspace_sptr;
using namespace Geometry;

/// Default constructor
MedianDetectorTest::MedianDetectorTest()
    : DetectorDiagnostic(), m_inputWS(), m_loFrac(0.1), m_hiFrac(1.5), m_minWsIndex(0), m_maxWsIndex(EMPTY_INT()),
      m_rangeLower(0.0), m_rangeUpper(0.0), m_solidAngle(false) {}

const std::string MedianDetectorTest::category() const { return "Diagnostics"; }

/// Declare algorithm properties
void MedianDetectorTest::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<HistogramValidator>()),
                  "Name of the input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "A MaskWorkspace where 0 denotes a masked spectra. Any spectra containing"
                  "a zero is also masked on the output");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  auto mustBePosInt = std::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty("LevelsUp", 0, mustBePosInt,
                  "Levels above pixel that will be used to compute the median.\n"
                  "If no level is specified, or 0, the median is over the whole "
                  "instrument.");
  declareProperty("SignificanceTest", 3.3, mustBePositive,
                  "Error criterion as a multiple of error bar i.e. to fail the "
                  "test, the magnitude of the\n"
                  "difference with respect to the median value must also "
                  "exceed this number of error bars");
  declareProperty("LowThreshold", 0.1, "Lower acceptable bound as fraction of median value");
  declareProperty("HighThreshold", 1.5, "Upper acceptable bound as fraction of median value");
  declareProperty("LowOutlier", 0.01, "Lower bound defining outliers as fraction of median value");
  declareProperty("HighOutlier", 100., "Upper bound defining outliers as fraction of median value");
  declareProperty("ExcludeZeroesFromMedian", false,
                  "If false (default) zeroes will be included in "
                  "the median calculation, otherwise they will not be included "
                  "but they will be left unmasked");

  declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
                  "The index number of the first spectrum to include in the calculation\n"
                  "(default 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePosInt,
                  "The index number of the last spectrum to include in the calculation\n"
                  "(default the last histogram)");
  declareProperty("RangeLower", EMPTY_DBL(),
                  "No bin with a boundary at an x value less than this will be included\n"
                  "in the summation used to decide if a detector is 'bad' (default: the\n"
                  "start of each histogram)");
  declareProperty("RangeUpper", EMPTY_DBL(),
                  "No bin with a boundary at an x value higher than this value will\n"
                  "be included in the summation used to decide if a detector is 'bad'\n"
                  "(default: the end of each histogram)");
  declareProperty("CorrectForSolidAngle", false, "Flag to correct for solid angle efficiency. False by default.");
  declareProperty("NumberOfFailures", 0, Direction::Output);
}

/** Executes the algorithm that includes calls to SolidAngle and Integration
 *
 *  @throw invalid_argument if there is an incompatible property value and so
 *the algorithm can't continue
 *  @throw runtime_error if algorithm cannot execute
 */
void MedianDetectorTest::exec() {
  retrieveProperties();
  // Ensures we have a workspace with a single bin. It will contain any input
  // masking and will be used to record any
  // required masking from this algorithm
  MatrixWorkspace_sptr countsWS =
      integrateSpectra(m_inputWS, m_minWsIndex, m_maxWsIndex, m_rangeLower, m_rangeUpper, true);

  // 0. Correct for solid angle, if desired
  if (m_solidAngle) {
    MatrixWorkspace_sptr solidAngle = getSolidAngles(m_minWsIndex, m_maxWsIndex);
    if (solidAngle != nullptr) {
      countsWS = countsWS / solidAngle;
    }
  }

  // create a vector of vectors of specIDs that will be used to calculate
  // medians
  std::vector<std::vector<size_t>> specmap = makeMap(countsWS);
  const bool excludeZeroes = getProperty("ExcludeZeroesFromMedian");
  MaskWorkspace_sptr maskWS;

  // 1. Calculate the median
  std::vector<double> median = calculateMedian(*countsWS, excludeZeroes, specmap);
  std::vector<double>::iterator medit;
  for (medit = median.begin(); medit != median.end(); ++medit) {
    g_log.debug() << "Median value = " << (*medit) << "\n";
  }
  // 2. Mask outliers
  int numFailed = maskOutliers(median, countsWS, specmap);

  // 3. Recalulate the median
  median = calculateMedian(*countsWS, excludeZeroes, specmap);
  for (medit = median.begin(); medit != median.end(); ++medit) {
    g_log.information() << "Median value with outliers removed = " << (*medit) << "\n";
  }

  maskWS = this->generateEmptyMask(countsWS);
  numFailed += doDetectorTests(countsWS, median, specmap, maskWS);
  g_log.information() << "Median test results:\n"
                      << "\tNumber of failures - " << numFailed << "\n";
  setProperty("NumberOfFailures", numFailed);

  setProperty("OutputWorkspace", maskWS);
}

/** Loads and checks the values passed to the algorithm
 *
 *  @throw invalid_argument if there is an incompatible property value so the
 *algorithm can't continue
 */
void MedianDetectorTest::retrieveProperties() {
  m_inputWS = getProperty("InputWorkspace");
  int maxWsIndex = static_cast<int>(m_inputWS->getNumberHistograms()) - 1;

  m_parents = getProperty("LevelsUp");
  m_minWsIndex = getProperty("StartWorkspaceIndex");
  if ((m_minWsIndex < 0) || (m_minWsIndex > maxWsIndex)) {
    g_log.warning("StartSpectrum out of range, changed to 0");
    m_minWsIndex = 0;
  }
  m_maxWsIndex = getProperty("EndWorkspaceIndex");
  if (m_maxWsIndex == EMPTY_INT())
    m_maxWsIndex = maxWsIndex;
  if ((m_maxWsIndex < 0) || (m_maxWsIndex > maxWsIndex)) {
    g_log.warning("EndSpectrum out of range, changed to max spectrum number");
    m_maxWsIndex = maxWsIndex;
  }
  if ((m_maxWsIndex < m_minWsIndex)) {
    g_log.warning("EndSpectrum can not be less than the StartSpectrum, changed "
                  "to max spectrum number");
    m_maxWsIndex = maxWsIndex;
  }

  m_loFrac = getProperty("LowThreshold");
  m_hiFrac = getProperty("HighThreshold");
  if (m_loFrac > m_hiFrac) {
    throw std::invalid_argument("The threshold for reading high must be "
                                "greater than the low threshold");
  }

  // Integration Range
  m_rangeLower = getProperty("RangeLower");
  m_rangeUpper = getProperty("RangeUpper");

  // Solid angle correction flag
  m_solidAngle = getProperty("CorrectForSolidAngle");
}

/** Makes a workspace with the total solid angle all the detectors in each
 * spectrum cover from the sample
 *  note returns an empty shared pointer on failure, uses the SolidAngle
 * algorithm
 * @param firstSpec :: the index number of the first histogram to analyse
 * @param lastSpec :: the index number of the last histogram to analyse
 * @return A pointer to the workspace (or an empty pointer)
 */
API::MatrixWorkspace_sptr MedianDetectorTest::getSolidAngles(int firstSpec, int lastSpec) {
  g_log.debug("Calculating solid angles");
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_fracDone, t1 = advanceProgress(RTGetSolidAngle);
  auto childAlg = createChildAlgorithm("SolidAngle", t0, t1, true);
  childAlg->setProperty("InputWorkspace", m_inputWS);
  childAlg->setProperty("StartWorkspaceIndex", firstSpec);
  childAlg->setProperty("EndWorkspaceIndex", lastSpec);
  try {
    // Execute the Child Algorithm, it could throw a runtime_error at this point
    // which would abort execution
    childAlg->execute();
    if (!childAlg->isExecuted()) {
      throw std::runtime_error("Unexpected problem calculating solid angles");
    }
  }
  // catch all exceptions because the solid angle calculation is optional
  catch (std::exception &) {
    g_log.warning("Precision warning:  Can't find detector geometry " + name() +
                  " will continue with the solid angles of all spectra set to "
                  "the same value");
    failProgress(RTGetSolidAngle);
    // The return is an empty workspace pointer, which must be handled by the
    // calling function
    MatrixWorkspace_sptr empty;
    // function returns normally
    return empty;
  }
  return childAlg->getProperty("OutputWorkspace");
}

/**
 * Mask the outlier values to get a better median value.
 * @param medianvec The median value calculated from the current counts.
 * @param countsWS The counts workspace. Any outliers will be masked here.
 * @param indexmap Index map.
 * @returns The number failed.
 */
int MedianDetectorTest::maskOutliers(const std::vector<double> &medianvec, const API::MatrixWorkspace_sptr &countsWS,
                                     std::vector<std::vector<size_t>> indexmap) {

  // Fractions of the median
  const double out_lo = getProperty("LowOutlier");
  const double out_hi = getProperty("HighOutlier");

  int numFailed(0);

  bool checkForMask = false;
  Geometry::Instrument_const_sptr instrument = countsWS->getInstrument();
  if (instrument != nullptr) {
    checkForMask = ((instrument->getSource() != nullptr) && (instrument->getSample() != nullptr));
  }
  auto &spectrumInfo = countsWS->mutableSpectrumInfo();

  for (size_t i = 0; i < indexmap.size(); ++i) {
    std::vector<size_t> &hists = indexmap[i];
    double median = medianvec[i];

    PARALLEL_FOR_IF(Kernel::threadSafe(*countsWS))
    for (int j = 0; j < static_cast<int>(hists.size()); ++j) { // NOLINT
      const double value = countsWS->y(hists[j])[0];
      if ((value == 0.) && checkForMask) {
        if (spectrumInfo.hasDetectors(hists[j]) && spectrumInfo.isMasked(hists[j])) {
          numFailed -= 1; // it was already masked
        }
      }
      if ((value < out_lo * median) && (value > 0.0)) {
        countsWS->getSpectrum(hists[j]).clearData();
        PARALLEL_CRITICAL(setMasked) {
          spectrumInfo.setMasked(hists[j], true);
          ++numFailed;
        }
      } else if (value > out_hi * median) {
        countsWS->getSpectrum(hists[j]).clearData();
        PARALLEL_CRITICAL(setMasked) {
          spectrumInfo.setMasked(hists[j], true);
          ++numFailed;
        }
      }
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }

  return numFailed;
}

/**
 * Takes a single valued histogram workspace and assesses which histograms are
 * within the limits.
 * Those that are not are masked on the input workspace.
 * @param countsWS :: Input/Output Integrated workspace to diagnose.
 * @param medianvec The median value calculated from the current counts.
 * @param indexmap Index map.
 * @param maskWS :: A mask workspace to apply.
 * @return The number of detectors that failed the tests, not including those
 * skipped.
 */
int MedianDetectorTest::doDetectorTests(const API::MatrixWorkspace_sptr &countsWS, const std::vector<double> &medianvec,
                                        const std::vector<std::vector<size_t>> &indexmap,
                                        const API::MatrixWorkspace_sptr &maskWS) {
  g_log.debug("Applying the criteria to find failing detectors");

  // A spectra can't fail if the statistics show its value is consistent with
  // the mean value,
  // check the error and how many errorbars we are away
  const double minSigma = getProperty("SignificanceTest");

  // prepare to report progress
  const int numSpec(m_maxWsIndex - m_minWsIndex);
  const auto progStep = static_cast<int>(ceil(numSpec / 30.0));
  int steps(0);

  const double deadValue(1.0);
  int numFailed(0);

  bool checkForMask = false;
  Geometry::Instrument_const_sptr instrument = countsWS->getInstrument();
  if (instrument != nullptr) {
    checkForMask = ((instrument->getSource() != nullptr) && (instrument->getSample() != nullptr));
  }
  const auto &spectrumInfo = countsWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*countsWS, *maskWS))
  for (int j = 0; j < static_cast<int>(indexmap.size()); ++j) {
    std::vector<size_t> hists = indexmap.at(j);
    double median = medianvec.at(j);
    const size_t nhist = hists.size();
    g_log.debug() << "new component with " << nhist << " spectra.\n";
    for (size_t i = 0; i < nhist; ++i) {
      g_log.debug() << "Counts workspace index=" << i << ", Mask workspace index=" << hists.at(i) << '\n';
      PARALLEL_START_INTERRUPT_REGION
      ++steps;
      // update the progressbar information
      if (steps % progStep == 0) {
        progress(advanceProgress(progStep * static_cast<double>(RTMarkDetects) / numSpec));
      }

      if (checkForMask && spectrumInfo.hasDetectors(hists.at(i))) {
        if (spectrumInfo.isMasked(hists.at(i))) {
          maskWS->mutableY(hists.at(i))[0] = deadValue;
          continue;
        }
        if (spectrumInfo.isMonitor(hists.at(i))) {
          // Don't include in calculation but don't mask it
          continue;
        }
      }

      const double signal = countsWS->y(hists.at(i))[0];

      // Mask out NaN and infinite
      if (!std::isfinite(signal)) {
        maskWS->mutableY(hists.at(i))[0] = deadValue;
        PARALLEL_ATOMIC
        ++numFailed;
        continue;
      }

      const double error = minSigma * countsWS->e(hists.at(i))[0];

      if ((signal < median * m_loFrac && (signal - median < -error)) ||
          (signal > median * m_hiFrac && (signal - median > error))) {
        maskWS->mutableY(hists.at(i))[0] = deadValue;
        PARALLEL_ATOMIC
        ++numFailed;
      }

      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    // Log finds
    g_log.information() << numFailed << " spectra failed the median tests.\n";
  }
  return numFailed;
}

} // namespace Mantid::Algorithms
