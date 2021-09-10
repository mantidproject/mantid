// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
namespace Mantid {
namespace MDAlgorithms {

/** CalculateCoverageDGS : Calculate coverage for single crystal direct geometry
  scattering
*/
class DLLExport CalculateCoverageDGS : public API::Algorithm {
public:
  CalculateCoverageDGS();
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"SetGoniometer", "SetUB"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string convention;
  /// limits for h,k,l,dE dimensions
  coord_t m_hmin, m_hmax, m_kmin, m_kmax, m_lmin, m_lmax, m_dEmin, m_dEmax;
  /// cached values for incident energy and momentum, final momentum min/max
  double m_Ei, m_ki, m_kfmin, m_kfmax;
  /// flag for integrated h,k,l,dE dimensions
  bool m_hIntegrated, m_kIntegrated, m_lIntegrated, m_dEIntegrated;
  /// cached X values along dimensions h,k,l, dE
  std::vector<double> m_hX, m_kX, m_lX, m_eX;
  /// index of h,k,l,dE dimensions in the output workspaces
  size_t m_hIdx, m_kIdx, m_lIdx, m_eIdx;
  /// (2*PiRUBW)^-1
  Mantid::Kernel::DblMatrix m_rubw;

  /// Normalization workspace (this is the coverage workspace)
  Mantid::DataObjects::MDHistoWorkspace_sptr m_normWS;

  std::vector<Kernel::VMD> calculateIntersections(const double theta, const double phi);
  void cacheDimensionXValues();
};

} // namespace MDAlgorithms
} // namespace Mantid
