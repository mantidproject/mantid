#ifndef MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_
#define MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** CalculateCoverageDGS : TODO: DESCRIPTION

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport CalculateCoverageDGS  : public API::Algorithm
  {
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

    std::vector<Kernel::VMD> calculateIntersections(const double theta, const double phi);

    /// limits for h,k,l,dE dimensions
    coord_t m_hmin, m_hmax, m_kmin, m_kmax, m_lmin, m_lmax, m_dEmin, m_dEmax;
    /// flag for integrated h,k,l,dE dimensions
    bool m_hIntegrated, m_kIntegrated, m_lIntegrated, m_dEIntegrated;
    /// (2*PiRUBW)^-1
    Mantid::Kernel::DblMatrix m_rubw;
    //void cacheDimensionXValues();
  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_CALCULATECOVERAGEDGS_H_ */
