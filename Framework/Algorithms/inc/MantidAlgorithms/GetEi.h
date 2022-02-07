// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/cow_ptr.h"

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

    @author Steve Williams ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
  National Laboratory
    @date 27/07/2009
*/
class MANTID_ALGORITHMS_DLL GetEi : public API::Algorithm {
public:
  GetEi();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GetEi"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the kinetic energy of neutrons leaving the source based "
           "on the time it takes for them to travel between two monitors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Inelastic\\Ei"; }

private:
  /// name of the tempory workspace that we create and use
  API::MatrixWorkspace_sptr m_tempWS;
  /// An estimate of the percentage of the algorithm runtimes that has been
  /// completed
  double m_fracCompl;
  /// used by the function findHalfLoc to indicate whether to search left or
  /// right
  enum direction {
    GO_LEFT = -1, ///< flag value to serch left
    GO_RIGHT = 1  ///< flag value to search right
  };

  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;

  void getGeometry(const API::MatrixWorkspace_const_sptr &WS, specnum_t mon0Spec, specnum_t mon1Spec,
                   double &monitor0Dist, double &monitor1Dist) const;
  std::vector<size_t> getMonitorWsIndexs(const API::MatrixWorkspace_const_sptr &WS, specnum_t specNum1,
                                         specnum_t specNum2) const;
  double timeToFly(double s, double E_KE) const;
  double getPeakCentre(const API::MatrixWorkspace_const_sptr &WS, const size_t monitIn, const double peakTime);
  void extractSpec(int wsInd, double start, double end);
  void getPeakEstimates(double &height, int64_t &centreInd, double &background) const;
  double findHalfLoc(MantidVec::size_type startInd, const double height, const double noise, const direction go) const;
  double neutron_E_At(double speed) const;
  void advanceProgress(double toAdd);

  /// the range of TOF X-values over which the peak will be searched is double
  /// this value, i.e. from the estimate of the peak position the search will go
  /// forward by this fraction and back by this fraction
  static const double HALF_WINDOW;
  /// ignore an peaks that are less than this factor of the background
  static const double PEAK_THRESH_H;
  /// ignore peaks where the half width times the ratio of the peak height to
  /// the background is less this
  static const double PEAK_THRESH_A;
  /// for peaks where the distance to the half heigth is less than this number
  /// of bins in either direction e.g. the FWHM is less than twice this number
  static const int64_t PEAK_THRESH_W;

  // for estimating algorithm progress
  static const double CROP;           ///< fraction of algorithm time taken up with running CropWorkspace
  static const double GET_COUNT_RATE; ///< fraction of algorithm taken by a
  /// single call to ConvertToDistribution
  static const double FIT_PEAK; ///< fraction required to find a peak
};

} // namespace Algorithms
} // namespace Mantid
