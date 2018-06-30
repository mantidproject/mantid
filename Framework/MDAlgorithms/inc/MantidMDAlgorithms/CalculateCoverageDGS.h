#ifndef MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_
#define MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
namespace Mantid {
namespace MDAlgorithms {

/** CalculateCoverageDGS : Calculate coverage for single crystal direct geometry
  scattering

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
  National Laboratory

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
class DLLExport CalculateCoverageDGS : public API::Algorithm {
public:
  CalculateCoverageDGS();
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SetGoniometer", "SetUB"};
  }
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

  std::vector<Kernel::VMD> calculateIntersections(const double theta,
                                                  const double phi);
  void cacheDimensionXValues();
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_ */
