#ifndef MANTID_API_MULTIPERIODGROUPWORKER_H_
#define MANTID_API_MULTIPERIODGROUPWORKER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Algorithm.h"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace API
  {

    /** MultiPeriodGroupWorker : Multiperiod group logic relating to determining a valid multiperiod group, and processing a
     * multiperiod group, as well as combining and returning the output.

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
    class DLLExport MultiPeriodGroupWorker
    {
    public:
      /// Convenience typdef for workspace names.
      typedef std::vector<boost::shared_ptr<Mantid::API::WorkspaceGroup> > VecWSGroupType;
      /// Constructor
      MultiPeriodGroupWorker();
      /// Copy constructor
      MultiPeriodGroupWorker(const std::string& workspacePropertyName);
      virtual ~MultiPeriodGroupWorker();
      /// Getter for the input workspace property name
      std::string getInputWorkspacePropertyName() const;
      /// Flag to indicate use of a custom workspace property
      bool useCustomWorkspaceProperty() const;
      /// Check groups
      VecWSGroupType findMultiPeriodGroups(Algorithm_sptr alg) const ;

    private:
      // Disable copy
      MultiPeriodGroupWorker(const MultiPeriodGroupWorker&);
      // Disable assignment
      MultiPeriodGroupWorker& operator=(const MultiPeriodGroupWorker&);

      void tryAddInputWorkspaceToInputGroups(Workspace_sptr ws, VecWSGroupType& vecWorkspaceGroups) const;

      /// Workspace property name
      std::string m_workspacePropertyName;

      /// Flag used to determine whether to use base or local virtual methods.
      bool m_useDefaultGroupingBehaviour;

      /// multi period group workspaces.
      VecWSGroupType m_multiPeriodGroups;

    };

  } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MULTIPERIODGROUPWORKER_H_ */
