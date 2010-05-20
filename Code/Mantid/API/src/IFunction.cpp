//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidKernel/UnitFactory.h"

#include "boost/lexical_cast.hpp"

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
    by derivative class. In the derived classes this method must return the derivatives of the resuduals function
    (defined in void Fit1D::function(const double*, double*, const double*, const double*, const double*, const int&))
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used.
 * @param out Derivatives
 * @param xValues X values for data points
 * @param nData Number of data points
 */
void IFunction::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
  throw Kernel::Exception::NotImplementedError("No derivative IFunction provided");
}

/** Initialize the function providing it the workspace
 * @param workspace The workspace to set
 * @param wi The workspace index
 * @param xMin The lower bin index
 * @param xMax The upper bin index
 */
void IFunction::setWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int wi,int xMin,int xMax)
{
  m_workspace = workspace;
  m_workspaceIndex = wi;
  m_xMinIndex = xMin;
  m_xMaxIndex = xMax;

  try
  {

    // check if parameter are specified in instrument definition file

    Geometry::ParameterMap& paramMap = m_workspace->instrumentParameters();

    // in some tests where workspace a created on the fly a spectra to detector map
    // is for convenience not created. 
    if ( !(m_workspace->spectraMap().nElements()) )  
      return;

    Geometry::IDetector_sptr det = m_workspace->getDetector(wi);

    // if det is a detector groupworkspace then take as the detector
    // the detector returned by det->getID()
    if ( boost::dynamic_pointer_cast<Geometry::DetectorGroup>(det) )
    {
      API::IInstrument_sptr inst = m_workspace->getInstrument();
      det = inst->getDetector(det->getID());
    }

    for (int i = 0; i < nParams(); i++)
    {
      if ( !isExplicitlySet(i) )
      {
        Geometry::Parameter_sptr param = paramMap.getRecursive(&(*det), parameterName(i), "fitting");
        if (param != Geometry::Parameter_sptr())
        {
          // get FitParameter
          const Geometry::FitParameter& fitParam = param->value<Geometry::FitParameter>();

          // check first if this parameter is actually specified for this function
          if ( name().compare(fitParam.getFunction()) == 0 )
          {
            // update value          
            IFunctionWithLocation* testWithLocation = dynamic_cast<IFunctionWithLocation*>(this);
            if ( testWithLocation == NULL || 
              (fitParam.getLookUpTable().containData() == false && fitParam.getFormula().compare("") == 0) )
            {
              setParameter(i, fitParam.getValue());
            }
            else
            {
              double centreValue = testWithLocation->centre();
              Kernel::Unit_sptr targetUnit;
              if ( fitParam.getFormula().compare("") == 0 )
                targetUnit = fitParam.getLookUpTable().getXUnit();  // from table
              else
                targetUnit =  Kernel::UnitFactory::Instance().create(fitParam.getFormulaUnit());  // from formula

              Kernel::Unit_sptr wsUnit = m_workspace->getAxis(0)->unit();

              // if units are different first convert centre value into unit of look up table
              if ( targetUnit->unitID().compare(wsUnit->unitID()) != 0 )
              {
                double factor,power;
                if (wsUnit->quickConversion(*targetUnit,factor,power) )
                {
                  centreValue = factor * std::pow(centreValue,power);
                }
                else
                {
                  double l1,l2,twoTheta;

                  // Get l1, l2 and theta  (see also RemoveBins.calculateDetectorPosition())
                  IInstrument_const_sptr instrument = m_workspace->getInstrument();
                  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
                  l1 = instrument->getSource()->getDistance(*sample);
                  Geometry::IDetector_const_sptr det = m_workspace->getDetector(wi);
                  if ( ! det->isMonitor() )
                  {
                    l2 = det->getDistance(*sample);
                    twoTheta = m_workspace->detectorTwoTheta(det);
                  }
                  else  // If this is a monitor then make l1+l2 = source-detector distance and twoTheta=0
                  {
                    l2 = det->getDistance(*(instrument->getSource()));
                    l2 = l2 - l1;
                    twoTheta = 0.0;
                  }

                  std::vector<double> endPoint;
                  endPoint.push_back(centreValue);
                  std::vector<double> emptyVec;
                  wsUnit->toTOF(endPoint,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
                  targetUnit->fromTOF(endPoint,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
                  centreValue = endPoint[0];
                }
              }  // end of: lookUpUnit->unitID().compare(wsUnit->unitID()) != 0
              setParameter(i, fitParam.getValue(centreValue));
            }  // end of update parameter value

            // add tie if specified for this parameter in instrument definition file
            if ( fitParam.getTie().compare("") )
            {  
              std::ostringstream str;
              str << fitParam.getValue();
              tie(parameterName(i), str.str());
            }

            // add constraint if specified for this parameter in instrument definition file
            if ( fitParam.getConstraint().compare("") )
            {  
              IConstraint* constraint = ConstraintFactory::Instance().createInitialized(this, fitParam.getConstraint());
              if ( fitParam.getConstraintPenaltyFactor().compare("") )
              {
                double penalty;
                try
                {
                  penalty = atof(fitParam.getConstraintPenaltyFactor().c_str());
                  constraint->setPenaltyFactor(penalty);
                }
                catch (...)
                {
                  g_log.warning() << "Can't set penalty factor for constraint\n";
                }
              }
              addConstraint(constraint);
            }
          }
        }
      }
    }
  }
  catch(...)
  {
  }
}

/** Update active parameters. Ties are applied.
 *  @param in Pointer to an array with active parameters values. Must be at least nActive() doubles long.
 */
void IFunction::updateActive(const double* in)
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
 * @param i The index of active parameter to set
 * @param value The new value for the parameter
 */
void IFunction::setActiveParameter(int i,double value)
{
  int j = indexOfActive(i);
  setParameter(j,value,false);
}

double IFunction::activeParameter(int i)const
{
  int j = indexOfActive(i);
  return getParameter(j);
}

/** Create a new tie. IFunctions can have their own types of ties.
 * @param parName The parameter name for this tie
 */
ParameterTie* IFunction::createTie(const std::string& parName)
{
  return new ParameterTie(this,parName);
}

/**
 * Ties a parameter to other parameters
 * @param parName The name of the parameter to tie.
 * @param expr    A math expression 
 */
ParameterTie* IFunction::tie(const std::string& parName,const std::string& expr)
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
 * This method can be used when constructing and editing the IFunction in a GUI
 * @param parName The name of the paramter which ties will be removed.
 */
void IFunction::removeTie(const std::string& parName)
{
  int i = parameterIndex(parName);
  this->removeTie(i);
}

/**
 * Calculate the Jacobian with respect to parameters actually declared in the IFunction
 * @param out The output Jacobian
 * @param xValues The x-values
 * @param nData The number of data points (and x-values).
 */
void IFunction::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
  this->functionDeriv(out,xValues,nData);
}

/**
 * Writes a string that can be used in Fit.IFunction to create a copy of this IFunction
 */
std::string IFunction::asString()const
{
  std::ostringstream ostr;
  ostr << "name="<<this->name();
  std::vector<std::string> attr = this->getAttributeNames();
  for(size_t i=0;i<attr.size();i++)
  {
    ostr<<','<<attr[i]<<'='<<this->getAttribute(attr[i]).value();
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
 * @param handler A new handler
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

/**
 * Operator <<
 * @param ostr The output stream
 * @param f The IFunction
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
protected:
  /// Apply if string
  std::string apply(const std::string& str)const{return str;}
  /// Apply if int
  std::string apply(const int& i)const{return boost::lexical_cast<std::string>(i);}
  /// Apply if double
  std::string apply(const double& d)const{return boost::lexical_cast<std::string>(d);}
};

std::string IFunction::Attribute::value()const
{
  AttValue tmp;
  return apply(tmp);
}

std::string IFunction::Attribute::asString()const
{
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
 * @param str The new value
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
 * @param d The new value
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
 * @param i The new value
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
   * @param str The value to set
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
  std::string m_value;
};

/** Set value from a string. Throws exception if the string has wrong format
 * @param str String representation of the new value
 */
void IFunction::Attribute::fromString(const std::string& str)
{
  SetValue tmp(str);
  apply(tmp);
}

} // namespace API
} // namespace Mantid
