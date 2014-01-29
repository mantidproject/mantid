#ifndef MANTID_CURVEFITTING_ConvolveWorkspaces_H_
#define MANTID_CURVEFITTING_ConvolveWorkspaces_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidCurveFitting/CubicSpline.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"

namespace Mantid
{
namespace CurveFitting
{
/** Convolution of two workspaces

*/
class Convolution_Spline : public API::ParamFunction, public API::IFunction1D,  virtual public API::IFunctionMW
{
public:
  Convolution_Spline()
  {
    //declareParameter("InputWorkspace","");
    //declareParameter("l",0);
  }

  std::string name()const{return "Convolution_Spline";}

  void function1D(double* out, const double* xValues, const size_t nData)const
  {
    API::MatrixWorkspace_const_sptr inputWorkspace = getMatrixWorkspace();
    //double l = getParameter("l");
    const auto & xIn = inputWorkspace->readX(0);
    const auto & yIn = inputWorkspace->readY(0);
    int size = static_cast<int>(xIn.size());

    boost::shared_ptr<CubicSpline> m_cspline = boost::make_shared<CubicSpline>();
    m_cspline->setAttributeValue("n", size);

    for (int i = 0; i < size; ++i)
    {
      m_cspline->setXAttribute(i, xIn[i]);
      m_cspline->setParameter(i, yIn[i]);
    }
    //calculate the interpolation
    m_cspline->function1D(out, xValues, nData);
  }
  void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
  {
    //std::string inputWorkspace = getParameter("InputWorkspace");
    DataObjects::Workspace2D_sptr inputWorkspace;
    double l = getParameter("l");
    const auto & xIn = inputWorkspace->readX(static_cast<size_t>(l));
    const auto & yIn = inputWorkspace->readY(static_cast<size_t>(l));
    int size = static_cast<int>(xIn.size());

    boost::shared_ptr<CubicSpline> m_cspline = boost::make_shared<CubicSpline>();
    m_cspline->setAttributeValue("n", size);

    for (int i = 0; i < size; ++i)
    {
      m_cspline->setXAttribute(i, xIn[i]);
      m_cspline->setParameter(i, yIn[i]);
    }
    //calculate the derivatives
   double* yValues(0);
    m_cspline->derivative1D(yValues, xValues, nData, 2);
    for(size_t i=0;i<nData;i++)
    {
      out->set(i,0,1.);
      out->set(i,1,xValues[i]);
      out->set(i,2,yValues[i]);
    }

  }
};
class DLLExport ConvolveWorkspaces : public API::Algorithm
{
public:
  ConvolveWorkspaces();
  virtual ~ConvolveWorkspaces();
  /// Algorithm's name
  virtual const std::string name() const { return "ConvolveWorkspaces"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility\\Workspaces"; }

private:
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  void convolve(MantidVec& xValues, const MantidVec& Y1, const MantidVec& Y2, MantidVec& out)const;
  API::Progress * prog;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_ConvolveWorkspaces_H_*/
