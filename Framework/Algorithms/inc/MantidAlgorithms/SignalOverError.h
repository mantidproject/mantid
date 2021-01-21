// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid {
namespace Algorithms {

/** Calculate Y/E for a Workspace2D

  @date 2011-12-05
*/
class MANTID_ALGORITHMS_DLL SignalOverError : public UnaryOperation {
public:
  SignalOverError();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Replace Y by Y/E for a MatrixWorkspace"; }

  int version() const override;
  const std::string category() const override;

private:
  // Overridden UnaryOperation methods
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut, double &EOut) override;
};

} // namespace Algorithms
} // namespace Mantid
