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
#include "MantidKernel/System.h"
#include "MantidHistogramData/Histogram.h"
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
    double phi;       ///< Azimuth angle of the detector (radians)
    double rho;       ///< Number density of scatters (angstroms^-3)
    double sigmaSc;   ///< Total scattering cross-section (barns)
    double sigmaAbs;  ///< Absorption cross-section at 2200m/s (barns)
    double cylRadius; ///< Radius of cylinder (m)
    double cylHeight; ///< Height of cylinder (m)
  };

  /// Constructor
  MayersSampleCorrectionStrategy(
      MayersSampleCorrectionStrategy::Parameters params,
      const Mantid::HistogramData::Histogram &inputHist);
  /// Destructor - defined in cpp file to use forward declaration with
  /// unique_ptr
  ~MayersSampleCorrectionStrategy();

  /// Return the correction factors
  Mantid::HistogramData::Histogram
  apply(const Mantid::HistogramData::Histogram &inputXVals);

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
  double tof(const size_t i) const;
  void seedRNG(const size_t seed);

  /// A copy of the correction parameters
  const Parameters m_pars;
  /// A reference to the tof vluaes
  const std::vector<double> &m_tof;
  /// A reference to the input signal values
  const std::vector<double> &m_sigin;
  /// A reference to the input error values
  const std::vector<double> &m_errin;
  // True if we have binned TOF values
  const bool m_histogram;
  /// Limits for the range of mu*r values to cover
  const std::pair<double, double> m_muRrange;
  /// Random number generator
  std::unique_ptr<Kernel::PseudoRandomNumberGenerator> m_rng;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGY_H_ */
