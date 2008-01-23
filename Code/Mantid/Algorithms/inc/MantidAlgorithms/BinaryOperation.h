#ifndef MANTID_ALGORITHM_BINARYOPERATION_H_
#define MANTID_ALGORITHM_BINARYOPERATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/TripleRef.h" 

#include <algorithm>
#include <functional>
#include <iterator>

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

    class DLLExport BinaryOperation : public API::Algorithm
    {
    public:
      /// Default constructor
      BinaryOperation() : API::Algorithm() {};
      /// Destructor
      virtual ~BinaryOperation() {};

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();    
      virtual void performBinaryOperation(API::Workspace::const_iterator it_in1, API::Workspace::const_iterator it_in2,
        API::Workspace::iterator it_out) =0;

      virtual const bool checkSizeCompatability(const API::Workspace_sptr lhs,const API::Workspace_sptr rhs) const;
      virtual const bool checkXarrayCompatability(const API::Workspace_sptr lhs, const API::Workspace_sptr rhs) const;
      virtual const int getRelativeLoopCount(const API::Workspace_sptr lhs, const API::Workspace_sptr rhs) const;
      virtual API::Workspace_sptr createOutputWorkspace(const API::Workspace_sptr lhs, const API::Workspace_sptr rhs) const;
      API::Workspace::const_iterator BinaryOperation::createConstIterator(const API::Workspace_sptr wsMain, const API::Workspace_sptr wsComparison) const;
      unsigned int BinaryOperation::getLoopDirection(const API::Workspace_sptr wsMain, const API::Workspace_sptr wsComparison) const;
    
      
      class BinaryOperation_fn : public std::binary_function<API::TripleRef<double>,API::TripleRef<double>,API::TripleRef<double> >
      {
      public:
        virtual ~BinaryOperation_fn() {};
        virtual API::TripleRef<double> operator()(const API::TripleRef<double>&,const API::TripleRef<double>&) =0;
      protected:
        ///The X value to return
        double ret_x;
        ///Temporary cache of calculated signal value
        double ret_sig;
        ///Temporary cache of calculated error value
        double ret_err;
      };

    private:
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_BINARYOPERATION_H_*/
