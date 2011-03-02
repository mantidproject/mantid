#ifndef MANTID_ALGORITHM_DIVIDE_H_
#define MANTID_ALGORITHM_DIVIDE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    Divide performs the division of two input workspaces.
    It inherits from the Algorithm class, and overrides
    the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> InputWorkspace2 - The name of the workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the division data </LI>
    </UL>

    @author Nick Draper
    @date 14/12/2007

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
    class DLLExport Divide : public BinaryOperation
    {
    public:
      /// Default constructor
      Divide() : BinaryOperation() {};
      /// Destructor
      virtual ~Divide() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Divide";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden BinaryOperation methods
      void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                  const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut);
      void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                  const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut);
      void setOutputUnits(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out);


      virtual void performEventBinaryOperation(DataObjects::EventList & lhs,
          const DataObjects::EventList & rhs);

      virtual void performEventBinaryOperation(DataObjects::EventList & lhs,
          const MantidVec& rhsX, const MantidVec& rhsY, const MantidVec& rhsE);

      virtual void performEventBinaryOperation(DataObjects::EventList & lhs,
          const double& rhsY, const double& rhsE);

      void checkRequirements();

      bool checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const;

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIVIDE_H_*/
