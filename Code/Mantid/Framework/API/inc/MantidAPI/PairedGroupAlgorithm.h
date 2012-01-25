#ifndef MANTID_ALGORITHM_PAIREDGROUPALGORITHM_H_
#define MANTID_ALGORITHM_PAIREDGROUPALGORITHM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid
{
  namespace API
  {
    /** 
    Abstract class that alters the processing of workspaces groups, This class should only be used for algorithms with two input workspaces.
    If two groups are presented then they will be processed in a pair wise manner.


    @author Nick Draper
    @date 07/12/2009

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    */
    class MANTID_API_DLL PairedGroupAlgorithm : public API::Algorithm
    {
    public:
      /// Default constructor
      PairedGroupAlgorithm();
      /// Destructor
      virtual ~PairedGroupAlgorithm();

    protected:

      API::Progress* m_progress;  ///< Progress reporting
    };

  } // namespace API
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PAIREDGROUPALGORITHM_H_*/
