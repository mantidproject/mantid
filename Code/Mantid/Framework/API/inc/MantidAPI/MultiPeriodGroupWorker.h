#ifndef MANTID_API_MULTIPERIODGROUPWORKER_H_
#define MANTID_API_MULTIPERIODGROUPWORKER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IAlgorithm.h"
#include <string>

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
      MultiPeriodGroupWorker();
      MultiPeriodGroupWorker(const std::string& workspacePropertyName);
      virtual ~MultiPeriodGroupWorker();
      /// Getter for the input workspace property name
      std::string getInputWorkspacePropertyName() const;
      /// Flag to indicate use of a custom workspace property
      bool useCustomWorkspaceProperty() const;
      /// Check groups
      bool checkGroups(IAlgorithm_sptr alg) const;

    private:
      MultiPeriodGroupWorker(const MultiPeriodGroupWorker&);
      MultiPeriodGroupWorker& operator=(const MultiPeriodGroupWorker&);

      /// Workspace property name
      std::string m_workspacePropertyName;

    };

  } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MULTIPERIODGROUPWORKER_H_ */
