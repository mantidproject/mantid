#ifndef MANTID_MDALGORITHMS_STITCH1D_H_
#define MANTID_MDALGORITHMS_STITCH1D_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/MultiPeriodGroupAlgorithm.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** Stitch1DMD : Stitches together two MDWorkspaces in Q-space.
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport Stitch1DMD  : public API::MultiPeriodGroupAlgorithm, public API::DeprecatedAlgorithm
  {
  public:
    Stitch1DMD();
    virtual ~Stitch1DMD();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Stitch two MD ReflectometryQ group workspaces together.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:

    std::string fetchInputPropertyName() const;
    void checkIndividualWorkspace(Mantid::API::IMDHistoWorkspace_const_sptr ws) const;
    void checkBothWorkspaces(Mantid::API::IMDHistoWorkspace_const_sptr rhsWorkspace, Mantid::API::IMDHistoWorkspace_const_sptr lhsWorkspace) const;
    Mantid::MDEvents::MDHistoWorkspace_sptr trimOutIntegratedDimension(Mantid::API::IMDHistoWorkspace_sptr ws);
    double integrateOver(Mantid::API::IMDHistoWorkspace_sptr ws, const double& startOverlap, const double& endOverlap);
    Mantid::MDEvents::MDHistoWorkspace_sptr create1DHistoWorkspace(const MantidVec& signals,const MantidVec& errors, const MantidVec& extents, const std::vector<int>& vecNBins, const std::vector<std::string> names, const std::vector<std::string>& units);
    void overlayOverlap(Mantid::MDEvents::MDHistoWorkspace_sptr original, Mantid::API::IMDHistoWorkspace_sptr overlap);
    Mantid::MDEvents::MDHistoWorkspace_sptr extractOverlapAsWorkspace(Mantid::API::IMDHistoWorkspace_sptr ws, const double& startOverlap, const double& endOverlap);
    void init();
    void exec();


  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_STITCH1D_H_ */
