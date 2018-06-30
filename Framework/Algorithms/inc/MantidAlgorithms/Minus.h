#ifndef MANTID_ALGORITHM_MINUS_H_
#define MANTID_ALGORITHM_MINUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid {
namespace Algorithms {
/**
Minus performs the difference of two input workspaces.
It inherits from the Algorithm class, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> InputWorkspace1 - The name of the workspace </LI>
<LI> InputWorkspace2 - The name of the workspace </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the
difference data </LI>
</UL>

@author Nick Draper
@date 14/12/2007

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
class DLLExport Minus : public BinaryOperation {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Minus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The Minus algorithm will subtract the data values and calculate "
           "the corresponding error values for two compatible workspaces.";
  }

  /// Algorithm's alias for identification overriding a virtual method
  const std::string alias() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Plus", "Divide", "Multiply"};
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
  bool checkUnitCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                              const API::MatrixWorkspace_const_sptr rhs) const;
  bool
  checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                     const API::MatrixWorkspace_const_sptr rhs) const override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MINUS_H_*/
