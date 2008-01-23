#ifndef MANTID_ALGORITHM_COMMUTATIVEBINARYOPERATION_H_
#define MANTID_ALGORITHM_COMMUTATIVEBINARYOPERATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/TripleRef.h" 

#include <algorithm>
#include <functional>
#include <iterator>

namespace Mantid
{
  namespace Algorithms
  {
    /** @class CommutativeBinaryOperation CommutativeBinaryOperation.h Algorithms/CommutativeBinaryOperation.h


    CommutativeBinaryOperation supports commutative binary operations on two input workspaces.
    It inherits from the binaryoperation class.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> InputWorkspace2 - The name of the workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the difference data </LI>
    </UL>

    @author Nick Draper
    @date 14/12/2007

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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

    class DLLExport CommutativeBinaryOperation : public BinaryOperation
    {
    public:
      /// Default constructor
      CommutativeBinaryOperation() : BinaryOperation() {};
      /// Destructor
      virtual ~CommutativeBinaryOperation() {};

    protected:
      // Overridden BinaryOperation methods
      virtual const bool checkSizeCompatability(const API::Workspace_sptr lhs,const API::Workspace_sptr rhs) const;
      virtual API::Workspace_sptr createOutputWorkspace(const API::Workspace_sptr lhs, const API::Workspace_sptr rhs) const;

    private:
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_COMMUTATIVEBINARYOPERATION_H_*/
