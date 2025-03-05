// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include <memory>
#include <utility>
#include <vector>

namespace Mantid {
namespace Kernel {
class MersenneTwister;
}
namespace Algorithms {

/**

  Applies the procedure found in section 4 of
  https://inis.iaea.org/search/search.aspx?orig_q=RN:20000574 for an array of
  tof, signal & error values
*/
class MANTID_ALGORITHMS_DLL MayersSampleCorrectionStrategy {
public:
  /**
   * Stores parameters for a single calculation for a given angle
   * and sample details
   */
  struct Parameters {
    bool mscat;       ///< If true then correct for multiple scattering
    double l1;        ///< Nominal distance from source to sample (m)
    double l2;        ///< Nominal distance from sample to detector (m)
    double twoTheta;  ///< Scattering angle of the detector (radians)
    double azimuth;   ///< Azimuth angle of the detector (radians)
    double rho;       ///< Number density of scatters (angstroms^-3)
    double sigmaSc;   ///< Total scattering cross-section (barns)
    double sigmaAbs;  ///< Absorption cross-section at 2200m/s (barns)
    double cylRadius; ///< Radius of cylinder (m)
    double cylHeight; ///< Height of cylinder (m)
    size_t msNEvents; ///< Number of second-order scatters per run
    size_t msNRuns;   ///< Number of runs to average ms correction over
  };

  /// Constructor
  MayersSampleCorrectionStrategy(MayersSampleCorrectionStrategy::Parameters params, HistogramData::Histogram inputHist);
  /// Destructor - defined in cpp file to use forward declaration with
  /// unique_ptr
  ~MayersSampleCorrectionStrategy();

  /// Return the correction factors
  Mantid::HistogramData::Histogram getCorrectedHisto();

  /// Calculate the self-attenuation factor for a single mu*r value
  double calculateSelfAttenuation(const double muR);
  /// Calculate the multiple scattering factor for a single mu*r value &
  /// absorption value
  std::pair<double, double> calculateMS(const size_t irp, const double muR, const double abs);

private:
  inline double muRmin() const { return m_muRrange.first; }
  inline double muRmax() const { return m_muRrange.second; }

  std::pair<double, double> calculateMuRange() const;
  double muR(const double flightPath, const double tof) const;
  double muR(const double sigt) const;
  double sigmaTotal(const double flightPath, const double tof) const;
  void seedRNG(const size_t seed);

  /// A copy of the correction parameters
  const Parameters m_pars;
  // Holds histogram to process
  const HistogramData::Histogram m_histogram;
  const HistogramData::Points m_tofVals;
  /// Holds the number of Y vals to process
  const size_t m_histoYSize;
  /// Limits for the range of mu*r values to cover
  const std::pair<double, double> m_muRrange;
  /// Random number generator
  std::unique_ptr<Kernel::MersenneTwister> m_rng;
};

} // namespace Algorithms
} // namespace Mantid
