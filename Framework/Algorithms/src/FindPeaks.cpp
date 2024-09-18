// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidAlgorithms/SmoothData.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/FitPeak.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/StartsWithValidator.h"
#include "MantidKernel/VectorHelper.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions/round.hpp>
#include <numeric>

#include <fstream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Indexing;

// const double MINHEIGHT = 2.00000001;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindPeaks)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FindPeaks::FindPeaks()
    : m_peakParameterNames(), m_bkgdParameterNames(), m_bkgdOrder(0), m_outPeakTableWS(), m_dataWS(),
      m_inputPeakFWHM(0), m_highBackground(false), m_rawPeaksTable(false), m_numTableParams(0),
      m_centreIndex(1) /* for Gaussian */, m_peakFuncType(""), m_backgroundType(""), m_vecPeakCentre(),
      m_vecFitWindows(), m_backgroundFunction(), m_peakFunction(), m_minGuessedPeakWidth(0), m_maxGuessedPeakWidth(0),
      m_stepGuessedPeakWidth(0), m_usePeakPositionTolerance(false), m_peakPositionTolerance(0.0), m_fitFunctions(),
      m_peakLeftIndexes(), m_peakRightIndexes(), m_minimizer("Levenberg-MarquardtMD"), m_costFunction(),
      m_minHeight(0.0), m_leastMaxObsY(0.), m_useObsCentre(false) {}

//----------------------------------------------------------------------------------------------
/** Initialize and declare properties.
 */
void FindPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "Name of the workspace to search");

  auto mustBeNonNegative = std::make_shared<BoundedValidator<int>>();
  mustBeNonNegative->setLower(0);
  declareProperty("WorkspaceIndex", EMPTY_INT(), mustBeNonNegative,
                  "If set, only this spectrum will be searched for peaks "
                  "(otherwise all are)");

  auto min = std::make_shared<BoundedValidator<int>>();
  min->setLower(1);
  // The estimated width of a peak in terms of number of channels
  declareProperty("FWHM", 7, min, "Estimated number of points covered by the fwhm of a peak (default 7)");

  // The tolerance allowed in meeting the conditions
  declareProperty("Tolerance", 4, min,
                  "A measure of the strictness desired in "
                  "meeting the condition on peak "
                  "candidates,\n"
                  "Mariscotti recommends 2 (default 4)");

  declareProperty(std::make_unique<ArrayProperty<double>>("PeakPositions"),
                  "Optional: enter a comma-separated list of the expected "
                  "X-position of the centre of the peaks. Only peaks near "
                  "these positions will be fitted.");

  declareProperty(std::make_unique<ArrayProperty<double>>("FitWindows"),
                  "Optional: enter a comma-separated list of the expected "
                  "X-position of windows to fit. The number of values must be "
                  "exactly double the number of specified peaks.");

  std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<API::IPeakFunction>();
  declareProperty("PeakFunction", "Gaussian", std::make_shared<StringListValidator>(peakNames));

  std::vector<std::string> bkgdtypes{"Flat", "Linear", "Quadratic"};
  declareProperty("BackgroundType", "Linear", std::make_shared<StringListValidator>(bkgdtypes), "Type of Background.");

  declareProperty("HighBackground", true, "Flag whether the input data has high background compared to peak heights.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty("MinGuessedPeakWidth", 2, mustBePositive,
                  "Minimum guessed peak width for fit. It is in unit of number of pixels.");

  declareProperty("MaxGuessedPeakWidth", 10, mustBePositive,
                  "Maximum guessed peak width for fit. It is in unit of number of pixels.");

  declareProperty("GuessedPeakWidthStep", 2, mustBePositive,
                  "Step of guessed peak width. It is in unit of number of pixels.");

  auto mustBePositiveDBL = std::make_shared<BoundedValidator<double>>();
  declareProperty("PeakPositionTolerance", EMPTY_DBL(), mustBePositiveDBL,
                  "Tolerance on the found peaks' positions against the input "
                  "peak positions.  Non-positive value indicates that this "
                  "option is turned off.");

  // The found peaks in a table
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("PeaksList", "", Direction::Output),
                  "The name of the TableWorkspace in which to store the list "
                  "of peaks found");

  declareProperty("RawPeakParameters", false,
                  "false generates table with effective centre/width/height "
                  "parameters. true generates a table with peak function "
                  "parameters");

  declareProperty("MinimumPeakHeight", DBL_MIN, "Minimum allowed peak height. ");

  declareProperty("MinimumPeakHeightObs", 0.0,
                  "Least value of the maximum observed Y value of a peak within "
                  "specified region.  If any peak's maximum observed Y value is smaller, "
                  "then "
                  "this peak will not be fit.  It is designed for EventWorkspace with "
                  "integer counts.");

  std::array<std::string, 2> costFuncOptions = {{"Chi-Square", "Rwp"}};
  declareProperty("CostFunction", "Chi-Square",
                  Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(costFuncOptions)), "Cost functions");

  std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();

  declareProperty("Minimizer", "Levenberg-MarquardtMD",
                  Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)),
                  "Minimizer to use for fitting. Minimizers available are "
                  "\"Levenberg-Marquardt\", \"Simplex\","
                  "\"Conjugate gradient (Fletcher-Reeves imp.)\", \"Conjugate "
                  "gradient (Polak-Ribiere imp.)\", \"BFGS\", and "
                  "\"Levenberg-MarquardtMD\"");

  declareProperty("StartFromObservedPeakCentre", true, "Use observed value as the starting value of peak centre. ");
}

//----------------------------------------------------------------------------------------------
/** Execute the findPeaks algorithm.
 */
void FindPeaks::exec() {
  // Process input
  processAlgorithmProperties();

  // Create those functions to fit
  createFunctions();

  // Set up output table workspace
  generateOutputPeakParameterTable();

  // Fit
  if (!m_vecPeakCentre.empty()) {
    if (!m_vecFitWindows.empty()) {
      if (m_vecFitWindows.size() != (m_vecPeakCentre.size() * 2)) {
        throw std::invalid_argument("Number of FitWindows must be exactly "
                                    "twice the number of PeakPositions");
      }
    }

    // Perform fit with fixed start positions.
    findPeaksGivenStartingPoints(m_vecPeakCentre, m_vecFitWindows);
  } else {
    // Use Mariscotti's method to find the peak centers
    m_usePeakPositionTolerance = false;
    this->findPeaksUsingMariscotti();
  }

  // Set output properties
  g_log.information() << "Total " << m_outPeakTableWS->rowCount() << " peaks found and successfully fitted.\n";
  setProperty("PeaksList", m_outPeakTableWS);
} // END: exec()

//----------------------------------------------------------------------------------------------
/** Process algorithm's properties
 */
void FindPeaks::processAlgorithmProperties() {
  // Input workspace
  m_dataWS = getProperty("InputWorkspace");

  // WorkspaceIndex
  int wsIndex = getProperty("WorkspaceIndex");
  if (!isEmpty(wsIndex)) {
    if (wsIndex >= static_cast<int>(m_dataWS->getNumberHistograms())) {
      g_log.warning() << "The value of WorkspaceIndex provided (" << wsIndex
                      << ") is larger than the size of this workspace (" << m_dataWS->getNumberHistograms() << ")\n";
      throw Kernel::Exception::IndexError(wsIndex, m_dataWS->getNumberHistograms() - 1,
                                          "FindPeaks WorkspaceIndex property");
    }
    m_indexSet = m_dataWS->indexInfo().makeIndexSet({static_cast<Indexing::GlobalSpectrumIndex>(wsIndex)});
  } else {
    m_indexSet = m_dataWS->indexInfo().makeIndexSet();
  }

  // Peak width
  m_inputPeakFWHM = getProperty("FWHM");
  int t1 = getProperty("MinGuessedPeakWidth");
  int t2 = getProperty("MaxGuessedPeakWidth");
  int t3 = getProperty("GuessedPeakWidthStep");
  if (t1 > t2 || t1 <= 0 || t3 <= 0) {
    std::stringstream errss;
    errss << "User specified wrong guessed peak width parameters (must be "
             "postive and make sense). "
          << "User inputs are min = " << t1 << ", max = " << t2 << ", step = " << t3;
    g_log.warning(errss.str());
    throw std::runtime_error(errss.str());
  }

  m_minGuessedPeakWidth = t1;
  m_maxGuessedPeakWidth = t2;
  m_stepGuessedPeakWidth = t3;

  m_peakPositionTolerance = getProperty("PeakPositionTolerance");
  m_usePeakPositionTolerance = true;
  if (isEmpty(m_peakPositionTolerance))
    m_usePeakPositionTolerance = false;

  // Specified peak positions, which is optional
  m_vecPeakCentre = getProperty("PeakPositions");
  if (!m_vecPeakCentre.empty())
    std::sort(m_vecPeakCentre.begin(), m_vecPeakCentre.end());
  m_vecFitWindows = getProperty("FitWindows");

  // Peak and ground type
  m_peakFuncType = getPropertyValue("PeakFunction");
  m_backgroundType = getPropertyValue("BackgroundType");

  // Fit algorithm
  m_highBackground = getProperty("HighBackground");

  // Peak parameters are give via a table workspace
  m_rawPeaksTable = getProperty("RawPeakParameters");

  // Minimum peak height
  m_minHeight = getProperty("MinimumPeakHeight");

  // About Fit
  m_minimizer = getPropertyValue("Minimizer");
  m_costFunction = getPropertyValue("CostFunction");

  m_useObsCentre = getProperty("StartFromObservedPeakCentre");

  m_leastMaxObsY = getProperty("MinimumPeakHeightObs");
}

//----------------------------------------------------------------------------------------------
/** Generate a table workspace for output peak parameters
 */
void FindPeaks::generateOutputPeakParameterTable() {
  m_outPeakTableWS = std::make_shared<TableWorkspace>();
  m_outPeakTableWS->addColumn("int", "spectrum");

  if (m_rawPeaksTable) {
    // Output raw peak parameters
    size_t numpeakpars = m_peakFunction->nParams();
    size_t numbkgdpars = m_backgroundFunction->nParams();
    m_numTableParams = numpeakpars + numbkgdpars;
    if (m_peakFuncType == "Gaussian")
      m_centreIndex = 1;
    else if (m_peakFuncType == "LogNormal")
      m_centreIndex = 1;
    else if (m_peakFuncType == "Lorentzian")
      m_centreIndex = 1;
    else if (m_peakFuncType == "PseudoVoigt")
      m_centreIndex = 2;
    else
      m_centreIndex = m_numTableParams; // bad value

    for (size_t i = 0; i < numpeakpars; ++i)
      m_outPeakTableWS->addColumn("double", m_peakParameterNames[i]);
    for (size_t i = 0; i < numbkgdpars; ++i)
      m_outPeakTableWS->addColumn("double", m_bkgdParameterNames[i]);
    // m_outPeakTableWS->addColumn("double", "f1.A2");
  } else {
    // Output centre, weight, height, A0, A1 and A2
    m_numTableParams = 6;
    m_centreIndex = 0;
    m_outPeakTableWS->addColumn("double", "centre");
    m_outPeakTableWS->addColumn("double", "width");
    m_outPeakTableWS->addColumn("double", "height");
    m_outPeakTableWS->addColumn("double", "backgroundintercept");
    m_outPeakTableWS->addColumn("double", "backgroundslope");
    m_outPeakTableWS->addColumn("double", "A2");
  }

  m_outPeakTableWS->addColumn("double", "chi2");
}

//----------------------------------------------------------------------------------------------
/** Find the start positions to fit peaks with given estimated peak centres
 * @param peakcentres :: vector of the center x-positions specified to perform
 * fits.
 * @param fitwindows :: vector of windows around each peak. Otherwise, windows
 * will be determined automatically.
 */
void FindPeaks::findPeaksGivenStartingPoints(const std::vector<double> &peakcentres,
                                             const std::vector<double> &fitwindows) {
  bool useWindows = (!fitwindows.empty());
  std::size_t numPeaks = peakcentres.size();

  // Loop over the spectra searching for peaks
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, m_indexSet.size());

  for (const auto spec : m_indexSet) {
    const auto &vecX = m_dataWS->x(spec);

    double practical_x_min = vecX.front();
    double practical_x_max = vecX.back();
    g_log.information() << "actual x-range = [" << practical_x_min << " -> " << practical_x_max << "]\n";
    {
      const auto &vecY = m_dataWS->y(spec);
      const auto &vecE = m_dataWS->e(spec);
      const size_t numY = vecY.size();
      size_t i_min = 1;
      for (; i_min < numY; ++i_min) {
        if ((vecY[i_min] != 0.) || (vecE[i_min] != 0)) {
          --i_min; // bring it back one
          break;
        }
      }
      practical_x_min = vecX[i_min];

      size_t i_max = numY - 2;
      for (; i_max > i_min; --i_max) {
        if ((vecY[i_max] != 0.) || (vecE[i_max] != 0)) {
          ++i_max; // bring it back one
          break;
        }
      }
      g_log.debug() << "Finding peaks from giving starting point, with interval i_min = " << i_min
                    << " i_max = " << i_max << '\n';
      practical_x_max = vecX[i_max];
    }
    g_log.information() << "practical x-range = [" << practical_x_min << " -> " << practical_x_max << "]\n";

    for (std::size_t ipeak = 0; ipeak < numPeaks; ipeak++) {
      // Try to fit at this center
      double x_center = peakcentres[ipeak];

      std::stringstream infoss;
      infoss << "Spectrum " << spec << ": Fit peak @ d = " << x_center;
      if (useWindows) {
        infoss << " inside fit window [" << fitwindows[2 * ipeak] << ", " << fitwindows[2 * ipeak + 1] << "]";
      }
      g_log.information(infoss.str());

      // Check whether it is the in data range
      if (x_center > practical_x_min && x_center < practical_x_max) {
        if (useWindows)
          fitPeakInWindow(m_dataWS, static_cast<int>(spec), x_center, fitwindows[2 * ipeak], fitwindows[2 * ipeak + 1]);
        else {
          bool hasLeftPeak = (ipeak > 0);
          double leftpeakcentre = 0.;
          if (hasLeftPeak)
            leftpeakcentre = peakcentres[ipeak - 1];

          bool hasRightPeak = (ipeak < numPeaks - 1);
          double rightpeakcentre = 0.;
          if (hasRightPeak)
            rightpeakcentre = peakcentres[ipeak + 1];

          fitPeakGivenFWHM(m_dataWS, static_cast<int>(spec), x_center, m_inputPeakFWHM, hasLeftPeak, leftpeakcentre,
                           hasRightPeak, rightpeakcentre);
        }
      } else {
        g_log.warning() << "Given peak centre " << x_center << " is out side of given data's range (" << practical_x_min
                        << ", " << practical_x_max << ").\n";
        addNonFitRecord(spec, x_center);
      }

    } // loop through the peaks specified

    m_progress->report();

  } // loop over spectra
}

//----------------------------------------------------------------------------------------------
/** Use the Mariscotti method to find the start positions and fit gaussian peaks
 */
void FindPeaks::findPeaksUsingMariscotti() {
  // At this point the data has not been smoothed yet.
  auto smoothedData = this->calculateSecondDifference(m_dataWS);

  // The optimum number of points in the smoothing, according to Mariscotti, is
  // 0.6*fwhm
  auto w = static_cast<int>(0.6 * m_inputPeakFWHM);
  // w must be odd
  if (!(w % 2))
    ++w;

  if (!m_dataWS->isRaggedWorkspace() && (m_dataWS->blocksize() <= static_cast<size_t>(w))) {
    std::stringstream errss;
    errss << "Block size of the workspace should be greater than:" << w;
    throw std::runtime_error(errss.str());
  }

  smoothData(smoothedData, w, g_z);

  // Now calculate the errors on the smoothed data
  this->calculateStandardDeviation(m_dataWS, smoothedData, w);

  // Calculate n1 (Mariscotti eqn. 18)
  const double kz = 1.22; // This kz corresponds to z=5 & w=0.6*fwhm - see Mariscotti Fig. 8
  const int n1 = boost::math::iround(kz * m_inputPeakFWHM);
  // Can't calculate n2 or n3 yet because they need i0
  const int tolerance = getProperty("Tolerance");

  // Loop over the spectra searching for peaks
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, m_indexSet.size());

  for (size_t k_out = 0; k_out < m_indexSet.size(); ++k_out) {
    const size_t k = m_indexSet[k_out];
    const auto &S = smoothedData[k_out].y();
    const auto &F = smoothedData[k_out].e();

    // This implements the flow chart given on page 320 of Mariscotti
    int i0 = 0, i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;
    for (int i = 1; i < static_cast<int>(S.size()); ++i) {

      int M = 0;
      if (S[i] > F[i])
        M = 1;
      else {
        S[i] > 0 ? M = 2 : M = 3;
      }

      if (S[i - 1] > F[i - 1]) {
        switch (M) {
        case 3:
          i3 = i;
        /* no break */
        // intentional fall-through
        case 2:
          i2 = i - 1;
          break;
        case 1:
          // do nothing
          break;
        default:
          assert(false);
          // should never happen
          break;
        }
      } else if (S[i - 1] > 0) {
        switch (M) {
        case 3:
          i3 = i;
          break;
        case 2:
          // do nothing
          break;
        case 1:
          i1 = i;
          break;
        default:
          assert(false);
          // should never happen
          break;
        }
      } else {
        switch (M) {
        case 3:
          // do nothing
          break;
        case 2: // fall through (i.e. same action if M = 1 or 2)
        case 1:
          i5 = i - 1;
          break;
        default:
          assert(false);
          // should never happen
          break;
        }
      }

      if (i5 && i1 && i2 && i3) // If i5 has been set then we should have the
                                // full set and can check conditions
      {
        i4 = i3; // Starting point for finding i4 - calculated below
        double num = 0.0, denom = 0.0;
        for (int j = i3; j <= i5; ++j) {
          // Calculate i4 - it's at the minimum value of Si between i3 & i5
          if (S[j] <= S[i4])
            i4 = j;
          // Calculate sums for i0 (Mariscotti eqn. 27)
          num += j * S[j];
          denom += S[j];
        }
        i0 = static_cast<int>(num / denom);

        // Check we have a correctly ordered set of points. If not, reset and
        // continue
        if (i1 > i2 || i2 > i3 || i3 > i4 || i5 <= i4) {
          i5 = 0;
          continue;
        }

        // Check if conditions are fulfilled - if any are not, loop onto the
        // next i in the spectrum
        // Mariscotti eqn. (14)
        if (std::abs(S[i4]) < 2 * F[i4]) {
          i5 = 0;
          continue;
        }
        // Mariscotti eqn. (19)
        if (abs(i5 - i3 + 1 - n1) > tolerance) {
          i5 = 0;
          continue;
        }
        // Calculate n2 (Mariscotti eqn. 20)
        int n2 = std::abs(boost::math::iround(0.5 * (F[i0] / S[i0]) * (n1 + tolerance)));
        const int n2b = std::abs(boost::math::iround(0.5 * (F[i0] / S[i0]) * (n1 - tolerance)));
        if (n2b > n2)
          n2 = n2b;
        // Mariscotti eqn. (21)
        const int testVal = n2 ? n2 : 1;
        if (i3 - i2 - 1 > testVal) {
          i5 = 0;
          continue;
        }
        // Calculate n3 (Mariscotti eqn. 22)
        int n3 = std::abs(boost::math::iround((n1 + tolerance) * (1 - 2 * (F[i0] / S[i0]))));
        const int n3b = std::abs(boost::math::iround((n1 - tolerance) * (1 - 2 * (F[i0] / S[i0]))));
        if (n3b < n3)
          n3 = n3b;
        // Mariscotti eqn. (23)
        if (i2 - i1 + 1 < n3) {
          i5 = 0;
          continue;
        }

        // If we get to here then we've identified a peak
        g_log.debug() << "Spectrum=" << k << " i0=" << i0 << " X=" << m_dataWS->x(k)[i0] << " i1=" << i1 << " i2=" << i2
                      << " i3=" << i3 << " i4=" << i4 << " i5=" << i5 << '\n';

        // Use i0, i2 and i4 to find out i_min and i_max, i0: right, i2: left,
        // i4: centre
        auto wssize = static_cast<int>(m_dataWS->x(k).size());

        int iwidth = i0 - i2;
        if (iwidth <= 0)
          iwidth = 1;

        int i_min = 1;
        if (i4 > 5 * iwidth)
          i_min = i4 - 5 * iwidth;

        int i_max = i4 + 5 * iwidth;
        if (i_max >= wssize)
          i_max = wssize - 1;

        this->fitSinglePeak(m_dataWS, static_cast<int>(k), i_min, i_max, i4);

        // reset and go searching for the next peak
        i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;
      }

    } // loop through a single spectrum

    m_progress->report();

  } // loop over spectra
}

//----------------------------------------------------------------------------------------------
/** Calculates the second difference of the data (Y values) in a workspace.
 *  Done according to equation (3) in Mariscotti: \f$ S_i = N_{i+1} - 2N_i +
 * N_{i+1} \f$.
 *  In the output workspace, the 2nd difference is in Y, X is unchanged and E
 * is zero.
 *  @param input :: The workspace to calculate the second difference of
 *  @return A workspace containing the second difference
 */
std::vector<Histogram> FindPeaks::calculateSecondDifference(const API::MatrixWorkspace_const_sptr &input) {
  std::vector<Histogram> diffed;

  // Loop over spectra
  for (const auto i : m_indexSet) {
    diffed.emplace_back(input->histogram(i));
    diffed.back().mutableY() = 0.0;
    diffed.back().mutableE() = 0.0;

    const auto &Y = input->y(i);
    auto &S = diffed.back().mutableY();
    // Go through each spectrum calculating the second difference at each point
    // First and last points in each spectrum left as zero (you'd never be able
    // to find peaks that close to the edge anyway)
    for (size_t j = 1; j < S.size() - 1; ++j) {
      S[j] = Y[j - 1] - 2 * Y[j] + Y[j + 1];
    }
  }

  return diffed;
}

//----------------------------------------------------------------------------------------------
/** Smooth data for Mariscotti.
 *  @param histograms :: Vector of histograms to be smoothed (inplace).
 *  @param w ::  The number of data points which should contribute to each
 * smoothed point
 *  @param g_z :: The number of smoothing steps given by g_z (should be 5)
 */
void FindPeaks::smoothData(std::vector<Histogram> &histograms, const int w, const int g_z) {
  for (auto &histogram : histograms)
    for (int i = 0; i < g_z; ++i)
      histogram = smooth(histogram, w);
}

//----------------------------------------------------------------------------------------------
/** Calculates the statistical error on the smoothed data.
 *  Uses Mariscotti equation (11), amended to use errors of input data rather
 * than sqrt(Y).
 *  @param input ::    The input data to the algorithm
 *  @param smoothed :: The smoothed dataBackgroud type is not supported in
 * FindPeak.cpp
 *  @param w ::        The value of w (the size of the smoothing 'window')
 *  @throw std::invalid_argument if w is greater than 19
 */
void FindPeaks::calculateStandardDeviation(const API::MatrixWorkspace_const_sptr &input,
                                           std::vector<HistogramData::Histogram> &smoothed, const int &w) {
  // Guard against anyone changing the value of z, which would mean different
  // phi values were needed (see Marriscotti p.312)
  static_assert(g_z == 5, "Value of z has changed!");
  // Have to adjust for fact that I normalise Si (unlike the paper)
  const auto factor = static_cast<int>(std::pow(static_cast<double>(w), g_z));

  const double constant = sqrt(static_cast<double>(this->computePhi(w))) / factor;

  for (size_t i = 0; i < m_indexSet.size(); ++i) {
    size_t i_in = m_indexSet[i];
    smoothed[i].mutableE() = input->e(i_in) * constant;
  }
}

//----------------------------------------------------------------------------------------------
/** Calculates the coefficient phi which goes into the calculation of the error
 * on the smoothed data
 *  Uses Mariscotti equation (11). Pinched from the GeneralisedSecondDifference
 * code.
 *  Can return a very big number, hence the type.
 *  @param  w The value of w (the size of the smoothing 'window')
 *  @return The value of phi(g_z,w)
 */
long long FindPeaks::computePhi(const int &w) const {
  const int m = (w - 1) / 2;
  int zz = 0;
  int max_index_prev = 1;
  int n_el_prev = 3;
  std::vector<long long> previous(n_el_prev);
  previous[0] = 1;
  previous[1] = -2;
  previous[2] = 1;

  // Can't happen at present
  if (g_z == 0)
    return std::accumulate(previous.begin(), previous.end(), static_cast<long long>(0),
                           VectorHelper::SumSquares<long long>());

  std::vector<long long> next;
  // Calculate the Cij iteratively.
  do {
    zz++;
    int max_index = zz * m + 1;
    int n_el = 2 * max_index + 1;
    next.resize(n_el);
    std::fill(next.begin(), next.end(), 0);
    for (int i = 0; i < n_el; ++i) {
      int delta = -max_index + i;
      for (int l = delta - m; l <= delta + m; l++) {
        int index = l + max_index_prev;
        if (index >= 0 && index < n_el_prev)
          next[i] += previous[index];
      }
    }
    previous.resize(n_el);
    std::copy(next.begin(), next.end(), previous.begin());
    max_index_prev = max_index;
    n_el_prev = n_el;
  } while (zz != g_z);

  const long long retval = std::accumulate(previous.begin(), previous.end(), static_cast<long long>(0),
                                           VectorHelper::SumSquares<long long>());
  g_log.debug() << "FindPeaks::computePhi - calculated value = " << retval << "\n";
  return retval;
}

//----------------------------------------------------------------------------------------------
/** Find the index of a value (or nearest) in a given the X data
 * @param vecX :: vector
 * @param x :: value to search
 * @return index of x in vector
 */
int FindPeaks::getIndex(const HistogramX &vecX, double x) {
  int index;

  if (x <= vecX.front()) {
    // Left or equal to lower boundary
    index = 0;
  } else if (x >= vecX.back()) {
    // Right or equal to upper boundary
    index = static_cast<int>(vecX.size()) - 1;
  } else {
    // within the range
    index = static_cast<int>(std::lower_bound(vecX.begin(), vecX.end(), x) - vecX.begin());

    // check lower boundary
    if (index == 0) {
      std::stringstream errss;
      errss << "Returned index = 0 for x = " << x << " with X[0] = " << vecX[0]
            << ". This situation is ruled out in this algorithm.";
      g_log.warning(errss.str());
      throw std::runtime_error(errss.str());
    } else if (x < vecX[index - 1] || x > vecX[index]) {
      std::stringstream errss;
      errss << "Returned x = " << x << " is not between " << vecX[index - 1] << " and " << vecX[index]
            << ", which are returned by lower_bound.";
      g_log.warning(errss.str());
      throw std::runtime_error(errss.str());
    }

    // Find the index of the nearest value to return
    if (x - vecX[index - 1] < vecX[index] - x)
      --index;
  }

  return index;
}

//----------------------------------------------------------------------------------------------
/** Attempts to fit a candidate peak given a center and width guess.
 * (This is not the CORE fit peak method)
 *
 *  @param input ::    The input workspace
 *  @param wsIndex :: The workspace index of the peak
 *  @param center_guess :: A guess of the X-value of the center of the peak, in
 *whatever units of the X-axis of the workspace.
 *  @param fitWidth :: A guess of the full-width-half-max of the peak, in # of
 *bins.
 *  @param hasleftpeak :: flag to show that there is a specified peak to its
 *left
 *  @param leftpeakcentre :: centre of left peak if existed
 *  @param hasrightpeak :: flag to show that there is a specified peak to its
 *right
 *  @param rightpeakcentre :: centre of the right peak if existed
 */
void FindPeaks::fitPeakGivenFWHM(const API::MatrixWorkspace_sptr &input, const int wsIndex, const double center_guess,
                                 const int fitWidth, const bool hasleftpeak, const double leftpeakcentre,
                                 const bool hasrightpeak, const double rightpeakcentre) {
  // The X axis you are looking at
  const auto &vecX = input->x(wsIndex);
  const auto &vecY = input->y(wsIndex);

  // Find i_center - the index of the center - The guess is within the X axis?
  int i_centre = this->getIndex(vecX, center_guess);

  // Set up lower fit boundary
  int i_min = i_centre - 5 * fitWidth;
  if (i_min < 1)
    i_min = 1;

  if (hasleftpeak) {
    // Use 2/3 distance as the seperation for right peak
    double xmin = vecX[i_min];
    double peaksepline = center_guess - (center_guess - leftpeakcentre) * 0.66;
    if (xmin < peaksepline)
      i_min = getIndex(vecX, peaksepline);
  }

  // Set up upper boundary
  int i_max = i_centre + 5 * fitWidth;
  if (i_max >= static_cast<int>(vecX.size()) - 1)
    i_max = static_cast<int>(vecY.size()) - 2;

  if (hasrightpeak) {
    // Use 2/3 distance as the separation for right peak
    double xmax = vecX[i_max];
    double peaksepline = center_guess + (rightpeakcentre - center_guess) * 0.66;
    if (xmax > peaksepline)
      i_max = getIndex(vecX, peaksepline);
  }

  // Check
  if (i_max - i_min <= 0)
    throw std::runtime_error("Impossible to i_min >= i_max.");

  std::stringstream outss;
  outss << "Fit peak with guessed FWHM:  starting center = " << center_guess << ", FWHM = " << fitWidth
        << ". Estimated peak fit window from given FWHM: " << vecX[i_min] << ", " << vecX[i_max];
  g_log.information(outss.str());

  fitSinglePeak(input, wsIndex, i_min, i_max, i_centre);
}

//----------------------------------------------------------------------------------------------
/** Attempts to fit a candidate peak with a given window of where peak resides
 *
 *  @param input    The input workspace
 *  @param wsIndex The workspace index of the peak
 *  @param centre_guess ::  Channel number of peak candidate i0 - the higher
 *side of the peak (right side)
 *  @param xmin    Minimum x value to find the peak
 *  @param xmax    Maximum x value to find the peak
 */
void FindPeaks::fitPeakInWindow(const API::MatrixWorkspace_sptr &input, const int wsIndex, const double centre_guess,
                                const double xmin, const double xmax) {
  // Check
  g_log.information() << "Fit Peak with given window:  Guessed center = " << centre_guess << "  x-min = " << xmin
                      << ", x-max = " << xmax << "\n";
  if (xmin >= centre_guess || xmax <= centre_guess) {
    g_log.warning("Peak centre is on the edge of Fit window. ");
    addNonFitRecord(wsIndex, centre_guess);
    return;
  }

  // The X axis you are looking at
  const auto &vecX = input->x(wsIndex);

  // The centre index
  int i_centre = this->getIndex(vecX, centre_guess);

  // The left index
  int i_min = getIndex(vecX, xmin);
  if (i_min >= i_centre) {
    g_log.warning() << "Input peak centre @ " << centre_guess << " is out side of minimum x = " << xmin
                    << ".  Input X ragne = " << vecX.front() << ", " << vecX.back() << "\n";
    addNonFitRecord(wsIndex, centre_guess);
    return;
  }

  // The right index
  int i_max = getIndex(vecX, xmax);
  if (i_max < i_centre) {
    g_log.warning() << "Input peak centre @ " << centre_guess << " is out side of maximum x = " << xmax << "\n";
    addNonFitRecord(wsIndex, centre_guess);
    return;
  }

  // finally do the actual fit
  fitSinglePeak(input, wsIndex, i_min, i_max, i_centre);
}

//----------------------------------------------------------------------------------------------
/** Fit a single peak
 * This is the fundametary peak fit function used by all kinds of input
 */
void FindPeaks::fitSinglePeak(const API::MatrixWorkspace_sptr &input, const int spectrum, const int i_min,
                              const int i_max, const int i_centre) {
  const auto &vecX = input->x(spectrum);
  const auto &vecY = input->y(spectrum);

  // Exclude peak with peak counts
  bool hasHighCounts = false;
  for (int i = i_min; i <= i_max; ++i)
    if (vecY[i] > m_leastMaxObsY) {
      hasHighCounts = true;
      break;
    }
  if (!hasHighCounts) {
    std::stringstream ess;
    ess << "Peak supposed at " << vecY[i_centre] << " does not have enough counts as " << m_leastMaxObsY;
    g_log.debug(ess.str());
    addNonFitRecord(spectrum, vecY[i_centre]);
    return;
  }

  {
    std::stringstream outss;
    outss << "Fit single peak in X-range " << vecX[i_min] << ", " << vecX[i_max] << ", centre at " << vecX[i_centre]
          << " (index = " << i_centre << "). ";
    g_log.information(outss.str());
  }

  // Estimate background
  std::vector<double> vecbkgdparvalue(3, 0.);
  std::vector<double> vecpeakrange(3, 0.);
  int usefpdresult = findPeakBackground(input, spectrum, i_min, i_max, vecbkgdparvalue, vecpeakrange);
  if (usefpdresult < 0) {
    // Estimate background roughly for a failed case
    estimateBackground(vecX, vecY, i_min, i_max, vecbkgdparvalue);
  }

  for (size_t i = 0; i < vecbkgdparvalue.size(); ++i)
    if (i < m_bkgdOrder)
      m_backgroundFunction->setParameter(i, vecbkgdparvalue[i]);

  // Estimate peak parameters
  double est_height(0.0), est_fwhm(0.0);
  size_t i_obscentre(0);
  double est_leftfwhm(0.0), est_rightfwhm(0.0);
  std::string errmsg = estimatePeakParameters(vecX, vecY, i_min, i_max, vecbkgdparvalue, i_obscentre, est_height,
                                              est_fwhm, est_leftfwhm, est_rightfwhm);
  if (!errmsg.empty()) {
    // Unable to estimate peak
    i_obscentre = i_centre;
    est_fwhm = 1.;
    est_height = 1.;
    g_log.warning(errmsg);
  }

  // Set peak parameters to
  if (m_useObsCentre)
    m_peakFunction->setCentre(vecX[i_obscentre]);
  else
    m_peakFunction->setCentre(vecX[i_centre]);
  m_peakFunction->setHeight(est_height);
  m_peakFunction->setFwhm(est_fwhm);

  if (usefpdresult < 0) {
    // Estimate peak range based on estimated linear background and peak
    // parameter estimated from observation
    if (!m_useObsCentre)
      i_obscentre = i_centre;
    estimatePeakRange(vecX, i_obscentre, i_min, i_max, est_leftfwhm, est_rightfwhm, vecpeakrange);
  }

  //-------------------------------------------------------------------------
  // Fit Peak
  //-------------------------------------------------------------------------
  std::vector<double> fitwindow(2);
  fitwindow[0] = vecX[i_min];
  fitwindow[1] = vecX[i_max];

  double costfuncvalue = callFitPeak(input, spectrum, m_peakFunction, m_backgroundFunction, fitwindow, vecpeakrange,
                                     m_minGuessedPeakWidth, m_maxGuessedPeakWidth, m_stepGuessedPeakWidth);

  bool fitsuccess = false;
  if (costfuncvalue < DBL_MAX && costfuncvalue >= 0. && m_peakFunction->height() > m_minHeight) {
    fitsuccess = true;
  }
  if (fitsuccess && m_usePeakPositionTolerance) {
    fitsuccess = (fabs(m_peakFunction->centre() - vecX[i_centre]) < m_peakPositionTolerance);
  }

  //-------------------------------------------------------------------------
  // Process Fit result
  //-------------------------------------------------------------------------
  // Update output
  if (fitsuccess)
    addInfoRow(spectrum, m_peakFunction, m_backgroundFunction, m_rawPeaksTable, costfuncvalue);
  else
    addNonFitRecord(spectrum, m_peakFunction->centre());
}

//----------------------------------------------------------------------------------------------
/** Find peak background given a certain range by
 * calling algorithm "FindPeakBackground"
 */
int FindPeaks::findPeakBackground(const MatrixWorkspace_sptr &input, int spectrum, size_t i_min, size_t i_max,
                                  std::vector<double> &vecBkgdParamValues, std::vector<double> &vecpeakrange) {
  const auto &vecX = input->x(spectrum);

  // Call FindPeakBackground
  auto estimate = createChildAlgorithm("FindPeakBackground");
  estimate->setLoggingOffset(1);
  estimate->setProperty("InputWorkspace", input);
  estimate->setProperty("WorkspaceIndex", spectrum);
  // estimate->setProperty("SigmaConstant", 1.0);
  std::vector<double> fwvec;
  fwvec.emplace_back(vecX[i_min]);
  fwvec.emplace_back(vecX[i_max]);
  estimate->setProperty("BackgroundType", m_backgroundType);
  estimate->setProperty("FitWindow", fwvec);
  estimate->executeAsChildAlg();
  // Get back the result
  Mantid::API::ITableWorkspace_sptr peaklisttablews = estimate->getProperty("OutputWorkspace");

  // Determine whether to use FindPeakBackground's result.
  int fitresult = -1;
  if (peaklisttablews->columnCount() < 7)
    throw std::runtime_error("No 7th column for use FindPeakBackground result or not. ");

  if (peaklisttablews->rowCount() > 0) {
    /// @todo Allow setting of fitresult. Currently, fitresult is left
    /// deliberately hidden by creating a new variable here with the same
    /// name. This should be fixed but it causes different behaviour which
    /// breaks several unit tests. The issue to deal with this is #13950. Other
    /// related issues are #13667, #15978 and #19773.
    const int hiddenFitresult = peaklisttablews->Int(0, 6);
    g_log.information() << "fitresult=" << hiddenFitresult << "\n";
  }

  // Local check whether FindPeakBackground gives a reasonable value
  vecpeakrange.resize(2);
  if (fitresult > 0) {
    // Use FitPeakBackgroud's result
    size_t i_peakmin, i_peakmax;
    i_peakmin = peaklisttablews->Int(0, 1);
    i_peakmax = peaklisttablews->Int(0, 2);

    g_log.information() << "FindPeakBackground successful. "
                        << "iMin = " << i_min << ", iPeakMin = " << i_peakmin << ", iPeakMax = " << i_peakmax
                        << ", iMax = " << i_max << "\n";

    if (i_peakmin < i_peakmax && i_peakmin > i_min + 2 && i_peakmax < i_max - 2) {
      // FIXME - It is assumed that there are 3 background parameters set to
      // FindPeaksBackground
      double bg0, bg1, bg2;
      bg0 = peaklisttablews->Double(0, 3);
      bg1 = peaklisttablews->Double(0, 4);
      bg2 = peaklisttablews->Double(0, 5);

      // Set output
      vecBkgdParamValues.resize(3, 0.);
      vecBkgdParamValues[0] = bg0;
      vecBkgdParamValues[1] = bg1;
      vecBkgdParamValues[2] = bg2;

      g_log.information() << "Background parameters (from FindPeakBackground) A0=" << bg0 << ", A1=" << bg1
                          << ", A2=" << bg2 << "\n";

      vecpeakrange[0] = vecX[i_peakmin];
      vecpeakrange[1] = vecX[i_peakmax];
    } else {
      // Do manual estimation again
      g_log.debug("FindPeakBackground result is ignored due to wrong in peak range. ");
    }
  } else {
    g_log.information("Failed to get background estimation\n");
  }

  std::stringstream outx;
  outx << "FindPeakBackground Result: Given window (" << vecX[i_min] << ", " << vecX[i_max]
       << ");  Determine peak range: (" << vecpeakrange[0] << ", " << vecpeakrange[1] << "). ";
  g_log.information(outx.str());

  return fitresult;
}

//----------------------------------------------------------------------------------------------
/** Estimate peak parameters
 * Assumption: pure peak workspace with background removed (but it might not be
 * true...)
 * @param vecX :: vector of X-axis
 * @param vecY :: vector of Y-axis
 * @param i_min :: start
 * @param i_max :: end
 * @param vecbkgdparvalues :: vector of background parameters (a0, a1, a2)
 * @param iobscentre :: (output) bin index of estimated peak centre (maximum
 * position)
 * @param height :: (output) estimated maximum
 * @param fwhm :: (output) estimated fwhm
 * @param leftfwhm :: (output) left side fhwm
 * @param rightfwhm :: (output) right side fwhm
 * @return error mesage
 */
std::string FindPeaks::estimatePeakParameters(const HistogramX &vecX, const HistogramY &vecY, size_t i_min,
                                              size_t i_max, const std::vector<double> &vecbkgdparvalues,
                                              size_t &iobscentre, double &height, double &fwhm, double &leftfwhm,
                                              double &rightfwhm) {
  // Search for maximum considering background
  const double bg0 = vecbkgdparvalues[0];
  double bg1 = 0;
  double bg2 = 0;
  if (vecbkgdparvalues.size() >= 2) {
    bg1 = vecbkgdparvalues[1];
    if (vecbkgdparvalues.size() >= 3)
      bg2 = vecbkgdparvalues[2];
  }

  // Starting value
  iobscentre = i_min;
  const double tmpx = vecX[i_min];
  height = vecY[i_min] - (bg0 + bg1 * tmpx + bg2 * tmpx * tmpx);
  double lowest = height;

  // Extreme case
  if (i_max == vecY.size())
    i_max = i_max - 1;

  // Searching
  for (size_t i = i_min + 1; i <= i_max; ++i) {
    const double x = vecX[i];
    const double tmpheight = vecY[i] - (bg0 + bg1 * x + bg2 * x * x);

    if (tmpheight > height) {
      iobscentre = i;
      height = tmpheight;
    } else if (tmpheight < lowest) {
      lowest = tmpheight;
    }
  }

  // Summarize on peak centre
  double obscentre = vecX[iobscentre];
  double drop = height - lowest;
  if (drop == 0) {
    // Flat spectrum.  No peak parameter can be estimated.
    // FIXME - should have a second notice such that there will be no more
    // fitting.
    return "Flat spectrum";
  } else if (height <= m_minHeight) {
    // The peak is not high enough!
    // FIXME - should have a second notice such that there will be no more
    // fitting.
    return "Fluctuation is less than minimum allowed value.";
  }

  // If maximum point is on the edge 2 points, return false.  One side of peak
  // must have at least 3 points
  if (iobscentre <= i_min + 1 || iobscentre >= i_max - 1) {
    std::stringstream dbss;
    dbss << "Maximum value on edge. Fit window is between " << vecX[i_min] << " and " << vecX[i_max]
         << ". Maximum value " << vecX[iobscentre] << " is located on (" << iobscentre << ").";
    return dbss.str();
  }

  // Search for half-maximum: no need to very precise

  // Slope at the left side of peak.
  leftfwhm = -1.;
  for (int i = static_cast<int>(iobscentre) - 1; i >= 0; --i) {
    double xleft = vecX[i];
    double yleft = vecY[i] - (bg0 + bg1 * xleft + bg2 * xleft * xleft);
    if (yleft <= 0.5 * height) {
      // Ideal case
      // FIXME - Need a linear interpolation on it!
      leftfwhm = obscentre - 0.5 * (vecX[i] + vecX[i + 1]);
      break;
    }
  }

  // Slope at the right side of peak
  rightfwhm = -1.;
  for (size_t i = iobscentre + 1; i <= i_max; ++i) {
    double xright = vecX[i];
    double yright = vecY[i] - (bg0 + bg1 * xright + bg2 * xright * xright);
    if (yright <= 0.5 * height) {
      rightfwhm = 0.5 * (vecX[i] + vecX[i - 1]) - obscentre;
      break;
    }
  }

  if (leftfwhm <= 0 || rightfwhm <= 0) {
    std::stringstream errmsg;
    errmsg << "Estimate peak parameters error (FWHM cannot be zero): Input "
              "data size = "
           << vecX.size() << ", Xmin = " << vecX[i_min] << "(" << i_min << "), Xmax = " << vecX[i_max] << "(" << i_max
           << "); "
           << "Estimated peak centre @ " << vecX[iobscentre] << "(" << iobscentre << ") with height = " << height
           << "; Lowest Y value = " << lowest << "; Output error: .  leftfwhm = " << leftfwhm
           << ", right fwhm = " << rightfwhm << ".";
    return errmsg.str();
  }

  fwhm = leftfwhm + rightfwhm;
  if (fwhm < 1.e-200) // very narrow peak
  {
    std::stringstream errmsg;
    errmsg << "Estimate peak parameters error (FWHM cannot be zero): Input "
              "data size = "
           << vecX.size() << ", Xmin = " << vecX[i_min] << "(" << i_min << "), Xmax = " << vecX[i_max] << "(" << i_max
           << "); "
           << "Estimated peak centre @ " << vecX[iobscentre] << "(" << iobscentre << ") with height = " << height
           << "; Lowest Y value = " << lowest << "; Output error: .  fwhm = " << fwhm << ".";
    return errmsg.str();
  }

  g_log.information() << "Estimated peak parameters: Centre = " << obscentre << ", Height = " << height
                      << ", FWHM = " << fwhm << " = " << leftfwhm << " + " << rightfwhm << ".\n";

  return std::string();
}

//----------------------------------------------------------------------------------------------
/** Estimate background parameter values and peak range
 * The background to estimate is a linear background.  Assuming the first and
 * last data points
 * cannot be a major part of the peak unless the fit window is too small.
 * @param X :: vec for X
 * @param Y :: vec for Y
 * @param i_min :: index of minimum in X to estimate background
 * @param i_max :: index of maximum in X to estimate background
 * @param vecbkgdparvalues :: vector of double for a0, a1 and a2 of background
 */
void FindPeaks::estimateBackground(const HistogramX &X, const HistogramY &Y, const size_t i_min, const size_t i_max,
                                   std::vector<double> &vecbkgdparvalues) {
  // Validate input
  if (i_min >= i_max)
    throw std::runtime_error("when trying to estimate the background parameter "
                             "values: i_min cannot larger or equal to i_max");
  if (vecbkgdparvalues.size() < 3)
    vecbkgdparvalues.resize(3, 0.);

  // FIXME - THIS IS A MAGIC NUMBER
  const size_t MAGICNUMBER = 12;
  size_t numavg;
  if (i_max - i_min > MAGICNUMBER)
    numavg = 3;
  else
    numavg = 1;

  // Get (x0, y0) and (xf, yf)
  double x0 = 0.0;
  double y0 = 0.0;
  double xf = 0.0;
  double yf = 0.0;

  for (size_t i = 0; i < numavg; ++i) {
    x0 += X[i_min + i];
    y0 += Y[i_min + i];

    xf += X[i_max - i];
    // X has one more value than Y
    yf += Y[i_max - i - 1];
  }
  x0 = x0 / static_cast<double>(numavg);
  y0 = y0 / static_cast<double>(numavg);
  xf = xf / static_cast<double>(numavg);
  yf = yf / static_cast<double>(numavg);

  // Esitmate
  vecbkgdparvalues[2] = 0.;
  if (m_bkgdOrder >= 1) {
    // linear background
    vecbkgdparvalues[1] = (y0 - yf) / (x0 - xf);
    vecbkgdparvalues[0] = (xf * y0 - x0 * yf) / (xf - x0);
  } else {
    // flat background
    vecbkgdparvalues[1] = 0.;
    vecbkgdparvalues[0] = 0.5 * (y0 + yf);
  }
}

//----------------------------------------------------------------------------------------------
/** Estimate peak range according to observed peak parameters and (linear)
 * background
 */
void FindPeaks::estimatePeakRange(const HistogramX &vecX, size_t i_centre, size_t i_min, size_t i_max,
                                  const double &leftfwhm, const double &rightfwhm, std::vector<double> &vecpeakrange) {
  // Check
  if (vecpeakrange.size() < 2)
    vecpeakrange.resize(2, 0.);

  if (i_centre < i_min || i_centre > i_max)
    throw std::runtime_error("Estimate peak range input centre is out of fit window. ");

  // Search peak left by using 6 * half of FWHM
  double peakleftbound = vecX[i_centre] - 6. * leftfwhm;
  double peakrightbound = vecX[i_centre] + 6. * rightfwhm;

  // Deal with case the peak boundary is too close to fit window
  auto ipeakleft = static_cast<size_t>(getIndex(vecX, peakleftbound));
  if (ipeakleft <= i_min) {
    size_t numbkgdpts = (i_centre - i_min) / 6;
    // FIXME - 3 is a magic number
    if (numbkgdpts < 3)
      numbkgdpts = 3;
    ipeakleft = i_min + numbkgdpts;
    if (ipeakleft >= i_centre)
      ipeakleft = i_min + 1;

    peakleftbound = vecX[ipeakleft];
  }

  auto ipeakright = static_cast<size_t>(getIndex(vecX, peakrightbound));
  if (ipeakright >= i_max) {
    size_t numbkgdpts = (i_max - i_centre) / 6;
    // FIXME - 3 is a magic number
    if (numbkgdpts < 3)
      numbkgdpts = 3;
    ipeakright = i_max - numbkgdpts;
    if (ipeakright <= i_centre)
      ipeakright = i_max - 1;

    peakrightbound = vecX[ipeakright];
  }

  // Set result to output vector
  vecpeakrange[0] = peakleftbound;
  vecpeakrange[1] = peakrightbound;
}

//----------------------------------------------------------------------------------------------
/** Add a row to the output table workspace.
 * @param spectrum :: spectrum number
 * @param peakfunction :: peak function
 * @param bkgdfunction :: background function
 * @param isoutputraw :: flag to output raw function parameters
 * @param mincost :: minimum/best cost function value
 */
void FindPeaks::addInfoRow(const size_t spectrum, const API::IPeakFunction_const_sptr &peakfunction,
                           const API::IBackgroundFunction_sptr &bkgdfunction, const bool isoutputraw,
                           const double mincost) {
  // Check input validity
  if (mincost < 0. || mincost >= DBL_MAX - 1.0E-10)
    throw std::runtime_error("Minimum cost indicates that fit fails.  This "
                             "method should not be called "
                             "under this circumstance. ");

  // Add fitted parameters to output table workspace
  API::TableRow t = m_outPeakTableWS->appendRow();

  // spectrum
  t << static_cast<int>(spectrum);

  // peak and background function parameters
  if (isoutputraw) {
    // Output of raw peak parameters
    size_t nparams = peakfunction->nParams();
    size_t nparamsb = bkgdfunction->nParams();

    size_t numcols = m_outPeakTableWS->columnCount();
    if (nparams + nparamsb + 2 != numcols) {
      throw std::runtime_error("Error 1307 number of columns do not matches");
    }

    for (size_t i = 0; i < nparams; ++i) {
      t << peakfunction->getParameter(i);
    }
    for (size_t i = 0; i < nparamsb; ++i) {
      t << bkgdfunction->getParameter(i);
    }
  } else {
    // Output of effective peak parameters
    // Set up parameters to peak function and get effective value
    double peakcentre = peakfunction->centre();
    double fwhm = peakfunction->fwhm();
    double height = peakfunction->height();

    t << peakcentre << fwhm << height;

    // Set up parameters to background function

    // FIXME - Use Polynomial for all 3 background types.
    double a0 = bkgdfunction->getParameter("A0");
    double a1 = 0.;
    if (bkgdfunction->name() != "FlatBackground")
      a1 = bkgdfunction->getParameter("A1");
    double a2 = 0;
    if (bkgdfunction->name() != "LinearBackground" && bkgdfunction->name() != "FlatBackground")
      a2 = bkgdfunction->getParameter("A2");

    t << a0 << a1 << a2;

    g_log.debug() << "Peak parameters found: cen=" << peakcentre << " fwhm=" << fwhm << " height=" << height
                  << " a0=" << a0 << " a1=" << a1 << " a2=" << a2;
  }
  g_log.debug() << " chsq=" << mincost << "\n";
  // Minimum cost function value
  t << mincost;
}

//----------------------------------------------------------------------------------------------
/** Add the fit record (failure) to output workspace
 * @param spectrum :: spectrum where the peak is
 * @param centre :: position of the peak centre
 */
void FindPeaks::addNonFitRecord(const size_t spectrum, const double centre) {
  // Add a new row
  API::TableRow t = m_outPeakTableWS->appendRow();

  g_log.information() << "Failed to fit peak at " << centre << "\n";
  // 1st column
  t << static_cast<int>(spectrum);

  // Parameters
  for (std::size_t i = 0; i < m_numTableParams; i++) {
    if (i == m_centreIndex)
      t << centre;
    else
      t << 0.;
  }

  // HUGE chi-square
  t << DBL_MAX;
}

//----------------------------------------------------------------------------------------------
/** Create functions and related variables
 */
void FindPeaks::createFunctions() {
  // Setup the background
  // FIXME (No In This Ticket)  Need to have a uniformed routine to name
  // background function
  std::string backgroundposix;
  if (m_backgroundType != "Quadratic") {
    // FlatBackground, LinearBackground, Quadratic
    backgroundposix = "Background";
  }
  m_backgroundFunction = std::dynamic_pointer_cast<IBackgroundFunction>(
      API::FunctionFactory::Instance().createFunction(m_backgroundType + backgroundposix));
  g_log.information() << "Background function (" << m_backgroundFunction->name() << ") has been created. "
                      << "\n";

  m_bkgdParameterNames = m_backgroundFunction->getParameterNames();
  // FIXME - Need to add method nOrder to background function;
  m_bkgdOrder = m_backgroundFunction->nParams() - 1;

  // Set up peak function
  m_peakFunction =
      std::dynamic_pointer_cast<IPeakFunction>(API::FunctionFactory::Instance().createFunction(m_peakFuncType));
  m_peakParameterNames = m_peakFunction->getParameterNames();
}

//----------------------------------------------------------------------------------------------
/** Fit a single peak function with background by calling algorithm callFitPeak
 */
double FindPeaks::callFitPeak(const MatrixWorkspace_sptr &dataws, int wsindex,
                              const API::IPeakFunction_sptr &peakfunction,
                              const API::IBackgroundFunction_sptr &backgroundfunction,
                              const std::vector<double> &vec_fitwindow, const std::vector<double> &vec_peakrange,
                              int minGuessFWHM, int maxGuessFWHM, int guessedFWHMStep, int estBackResult) {
  std::stringstream dbss;
  dbss << "[Call FitPeak] Fit 1 peak at X = " << peakfunction->centre() << " of spectrum " << wsindex;
  g_log.information(dbss.str());

  double userFWHM = m_peakFunction->fwhm();
  bool fitwithsteppedfwhm = (guessedFWHMStep > 0);

  FitOneSinglePeak fitpeak;
  fitpeak.setChild(true);
  fitpeak.setWorskpace(dataws, wsindex);
  fitpeak.setFitWindow(vec_fitwindow[0], vec_fitwindow[1]);
  fitpeak.setFittingMethod(m_minimizer, m_costFunction);
  fitpeak.setFunctions(peakfunction, backgroundfunction);
  fitpeak.setupGuessedFWHM(userFWHM, minGuessFWHM, maxGuessFWHM, guessedFWHMStep, fitwithsteppedfwhm);
  fitpeak.setPeakRange(vec_peakrange[0], vec_peakrange[1]);

  if (estBackResult == 1) {
    g_log.information("simpleFit");
    fitpeak.simpleFit();
  } else if (m_highBackground) {
    g_log.information("highBkgdFit");
    fitpeak.highBkgdFit();
  } else {
    g_log.information("simpleFit");
    fitpeak.simpleFit();
  }

  double costfuncvalue = fitpeak.getFitCostFunctionValue();
  std::string dbinfo = fitpeak.getDebugMessage();
  g_log.information(dbinfo);

  return costfuncvalue;
}

//----------------------------------------------------------------------------------------------
/** Get the peak parameter values from m_peakFunction and output to a list in
 * the same
 * order of m_peakParameterNames
 */
std::vector<double> FindPeaks::getStartingPeakValues() {
  size_t numpeakpars = m_peakFunction->nParams();
  std::vector<double> vec_value(numpeakpars);
  for (size_t i = 0; i < numpeakpars; ++i) {
    double parvalue = m_peakFunction->getParameter(i);
    vec_value[i] = parvalue;
  }

  return vec_value;
}

} // namespace Mantid::Algorithms

// 0.5044, 0.5191, 0.535, 0.5526, 0.5936, 0.6178, 0.6453, 0.6768, 0.7134,
// 0.7566, 0.8089, 0.8737, 0.9571, 1.0701, 1.2356, 1.5133, 2.1401
