#ifndef MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_
#define MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Matrix.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
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
  virtual ~CalculateCoverageDGS();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();

  /// limits for h,k,l,dE dimensions
  coord_t m_hmin, m_hmax, m_kmin, m_kmax, m_lmin, m_lmax, m_dEmin, m_dEmax;
  /// cached values for incident energy and momentum
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
  Mantid::MDEvents::MDHistoWorkspace_sptr m_normWS;

  std::vector<Kernel::VMD> calculateIntersections(const double theta,
                                                  const double phi);
  void cacheDimensionXValues();
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_ */
