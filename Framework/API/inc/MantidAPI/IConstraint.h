// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParameterReference.h"
#include <string>

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class Expression;
/** An interface to a constraint.

    @author Anders Markvardsen, ISIS, RAL
    @date 12/11/2009
*/
class MANTID_API_DLL IConstraint : public ParameterReference {
public:
  /// Initialize the constraint from an expression
  virtual void initialize(IFunction *fun, const Expression &expr, bool isDefault = false) = 0;

  /// Returns a penalty number which is bigger than or equal to zero
  /// If zero it means that the constraint is not penalized. If larger
  /// than zero the constraint is penalized where the larger this number
  /// is the larger the penalty
  virtual double check() = 0;

  /// Returns the derivative of the penalty for each active parameter
  virtual double checkDeriv() = 0;

  /// Returns the derivative of the penalty for each active parameter
  virtual double checkDeriv2() = 0;

  /// Set the parameters of IFitFunction to satisfy constraint. For example
  /// for a BoundaryConstraint this if param value less than lower boundary
  /// it is set to that value and vice versa for if the param value is larger
  /// than the upper boundary value.
  virtual void setParamToSatisfyConstraint() = 0;

  /// set the penalty factor for the constraint
  /// Set panelty factor. The larger the number to thigter the constraint. This
  /// number
  /// must be set to a number larger than zero
  /// @param c :: the penalt factor
  virtual void setPenaltyFactor(const double &c) = 0;

  /// get the penalty factor for the constraint
  virtual double getPenaltyFactor() const = 0;

  /// Return the string that can be used in this->initialize() to recreate this
  /// constraint
  virtual std::string asString() const = 0;

  /// Return the value for default fitting penalties
  static double getDefaultPenaltyFactor() { return m_defaultPenaltyFactor; }

protected:
  /// default penalty factor for constraints
  static constexpr double m_defaultPenaltyFactor = 1000;
};

} // namespace API
} // namespace Mantid
