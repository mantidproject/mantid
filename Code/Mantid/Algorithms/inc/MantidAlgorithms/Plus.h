#ifndef MANTID_ALGORITHM_PLUS_H_
#define MANTID_ALGORITHM_PLUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/TripleRef.h" 
#include "MantidAlgorithms/CommutativeBinaryOperation.h"

#include <algorithm>
#include <functional>
#include <iterator>

namespace Mantid
{
  namespace Algorithms
  {

    /** @class Plus Plus.h Algorithms/Plus.h


    Plus performs the difference of two input workspaces.
    It inherits from the Algorithm class, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> InputWorkspace2 - The name of the workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the added data </LI>
    </UL>

    @author Dickon Champion, RAL
    @date 12/12/2007

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

    class DLLExport Plus : public CommutativeBinaryOperation
    {
    public:
      /// Default constructor
      Plus() : CommutativeBinaryOperation() {};
      /// Destructor
      virtual ~Plus() {};
      virtual const std::string name() const { return "Plus";};///< Algorithm's name for identification
      virtual const std::string version() const { return "1";};///< Algorithm's name for identification

    private:
      // Overridden BinaryOperation methods
      void performBinaryOperation(API::Workspace::const_iterator it_in1, API::Workspace::const_iterator it_in2,
        API::Workspace::iterator it_out);

      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;

      class Plus_fn : public BinaryOperation::BinaryOperation_fn
      {
      public:
        API::TripleRef<double> operator()(const API::TripleRef<double>&,const API::TripleRef<double>&);
      };

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PLUS_H_*/
