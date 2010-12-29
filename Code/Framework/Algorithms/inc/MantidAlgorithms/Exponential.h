#ifndef MANTID_ALGORITHM_EXPONENTIAL_H_
#define MANTID_ALGORITHM_EXPONENTIAL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    Exponential performs the exponential function on an input workspace.
    It inherits from the Algorithm class, and overrides
    the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> Exponent -        The value of the exponent to raise each value to </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the output data </LI>
    </UL>

    @author Ron Fowler
    @date 12/05/2010

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport Exponential : public UnaryOperation
    {
    public:
      /// Default constructor
      Exponential() : UnaryOperation() {};
      /// Destructor
      virtual ~Exponential() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Exponential";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}

    private:
      // Overridden UnaryOperation methods
      void performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut);

      /*
      void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                  const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut);
      void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                  const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut);
      */
      // try and set output units field
      //void setOutputUnits(const API::MatrixWorkspace_const_sptr lhs,API::MatrixWorkspace_sptr out);
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_EXPONENTIAL_H_*/
