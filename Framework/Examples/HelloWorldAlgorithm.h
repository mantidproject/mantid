// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Algorithm basic test class.

    @author Matt Clarke, ISIS, RAL
    @date 09/11/2007
 */
class HelloWorldAlgorithm : public API::Algorithm {
public:
  /// no arg constructor
  HelloWorldAlgorithm() : API::Algorithm() {}
  /// virtual destructor
  ~HelloWorldAlgorithm() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "HelloWorldAlgorithm"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Examples"; }
  const std::string summary() const override { return "Summary of this algorithm - Outputs Hello World!."; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
