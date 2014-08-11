#ifndef MANTID_MDALGORITHMS_SXDMDNORM_H_
#define MANTID_MDALGORITHMS_SXDMDNORM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"
#include <unordered_map>

namespace Mantid
{
namespace MDAlgorithms
{

  /** SXDMDNorm : Generate MD normalization for single crystal diffraction
    
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
  class DLLExport SXDMDNorm  : public SlicingAlgorithm
  {
  public:
    SXDMDNorm();
    virtual ~SXDMDNorm();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();
    std::vector<Mantid::Kernel::VMD> calculateIntersections(Mantid::Geometry::IDetector_const_sptr detector);
    size_t m_nDims;
    Mantid::MDEvents::MDHistoWorkspace_sptr m_normWS;
    Mantid::API::IMDEventWorkspace_sptr m_inputWS;
    ///limits for h,k,l dimensions
    coord_t hMin,hMax,kMin,kMax,lMin,lMax;
    ///flag for integrated h,k,l dimensions
    bool hIntegrated,kIntegrated,lIntegrated;
    ///index of h,k,l dimensions in the output workspaces
    size_t hIndex,kIndex,lIndex;
    ///name of other dimensions
    std::vector<std::string> otherDims;
    ///limits for other dimensions
    std::vector<coord_t> otherDimsMin,otherDimsMax;
    ///flag id other dimensions are integrated
    std::vector<bool> otherDimsIntegrated;
    ///index of other dimensions in the output workspaces
    std::vector<size_t> otherDimsIndex;
    ///map dimensions
    std::unordered_map<size_t,size_t> dim;
  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_SXDMDNORM_H_ */
