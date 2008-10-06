#ifndef MANTID_ALGORITHM_BINARYOPERATION_H_
#define MANTID_ALGORITHM_BINARYOPERATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <algorithm>
#include <functional>
#include <iterator>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/LocatedDataValue.h"

namespace Mantid
{

  namespace Algorithms
  {
    /** @class BinaryOperation BinaryOperation.h Algorithms/BinaryOperation.h


    BinaryOperation supports the implmentation of a binary operation on two input workspaces.
    It inherits from the Algorithm class, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> InputWorkspace2 - The name of the workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the difference data </LI>
    </UL>

    @author Nick Draper
    @date 14/12/2007

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

    class DLLExport BinaryOperation : public API::Algorithm
    {
    public:
      /// Default constructor
      BinaryOperation() : API::Algorithm() {};
      /// Destructor
      virtual ~BinaryOperation() {};

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "General";}

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();

      /** Abstract method to perform the binary operation in the inheriting class.
      * @param it_in1 The const iterator to the lhs data item
      * @param it_in2 The const iterator to the rhs data item
      * @param it_out The output iterator to the new workspace
      */
      virtual void performBinaryOperation(API::Workspace::const_iterator it_in1, API::Workspace::const_iterator it_in2,
        API::Workspace::iterator it_out) =0;

      /// Checks the compatibility of the two workspaces
      virtual const bool checkCompatibility(const API::Workspace_const_sptr lhs,const API::Workspace_const_sptr rhs) const;
      /// Checks the overall size compatibility of two workspaces
      virtual const bool checkSizeCompatibility(const API::Workspace_const_sptr lhs,const API::Workspace_const_sptr rhs) const;
      /// Checks the compatibility the X arrays of two workspaces
      virtual const bool checkXarrayCompatibility(const API::Workspace_const_sptr lhs, const API::Workspace_const_sptr rhs) const;
      /// Returns the number of times lhs will have to loop to match the size of rhs
      virtual const int getRelativeLoopCount(const API::Workspace_const_sptr lhs, const API::Workspace_const_sptr rhs) const;
      /// Creates a suitable output workspace for two input workspaces
      virtual API::Workspace_sptr createOutputWorkspace(const API::Workspace_const_sptr lhs, const API::Workspace_const_sptr rhs) const;
      /// Creates a const iterator with appropriate looping settings.
      API::Workspace::const_iterator createConstIterator(const API::Workspace_const_sptr wsMain, const API::Workspace_const_sptr wsComparison) const;
      /// Gets the looping orientation for a looping iterator
      unsigned int getLoopDirection(const API::Workspace_const_sptr wsMain, const API::Workspace_const_sptr wsComparison) const;


      /// Abstract internal class providing the binary function
      class BinaryOperation_fn : public std::binary_function<API::LocatedDataRef,API::LocatedDataRef,API::LocatedDataRef >
      {
      public:
        /// Virtual destructor
        virtual ~BinaryOperation_fn()
        { }

        /** Abstract function that performs each element of the binary function
        * @param a The lhs data element
        * @param b The rhs data element
        * @returns The result data element
        */
        virtual API::LocatedDataValue& operator()(const API::ILocatedData& a,const API::ILocatedData& b) =0;
      protected:
        /**Temporary cache of the Histogram Result.
        This save creating a new object for each iteration and removes lifetime issues.
        */
        API::LocatedDataValue result;

      };

    private:
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_BINARYOPERATION_H_*/
