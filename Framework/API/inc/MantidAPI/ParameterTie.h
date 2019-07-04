// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_PARAMETERTIE_H_
#define MANTID_API_PARAMETERTIE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ParameterReference.h"

#include <map>
#include <memory>
#include <vector>

namespace mu {
class Parser;
}

namespace Mantid {
namespace API {
/** Ties fitting parameters. A tie is a formula that is used to
    calculate the value of a function parameter based on the values of other
   parameters.
    A tied parameter is not considered independent and doesn't take part in
   fitting.
    Its value is always calculated with its tie's formula.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 28/10/2009
  */
class MANTID_API_DLL ParameterTie final : public ParameterReference {
public:
  /// Constructor
  ParameterTie(IFunction *funct, const std::string &parName,
               const std::string &expr = "", bool isDefault = false);
  /// Destructor
  ~ParameterTie() override;
  /// Set the tie expression
  virtual void set(const std::string &expr);
  /// Evaluate the expression
  virtual double eval();
  /// Return the string that can be used to recreate this tie
  virtual std::string asString(const IFunction *fun = nullptr) const;

  /// Check if the tie has any references to certain parameters
  bool findParametersOf(const IFunction *fun) const;
  /// Check if the tie is a constant
  bool isConstant() const;
  /// Get a list of parameters on the right-hand side of the equation
  std::vector<ParameterReference> getRHSParameters() const;

protected:
  std::unique_ptr<mu::Parser> m_parser; ///< math parser
  /// Store for parameters used in the tie. The map's key is address used by the
  /// mu::Parser
  std::map<double *, ParameterReference> m_varMap;
  /// Keep the function that was passed to the constructor
  IFunction *m_function1;
  /// Keep the template for the input string passed to this->set(...)
  /// In the template CompositeFunction prefixes are replaced with placeholders
  std::string m_expression;

private:
  /// MuParser callback function
  static double *AddVariable(const char *varName, void *palg);
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PARAMETERTIE_H_*/
