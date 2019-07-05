// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/EstimatePolynomial.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Statistics.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(FindPeakBackground)

//----------------------------------------------------------------------------------------------
/** Define properties
 */
void FindPeakBackground::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "Anonymous", Direction::Input),
                  "Name of input MatrixWorkspace that contains peaks.");

  declareProperty("WorkspaceIndex", EMPTY_INT(),
                  "workspace indices to have peak and background separated. "
                  "No default is taken. ");

  declareProperty(
      "SigmaConstant", 1.0,
      "Multiplier of standard deviations of the variance for convergence of "
      "peak elimination.  Default is 1.0. ");

  declareProperty(std::make_unique<ArrayProperty<double>>("FitWindow"),
                  "Optional: enter a comma-separated list of the minimum and "
                  "maximum X-positions of window to fit.  "
                  "The window is the same for all indices in workspace. The "
                  "length must be exactly two.");

  std::vector<std::string> bkgdtypes{"Flat", "Linear", "Quadratic"};
  declareProperty("BackgroundType", "Linear",
                  boost::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background.");

  // The found peak in a table
  declareProperty(
      std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "The name of the TableWorkspace in which to store the background found "
      "for each index.  "
      "Table contains the indices of the beginning and ending of peak "
      "and the estimated background coefficients for the constant, linear, and "
      "quadratic terms.");
}

void FindPeakBackground::findWindowIndex(
    const HistogramData::Histogram &histogram, size_t &l0, size_t &n) {
  auto &inpX = histogram.x();
  auto &inpY = histogram.y();
  size_t sizey = inpY.size(); // inpWS->y(inpwsindex).size();

  // determine the fit window with their index in X (or Y)
  n = sizey;
  l0 = 0;
  if (m_vecFitWindows.size() > 1) {
    Mantid::Algorithms::FindPeaks fp;
    l0 = fp.getIndex(inpX, m_vecFitWindows[0]);
    n = fp.getIndex(inpX, m_vecFitWindows[1]);
    if (n < sizey)
      n++;
  }
}

//----------------------------------------------------------------------------------------------
/** Execute body
 */
void FindPeakBackground::exec() {
  // Get input and validate
  processInputProperties();
  auto histogram = m_inputWS->histogram(m_inputWSIndex);

  size_t l0, n;
  findWindowIndex(histogram, l0, n);

  // m_vecFitWindows won't be used again form this point till end.

  // Set up output table workspace
  createOutputWorkspaces();

  // 3. Get Y values
  Progress prog(this, 0.0, 1.0, 1);

  std::vector<size_t> peak_min_max_indexes;
  std::vector<double> bkgd3;
  int goodfit = findBackground(histogram, l0, n, peak_min_max_indexes, bkgd3);

  if (goodfit > 0) {
    size_t min_peak = peak_min_max_indexes[0];
    size_t max_peak = peak_min_max_indexes[1];
    double a0 = bkgd3[0];
    double a1 = bkgd3[1];
    double a2 = bkgd3[2];
    API::TableRow t = m_outPeakTableWS->getRow(0);
    t << static_cast<int>(m_inputWSIndex) << static_cast<int>(min_peak)
      << static_cast<int>(max_peak) << a0 << a1 << a2 << goodfit;
  }

  prog.report();

  // 4. Set the output
  setProperty("OutputWorkspace", m_outPeakTableWS);
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FindPeakBackground::findBackground
 * @param histogram
 * @param l0
 * @param n
 * @param peak_min_max_indexes
 * @param bkgd3
 * @return
 */
int FindPeakBackground::findBackground(
    const HistogramData::Histogram &histogram, const size_t &l0,
    const size_t &n, std::vector<size_t> &peak_min_max_indexes,
    std::vector<double> &bkgd3) {
  const size_t sizex = histogram.x().size();
  const auto &inpY = histogram.y();
  const size_t sizey = inpY.size();

  int goodfit(0);

  // Find background
  double Ymean, Yvariance, Ysigma;
  MantidVec maskedY;
  auto in = std::min_element(inpY.cbegin(), inpY.cend());
  double bkg0 = inpY[in - inpY.begin()];
  for (size_t l = l0; l < n; ++l) {
    maskedY.push_back(inpY[l] - bkg0);
  }
  MantidVec mask(n - l0, 0.0);
  double xn = static_cast<double>(n - l0);
  if ((0. == xn) || (0. == xn - 1.0))
    throw std::runtime_error(
        "The number of Y values in the input workspace for the "
        "workspace index given, minus 'l0' or minus 'l0' minus 1, is 0. This "
        "will produce a "
        "divide-by-zero");
  do {
    Statistics stats = getStatistics(maskedY);
    Ymean = stats.mean;
    Yvariance = stats.standard_deviation * stats.standard_deviation;
    Ysigma = std::sqrt((moment4(maskedY, static_cast<size_t>(xn), Ymean) -
                        (xn - 3.0) / (xn - 1.0) * Yvariance) /
                       xn);
    MantidVec::const_iterator it =
        std::max_element(maskedY.begin(), maskedY.end());
    const size_t pos = it - maskedY.begin();
    maskedY[pos] = 0;
    mask[pos] = 1.0;
  } while (std::abs(Ymean - Yvariance) > m_sigmaConstant * Ysigma);

  if (n - l0 > 5) {
    // remove single outliers
    if (mask[1] == mask[2] && mask[2] == mask[3])
      mask[0] = mask[1];
    if (mask[0] == mask[2] && mask[2] == mask[3])
      mask[1] = mask[2];
    for (size_t l = 2; l < n - l0 - 3; ++l) {
      if (mask[l - 1] == mask[l + 1] &&
          (mask[l - 1] == mask[l - 2] || mask[l + 1] == mask[l + 2])) {
        mask[l] = mask[l + 1];
      }
    }
    if (mask[n - l0 - 2] == mask[n - l0 - 3] &&
        mask[n - l0 - 3] == mask[n - l0 - 4])
      mask[n - l0 - 1] = mask[n - l0 - 2];
    if (mask[n - l0 - 1] == mask[n - l0 - 3] &&
        mask[n - l0 - 3] == mask[n - l0 - 4])
      mask[n - l0 - 2] = mask[n - l0 - 1];

    // mask regions not connected to largest region
    // for loop can start > 1 for multiple peaks
    vector<cont_peak> peaks;
    if (mask[0] == 1) {
      peaks.emplace_back();
      peaks.back().start = l0;
    }
    for (size_t l = 1; l < n - l0; ++l) {
      if (mask[l] != mask[l - 1] && mask[l] == 1) {
        peaks.emplace_back();
        peaks.back().start = l + l0;
      } else if (!peaks.empty()) {
        size_t ipeak = peaks.size() - 1;
        if (mask[l] != mask[l - 1] && mask[l] == 0) {
          peaks[ipeak].stop = l + l0;
        }
        if (inpY[l + l0] > peaks[ipeak].maxY)
          peaks[ipeak].maxY = inpY[l + l0];
      }
    }
    size_t min_peak, max_peak;
    if (!peaks.empty()) {
      g_log.debug() << "Peaks' size = " << peaks.size()
                    << " -> esitmate background. \n";
      if (peaks.back().stop == 0)
        peaks.back().stop = n - 1;
      std::sort(peaks.begin(), peaks.end(), by_len());

      // save endpoints
      min_peak = peaks[0].start;
      // extra point for histogram input - TODO change to use Histogram better
      max_peak = peaks[0].stop + sizex - sizey;
      goodfit = 1;
    } else {
      // assume the whole thing is background
      g_log.debug("Peaks' size = 0 -> whole region assumed background");
      min_peak = n;
      max_peak = l0;

      goodfit = 2;
    }

    double a0 = 0., a1 = 0., a2 = 0.;
    estimateBackground(histogram, l0, n, min_peak, max_peak, (!peaks.empty()),
                       a0, a1, a2);

    // Add a new row
    peak_min_max_indexes.resize(2);
    peak_min_max_indexes[0] = min_peak;
    peak_min_max_indexes[1] = max_peak;

    bkgd3.resize(3);
    bkgd3[0] = a0;
    bkgd3[1] = a1;
    bkgd3[2] = a2;
  }

  return goodfit;
}

//----------------------------------------------------------------------------------------------
/** Estimate background
 * @param histogram :: data to find peak background in
 * @param i_min :: index of minimum in X to estimate background
 * @param i_max :: index of maximum in X to estimate background
 * @param p_min :: index of peak min in X to estimate background
 * @param p_max :: index of peak max in X to estimate background
 * @param hasPeak :: ban data in the peak range
 * @param out_bg0 :: interception
 * @param out_bg1 :: slope
 * @param out_bg2 :: a2 = 0
 */
void FindPeakBackground::estimateBackground(
    const HistogramData::Histogram &histogram, const size_t i_min,
    const size_t i_max, const size_t p_min, const size_t p_max,
    const bool hasPeak, double &out_bg0, double &out_bg1, double &out_bg2) {
  double redux_chisq;
  if (hasPeak) {
    HistogramData::estimateBackground(m_backgroundOrder, histogram, i_min,
                                      i_max, p_min, p_max, out_bg0, out_bg1,
                                      out_bg2, redux_chisq);
  } else {
    HistogramData::estimatePolynomial(m_backgroundOrder, histogram, i_min,
                                      i_max, out_bg0, out_bg1, out_bg2,
                                      redux_chisq);
  }

  g_log.information() << "Estimated background: A0 = " << out_bg0
                      << ", A1 = " << out_bg1 << ", A2 = " << out_bg2 << "\n";
}
//----------------------------------------------------------------------------------------------
/** Calculate 4th moment
 * @param X :: vec for X
 * @param n :: length of vector
 * @param mean :: mean of X
 */
double FindPeakBackground::moment4(MantidVec &X, size_t n, double mean) {
  double sum = 0.0;
  for (size_t i = 0; i < n; ++i) {
    sum += (X[i] - mean) * (X[i] - mean) * (X[i] - mean) * (X[i] - mean);
  }
  sum /= static_cast<double>(n);
  return sum;
}

//----------------------------------------------------------------------------------------------
void FindPeakBackground::processInputProperties() {
  // process input workspace and workspace index
  m_inputWS = getProperty("InputWorkspace");

  int inpwsindex = getProperty("WorkspaceIndex");
  if (isEmpty(inpwsindex)) {
    // Default
    if (m_inputWS->getNumberHistograms() == 1) {
      inpwsindex = 0;
    } else {
      throw runtime_error("WorkspaceIndex must be given. ");
    }
  } else if (inpwsindex < 0 ||
             inpwsindex >= static_cast<int>(m_inputWS->getNumberHistograms())) {
    stringstream errss;
    errss << "Input workspace " << m_inputWS->getName() << " has "
          << m_inputWS->getNumberHistograms()
          << " spectra.  Input workspace index " << inpwsindex
          << " is out of boundary. ";
    throw runtime_error(errss.str());
  }
  m_inputWSIndex = static_cast<size_t>(inpwsindex);

  std::vector<double> fitwindow = getProperty("FitWindow");
  setFitWindow(fitwindow);

  // background
  m_backgroundType = getPropertyValue("BackgroundType");
  size_t bkgdorder = 0;
  if (m_backgroundType == "Linear")
    bkgdorder = 1;
  else if (m_backgroundType == "Quadratic")
    bkgdorder = 2;
  setBackgroundOrder(bkgdorder);

  // sigma constant
  double k = getProperty("SigmaConstant");
  setSigma(k);
}

/// set sigma constant
void FindPeakBackground::setSigma(const double &sigma) {
  m_sigmaConstant = sigma;
}

/// set background order
void FindPeakBackground::setBackgroundOrder(size_t order) {
  m_backgroundOrder = order;
}

//----------------------------------------------------------------------------------------------
/** set fit window
 * @brief FindPeakBackground::setFitWindow
 * @param fitwindow
 */
void FindPeakBackground::setFitWindow(const std::vector<double> &fitwindow) {
  // validate input
  if ((fitwindow.size() == 2) && fitwindow[0] >= fitwindow[1]) {
    throw std::invalid_argument("Fit window has either wrong item number or "
                                "window value is not in ascending order.");
  }

  // m_vecFitWindows.resize(2);
  // copy the input to class variable
  m_vecFitWindows = fitwindow;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FindPeakBackground::createOutputWorkspaces
 */
void FindPeakBackground::createOutputWorkspaces() {
  // Set up output table workspace
  m_outPeakTableWS = boost::make_shared<TableWorkspace>();
  m_outPeakTableWS->addColumn("int", "wksp_index");
  m_outPeakTableWS->addColumn("int", "peak_min_index");
  m_outPeakTableWS->addColumn("int", "peak_max_index");
  m_outPeakTableWS->addColumn("double", "bkg0");
  m_outPeakTableWS->addColumn("double", "bkg1");
  m_outPeakTableWS->addColumn("double", "bkg2");
  m_outPeakTableWS->addColumn("int", "GoodFit");

  m_outPeakTableWS->appendRow();
}

} // namespace Algorithms
} // namespace Mantid
