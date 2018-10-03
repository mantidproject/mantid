// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GETEI2_H_
#define MANTID_ALGORITHMS_GETEI2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

//----------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------
namespace Mantid {
namespace HistogramData {
class HistogramY;
class HistogramX;
class HistogramE;
} // namespace HistogramData
} // namespace Mantid
namespace Mantid {
namespace Algorithms {
/** Requires an estimate for the initial neutron energy which it uses to
  search for monitor peaks and from these calculate an accurate energy

    Required Properties:
    <UL>
    <LI>InputWorkspace - The X units of this workspace must be time of flight
  with times in micro-seconds</LI>
    <LI>Monitor1ID - The detector ID of the first monitor</LI>
    <LI>Monitor2ID - The detector ID of the second monitor</LI>
    <LI>EnergyEstimate - An approximate value for the typical incident energy,
  energy of neutrons leaving the source (meV)</LI>
    <LI>IncidentEnergy - The calculated energy</LI>
    </UL>

    @author Martyn Gigg ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
  National Laboratory
    @date 31/03/2010
*/
class DLLExport GetEi2 : public API::Algorithm {
public:
  /// Default constructor
  GetEi2();
  /// Initialize the algorithm
  void init() override;
  /// Execute the algorithm
  void exec() override;

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GetEi"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the kinetic energy of neutrons leaving the source based "
           "on the time it takes for them to travel between two monitors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"GetAllEi", "GetEiMonDet", "GetEiT0atSNS"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Inelastic\\Ei"; }

private:
  /// Calculate Ei from the initial guess given
  double calculateEi(const double initial_guess);
  /// Get the distance from the source of the detector at the workspace index
  /// given
  double getDistanceFromSource(const size_t ws_index,
                               const API::SpectrumInfo &spectrumInfo) const;
  /// Calculate the peak position within the given window
  double calculatePeakPosition(const size_t ws_index, const double t_min,
                               const double t_max);
  /// Extract the required spectrum from the input workspace
  API::MatrixWorkspace_sptr
  extractSpectrum(const size_t ws_index, const double start, const double end);
  /// Calculate peak width
  double calculatePeakWidthAtHalfHeight(API::MatrixWorkspace_sptr data_ws,
                                        const double prominence,
                                        std::vector<double> &peak_x,
                                        std::vector<double> &peak_y,
                                        std::vector<double> &peak_e) const;
  /// Calculate the value of the first moment of the given spectrum
  double calculateFirstMoment(API::MatrixWorkspace_sptr monitor_ws,
                              const double prominence);
  /// Rebin the given workspace using the given parameters
  API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr monitor_ws,
                                  const double first, const double width,
                                  const double end);
  /// Integrate the point data
  void integrate(double &integral_val, double &integral_err,
                 const HistogramData::HistogramX &x,
                 const HistogramData::HistogramY &s,
                 const HistogramData::HistogramE &e, const double xmin,
                 const double xmax) const;
  /// Store the incident energy within the sample object
  void storeEi(const double ei) const;

  /// The input workspace
  API::MatrixWorkspace_sptr m_input_ws;
  /// The calculated position of the first peak
  std::pair<int, double> m_peak1_pos;
  /// True if the Ei should be fixed at the guess energy
  bool m_fixedei;
  /// Conversion factor between time and energy
  double m_t_to_mev;
  /// The percentage deviation from the estimated peak time that defines the
  /// peak region
  double m_tof_window;
  /// Number of std deviations to consider a peak
  const double m_peak_signif;
  /// Number of std deviations to consider a peak for the derivative
  const double m_peak_deriv;
  /// The fraction of the peak width for the new bins
  const double m_binwidth_frac;
  /// The fraction of the peakwidth to use in calculating background points
  const double m_bkgd_frac;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_GETEI2_H_*/
