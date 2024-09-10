// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidCrystal/FindGoniometerAngles.h"

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"

namespace Mantid {
namespace Crystal {
using Mantid::API::IPeaksWorkspace;
using Mantid::API::IPeaksWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::Geometry::Goniometer;
using Mantid::Geometry::IndexingUtils;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::Direction;
using Mantid::Kernel::V3D;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindGoniometerAngles)

namespace {

DblMatrix createRotationMatrix(const double phi, const double chi, const double omega) {
  Goniometer g;
  g.pushAxis("omega", 0., 1., 0., omega);
  g.pushAxis("chi", 0., 0., 1., chi);
  g.pushAxis("phi", 0., 1., 0., phi);
  return g.getR();
}

std::pair<int, double> NumIndexed(const DblMatrix &UB, const IPeaksWorkspace_sptr peakWS, const double tolerance,
                                  const double phi, const double chi, const double omega) {
  DblMatrix UBinv = UB;
  UBinv.Invert();

  auto Rinv = createRotationMatrix(phi, chi, omega);
  Rinv.Invert();

  int n_indexed{0};
  double sum_sq_err{0};

  for (int i{0}; i < peakWS->getNumberPeaks(); i++) {
    const auto &peak = peakWS->getPeak(i);
    const auto q_lab = peak.getQLabFrame();
    const auto q_sample = Rinv * q_lab;
    const auto hkl = UBinv * q_sample / (2 * M_PI);

    if (IndexingUtils::ValidIndex(hkl, tolerance)) {
      n_indexed++;
      V3D HKL = hkl;
      HKL.round();
      V3D q_error = UB * HKL;
      q_error *= 2 * M_PI;
      q_error -= q_sample;
      sum_sq_err += q_error.norm2();
    }
  }

  return std::make_pair(n_indexed, sum_sq_err);
}
} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string FindGoniometerAngles::name() const { return "FindGoniometerAngles"; }

/// Algorithm's version for identification. @see Algorithm::version
int FindGoniometerAngles::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FindGoniometerAngles::category() const { return "Crystal\\Corrections"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string FindGoniometerAngles::summary() const {
  return "Do a brute force search for the goniometer rotation angles that maximize the number of peaks indexed by the "
         "specified UB.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FindGoniometerAngles::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeaksWorkspace", "", Direction::Input),
                  "Workspace of Peaks with UB loaded");
  declareProperty(
      "MaxAngle", 5.0,
      "The maximum change in angle to try for any of the goniometer angles, phi, chi and omega, in degrees.",
      Direction::Input);
  declareProperty("Tolerance", 0.15, "The tolerance on Miller indices for a peak to be considered indexed",
                  Direction::Input);
  declareProperty("Apply", false, "Update goniometer in peaks workspace");
  declareProperty("Phi", 0.0, "Phi found", Direction::Output);
  declareProperty("Chi", 0.0, "Chi found", Direction::Output);
  declareProperty("Omega", 0.0, "Omega found", Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void FindGoniometerAngles::exec() {

  const double tolerance = getProperty("Tolerance");
  const double max_angle = getProperty("MaxAngle");
  IPeaksWorkspace_sptr peakWS = getProperty("PeaksWorkspace");

  const auto UB = peakWS->sample().getOrientedLattice().getUB();

  auto g = peakWS->run().getGoniometer();
  const std::vector<double> YZY = g.getEulerAngles("YZY");

  double omega = YZY[0];
  double chi = YZY[1];
  double phi = YZY[2];

  auto results = NumIndexed(UB, peakWS, tolerance, phi, chi, omega);
  g_log.information() << "Starting          Max Indexed = " << results.first << " Err = " << results.second
                      << " Phi: " << phi << " Chi: " << chi << " Omega: " << omega << "\n";

  const int N_TRIES = 5;

  double phi_offset{phi};
  double chi_offset{chi};
  double omega_offset{omega};
  double best_phi{0};
  double best_chi{0};
  double best_omega{0};

  auto progress = std::make_unique<API::Progress>(this, 0.0, 1.0, N_TRIES);

  for (int range{1}; range <= N_TRIES; range++) {
    double max_quality{0};
    double max_error{0};
    int max_indexed{0};

    double step = max_angle / range;

    while (step > sqrt(1e-5)) {
      for (int i{-range}; i <= range; i++)
        for (int j{-range}; j <= range; j++)
          for (int k{-range}; k <= range; k++) {
            phi = phi_offset + i * step;
            chi = chi_offset + j * step;
            omega = omega_offset + k * step;
            results = NumIndexed(UB, peakWS, tolerance, phi, chi, omega);
            const double quality = results.first - 0.1 * results.second / results.first;

            if (quality > max_quality) {
              max_quality = quality;
              max_indexed = results.first;
              max_error = results.second;

              best_phi = phi;
              best_chi = chi;
              best_omega = omega;
            }
          }

      phi_offset = best_phi;
      chi_offset = best_chi;
      omega_offset = best_omega;

      step *= M_SQRT1_2;
    }

    g_log.information() << "Range Factor = " << range << "  "
                        << "Max Indexed = " << max_indexed << " Err = " << max_error << " Phi: " << best_phi
                        << " Chi: " << best_chi << " Omega: " << best_omega << "\n";
    progress->report();
  }

  setProperty("Phi", best_phi);
  setProperty("Chi", best_chi);
  setProperty("Omega", best_omega);

  if (getProperty("Apply")) {
    const auto R = createRotationMatrix(best_phi, best_chi, best_omega);
    // set the goniometer on the workspace
    peakWS->mutableRun().setGoniometer(R, false);

    // set the goniometer on the peaks and reset the q_lab so that the q_sample is recalculated
    for (int i{0}; i < peakWS->getNumberPeaks(); i++) {
      auto &peak = peakWS->getPeak(i);
      const auto q_lab = peak.getQLabFrame();
      peak.setGoniometerMatrix(R);
      peak.setQLabFrame(q_lab, std::nullopt);
    }
  }
}

} // namespace Crystal
} // namespace Mantid
