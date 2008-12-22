#ifndef MANTID_ALGORITHM_PLUS_H_
#define MANTID_ALGORITHM_PLUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/LocatedDataRef.h" 
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
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Plus";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;}

    private:
      // Overridden BinaryOperation methods
      void performBinaryOperation(API::Workspace::const_iterator it_in1, API::Workspace::const_iterator it_in2,
        API::Workspace::iterator it_out);

      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;

      /// Internal class providing the binary function
      class Plus_fn : public BinaryOperation::BinaryOperation_fn
      {
      public:
        /// Constructor
        Plus_fn(BinaryOperation *op,int count) : BinaryOperation::BinaryOperation_fn(op,count) {}
        API::LocatedDataValue& operator()(const API::ILocatedData&,const API::ILocatedData&);
      };

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PLUS_H_*/
