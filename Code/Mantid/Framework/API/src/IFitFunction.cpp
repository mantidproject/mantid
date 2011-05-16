//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/SpectraDetectorMap.h"

#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  
  Kernel::Logger& IFitFunction::g_log = Kernel::Logger::get("IFitFunction");

/**
 * Destructor
 */
  IFitFunction::~IFitFunction()
  {
    if (m_handler)
    {
      delete m_handler;
    }
  }

/** Base class implementation of derivative IFitFunction throws error. This is to check if such a function is provided
    by derivative class. In the derived classes this method must return the derivatives of the function
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used or the derivatives are computed numerically.
 * @param out :: Pointer to a Jacobian matrix. If it is NULL the method is called in order to check whether it's implemented or not.
 *      If the derivatives are implemented the method must simply return, otherwise it must throw Kernel::Exception::NotImplementedError.
 */
void IFitFunction::functionDeriv(Jacobian* out)
{
  UNUSED_ARG(out);
  throw ("No derivative IFitFunction provided");
}



/** Update active parameters. Ties are applied.
 *  @param in :: Pointer to an array with active parameters values. Must be at least nActive() doubles long.
 */
void IFitFunction::updateActive(const double* in)
{
  if (in)
    for(int i=0;i<nActive();i++)
    {
      setActiveParameter(i,in[i]);
    }
  applyTies();
}

/**
 * Sets active parameter i to value. Ties are not applied.
 * @param i :: The index of active parameter to set
 * @param value :: The new value for the parameter
 */
void IFitFunction::setActiveParameter(int i,double value)
{
  int j = indexOfActive(i);
  setParameter(j,value,false);
}

double IFitFunction::activeParameter(int i)const
{
  int j = indexOfActive(i);
  return getParameter(j);
}

/** Create a new tie. IFitFunctions can have their own types of ties.
 * @param parName :: The parameter name for this tie
 * @return a new parameter tie
 */
ParameterTie* IFitFunction::createTie(const std::string& parName)
{
  return new ParameterTie(this,parName);
}

/**
 * Ties a parameter to other parameters
 * @param parName :: The name of the parameter to tie.
 * @param expr ::    A math expression 
 * @return newly ties parameters
 */
ParameterTie* IFitFunction::tie(const std::string& parName,const std::string& expr)
{
  ParameterTie* tie = this->createTie(parName);
  int i = getParameterIndex(*tie);
  if (i < 0)
  {
    delete tie;
    throw std::logic_error("Parameter "+parName+" was not found.");
  }

  //if (!this->isActive(i))
  //{
  //  delete tie;
  //  throw std::logic_error("Parameter "+parName+" is already tied.");
  //}
  tie->set(expr);
  addTie(tie);
  this->removeActive(i);
  return tie;
}

/** Removes the tie off a parameter. The parameter becomes active
 * This method can be used when constructing and editing the IFitFunction in a GUI
 * @param parName :: The name of the parameter which ties will be removed.
 */
void IFitFunction::removeTie(const std::string& parName)
{
  int i = parameterIndex(parName);
  this->removeTie(i);
}

/**
  * If any of the parameters do not satisfy a constraint penalty values will be added to the function output.
  * This method is called by Fit algorithm after calling function(double*out)
  * @param out :: The output form function(double* out) to which the penalty will be added.
  */
void IFitFunction::addPenalty(double *out)const
{
    double penalty = 0.;
    for(int i=0;i<nParams();++i)
    {
      API::IConstraint* c = getConstraint(i);
      if (c)
      {
        penalty += c->check();
      }
    }

    int n = dataSize() - 1;
    // add penalty to first and last point and every 10th point in between
    if ( penalty != 0.0 )
    {
      out[0] += penalty;
      out[n] += penalty;

      for (int i = 9; i < n; i+=10)
      {
        out[i] += penalty;
      }
    }
}

/**
  * If a penalty was added to the function output the derivatives are modified accordingly.
  * This method is called by Fit algorithm after calling functionDeriv(Jacobian *out)
  * @param out :: The Jacobian to be corrected
  */
void IFitFunction::addPenaltyDeriv(Jacobian *out)const
{
  int n = dataSize() - 1;
  for(int i=0;i<nParams();++i)
  {  
    API::IConstraint* c = getConstraint(i);
    if (c)
    {
      double penalty = c->checkDeriv();
      if ( penalty != 0.0 )
      {
        int ia = activeIndex(i);
        double deriv = out->get(0,ia);
        out->set(0,ia,deriv + penalty);
        deriv = out->get(n,ia);
        out->set(n,ia,deriv+penalty);

        for (int j = 9; j < n; j+=10)
        {
          deriv = out->get(j,ia);
          out->set(j,ia,deriv+penalty);
        }
      }
    } // if (c)
  }

}

/**
 * Writes a string that can be used in Fit.IFitFunction to create a copy of this IFitFunction
 * @return string representation of the function
 */
std::string IFitFunction::asString()const
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
  for(int i=0;i<nParams();i++)
  {
    ostr<<','<<parameterName(i)<<'='<<getParameter(i);
  }
  std::string constraints;
  for(int i=0;i<nParams();i++)
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
  for(int i=0;i<nParams();i++)
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
void IFitFunction::setHandler(FitFunctionHandler* handler)
{
  m_handler = handler;
  if (handler && handler->function() != this)
  {
    throw std::runtime_error("Function handler points to a different function");
  }
  m_handler->init();
}

/**
 * Operator <<
 * @param ostr :: The output stream
 * @param f :: The IFitFunction
 */
std::ostream& operator<<(std::ostream& ostr,const IFitFunction& f)
{
  ostr << f.asString();
  return ostr;
}

/**
 * Const attribute visitor returning the type of the attribute
 */
class AttType: public IFitFunction::ConstAttributeVisitor<std::string>
{
protected:
  /// Apply if string
  std::string apply(const std::string&)const{return "std::string";}
  /// Apply if int
  std::string apply(const int&)const{return "int";}
  /// Apply if double
  std::string apply(const double&)const{return "double";}
};

std::string IFitFunction::Attribute::type()const
{
  AttType tmp;
  return apply(tmp);
}

/**
 * Const attribute visitor returning the type of the attribute
 */
class AttValue: public IFitFunction::ConstAttributeVisitor<std::string>
{
public:
  AttValue(bool quoteString=false) : 
    IFitFunction::ConstAttributeVisitor<std::string>(),
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

std::string IFitFunction::Attribute::value()const
{
  AttValue tmp(m_quoteValue);
  return apply(tmp);
}

std::string IFitFunction::Attribute::asString()const
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

std::string IFitFunction::Attribute::asQuotedString()const
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

std::string IFitFunction::Attribute::asUnquotedString()const
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

int IFitFunction::Attribute::asInt()const
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

double IFitFunction::Attribute::asDouble()const
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
void IFitFunction::Attribute::setString(const std::string& str)
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
void IFitFunction::Attribute::setDouble(const double& d)
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
void IFitFunction::Attribute::setInt(const int& i)
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
class SetValue: public IFitFunction::AttributeVisitor<>
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
void IFitFunction::Attribute::fromString(const std::string& str)
{
  SetValue tmp(str);
  apply(tmp);
}

} // namespace API
} // namespace Mantid
