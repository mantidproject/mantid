// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Set the uncertainties of the data to zero.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input workspace. </LI>
 <LI> OutputWorkspace - The name of the output workspace. </LI>
 </UL>

 @date 10/12/2010
 */
class MANTID_ALGORITHMS_DLL SetUncertainties : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm creates a workspace which is the duplicate of the "
           "input, but where the error value for every bin has been set to "
           "zero.";
  }

  /// Algorithm's version
  int version() const override;

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\Errors"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
