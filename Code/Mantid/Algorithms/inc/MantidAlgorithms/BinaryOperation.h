#ifndef MANTID_ALGORITHM_BINARYOPERATION_H_
#define MANTID_ALGORITHM_BINARYOPERATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
//#include <algorithm>
//#include <functional>
//#include <iterator>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/LocatedDataValue.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    BinaryOperation supports the implmentation of a binary operation on two input workspaces.
    It inherits from the Algorithm class, and overrides the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> InputWorkspace2 - The name of the workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the difference data </LI>
    </UL>

    @author Nick Draper
    @date 14/12/2007

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
      virtual const std::string category() const { return "Arithmetic";}

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();

      /// The name of the first input workspace property
			virtual const std::string inputPropName1() const { return "InputWorkspace_1";}
      /// The name of the second input workspace property
			virtual const std::string inputPropName2() const { return "InputWorkspace_2";}
      /// The name of the output workspace property
			virtual const std::string outputPropName() const { return "OutputWorkspace";}

      /// Checks the compatibility of the two workspaces
      virtual const bool checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const;
      /// Checks the overall size compatibility of two workspaces
      virtual const bool checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const;
      /// Checks the compatibility the X arrays of two workspaces
      virtual const bool checkXarrayCompatibility(const API::MatrixWorkspace_const_sptr lhs, const API::MatrixWorkspace_const_sptr rhs) const;

      /** Carries out the binary operation on a single spectrum.
       *  @param lhsX The X values, made available if required.
       *  @param lhsY The vector of lhs data values
       *  @param lhsE The vector of lhs error values
       *  @param rhsY The vector of rhs data values
       *  @param rhsE The vector of rhs error values
       *  @param YOut The vector to hold the data values resulting from the operation
       *  @param EOut The vector to hold the error values resulting from the operation
       */
      virtual void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                          const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut) = 0;
      /** Carries out the binary operation when the right hand operand is a single number.
       *  @param lhsX The X values, made available if required.
       *  @param lhsY The vector of lhs data values
       *  @param lhsE The vector of lhs error values
       *  @param rhsY The rhs data value
       *  @param rhsE The rhs error value
       *  @param YOut The vector to hold the data values resulting from the operation
       *  @param EOut The vector to hold the error values resulting from the operation
       */
      virtual void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                          const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut) = 0;
      
    private:
      void doSingleValue(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out);
      void doSingleSpectrum(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out);
      void doSingleColumn(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out);
      void do2D(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out);
            
      friend class BinaryOperation_fn;
      /// Abstract internal class providing the binary function
      class BinaryOperation_fn : public std::binary_function<API::LocatedDataRef,API::LocatedDataRef,API::LocatedDataRef >
      {
      public:
        /// Constructor
        BinaryOperation_fn(BinaryOperation* op,int count):m_count(count),m_progress(0),m_progress_step(count/100),m_op(op)
        { if (m_progress_step == 0) m_progress_step = 1; }
        /// Virtual destructor
        virtual ~BinaryOperation_fn()
        { }
        /** Abstract function that performs each element of the binary function
        * @param a The lhs data element
        * @param b The rhs data element
        * @returns The result data element
        */
        virtual API::LocatedDataValue& operator()(const API::ILocatedData& a,const API::ILocatedData& b) = 0;
      protected:
        /**Temporary cache of the Histogram Result.
        This save creating a new object for each iteration and removes lifetime issues.
        */
        API::LocatedDataValue result;
        int m_count;         ///< Total number of individual operations.
        int m_progress;      ///< Number of performed operations;
        int m_progress_step; ///< Call progerss(...) every m_progress_step operations.
        BinaryOperation *m_op; ///< BinaryOperation
        /// Reports algorithm progress from operator()
        void report_progress(){m_op->report_progress(double(m_progress)/m_count);}
      };
      /// Gives access to Algorithm's protected methods
      void report_progress(double p)
      {
          progress(p);
          interruption_point();
      }
    private:
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_BINARYOPERATION_H_*/
