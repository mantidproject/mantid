// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IConstraint.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Constraints {
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------

/**
A boundary constraint is designed to be used to set either
upper or lower (or both) boundaries on a single parameter.

@author Anders Markvardsen, ISIS, RAL
@date 13/11/2009
*/
class MANTID_CURVEFITTING_DLL BoundaryConstraint : public API::IConstraint {
public:
  /// Default constructor
  BoundaryConstraint();

  /// Constructor with no boundary arguments
  BoundaryConstraint(const std::string &paramName);

  /// Constructor with boundary arguments
  BoundaryConstraint(API::IFunction *fun, const std::string &paramName, const double lowerBound,
                     const double upperBound, bool isDefault = false);

  /// Constructor with lower boundary argument
  BoundaryConstraint(API::IFunction *fun, const std::string &paramName, const double lowerBound,
                     bool isDefault = false);

  /// Initialize the constraint from an expression
  void initialize(API::IFunction *fun, const API::Expression &expr, bool isDefault) override;

  /// implement IConstraint virtual functions
  void setPenaltyFactor(const double &c) override;
  double getPenaltyFactor() const override { return m_penaltyFactor; }

  /// Return if it has a lower bound
  bool hasLower() const { return m_hasLowerBound; }
  /// Return if it has a lower bound
  bool hasUpper() const { return m_hasUpperBound; }
  /// Return the lower bound value
  const double &lower() const { return m_lowerBound; }
  /// Return the upper bound value
  const double &upper() const { return m_upperBound; }

  /// Set lower bound value
  void setLower(const double &value) {
    m_hasLowerBound = true;
    m_lowerBound = value;
  }
  /// Set upper bound value
  void setUpper(const double &value) {
    m_hasUpperBound = true;
    m_upperBound = value;
  }
  /// Clear lower bound value
  void clearLower() {
    m_hasLowerBound = false;
    m_lowerBound = double();
  }
  /// Clear upper bound value
  void clearUpper() {
    m_hasUpperBound = false;
    m_upperBound = double();
  }

  /// Set both bounds (lower and upper) at the same time
  void setBounds(const double &lower, const double &upper) {
    setLower(lower);
    setUpper(upper);
  }

  /// Clear both bounds (lower and upper) at the same time
  void clearBounds() {
    clearLower();
    clearUpper();
  }

  /// Get parameter name
  //  std::string getParameterName() const { return m_parameterName; }

  /// overwrite IConstraint base class methods
  double check() override;
  double checkDeriv() override;
  double checkDeriv2() override;
  void setParamToSatisfyConstraint() override;
  std::string asString() const override;

private:
  /// Penalty factor for the given boundary constraint
  double m_penaltyFactor;

  /// name of parameter you want to constraint
  // std::string m_parameterName;

  /// has a lower bound set true/false
  bool m_hasLowerBound;
  /// has a upper bound set true/false
  bool m_hasUpperBound;
  /// the lower bound
  double m_lowerBound;
  /// the upper bound
  double m_upperBound;
};

} // namespace Constraints
} // namespace CurveFitting
} // namespace Mantid
