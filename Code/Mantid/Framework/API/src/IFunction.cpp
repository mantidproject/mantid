//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"

#include <boost/lexical_cast.hpp>

#include <Poco/StringTokenizer.h>

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  
  Kernel::Logger& IFunction::g_log = Kernel::Logger::get("IFunction");

/**
 * Destructor
 */
  IFunction::~IFunction()
  {
    if (m_handler)
    {
      delete m_handler;
    }
  }

/** Base class implementation of derivative IFunction throws error. This is to check if such a function is provided
    by derivative class. In the derived classes this method must return the derivatives of the function
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used or the derivatives are computed numerically.
 * @param domain :: The domain of the function
 * @param jacobian :: Pointer to a Jacobian matrix. If it is NULL the method is called in order to check whether it's implemented or not.
 *      If the derivatives are implemented the method must simply return, otherwise it must throw Kernel::Exception::NotImplementedError.
 */
void IFunction::functionDeriv(const FunctionDomain& domain, Jacobian& jacobian)
{
  UNUSED_ARG(domain);
  UNUSED_ARG(jacobian);
  throw ("No derivative IFunction provided");
}

/**
 * Ties a parameter to other parameters
 * @param parName :: The name of the parameter to tie.
 * @param expr ::    A math expression 
 * @return newly ties parameters
 */
ParameterTie* IFunction::tie(const std::string& parName,const std::string& expr)
{
  ParameterTie* ti = new ParameterTie(this,parName,expr);
  addTie(ti);
  this->fix(getParameterIndex(*ti));
  return ti;
}

/** Removes the tie off a parameter. The parameter becomes active
 * This method can be used when constructing and editing the IFunction in a GUI
 * @param parName :: The name of the parameter which ties will be removed.
 */
void IFunction::removeTie(const std::string& parName)
{
  size_t i = parameterIndex(parName);
  this->removeTie(i);
}

/**
 * Writes a string that can be used in Fit.IFunction to create a copy of this IFunction
 * @return string representation of the function
 */
std::string IFunction::asString()const
{
  std::ostringstream ostr;
  ostr << "name="<<this->name();
  std::vector<std::string> attr = this->getAttributeNames();
  for(size_t i=0;i<attr.size();i++)
  {
    std::string attName = attr[i];
    std::string attValue = this->getAttribute(attr[i]).value();
    if (!attValue.empty())
    {
      ostr<<','<<attName<<'='<<attValue;
    }
  }
  for(size_t i=0;i<nParams();i++)
  {
    ostr<<','<<parameterName(i)<<'='<<getParameter(i);
  }
  std::string constraints;
  for(size_t i=0;i<nParams();i++)
  {
    const IConstraint* c = getConstraint(i);
    if (c)
    {
      std::string tmp = c->asString();
      if (!tmp.empty())
      {
        if (!constraints.empty())
        {
          constraints += ",";
        }
        constraints += tmp;
      }
    }
  }
  if (!constraints.empty())
  {
    ostr << ",constraints=(" << constraints << ")";
  }

  std::string ties;
  for(size_t i=0;i<nParams();i++)
  {
    const ParameterTie* tie = getTie(i);
    if (tie)
    {
      std::string tmp = tie->asString(this);
      if (!tmp.empty())
      {
        if (!ties.empty())
        {
          ties += ",";
        }
        ties += tmp;
      }
    }
  }
  if (!ties.empty())
  {
    ostr << ",ties=(" << ties << ")";
  }
  return ostr.str();
}

/** Set a function handler
 * @param handler :: A new handler
 */
void IFunction::setHandler(FunctionHandler* handler)
{
  m_handler = handler;
  if (handler && handler->function() != this)
  {
    throw std::runtime_error("Function handler points to a different function");
  }
  m_handler->init();
}

/// Function to return all of the categories that contain this function
const std::vector<std::string> IFunction::categories() const
{
  std::vector < std::string > res;
  Poco::StringTokenizer tokenizer(category(), categorySeparator(),
      Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
  Poco::StringTokenizer::Iterator h = tokenizer.begin();

  for (; h != tokenizer.end(); ++h)
  {
    res.push_back(*h);
  }

  return res;
}

/**
 * Operator <<
 * @param ostr :: The output stream
 * @param f :: The IFunction
 */
std::ostream& operator<<(std::ostream& ostr,const IFunction& f)
{
  ostr << f.asString();
  return ostr;
}

/**
 * Const attribute visitor returning the type of the attribute
 */
class AttType: public IFunction::ConstAttributeVisitor<std::string>
{
protected:
  /// Apply if string
  std::string apply(const std::string&)const{return "std::string";}
  /// Apply if int
  std::string apply(const int&)const{return "int";}
  /// Apply if double
  std::string apply(const double&)const{return "double";}
};

std::string IFunction::Attribute::type()const
{
  AttType tmp;
  return apply(tmp);
}

/**
 * Const attribute visitor returning the type of the attribute
 */
class AttValue: public IFunction::ConstAttributeVisitor<std::string>
{
public:
  AttValue(bool quoteString=false) : 
    IFunction::ConstAttributeVisitor<std::string>(),
    m_quoteString(quoteString) 
  {
  }

protected:
  /// Apply if string
  std::string apply(const std::string& str)const
  {
    return (m_quoteString) ? std::string("\"" + str + "\"") : str;
  }
  /// Apply if int
  std::string apply(const int& i)const{return boost::lexical_cast<std::string>(i);}
  /// Apply if double
  std::string apply(const double& d)const{return boost::lexical_cast<std::string>(d);}

private:
  /// Flag to quote a string value returned
  bool m_quoteString;
};

std::string IFunction::Attribute::value()const
{
  AttValue tmp(m_quoteValue);
  return apply(tmp);
}

std::string IFunction::Attribute::asString()const
{
  if( m_quoteValue ) return asQuotedString();
  
  try
  {
    return boost::get<std::string>(m_data);
  }
  catch(...)
  {
    throw std::runtime_error("Trying to access a "+type()+" attribute "
      "as string");
  }
}

std::string IFunction::Attribute::asQuotedString()const
{
  std::string attr;

  try
  {
    attr = boost::get<std::string>(m_data);
  }
  catch(...)
  {
    throw std::runtime_error("Trying to access a "+type()+" attribute "
      "as string");
  }
  std::string quoted(attr);
  if( *(attr.begin()) != '\"' ) quoted = "\"" + attr;
  if( *(quoted.end() - 1) != '\"' ) quoted += "\"";

  return quoted;
}

std::string IFunction::Attribute::asUnquotedString()const
{
  std::string attr;

  try
  {
    attr = boost::get<std::string>(m_data);
  }
  catch(...)
  {
    throw std::runtime_error("Trying to access a "+type()+" attribute "
      "as string");
  }
  std::string unquoted(attr);
  if( *(attr.begin()) == '\"' ) unquoted = std::string(attr.begin() + 1, attr.end() - 1);
  if( *(unquoted.end() - 1) == '\"' ) unquoted = std::string(unquoted.begin(), unquoted.end() - 1);
  
  return unquoted;
}

int IFunction::Attribute::asInt()const
{
  try
  {
    return boost::get<int>(m_data);
  }
  catch(...)
  {
    throw std::runtime_error("Trying to access a "+type()+" attribute "
      "as int");
  }
}

double IFunction::Attribute::asDouble()const
{
  try
  {
    return boost::get<double>(m_data);
  }
  catch(...)
  {
    throw std::runtime_error("Trying to access a "+type()+" attribute "
      "as double");
  }
}

/** Sets new value if attribute is a string. If the type is wrong 
 * throws an exception
 * @param str :: The new value
 */
void IFunction::Attribute::setString(const std::string& str)
{
  try
  {
    boost::get<std::string>(m_data) = str;
  }
  catch(...)
  {
    throw std::runtime_error("Trying to access a "+type()+" attribute "
      "as string");
  }
}

/** Sets new value if attribute is a double. If the type is wrong 
 * throws an exception
 * @param d :: The new value
 */
void IFunction::Attribute::setDouble(const double& d)
{
  try
  {
    boost::get<double>(m_data) = d;
  }
  catch(...)
  {
    throw std::runtime_error("Trying to access a "+type()+" attribute "
      "as double");
  }
}

/** Sets new value if attribute is an int. If the type is wrong 
 * throws an exception
 * @param i :: The new value
 */
void IFunction::Attribute::setInt(const int& i)
{
  try
  {
    boost::get<int>(m_data) = i;
  }
  catch(...)
  {
    throw std::runtime_error("Trying to access a "+type()+" attribute "
      "as int");
  }
}

/**
 * Attribute visitor setting new value to an attribute
 */
class SetValue: public IFunction::AttributeVisitor<>
{
public:
  /**
   * Constructor
   * @param value :: The value to set
   */
  SetValue(const std::string& value):m_value(value){}
protected:
  /// Apply if string
  void apply(std::string& str)const{str = m_value;}
  /// Apply if int
  void apply(int& i)const
  {
    std::istringstream istr(m_value+" ");
    istr >> i;
    if (!istr.good()) throw std::invalid_argument("Failed to set int attribute "
      "from string "+m_value);
  }
  /// Apply if double
  void apply(double& d)const
  {
    std::istringstream istr(m_value+" ");
    istr >> d;
    if (!istr.good()) throw std::invalid_argument("Failed to set double attribute "
      "from string "+m_value);
  }
private:
  std::string m_value; ///<the value as a string
};

/** Set value from a string. Throws exception if the string has wrong format
 * @param str :: String representation of the new value
 */
void IFunction::Attribute::fromString(const std::string& str)
{
  SetValue tmp(str);
  apply(tmp);
}

} // namespace API
} // namespace Mantid
