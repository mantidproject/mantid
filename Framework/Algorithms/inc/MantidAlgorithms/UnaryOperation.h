// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
UnaryOperation supports the implementation of a Unary operation on an input
workspace.
It inherits from the Algorithm class, and overrides the init() & exec() methods.
Concrete sub-classes should implement (or re-implement, if necessary) the
protected
methods of this class. The init() & exec() methods should be extended only in
unusual circumstances, and NEVER overridden.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the input workspace </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
</UL>

@author Russell Taylor, Tessella plc
@date 24/03/2009
*/
class MANTID_ALGORITHMS_DLL UnaryOperation : public API::Algorithm {
public:
  /// Algorithm's category for identification
  const std::string category() const override { return "Arithmetic"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Supports the implementation of a Unary operation on an input "
           "workspace.";
  }

protected:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  virtual void execEvent();
  template <class T> void unaryOperationEventHelper(std::vector<T> &wevector);
  /// The name of the input workspace property
  virtual const std::string inputPropName() const { return "InputWorkspace"; }
  /// The name of the output workspace property
  virtual const std::string outputPropName() const { return "OutputWorkspace"; }

  /// A virtual function in which additional properties of an algorithm should
  /// be declared. Called by init().
  virtual void defineProperties() { /*Empty in base class*/ }
  /// A virtual function in which additional properties should be retrieved into
  /// member variables. Called by exec().
  virtual void retrieveProperties() { /*Empty in base class*/ }

  /** Carries out the Unary operation on the current 'cell'
   *  @param XIn :: The X value. This will be the bin centre for histogram
   * workspaces.
   *  @param YIn :: The input data value
   *  @param EIn :: The input error value
   *  @param YOut :: A reference to the output data
   *  @param EOut :: A reference to the output error
   */
  virtual void performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut,
                                     double &EOut) = 0;

  /// flag to use histogram representation instead of events for certain
  /// algorithms
  bool useHistogram{false};
};

} // namespace Algorithms
} // namespace Mantid
