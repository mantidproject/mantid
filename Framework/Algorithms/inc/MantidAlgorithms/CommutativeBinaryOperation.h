// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_COMMUTATIVEBINARYOPERATION_H_
#define MANTID_ALGORITHMS_COMMUTATIVEBINARYOPERATION_H_

#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid {
namespace Algorithms {
/**
CommutativeBinaryOperation supports commutative binary operations on two input
workspaces.
In Commutative operations it does not matter if the order of the two input
workspaces is reversed.
e.g. a+b is the same as b+a, and a*b is the same as b*a.
It inherits from the BinaryOperation class.

@author Nick Draper
@date 23/01/2008
*/
class DLLExport CommutativeBinaryOperation : public BinaryOperation {
public:
protected:
  // Overridden BinaryOperation method
  /// Checks the overall size compatability of two workspaces
  std::string checkSizeCompatibility(
      const API::MatrixWorkspace_const_sptr lhs,
      const API::MatrixWorkspace_const_sptr rhs) const override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_COMMUTATIVEBINARYOPERATION_H_*/
