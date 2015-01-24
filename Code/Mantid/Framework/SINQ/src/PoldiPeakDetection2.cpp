//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiPeakDetection2.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ConstraintFactory.h"

#include <boost/shared_ptr.hpp>

#include <cmath>

using namespace std;

namespace Mantid {
namespace Poldi {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiPeakDetection2)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Initialisation method.
void PoldiPeakDetection2::init() {

  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "InputWorkspace", "", Direction::InOut),
                  "The input Workspace2D containing the correlated function"
                  "with columns containing key summary information about the "
                  "Poldi spectra.");

  // defaut threshold used to detect peaks
  double PeakDetectionThreshold = 0.2;
  declareProperty(
      "PeakDetectionThreshold", PeakDetectionThreshold,
      "Threshold for the peak detection,\n"
      "default value is 0.2.\n"
      "The intensity max of a peak is at least 20% the whole max intensity.\n"
      "Has to be between 0 and 1");

  // Data
  declareProperty(
      new WorkspaceProperty<DataObjects::TableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
      "The output TableWorkspace containing the detected peak information"
      "with one row per peak");
}

/** ***************************************************************** */

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 */
void PoldiPeakDetection2::exec() {

  g_log.information() << "_Poldi  start conf --------------  " << std::endl;

  ////////////////////////////////////////////////////////////////////////
  // About the workspace
  ////////////////////////////////////////////////////////////////////////

  this->ws_auto_corr = this->getProperty("InputWorkspace");
  Mantid::MantidVec &X = ws_auto_corr->dataX(0);
  Mantid::MantidVec &Y = ws_auto_corr->dataY(0);

  this->nb_d_channel = Y.size();
  g_log.information() << "                 nb_d_channel = "
                      << this->nb_d_channel << std::endl;

  DataObjects::TableWorkspace_sptr ws_sample_logs =
      this->getProperty("OutputWorkspace");

  ////////////////////////////////////////////////////////////////////////
  // About the output workspace, to store the peaks information
  ////////////////////////////////////////////////////////////////////////

  ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable();

  outputws->addColumn("double", "PeakCentre");
  outputws->addColumn("double", "Height");
  outputws->addColumn("double", "Sigma");
  outputws->addColumn("int", "imin");
  outputws->addColumn("int", "ipos");
  outputws->addColumn("int", "imax");

  ////////////////////////////////////////////////////////////////////////
  // About the workspace
  ////////////////////////////////////////////////////////////////////////

  // to remove the wires used already fitted
  this->table_dead_wires.resize(nb_d_channel);
  for (size_t ipk = 0; ipk < nb_d_channel; ipk++) {
    table_dead_wires[ipk] = true;
  }

  g_log.information() << "_Poldi ws loaded --------------  " << std::endl;

  ////////////////////////////////////////////////////////////////////////
  // About the threshold
  ////////////////////////////////////////////////////////////////////////

  double PeakDetectionThreshold = this->getProperty("PeakDetectionThreshold");

  ////////////////////////////////////////////////////////////////////////
  // initialisation before peak detection
  ////////////////////////////////////////////////////////////////////////
  // peak positionned at the max intensity
  size_t imax = this->getIndexOfMax(); // indeice of max value
  double vmax0 = Y[imax];              // max value
  g_log.information() << "_Poldi peak detection  : imax = " << imax
                      << std::endl;
  double vmax = vmax0;
  g_log.information() << "                         vmax = " << vmax
                      << std::endl;

  int count = 0; // number of detected peaks

  // reset of the 3rd line of the correlation ws, to store the simulated peaks
  Mantid::MantidVec &Y2 = this->ws_auto_corr->dataY(2);
  for (size_t i = 0; i < nb_d_channel; i++) {
    Y2[i] = 0;
  }

  // peak detection while there is intensity higher than 20% of the max
  // intensity
  while ((imax > 0) && (vmax > PeakDetectionThreshold * vmax0)) {
    count++; // one peak detected
    g_log.information() << "_Poldi peak detection, search for peak " << count
                        << std::endl;

    // indices and values for the fwhm detection
    size_t ifwhm_min = imax;
    size_t ifwhm_max = imax;
    double vfwhm_min = vmax;
    double vfwhm_max = vmax;
    // fwhm detection
    for (; vfwhm_min > 0.5 * vmax; ifwhm_min--, vfwhm_min = Y[ifwhm_min]) {
    }
    for (; vfwhm_max > 0.5 * vmax; ifwhm_max++, vfwhm_max = Y[ifwhm_max]) {
    }
    double fwhm = X[ifwhm_max] - X[ifwhm_min + 1];

    // determination of the range used for the peak definition
    size_t ipeak_min =
        max(0, imax - int(2.5 * static_cast<double>(imax - ifwhm_min)));
    size_t ipeak_max = min(
        nb_d_channel, imax + int(2.5 * static_cast<double>(ifwhm_max - imax)));
    size_t i_delta_peak = ipeak_max - ipeak_min;

    // the used wires are removed
    for (size_t i = ipeak_min; i < ipeak_max; i++) {
      table_dead_wires[i] = false;
    }

    // parameters for the gaussian peak fit
    double center = X[imax];
    double sigma = fwhm;
    double height = vmax;

    g_log.debug() << "_Poldi peak before   " << center << "\t" << sigma << "\t"
                  << height << std::endl;
    g_log.debug() << "_Poldi peak xmin/max " << X[ipeak_min - 1] << "\t"
                  << X[ipeak_max + 1] << std::endl;
    double doFit = doFitGaussianPeak(ws_auto_corr, 0, center, sigma, height,
                                     X[ipeak_min - 2 * i_delta_peak],
                                     X[ipeak_max + 2 * i_delta_peak]);
    if (!doFit) {
      g_log.error() << "_Poldi peak after    : fit failed" << std::endl;
    }
    g_log.debug() << "_Poldi peak after    " << center << "\t" << sigma << "\t"
                  << height << std::endl;

    // the information are stored in the ws, fit or not fit...
    TableRow t0 = outputws->appendRow();
    t0 << center << height << 2.35 * sigma << int(ipeak_min) << int(imax)
       << int(ipeak_max);

    // the simulated peak is stored in the correlation ws, row 3
    const double &weight = pow(1 / sigma, 2);
    for (size_t i = 0; i < this->nb_d_channel; i++) {
      double diff = X[i] - center;
      Y2[i] += height * exp(-0.5 * diff * diff * weight);
    }

    // reinitialisation for the next peak detection
    imax = this->getIndexOfMax();
    vmax = Y[imax];
  }

  // keep only the used wires in the correlated ws, should correspond to the
  // sinulated peak at the end
  Mantid::MantidVec &Y1 = this->ws_auto_corr->dataY(1);
  for (size_t i = 0; i < nb_d_channel; i++) {
    if (!table_dead_wires[i]) {
      Y1[i] = Y[i];
    } else {
      Y1[i] = 0;
    }
  }

  // store the peaks information ws in the mainframe
  setProperty("OutputWorkspace", outputws);
}

/** Fit peak without background i.e, with background removed
 *
 *  inspire from FitPowderDiffPeaks.cpp
 *
    @param dataws :: input raw data for the fit
    @param workspaceindex :: indice of the row to use
    @param center :: gaussian parameter - center
    @param sigma :: gaussian parameter - width
    @param height :: gaussian parameter - height
    @param startX :: fit range - start X value
    @param endX :: fit range - end X value
    @returns A boolean status flag, true for fit success, false else
  */
bool PoldiPeakDetection2::doFitGaussianPeak(
    DataObjects::Workspace2D_sptr dataws, int workspaceindex, double &center,
    double &sigma, double &height, double startX, double endX) {
  // 1. Estimate
  sigma = sigma * 0.5;

  // 2. Use factory to generate Gaussian
  auto temppeak = API::FunctionFactory::Instance().createFunction("Gaussian");
  auto gaussianpeak = boost::dynamic_pointer_cast<API::IPeakFunction>(temppeak);
  gaussianpeak->setHeight(height);
  gaussianpeak->setCentre(center);
  gaussianpeak->setFwhm(sigma);

  // 3. Constraint
  double centerleftend = center - sigma * 0.5;
  double centerrightend = center + sigma * 0.5;
  std::ostringstream os;
  os << centerleftend << " < PeakCentre < " << centerrightend;
  auto *centerbound = API::ConstraintFactory::Instance().createInitialized(
      gaussianpeak.get(), os.str(), false);
  gaussianpeak->addConstraint(centerbound);

  // 4. Fit
  API::IAlgorithm_sptr fitalg = createChildAlgorithm("Fit", -1, -1, true);
  fitalg->initialize();

  fitalg->setProperty(
      "Function", boost::dynamic_pointer_cast<API::IFunction>(gaussianpeak));
  fitalg->setProperty("InputWorkspace", dataws);
  fitalg->setProperty("WorkspaceIndex", workspaceindex);
  fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", 1000);
  fitalg->setProperty("Output", "FitGaussianPeak");
  fitalg->setProperty("StartX", startX);
  fitalg->setProperty("EndX", endX);

  // 5.  Result
  bool successfulfit = fitalg->execute();
  if (!fitalg->isExecuted() || !successfulfit) {
    // Early return due to bad fit
    g_log.warning() << "Fitting Gaussian peak for peak around "
                    << gaussianpeak->centre() << std::endl;
    return false;
  }

  // 6. Get result
  center = gaussianpeak->centre();
  height = gaussianpeak->height();
  double fwhm = gaussianpeak->fwhm();
  if (fwhm <= 0.0) {
    return false;
  }
  //	  sigma = fwhm*2;
  //  sigma = fwhm/2.35;

  return true;
}

/** return the indice of the maximal value in the graph

  @returns imax :: indice of max value;
  */
int PoldiPeakDetection2::getIndexOfMax() {
  int imax = -1;
  double vmax = 0;
  double temp;
  for (size_t i = 0; i < nb_d_channel; i++) {
    if (table_dead_wires[i]) {
      temp = this->ws_auto_corr->dataY(0)[i];
      if (temp > vmax) {
        vmax = temp;
        imax = static_cast<int>(i);
      }
    }
  }
  return imax;
}

} // namespace Poldi
} // namespace Mantid
