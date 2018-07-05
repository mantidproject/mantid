#ifndef MANTID_ALGORITHM_PLUS_H_
#define MANTID_ALGORITHM_PLUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/CommutativeBinaryOperation.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/**
Plus performs the difference of two input workspaces.
It inherits from the Algorithm class, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> InputWorkspace1 - The name of the workspace </LI>
<LI> InputWorkspace2 - The name of the workspace </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the added
data </LI>
</UL>

@author Dickon Champion, RAL
@date 12/12/2007

Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class DLLExport Plus : public CommutativeBinaryOperation {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Plus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The Plus algorithm will add the data values and calculate the "
           "corresponding error values in two compatible workspaces. ";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Divide", "Minus", "Multiply"};
  }

private:
  // Overridden BinaryOperation methods
  void performBinaryOperation(const MantidVec &lhsX, const MantidVec &lhsY,
                              const MantidVec &lhsE, const MantidVec &rhsY,
                              const MantidVec &rhsE, MantidVec &YOut,
                              MantidVec &EOut) override;
  void performBinaryOperation(const MantidVec &lhsX, const MantidVec &lhsY,
                              const MantidVec &lhsE, const double rhsY,
                              const double rhsE, MantidVec &YOut,
                              MantidVec &EOut) override;
  void performEventBinaryOperation(DataObjects::EventList &lhs,
                                   const DataObjects::EventList &rhs) override;
  void performEventBinaryOperation(DataObjects::EventList &lhs,
                                   const MantidVec &rhsX, const MantidVec &rhsY,
                                   const MantidVec &rhsE) override;
  void performEventBinaryOperation(DataObjects::EventList &lhs,
                                   const double &rhsY,
                                   const double &rhsE) override;

  void checkRequirements() override;
  std::string checkSizeCompatibility(
      const API::MatrixWorkspace_const_sptr lhs,
      const API::MatrixWorkspace_const_sptr rhs) const override;
  bool
  checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                     const API::MatrixWorkspace_const_sptr rhs) const override;
  void operateOnRun(const API::Run &lhs, const API::Run &rhs,
                    API::Run &ans) const override;

  // Overridden event-specific operation
  bool checkUnitCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                              const API::MatrixWorkspace_const_sptr rhs) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PLUS_H_*/
