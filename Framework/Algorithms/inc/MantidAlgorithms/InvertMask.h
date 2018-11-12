// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_INVERTMASK_H_
#define MANTID_ALGORITHMS_INVERTMASK_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** InvertMask : TODO: DESCRIPTION

  @date 2012-06-01
*/
class DLLExport InvertMask : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "InvertMask"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm inverts every mask bit in a MaskWorkspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"BinaryOperateMasks", "MaskDetectors"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  // Implement abstract Algorithm methods
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_INVERTMASK_H_ */
