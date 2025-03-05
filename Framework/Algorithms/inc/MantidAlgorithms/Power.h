// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid {
namespace Algorithms {

/**
 Provides the ability to raise the values in the workspace to a specified power.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the workspace to correct</LI>
 <LI> OutputWorkspace - The name of the corrected workspace (can be the same as
 the input one)</LI>
 <LI> exponent        - The exponent to use in the power calculation</LI>
 </UL>

 @author Owen Arnold, Tessella plc
 @date 12/04/2010
 */

class MANTID_ALGORITHMS_DLL Power : public UnaryOperation {
public:
  /// Default constructor
  Power();
  /// Algorithm's name for identification
  const std::string name() const override { return "Power"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The Power algorithm will raise the base workspace to a particular "
           "power. Corresponding error values will be created.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Exponential", "Logarithm"}; }

private:
  // Overridden UnaryOperation methods
  void defineProperties() override;
  void retrieveProperties() override;
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut, double &EOut) override;
  /// calculate the power
  inline double calculatePower(const double base, const double exponent);
  /// Exponent to raise the base workspace to
  double m_exponent;
};
} // namespace Algorithms
} // namespace Mantid
