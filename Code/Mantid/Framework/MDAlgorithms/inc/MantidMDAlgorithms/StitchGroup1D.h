#ifndef MANTID_MDALGORITHMS_STITCHGROUP1D_H_
#define MANTID_MDALGORITHMS_STITCHGROUP1D_H_

#include "MantidKernel/System.h"
#include "MantidAPI/MultiPeriodGroupAlgorithm.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace API
  {
    class IMDHistoWorkspace;
  }
  namespace MDEvents
  {
    class MDHistoWorkspace;
  }
namespace MDAlgorithms
{

  /** StitchGroup1D : TODO: DESCRIPTION
    
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport StitchGroup1D  : public API::MultiPeriodGroupAlgorithm
  {
  public:
    StitchGroup1D();
    virtual ~StitchGroup1D();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:

    std::string fetchInputPropertyName() const;
    void checkIndividualWorkspace(boost::shared_ptr<const API::IMDHistoWorkspace> workspace) const;
    void checkBothWorkspaces(boost::shared_ptr<const API::IMDHistoWorkspace> rhsWorkspace, boost::shared_ptr<const API::IMDHistoWorkspace> lhsWorkspace) const;
    boost::shared_ptr<MDEvents::MDHistoWorkspace> trimOutIntegratedDimension(boost::shared_ptr<API::IMDHistoWorkspace> ws);
    double integrateOver(boost::shared_ptr<API::IMDHistoWorkspace> ws, const double& startOverlap, const double& endOverlap);

    void overlayOverlap(boost::shared_ptr<MDEvents::MDHistoWorkspace> sum, boost::shared_ptr<API::IMDHistoWorkspace> overlap);
    boost::shared_ptr<MDEvents::MDHistoWorkspace> extractOverlapAsWorkspace(boost::shared_ptr<API::IMDHistoWorkspace> scaledWorkspace1, const double& startOverlap, const double& endOverlap);

    virtual void initDocs();
    void init();
    void exec();


  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_STITCHGROUP1D_H_ */