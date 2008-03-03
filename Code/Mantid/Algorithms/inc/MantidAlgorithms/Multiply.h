#ifndef MANTID_ALGORITHM_MULTIPLY_H_
#define MANTID_ALGORITHM_MULTIPLY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/LocatedDataRef.h" 
#include "MantidAlgorithms/CommutativeBinaryOperation.h"

#include <algorithm>
#include <functional>
#include <iterator>

namespace Mantid
{
  namespace Algorithms
  {
    /** @class Multiply Multiply.h Algorithms/Multiply.h


    Multiply performs the product of two input workspaces.
    It inherits from the Algorithm class, and overrides
    the init()&+ exec()  methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> InputWorkspace2 - The name of the workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the product data </LI>
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

    class DLLExport Multiply : public CommutativeBinaryOperation
    {
    public:
      /// Default constructor
      Multiply() : CommutativeBinaryOperation() {};
      /// Destructor
      virtual ~Multiply() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Multiply";};
      /// Algorithm's version for identification overriding a virtual method
      virtual const std::string version() const { return "1";};

    private:
      // Overridden BinaryOperation methods
      void performBinaryOperation(API::Workspace::const_iterator it_in1, API::Workspace::const_iterator it_in2,
        API::Workspace::iterator it_out);
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;

      class Multiply_fn : public BinaryOperation::BinaryOperation_fn
      {
      public:
        API::LocatedDataValue& operator()(const API::ILocatedData&,const API::ILocatedData&);

      };

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MULTIPLY_H_*/
