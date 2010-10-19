#ifndef MANTID_ALGORITHM_COMMUTATIVEBINARYOPERATION_H_
#define MANTID_ALGORITHM_COMMUTATIVEBINARYOPERATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    CommutativeBinaryOperation supports commutative binary operations on two input workspaces.
    In Commutative operations it does not matter if the order of the two input workspaces is reversed.
    e.g. a+b is the same as b+a, and a*b is the same as b*a.
    It inherits from the BinaryOperation class.

    @author Nick Draper
    @date 23/01/2008

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
    class DLLExport CommutativeBinaryOperation : public BinaryOperation
    {
    public:
      /// Default constructor
      CommutativeBinaryOperation() : BinaryOperation() {};
      /// Destructor
      virtual ~CommutativeBinaryOperation() {};

    protected:
      // Overridden BinaryOperation method
      /// Checks the overall size compatability of two workspaces
      virtual bool checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_COMMUTATIVEBINARYOPERATION_H_*/
