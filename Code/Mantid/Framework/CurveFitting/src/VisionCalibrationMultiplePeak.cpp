//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <boost/make_shared.hpp>

#include "MantidKernel/ListValidator.h"
#include "MantidKernel/BoundedValidator.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"

#include "MantidCurveFitting/VisionCalibrationMultiplePeak.h"
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Quadratic.h"

namespace Mantid
{
namespace CurveFitting
{
DECLARE_FUNCTION(VisionCalibrationMock);
DECLARE_FUNCTION(VisionCalibrationLBGGs);
DECLARE_FUNCTION(VisionCalibrationLBGGm); // Register VisionCalibrationLBGGs in the function factory
DECLARE_ALGORITHM(VisionCalibrationMultiplePeak); // Register VisionCalibrationMultiplePeak in the algorithm factory

VisionCalibrationMock::VisionCalibrationMock() : m_attName("spectrumIndex") , m_att(0)
{
  declareParameter("A0", 0.0,"linear background constant");
  declareParameter("A1", 0.0,"linear background slope");

  declareParameter("B10", 1.0,"first quadratic constant");
//  declareParameter("B11", 0.0,"first quadratic slope");
//  declareParameter("B12", 0.0,"first quadratic curvature");

  declareParameter("PeakCenter1", 0.0,"first Gaussian peak center");
  declareParameter("Sigma1", 1.0,"first Gaussian standard deviation");

  declareParameter("B20", 1.0,"second quadratic constant");
//  declareParameter("B21", 0.0,"second quadratic slope");
//  declareParameter("B22", 0.0,"second quadratic curvature");

  declareParameter("PeakCenter2", 0.0,"second Gaussian peak center");
  declareParameter("Sigma2", 1.0,"second Gaussian standard deviation");

}//VisionCalibrationMock::VisionCalibrationMock

void VisionCalibrationMock::setActiveParameter(size_t i,double value)
{
  if ( !isActive(i) )  throw std::runtime_error("Attempt to use an inactive parameter");
  if (parameterName(i) == "Sigma1")
    setParameter(i,sqrt(fabs(1./value)),false); //set the active parameter as (1/Sigma1^2)
  else if (parameterName(i) == "Sigma2")
    setParameter(i,sqrt(fabs(1./value)),false);
  else
   setParameter(i,value,false);
} // setActiveParameter

double VisionCalibrationMock::activeParameter(size_t i)const
{
  if ( !isActive(i) )  throw std::runtime_error("Attempt to use an inactive parameter");
  if (parameterName(i) == "Sigma1")
    return 1./pow(getParameter(i),2); // return (1/Sigma1^2) in place of Sigma1
  else if (parameterName(i) == "Sigma2")
    return 1./pow(getParameter(i),2);
  else
    return getParameter(i);
} // setActiveParameter

void VisionCalibrationMock::setAttribute(const std::string& attName,const Attribute& att)
{
  if (attName == m_attName)
  {
    m_att.setInt( att.asInt() );
    return ;
  }
  g_log.error(attName+" is not an attribute of this function");
} // setAttribute

API::IFunction::Attribute VisionCalibrationMock::getAttribute(const std::string& attName)const
{
  if (attName == m_attName) return m_att;
  g_log.error(attName+" is not an attribute of this function");
  return Attribute(-1);
}  // getAttribute

void VisionCalibrationMock::function1D(double* out, const double* xValues, const size_t nData) const
{
  const double &a0 = getParameter("A0");
  const double &a1 = getParameter("A1");

  const double &b10 = getParameter("B10");
//  const double &b11 = getParameter("B11");
//  const double &b12 = getParameter("B12");

  const double &pc1 = getParameter("PeakCenter1");
  const double &s1 = getParameter("Sigma1");
  const double &r1 = pow(1/s1,2); // active parameter in place of Sigma1

  const double &b20 = getParameter("B20");
//  const double &b21 = getParameter("B21");
//  const double &b22 = getParameter("B22");

  const double &pc2 = getParameter("PeakCenter2");
  const double &s2 = getParameter("Sigma2");
  const double &r2 = pow(1/s2,2);

  const double spc = (double)( m_att.asInt() );

  for(size_t i=0; i<nData; i++)
  {
    double x = xValues[i];

    double lb = a0+ a1 * x;

//    double q1 = b10 + spc * (b11 + b12 * spc);
    double q1 = b10;
    double d1 = x-pc1;
    double g1 = exp(-0.5 * r1 * d1 * d1);

//    double q2 = b20 + spc * (b21 + b22 * spc);
    double q2 = b20;
    double d2 = x-pc2;
    double g2 = exp(-0.5 * r2 * d2 * d2);

    out[i] = lb + q1*g1 + q2*g2;
  }
}//function1D

void VisionCalibrationMock::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
{
  const double &b10 = getParameter("B10");
//  const double &b11 = getParameter("B11");
//  const double &b12 = getParameter("B12");

  const double &pc1 = getParameter("PeakCenter1");
  const double &s1 = getParameter("Sigma1");
  const double &r1 = pow(1/s1,2); // active parameter in place of Sigma1

  const double &b20 = getParameter("B20");
//  const double &b21 = getParameter("B21");
//  const double &b22 = getParameter("B22");

  const double &pc2 = getParameter("PeakCenter2");
  const double &s2 = getParameter("Sigma2");
  const double &r2 = pow(1/s2,2);

  const double spc = (double)( m_att.asInt() );

  for(size_t i=0; i<nData; i++)
  {
    double x = xValues[i];

//    double q1 = b10 + spc * (b11 + b12 * spc);
    double q1 = b10;
    double d1 = x-pc1;
    double g1 = exp(-0.5 * r1 * d1 * d1);

//    double q2 = b20 + spc * (b21 + b22 * spc);
    double q2 = b20;
    double d2 = x-pc2;
    double g2 = exp(-0.5 * r2 * d2 * d2);

    //partial derivative with respect to a0
    out->set(i,0,1.0);
    //partial derivative with respect to a1
    out->set(i,1,x);

    //partial derivative with respect to b10
    out->set(i,2,g1);
    //partial derivative with respect to b11
//    out->set(i,3,x*g1);
    //partial derivative with respect to b12
//    out->set(i,4,x*x*g1);

    //partial derivative with respect to r1 (not s1)
//    out->set(i,5,q1*(-0.5*d1*d1*g1));
      out->set(i,3,q1*(-0.5*d1*d1*g1));
    //partial derivative with respect to pc1
//    out->set(i,6,q1*(r1*d1*g1));
      out->set(i,4,q1*(r1*d1*g1));

    //partial derivative with respect to b20
//    out->set(i,7,g2);
      out->set(i,5,g2);
    //partial derivative with respect to b21
//    out->set(i,8,x*g2);
    //partial derivative with respect to b22
//    out->set(i,9,x*x*g2);

    //partial derivative with respect to r2 (not s2)
//    out->set(i,10,q2*(-0.5*d2*d2*g2));
    out->set(i,6,q2*(-0.5*d2*d2*g2));
    //partial derivative with respect to pc2
//    out->set(i,11,q2*(r2*d2*g2));
    out->set(i,7,q2*(r2*d2*g2));

  }

}// functionDeriv1D


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


VisionCalibrationLBGGs::VisionCalibrationLBGGs() : m_attName("spectrumIndex") , m_att(0)
{
  declareParameter("A0", 0.0,"linear background constant");
  declareParameter("A1", 0.0,"linear background slope");

  declareParameter("B10", 1.0,"first quadratic constant");
  declareParameter("B11", 0.0,"first quadratic slope");
  declareParameter("B12", 0.0,"first quadratic curvature");

  declareParameter("PeakCenter1", 0.0,"first Gaussian peak center");
  declareParameter("Sigma1", 1.0,"first Gaussian standard deviation");

  declareParameter("B20", 1.0,"second quadratic constant");
  declareParameter("B21", 0.0,"second quadratic slope");
  declareParameter("B22", 0.0,"second quadratic curvature");

  declareParameter("PeakCenter2", 0.0,"second Gaussian peak center");
  declareParameter("Sigma2", 1.0,"second Gaussian standard deviation");

}//VisionCalibrationLBGGs::VisionCalibrationLBGGs

void VisionCalibrationLBGGs::setActiveParameter(size_t i,double value)
{
  if ( !isActive(i) )  throw std::runtime_error("Attempt to use an inactive parameter");
  if (parameterName(i) == "Sigma1")
    setParameter(i,sqrt(fabs(1./value)),false); //set the active parameter as (1/Sigma1^2)
  else if (parameterName(i) == "Sigma2")
    setParameter(i,sqrt(fabs(1./value)),false);
  else
   setParameter(i,value,false);
} // setActiveParameter

double VisionCalibrationLBGGs::activeParameter(size_t i)const
{
  if ( !isActive(i) )  throw std::runtime_error("Attempt to use an inactive parameter");
  if (parameterName(i) == "Sigma1")
    return 1./pow(getParameter(i),2); // return (1/Sigma1^2) in place of Sigma1
  else if (parameterName(i) == "Sigma2")
    return 1./pow(getParameter(i),2);
  else
    return getParameter(i);
} // setActiveParameter

void VisionCalibrationLBGGs::setAttribute(const std::string& attName,const Attribute& att)
{
  if (attName == m_attName)
  {
    m_att.setInt( att.asInt() );
    return ;
  }
  g_log.error(attName+" is not an attribute of this function");
} // setAttribute

API::IFunction::Attribute VisionCalibrationLBGGs::getAttribute(const std::string& attName)const
{
  if (attName == m_attName) return m_att;
  g_log.error(attName+" is not an attribute of this function");
  return Attribute(-1);
}  // getAttribute

void VisionCalibrationLBGGs::function1D(double* out, const double* xValues, const size_t nData) const
{
  const double &a0 = getParameter("A0");
  const double &a1 = getParameter("A1");

  const double &b10 = getParameter("B10");
  const double &b11 = getParameter("B11");
  const double &b12 = getParameter("B12");

  const double &pc1 = getParameter("PeakCenter1");
  const double &s1 = getParameter("Sigma1");
  const double &r1 = pow(1/s1,2); // active parameter in place of Sigma1

  const double &b20 = getParameter("B20");
  const double &b21 = getParameter("B21");
  const double &b22 = getParameter("B22");

  const double &pc2 = getParameter("PeakCenter2");
  const double &s2 = getParameter("Sigma2");
  const double &r2 = pow(1/s2,2);

  const double spc = (double)( m_att.asInt() );

  for(size_t i=0; i<nData; i++)
  {
    double x = xValues[i];

    double lb = a0+ a1 * x;

    double q1 = b10 + spc * (b11 + b12 * spc);
    double d1 = x-pc1;
    double g1 = exp(-0.5 * r1 * d1 * d1);

    double q2 = b20 + spc * (b21 + b22 * spc);
    double d2 = x-pc2;
    double g2 = exp(-0.5 * r2 * d2 * d2);

    out[i] = lb + q1*g1 + q2*g2;
  }
}//function1D

void VisionCalibrationLBGGs::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
{
  const double &b10 = getParameter("B10");
  const double &b11 = getParameter("B11");
  const double &b12 = getParameter("B12");

  const double &pc1 = getParameter("PeakCenter1");
  const double &s1 = getParameter("Sigma1");
  const double &r1 = pow(1/s1,2); // active parameter in place of Sigma1

  const double &b20 = getParameter("B20");
  const double &b21 = getParameter("B21");
  const double &b22 = getParameter("B22");

  const double &pc2 = getParameter("PeakCenter2");
  const double &s2 = getParameter("Sigma2");
  const double &r2 = pow(1/s2,2);

  const double spc = (double)( m_att.asInt() );

  for(size_t i=0; i<nData; i++)
  {
    double x = xValues[i];

    double q1 = b10 + spc * (b11 + b12 * spc);
    double d1 = x-pc1;
    double g1 = exp(-0.5 * r1 * d1 * d1);

    double q2 = b20 + spc * (b21 + b22 * spc);
    double d2 = x-pc2;
    double g2 = exp(-0.5 * r2 * d2 * d2);

    //partial derivative with respect to a0
    out->set(i,0,1.0);
    //partial derivative with respect to a1
    out->set(i,1,x);

    //partial derivative with respect to b10
    out->set(i,2,g1);
    //partial derivative with respect to b11
    out->set(i,3,x*g1);
    //partial derivative with respect to b12
    out->set(i,4,x*x*g1);

    //partial derivative with respect to r1 (not s1)
    out->set(i,5,q1*(-0.5*d1*d1*g1));
    //partial derivative with respect to pc1
    out->set(i,6,q1*(r1*d1*g1));

    //partial derivative with respect to b20
    out->set(i,7,g2);
    //partial derivative with respect to b21
    out->set(i,8,x*g2);
    //partial derivative with respect to b22
    out->set(i,9,x*x*g2);

    //partial derivative with respect to r2 (not s2)
    out->set(i,10,q2*(-0.5*d2*d2*g2));
    //partial derivative with respect to pc2
    out->set(i,11,q2*(r2*d2*g2));

  }

}// functionDeriv1D

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void VisionCalibrationLBGGm::setAttribute(const std::string& attName,const Attribute& att)
{
  if (attName == m_attName)
  {
    m_att.setInt( att.asInt() );
    return;
  }
  g_log.error(attName+" is not an attribute of this function");
} // setAttribute

API::IFunction::Attribute VisionCalibrationLBGGm::getAttribute(const std::string& attName)const
{
  if (attName == m_attName) return m_att;
  g_log.error(attName+" is not an attribute of this function");
  return Attribute(-1);
}  // getAttribute

void VisionCalibrationLBGGm::addFunctions()
{
  int range = abs( m_att.asInt() );
  if( !range )
  {
    g_log.error("setAttribute first");
    return;
  }
  // create ties if more than one function
  if(range>1)
  {
    m_ties.push_back("f0.B10");
    m_ties.push_back("f0.B11");
    m_ties.push_back("f0.B12");
    m_ties.push_back("f0.B10");
    m_ties.push_back("f0.B11");
    m_ties.push_back("f0.B12");
  }
  // dIdx signals a negative range
  int dIdx = m_att.asInt()>=0 ?  1 : -1;
  // add function(s and ties)
  for(int i=0; i<range; i++)
  {
    int spectrumIndex = i*dIdx;
    API::IFunction_sptr func1D = API::FunctionFactory::Instance().createFunction("VisionCalibrationLBGGs");
    func1D->setAttribute("spectrumIndex", Attribute(spectrumIndex) );
    if(i>0)
    {
      std::string str_i = boost::lexical_cast<std::string>(i);
      m_ties.at(0) += "=f" + str_i + ".B10";
      m_ties.at(1) += "=f" + str_i + ".B11";
      m_ties.at(2) += "=f" + str_i + ".B12";
      m_ties.at(3) += "=f" + str_i + ".B20";
      m_ties.at(4) += "=f" + str_i + ".B21";
      m_ties.at(5) += "=f" + str_i + ".B22";
    }
    this->addFunction(func1D);
  }
  // impose the ties
  if(range>1)
  {
    std::string allTies( m_ties.at(0) );
    for(auto it = m_ties.begin(); it != m_ties.end(); ++it)  allTies += ","+*it;
    this->addTies( allTies );
  }

} // addFunctions


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


VisionCalibrationMultiplePeak::VisionCalibrationMultiplePeak()
{
  m_fitModelsMap.insert( mss::value_type("LBGG","VisionCalibrationLBGGm") );
  m_modelsDescription.insert( mss::value_type("LBGG", "linear background plus two exponentials") );
}

void VisionCalibrationMultiplePeak::init()
{
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace","",Kernel::Direction::Input),
    "Workspace with the spectrum to fit");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Kernel::Direction::Output),
    "Name of the workspace that will contain the result");

  // select the fitting model with a pull-down menu
  std::vector<std::string> fitModelNames;
  for(auto it = m_fitModelsMap.begin(); it != m_fitModelsMap.end(); ++it)
    fitModelNames.push_back(it->first);
  std::string modelsDescription("");
  for(auto it = m_modelsDescription.begin(); it != m_modelsDescription.end(); ++it)
    modelsDescription += it->first+": "+it->second + "\n";
  declareProperty("FitModel", "LBGG", boost::make_shared<Kernel::StringListValidator>(fitModelNames),
    "The model used for fitting. Available models:\n"+modelsDescription);

  // workspace index of the pixel showing maximum intensity
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int> >();
  mustBePositive->setLower(0);
  declareProperty("startIndex",EMPTY_INT(),mustBePositive,
    "Workspace index with maximum intensity in the tube");

  // range of pixels below and above the pixel showing maximum intensity
  declareProperty("endIndex",EMPTY_INT(),
    "index of last included pixel in the fit. Can be same or smaller than start index");

} // init

void VisionCalibrationMultiplePeak::exec()
{

  // instantiate the fitting function model
  std::string fitModel = this->getPropertyValue("FitModel");
  m_fitModel = API::FunctionFactory::Instance().createFunction(fitModel);

  // add the onedimensional fitting functions to the model
  int sIdx = getProperty("startIndex");
  int eIdx = getProperty("endIndex");
  m_fitModel->setAttribute( "range", API::IFunction::Attribute(1+ eIdx - sIdx) );

  // retrieve the workspace and the central pixel
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

} // exec



} // namespace CurveFitting
} // namespace Mantid
