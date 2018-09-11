#ifndef MANTID_GEOMETRY_FITPARAMETER_H_
#define MANTID_GEOMETRY_FITPARAMETER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Interpolation.h"

namespace Mantid {
namespace Geometry {
/**
Store information about a fitting parameter such as its value
if it is constrained or tied. Main purpose is to use this
class as a type for storing information about a fitting parameter
in the parameter map of the workspace.

@author Anders Markvardsen, ISIS, RAL
@date 26/2/2010

Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL FitParameter {
public:
  /// Constructor
  FitParameter() : m_value(0.0), m_tie(""), m_function(""){};

  /// get paramter value
  double getValue(const double &at) const;
  /// get paramter value
  double getValue() const;
  /// set parameter value
  void setValue(double value) { m_value = value; }
  /// get tie
  const std::string &getTie() const { return m_tie; }
  /// set tie
  void setTie(const std::string &tie) { m_tie = tie; }
  /// get constraint
  std::string getConstraint() const;
  /// get penalty factor
  const std::string &getConstraintPenaltyFactor() const {
    return m_constraintPenaltyFactor;
  }
  /// set constraint min
  void setConstraintMin(const std::string &constraintMin) {
    m_constraintMin = constraintMin;
  }
  /// set constraint max
  void setConstraintMax(const std::string &constraintMax) {
    m_constraintMax = constraintMax;
  }
  /// set the constraint penalty
  void setConstraintPenaltyFactor(const std::string &constraintPenaltyFactor) {
    m_constraintPenaltyFactor = constraintPenaltyFactor;
  }
  /// get formula
  const std::string &getFormula() const { return m_formula; }
  /// set formula
  void setFormula(const std::string &formula) { m_formula = formula; }
  /// get formula unit, and Empty string is no unit has been specified
  const std::string &getFormulaUnit() const { return m_formulaUnit; }
  /// set formula unit
  void setFormulaUnit(const std::string &formulaUnit) {
    m_formulaUnit = formulaUnit;
  }
  /// get result formula unit, and Empty string is no unit has been specified
  const std::string &getResultUnit() const { return m_resultUnit; }
  /// set result formula unit
  void setResultUnit(const std::string &resultUnit) {
    m_resultUnit = resultUnit;
  }
  /// get function
  const std::string &getFunction() const { return m_function; }
  /// set function
  void setFunction(const std::string &function) { m_function = function; }
  /// get name
  const std::string &getName() const { return m_name; }
  /// set name
  void setName(const std::string &name) { m_name = name; }
  /// get look up table
  const Kernel::Interpolation &getLookUpTable() const { return m_lookUpTable; }
  /// set look up table
  void setLookUpTable(const Kernel::Interpolation &lookupTable) {
    m_lookUpTable = lookupTable;
  }

  /// Prints object to stream
  void printSelf(std::ostream &os) const;

private:
  /// value of parameter
  mutable double m_value;
  /// tie of parameter
  std::string m_tie;
  std::string m_constraintMin;           ///< constraint min boundary
  std::string m_constraintMax;           ///< constraint max boundary
  std::string m_constraintPenaltyFactor; ///< the penalty factor
  /// name of fitting function
  std::string m_function;
  /// name of parameter
  std::string m_name;
  /// look up table
  Kernel::Interpolation m_lookUpTable;

  std::string m_formula;     ///< the formula
  std::string m_formulaUnit; ///< the unit that the formula expects
  std::string m_resultUnit;  ///< the result unit
};

// defining operator << and >>
MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &,
                                             const FitParameter &);
MANTID_GEOMETRY_DLL std::istream &operator>>(std::istream &, FitParameter &);

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_FITPARAMETER_H_*/
