#include "MantidCurveFitting/SplineWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidCurveFitting/CubicSpline.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include <gsl/gsl_sf_erf.h>
#include <cmath>


using namespace Mantid::API;

namespace Mantid
{
namespace CurveFitting
{

  //----------------------------------------------------------------------------------------------
  DECLARE_FUNCTION(SplineWorkspace)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SplineWorkspace::SplineWorkspace()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
  */
  SplineWorkspace::~SplineWorkspace()
  {
  }

  /**
  * Define the fittable parameters
  */
  void SplineWorkspace::init()
  {
  }

/** Main function
    read workspace and interpolate
  */
void SplineWorkspace::function1D(double* out, const double* xValues, const size_t nData) const
{
  MatrixWorkspace_const_sptr inputWorkspace = getMatrixWorkspace();
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


void SplineWorkspace::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData)
{
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


} // namespace CurveFitting
} // namespace Mantid
