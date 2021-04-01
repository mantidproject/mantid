// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** HyspecScharpfCorrection : Divide by cos(2alpha) where alpha is the angle
  between incident beam and the polarization direction. It assumes scattering
  in the horizontal plane
*/
class MANTID_ALGORITHMS_DLL HyspecScharpfCorrection : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void execEvent();
  /**
   * Execute Scharpf correction for event lists
   * @param wevector the list of events to correct
   * @param thPlane the in-plane angle for the detector corresponding to the
   * event list
   */
  template <class T> void ScharpfEventHelper(std::vector<T> &wevector, double thPlane);
  /**
   * @brief calculate the Scharph angle correction factor
   * @param kfki kf/ki
   * @param thPlane the in-plane angle of the detector
   * @return factor
   */
  float calculateFactor(const double kfki, const double thPlane);
  /// The user selected (input) workspace
  Mantid::API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  Mantid::API::MatrixWorkspace_sptr m_outputWS;
  /// In plane angle beween polarization and incident beam (in degrees)
  double m_angle;
  /// Lower limit  for abs(cos(2*Scharpf angle)), below which intensities are 0
  double m_precision;
  /// Incident energy
  double m_Ei;
};

} // namespace Algorithms
} // namespace Mantid
