#ifndef MANTID_API_MUTLIPERIODGROUPALGORITHM_H_
#define MANTID_API_MUTLIPERIODGROUPALGORITHM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace API
{

  /** MutliPeriodGroupAlgorithm : Abstract algorithm. Algorithms that need special processing for Mutli-Period group workspaces should inherit from this 
  algorithm rather than from Algorithm directly. This algorithm processes workspaces in each group input in a pair-wise fashion to give a group workspace output.
    
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
  class DLLExport MultiPeriodGroupAlgorithm : public Algorithm
  {
  public:
    MultiPeriodGroupAlgorithm();
    virtual ~MultiPeriodGroupAlgorithm();

  private:
    /// Overriden from Algorithm base
    virtual bool checkGroups();
    /// Overriden from Algorithm base.
    virtual bool processGroups();

    std::string createFormattedInputWorkspaceNames(const size_t& periodIndex) const;
    void validateMultiPeriodGroupInputs(const size_t& nInputWorkspaces) const;

    /// Flag used to determine whether to use base or local virtual methods.
    bool m_useDefaultGroupingBehaviour;
    /// Convenience typdef for workspace names.
    typedef std::vector<boost::shared_ptr<Mantid::API::WorkspaceGroup> > VecWSGroupType;
    /// multi period group workspaces.
    VecWSGroupType m_multiPeriodGroups;

  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MUTLIPERIODGROUPALGORITHM_H_ */