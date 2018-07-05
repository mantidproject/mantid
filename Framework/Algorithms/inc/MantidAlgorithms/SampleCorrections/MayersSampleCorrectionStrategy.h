#ifndef MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGY_H_
#define MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGY_H_
/**
  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/System.h"
#include <memory>
#include <utility>
#include <vector>

namespace Mantid {
namespace Kernel {
class PseudoRandomNumberGenerator;
}
namespace Algorithms {

/**

  Applies the procedure found in section 4 of
  https://inis.iaea.org/search/search.aspx?orig_q=RN:20000574 for an array of
  tof, signal & error values
*/
class DLLExport MayersSampleCorrectionStrategy {
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
  MayersSampleCorrectionStrategy(
      MayersSampleCorrectionStrategy::Parameters params,
      const Mantid::HistogramData::Histogram &inputHist);
  /// Destructor - defined in cpp file to use forward declaration with
  /// unique_ptr
  ~MayersSampleCorrectionStrategy();

  /// Return the correction factors
  Mantid::HistogramData::Histogram getCorrectedHisto();

  /// Calculate the self-attenuation factor for a single mu*r value
  double calculateSelfAttenuation(const double muR);
  /// Calculate the multiple scattering factor for a single mu*r value &
  /// absorption value
  std::pair<double, double> calculateMS(const size_t irp, const double muR,
                                        const double abs);

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
  const HistogramData::Histogram &m_histogram;
  const HistogramData::Points m_tofVals;
  /// Holds the number of Y vals to process
  const size_t m_histoYSize;
  /// Limits for the range of mu*r values to cover
  const std::pair<double, double> m_muRrange;
  /// Random number generator
  std::unique_ptr<Kernel::PseudoRandomNumberGenerator> m_rng;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGY_H_ */
