// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Compute the generalised second difference of a spectrum or several spectra
 based on the method
 * described by M.A. Mariscotti., Nuclear Instruments and Methods 50, 309
 (1967).
 * This is used for example by a peak-finding algorithm.
 * Given a spectrum with value yi (0<=i<n), the generalised second difference
 corresponds to
 * the second difference curve, smoothed by averaging each point in the interval
 [-m,+m],
 * and applying this procedure z times.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace.
 </LI>
    <LI> m               - The number of points for averaging, i.e. summing will
 be done in the range [y(i-m),y(i+m)] </LI>
    <LI> z               - The number of iteration steps in the averaging
 procedure </LI>
    <LI> Spectra_min  - Lower bound of the spectra range </LI>
    <LI> Spectra_max  - Upper bound of the spectra range  </LI>
    </UL>

    @author Laurent C Chapon, ISIS Facility Rutherford Appleton Laboratory
    @date 26/12/2008
*/
class MANTID_ALGORITHMS_DLL GeneralisedSecondDifference : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "GeneralisedSecondDifference"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Computes the generalised second difference of a spectrum or "
           "several spectra.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Arithmetic"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  ///
  void computePrefactors();
  /// Vector that contains the prefactor coefficients Cij in the range
  /// [-zm-1,zm+1]
  std::vector<double> m_Cij;
  /// Vector that contains the prefactor coefficients Cij^2 in the range
  /// [-zm-1,zm+1]
  std::vector<double> m_Cij2;
  /// Contains the value of the property z
  int m_z = 0;
  /// Contains the value of the property m
  int m_m = 0;
  /// Progress reporting
};

} // Namespace Algorithms
} // Namespace Mantid
