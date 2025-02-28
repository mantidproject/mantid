// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/UnaryOperation.h"
namespace Mantid {
namespace Algorithms {
/** Takes a workspace as input and calculates the natural logarithm of number of
   counts for each 1D spectrum.
    The algorithm creates a new workspace containing logarithms of signals
   (numbers of counts) in

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> Filler  -- A workspace can normally have zeros in it and the logarithm
   goes to minys infinity for zeros.
                    This field keeps value, that should be placed in the
   workspace instead of minus infinity </LI>
    <LI> Natural -- Natural or base 10 logarithm. Default is natural </LI>
    </UL>

    @author AB,    ISIS, Rutherford Appleton Laboratory
    @date 12/05/2010
 */
class MANTID_ALGORITHMS_DLL Logarithm : public UnaryOperation {
public:
  Logarithm();
  /// Algorithm's name for identification
  const std::string name() const override { return "Logarithm"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Logarithm function calculates the logarithm of the data, held in a "
           "workspace. A user can choose between natural (default) or base 10 "
           "logarithm";
  }

  /// Algorithm's version for identification
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Power", "Exponential"}; }
  /// Algorithm's category for identification
  // cppcheck-suppress uselessOverride
  const std::string category() const override { return "Arithmetic"; }

private:
  /// The value to replace ln(0)
  double log_Min;
  /// If the logarithm natural or 10-based
  bool is_natural;

  /// Declare additional properties for this algorithm
  void defineProperties() override;
  /// get properties from GUI
  void retrieveProperties() override;
  /// Actually the function, which is run on values when the operation is
  /// performed
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut, double &EOut) override;
};

} // End namespace Algorithms
} // End namespace Mantid
