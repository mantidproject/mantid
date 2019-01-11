// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SIGNALOVERERROR_H_
#define MANTID_ALGORITHMS_SIGNALOVERERROR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/UnaryOperation.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** Calculate Y/E for a Workspace2D

  @date 2011-12-05
*/
class DLLExport SignalOverError : public UnaryOperation {
public:
  SignalOverError();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Replace Y by Y/E for a MatrixWorkspace";
  }

  int version() const override;
  const std::string category() const override;

private:
  // Overridden UnaryOperation methods
  void performUnaryOperation(const double XIn, const double YIn,
                             const double EIn, double &YOut,
                             double &EOut) override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SIGNALOVERERROR_H_ */
