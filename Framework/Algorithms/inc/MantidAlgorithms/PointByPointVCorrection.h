// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** First attempt at spectrum by spectrum division for vanadium normalisation
   correction.

    @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
    @date 04/03/2009
*/
class MANTID_ALGORITHMS_DLL PointByPointVCorrection : public API::Algorithm {
public:
  PointByPointVCorrection();
  ~PointByPointVCorrection() override;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PointByPointVCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Spectrum by spectrum division for vanadium normalisation "
           "correction.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\Corrections;CorrectionFunctions\\SpecialCorrections";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void check_validity(API::MatrixWorkspace_const_sptr &w1, API::MatrixWorkspace_const_sptr &w2,
                      API::MatrixWorkspace_sptr &out);
  void check_masks(const API::MatrixWorkspace_const_sptr &w1, const API::MatrixWorkspace_const_sptr &w2,
                   const int &index) const;
};

} // namespace Algorithms
} // namespace Mantid
