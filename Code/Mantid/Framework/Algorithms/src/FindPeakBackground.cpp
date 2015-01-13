#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Statistics.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

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
/** Constructor
 */
FindPeakBackground::FindPeakBackground() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FindPeakBackground::~FindPeakBackground() {}

//----------------------------------------------------------------------------------------------
/** Define properties
  */
void FindPeakBackground::init() {
  auto inwsprop = new WorkspaceProperty<MatrixWorkspace>(
      "InputWorkspace", "Anonymous", Direction::Input);
  declareProperty(inwsprop,
                  "Name of input MatrixWorkspace that contains peaks.");

  declareProperty("WorkspaceIndex", EMPTY_INT(),
                  "workspace indices to have peak and background separated. "
                  "No default is taken. ");

  declareProperty(
      "SigmaConstant", 1.0,
      "Multiplier of standard deviations of the variance for convergence of "
      "peak elimination.  Default is 1.0. ");

  declareProperty(new ArrayProperty<double>("FitWindow"),
                  "Optional: enter a comma-separated list of the minimum and "
                  "maximum X-positions of window to fit.  "
                  "The window is the same for all indices in workspace. The "
                  "length must be exactly two.");

  std::vector<std::string> bkgdtypes;
  bkgdtypes.push_back("Flat");
  bkgdtypes.push_back("Linear");
  bkgdtypes.push_back("Quadratic");
  declareProperty("BackgroundType", "Linear",
                  boost::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background.");

  // The found peak in a table
  declareProperty(
      new WorkspaceProperty<API::ITableWorkspace>("OutputWorkspace", "",
                                                  Direction::Output),
      "The name of the TableWorkspace in which to store the background found "
      "for each index.  "
      "Table contains the indices of the beginning and ending of peak "
      "and the estimated background coefficients for the constant, linear, and "
      "quadratic terms.");
}

//----------------------------------------------------------------------------------------------
/** Execute body
  */
void FindPeakBackground::exec() {
  // Get input and validate
  MatrixWorkspace_const_sptr inpWS = getProperty("InputWorkspace");
  int inpwsindex = getProperty("WorkspaceIndex");
  std::vector<double> m_vecFitWindows = getProperty("FitWindow");
  m_backgroundType = getPropertyValue("BackgroundType");
  double k = getProperty("SigmaConstant");

  if (isEmpty(inpwsindex)) {
    // Default
    if (inpWS->getNumberHistograms() == 1) {
      inpwsindex = 0;
    } else {
      throw runtime_error("WorkspaceIndex must be given. ");
    }
  } else if (inpwsindex < 0 ||
             inpwsindex >= static_cast<int>(inpWS->getNumberHistograms())) {
    stringstream errss;
    errss << "Input workspace " << inpWS->name() << " has "
          << inpWS->getNumberHistograms() << " spectra.  Input workspace index "
          << inpwsindex << " is out of boundary. ";
    throw runtime_error(errss.str());
  }

  // Generate output
  const MantidVec &inpX = inpWS->readX(inpwsindex);
  size_t sizex = inpWS->readX(inpwsindex).size();
  size_t sizey = inpWS->readY(inpwsindex).size();
  size_t n = sizey;
  size_t l0 = 0;

  if (m_vecFitWindows.size() > 1) {
    Mantid::Algorithms::FindPeaks fp;
    l0 = fp.getVectorIndex(inpX, m_vecFitWindows[0]);
    n = fp.getVectorIndex(inpX, m_vecFitWindows[1]);
    if (n < sizey)
      n++;
  }

  // Set up output table workspace
  API::ITableWorkspace_sptr m_outPeakTableWS =
      WorkspaceFactory::Instance().createTable("TableWorkspace");
  m_outPeakTableWS->addColumn("int", "wksp_index");
  m_outPeakTableWS->addColumn("int", "peak_min_index");
  m_outPeakTableWS->addColumn("int", "peak_max_index");
  m_outPeakTableWS->addColumn("double", "bkg0");
  m_outPeakTableWS->addColumn("double", "bkg1");
  m_outPeakTableWS->addColumn("double", "bkg2");
  m_outPeakTableWS->addColumn("int", "GoodFit");

  m_outPeakTableWS->appendRow();

  // 3. Get Y values
  Progress prog(this, 0, 1.0, 1);

  // Find background

  const MantidVec &inpY = inpWS->readY(inpwsindex);

  double Ymean, Yvariance, Ysigma;
  MantidVec maskedY;
  MantidVec::const_iterator in = std::min_element(inpY.begin(), inpY.end());
  double bkg0 = inpY[in - inpY.begin()];
  for (size_t l = l0; l < n; ++l) {
    maskedY.push_back(inpY[l] - bkg0);
  }
  MantidVec mask(n - l0, 0.0);
  double xn = static_cast<double>(n - l0);
  do {
    Statistics stats = getStatistics(maskedY);
    Ymean = stats.mean;
    Yvariance = stats.standard_deviation * stats.standard_deviation;
    Ysigma = std::sqrt((moment4(maskedY, n - l0, Ymean) -
                        (xn - 3.0) / (xn - 1.0) * Yvariance) /
                       xn);
    MantidVec::const_iterator it =
        std::max_element(maskedY.begin(), maskedY.end());
    const size_t pos = it - maskedY.begin();
    maskedY[pos] = 0;
    mask[pos] = 1.0;
  } while (std::abs(Ymean - Yvariance) > k * Ysigma);

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
      peaks.push_back(cont_peak());
      peaks[peaks.size() - 1].start = l0;
    }
    for (size_t l = 1; l < n - l0; ++l) {
      if (mask[l] != mask[l - 1] && mask[l] == 1) {
        peaks.push_back(cont_peak());
        peaks[peaks.size() - 1].start = l + l0;
      } else if (peaks.size() > 0) {
        size_t ipeak = peaks.size() - 1;
        if (mask[l] != mask[l - 1] && mask[l] == 0) {
          peaks[ipeak].stop = l + l0;
        }
        if (inpY[l + l0] > peaks[ipeak].maxY)
          peaks[ipeak].maxY = inpY[l + l0];
      }
    }
    size_t min_peak, max_peak;
    double a0, a1, a2;
    int goodfit;
    if (peaks.size() > 0) {
      g_log.debug() << "Peaks' size = " << peaks.size()
                    << " -> esitmate background. \n";
      if (peaks[peaks.size() - 1].stop == 0)
        peaks[peaks.size() - 1].stop = n - 1;
      std::sort(peaks.begin(), peaks.end(), by_len());

      // save endpoints
      min_peak = peaks[0].start;
      // extra point for histogram input
      max_peak = peaks[0].stop + sizex - sizey;
      estimateBackground(inpX, inpY, l0, n, peaks[0].start, peaks[0].stop, a0,
                         a1, a2);
      goodfit = 1;
    } else {
      // assume background is 12 first and last points
      g_log.debug("Peaks' size = 0 -> zero background.");
      min_peak = l0 + 12;
      max_peak = n - 13;
      if (min_peak > sizey)
        min_peak = sizey - 1;
      // FIXME : as it is assumed that background is 12 first and 12 last, then
      //         why not do a simple fit here!
      a0 = 0.0;
      a1 = 0.0;
      a2 = 0.0;
      goodfit = -1;
    }

    // Add a new row
    API::TableRow t = m_outPeakTableWS->getRow(0);
    t << static_cast<int>(inpwsindex) << static_cast<int>(min_peak)
      << static_cast<int>(max_peak) << a0 << a1 << a2 << goodfit;
  }

  prog.report();

  // 4. Set the output
  setProperty("OutputWorkspace", m_outPeakTableWS);

  return;
}

//----------------------------------------------------------------------------------------------
/** Estimate background
* @param X :: vec for X
* @param Y :: vec for Y
* @param i_min :: index of minimum in X to estimate background
* @param i_max :: index of maximum in X to estimate background
* @param p_min :: index of peak min in X to estimate background
* @param p_max :: index of peak max in X to estimate background
* @param out_bg0 :: interception
* @param out_bg1 :: slope
* @param out_bg2 :: a2 = 0
*/
void FindPeakBackground::estimateBackground(
    const MantidVec &X, const MantidVec &Y, const size_t i_min,
    const size_t i_max, const size_t p_min, const size_t p_max, double &out_bg0,
    double &out_bg1, double &out_bg2) {
  // Validate input
  if (i_min >= i_max)
    throw std::runtime_error("i_min cannot larger or equal to i_max");
  if (p_min >= p_max)
    throw std::runtime_error("p_min cannot larger or equal to p_max");

  // set all parameters to zero
  out_bg0 = 0.;
  out_bg1 = 0.;
  out_bg2 = 0.;

  // accumulate sum
  double sum = 0.0;
  double sumX = 0.0;
  double sumY = 0.0;
  double sumX2 = 0.0;
  double sumXY = 0.0;
  double sumX2Y = 0.0;
  double sumX3 = 0.0;
  double sumX4 = 0.0;
  for (size_t i = i_min; i < i_max; ++i) {
    if (i >= p_min && i < p_max)
      continue;
    sum += 1.0;
    sumX += X[i];
    sumX2 += X[i] * X[i];
    sumY += Y[i];
    sumXY += X[i] * Y[i];
    sumX2Y += X[i] * X[i] * Y[i];
    sumX3 += X[i] * X[i] * X[i];
    sumX4 += X[i] * X[i] * X[i] * X[i];
  }

  // Estimate flat background
  double bg0_flat = 0.;
  if (sum != 0.)
    bg0_flat = sumY / sum;

  // Estimate linear - use Cramer's rule for 2 x 2 matrix
  double bg0_linear = 0.;
  double bg1_linear = 0.;
  double determinant = sum * sumX2 - sumX * sumX;
  if (determinant != 0) {
    bg0_linear = (sumY * sumX2 - sumX * sumXY) / determinant;
    bg1_linear = (sum * sumXY - sumY * sumX) / determinant;
  }

  // Estimate quadratic - use Cramer's rule for 3 x 3 matrix

  // | a b c |
  // | d e f |
  // | g h i |
  // 3 x 3 determinate:  aei+bfg+cdh-ceg-bdi-afh

  double bg0_quadratic = 0.;
  double bg1_quadratic = 0.;
  double bg2_quadratic = 0.;
  determinant = sum * sumX2 * sumX4 + sumX * sumX3 * sumX2 +
                sumX2 * sumX * sumX3 - sumX2 * sumX2 * sumX2 -
                sumX * sumX * sumX4 - sum * sumX3 * sumX3;
  if (determinant != 0) {
    bg0_quadratic =
        (sumY * sumX2 * sumX4 + sumX * sumX3 * sumX2Y + sumX2 * sumXY * sumX3 -
         sumX2 * sumX2 * sumX2Y - sumX * sumXY * sumX4 - sumY * sumX3 * sumX3) /
        determinant;
    bg1_quadratic =
        (sum * sumXY * sumX4 + sumY * sumX3 * sumX2 + sumX2 * sumX * sumX2Y -
         sumX2 * sumXY * sumX2 - sumY * sumX * sumX4 - sum * sumX3 * sumX2Y) /
        determinant;
    bg2_quadratic =
        (sum * sumX2 * sumX2Y + sumX * sumXY * sumX2 + sumY * sumX * sumX3 -
         sumY * sumX2 * sumX2 - sumX * sumX * sumX2Y - sum * sumXY * sumX3) /
        determinant;
  }

  // calculate the chisq - not normalized by the number of points
  double chisq_flat = 0.;
  double chisq_linear = 0.;
  double chisq_quadratic = 0.;
  if (sum != 0) {
    for (size_t i = i_min; i < i_max; ++i) {
      if (i >= p_min && i < p_max)
        continue;

      // accumulate for flat
      chisq_flat += (bg0_flat - Y[i]) * (bg0_flat - Y[i]);

      // accumulate for linear
      double temp = bg0_linear + bg1_linear * X[i] - Y[i];
      chisq_linear += (temp * temp);

      // accumulate for quadratic
      temp = bg0_quadratic + bg1_quadratic * X[i] +
             bg2_quadratic * X[i] * X[i] - Y[i];
      chisq_quadratic += (temp * temp);
    }
  }
  const double INVALID_CHISQ(1.e10); // big invalid value
  if (m_backgroundType == "Flat") {
    chisq_linear = INVALID_CHISQ;
    chisq_quadratic = INVALID_CHISQ;
  } else if (m_backgroundType == "Linear") {
    chisq_quadratic = INVALID_CHISQ;
  }

  // choose the right background function to apply
  if ((chisq_quadratic < chisq_flat) && (chisq_quadratic < chisq_linear)) {
    out_bg0 = bg0_quadratic;
    out_bg1 = bg1_quadratic;
    out_bg2 = bg2_quadratic;
  } else if ((chisq_linear < chisq_flat) && (chisq_linear < chisq_quadratic)) {
    out_bg0 = bg0_linear;
    out_bg1 = bg1_linear;
  } else {
    out_bg0 = bg0_flat;
  }

  g_log.debug() << "Estimated background: A0 = " << out_bg0
                << ", A1 = " << out_bg1 << ", A2 = " << out_bg2 << "\n";

  return;
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
} // namespace Algorithms
} // namespace Mantid
