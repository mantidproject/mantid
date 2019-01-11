// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_
#define MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** BinaryOperateMasks : TODO: DESCRIPTION
 */
class DLLExport BinaryOperateMasks : public API::Algorithm {
public:
  BinaryOperateMasks();
  ~BinaryOperateMasks() override;

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "BinaryOperateMasks"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs binary operation, including and, or and xor, on two mask "
           "Workspaces, i.e., SpecialWorkspace2D.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"InvertMask"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_ */
