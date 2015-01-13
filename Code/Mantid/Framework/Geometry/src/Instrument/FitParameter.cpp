//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include <Poco/StringTokenizer.h>
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid {
namespace Geometry {
namespace {
/// static logger object
Kernel::Logger g_log("FitParameter");
}

/**
  Get constraint string.
  @return Constraint string
*/
std::string FitParameter::getConstraint() const {

  if (m_constraintMin.compare("") == 0 && m_constraintMax.compare("") == 0)
    return std::string("");

  std::stringstream constraint;
  size_t foundMinPercentage, foundMaxPercentage;
  foundMinPercentage = m_constraintMin.find('%');
  foundMaxPercentage = m_constraintMax.find('%');
  double min = 0;
  double max = 0;
  if (m_constraintMin.compare("")) {
    if (foundMinPercentage != std::string::npos)
      min =
          atof(m_constraintMin.substr(0, m_constraintMin.size() - 1).c_str()) *
          m_value * 0.01;
    else
      min = atof(m_constraintMin.c_str());
  }
  if (m_constraintMax.compare("")) {
    if (foundMaxPercentage != std::string::npos)
      max =
          atof(m_constraintMax.substr(0, m_constraintMax.size() - 1).c_str()) *
          m_value * 0.01;
    else
      max = atof(m_constraintMax.c_str());
  }

  if (m_constraintMin.compare("") && m_constraintMax.compare("")) {
    constraint << min << " < " << m_name << " < " << max;
  } else if (m_constraintMin.compare("")) {
    constraint << min << " < " << m_name;
  } else {
    constraint << m_name << " < " << max;
  }

  return constraint.str();
}

/**
  Get parameter value. The default parameter 'at' is ignored expect if
  the value of the parameter is determined from a look up table or a formula.
  @param at :: number to return the value at
  @return the value of the fit parameter
*/
double FitParameter::getValue(const double &at) const {
  if (m_lookUpTable.containData()) {
    m_value = m_lookUpTable.value(at);
    return m_value;
  }

  if (m_formula.compare("") != 0) {
    size_t found;
    std::string equationStr = m_formula;
    std::string toReplace = "centre"; // replace this string in formula
    size_t len = toReplace.size();
    found = equationStr.find(toReplace);
    std::stringstream readDouble;
    readDouble << at;
    std::string extractedValueStr = readDouble.str();
    if (found != std::string::npos)
      equationStr.replace(found, len, extractedValueStr);

    // check if more than one string to replace in m_eq

    while (equationStr.find(toReplace) != std::string::npos) {
      found = equationStr.find(toReplace);
      equationStr.replace(found, len, extractedValueStr);
    }

    try {
      mu::Parser p;
      p.SetExpr(equationStr);
      m_value = p.Eval();
      return m_value;
    } catch (mu::Parser::exception_type &e) {
      g_log.error() << "Cannot evaluate fitting parameter formula."
                    << " Formula which cannot be passed is " << m_formula
                    << ". Muparser error message is: " << e.GetMsg()
                    << std::endl;
    }
  }

  return m_value;
}

/**
  Get parameter value.
  @return the value of the fit parameter
*/
double FitParameter::getValue() const { return m_value; }

/**
  Prints object to stream
  @param os :: the Stream to output to
*/
void FitParameter::printSelf(std::ostream &os) const {
  os << m_value << " , " << m_function << " , " << m_name << " , "
     << m_constraintMin << " , " << m_constraintMax << " , "
     << m_constraintPenaltyFactor << " , " << m_tie << " , " << m_formula
     << " , " << m_formulaUnit << " , " << m_resultUnit << " , "
     << m_lookUpTable;
  return;
}

/**
  Prints the value of parameter
  @param os :: the Stream to output to
  @param f :: the FitParameter to output
  @return the output stream
  */
std::ostream &operator<<(std::ostream &os, const FitParameter &f) {
  f.printSelf(os);
  return os;
}

/**
  Reads in information about a fitting parameter. The expected format is a comma
  separated
  list of 3 or more entries. The list will be read according to:

     1st (0) : parameter value (which is converted to float)
     2nd (1) : fitting function this parameter belong to
     3rd (2) : parameter name
     4th (3) : constrain min
     5th (4) : constrain max
     6th (5) : constrain penalty factor
     7th (6) : set tie
     8th (7) : set formula
     9th (8) : set formula unit
     10th (9) : set result unit
     11th onwards (10-) : read lookup table values

  Information about fitting \<parameter\> can be found on
  www.mantidproject.org/IDF.
  Note also printSelf() does the reverse of the this method, i.e. print of the
  information
  of a parameter as listed above.
  @param in :: Input Stream
  @param f :: FitParameter to write to
  @return Current state of stream
*/
std::istream &operator>>(std::istream &in, FitParameter &f) {

  typedef Poco::StringTokenizer tokenizer;
  std::string str;
  getline(in, str);
  tokenizer values(str, ",", tokenizer::TOK_TRIM);

  if (values.count() <= 2) {
    g_log.warning()
        << "Expecting a comma separated list of at each three entries"
        << " (any of which may be empty strings) to set information about a "
           "fitting parameter"
        << " instead of: " << str << std::endl;
    return in;
  }

  try {
    f.setValue() = boost::lexical_cast<double>(values[0]);
  } catch (boost::bad_lexical_cast &) {
    f.setValue() = 0.0;

    if (!values[0].empty()) {
      g_log.warning() << "Could not read " << values[0] << " as double for "
                      << " fitting parameter: " << values[1] << ":" << values[2]
                      << std::endl;
    }
  }

  // read remaining required entries

  f.setFunction() = values[1];
  f.setName() = values[2];

  // read optional entries

  try {
    f.setConstraintMin() = values[3];
  } catch (...) {
    f.setConstraintMin() = "";
  }

  try {
    f.setConstraintMax() = values[4];
  } catch (...) {
    f.setConstraintMax() = "";
  }

  try {
    f.setConstraintPenaltyFactor() = values[5];
  } catch (...) {
    f.setConstraintPenaltyFactor() = "";
  }

  try {
    f.setTie() = values[6];
  } catch (...) {
    f.setTie() = "";
  }

  try {
    f.setFormula() = values[7];
  } catch (...) {
    f.setFormula() = "";
  }

  try {
    f.setFormulaUnit() = values[8];
  } catch (...) {
    f.setFormulaUnit() = "";
  }

  try {
    f.setResultUnit() = values[9];
  } catch (...) {
    f.setResultUnit() = "";
  }

  if (values.count() > 10) {
    std::stringstream str(values[10]);
    str >> f.setLookUpTable();
  }

  return in;
}

} // namespace Geometry
} // namespace Mantid
