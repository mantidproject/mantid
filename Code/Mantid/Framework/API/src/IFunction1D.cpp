//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/muParser_Silent.h"
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  using namespace Geometry;
  
  Kernel::Logger& IFunction1D::g_log = Kernel::Logger::get("IFunction1D");

namespace
{
  /**
   * A simple implementation of Jacobian.
   */
  class SimpleJacobian: public Jacobian
  {
  public:
    /// Constructor
    SimpleJacobian(size_t nData,size_t nParams):m_nData(nData),m_nParams(nParams),m_data(nData*nParams){}
    /// Setter
    virtual void set(size_t iY, size_t iP, double value)
    {
      m_data[iY * m_nParams + iP] = value;
    }
    /// Getter
    virtual double get(size_t iY, size_t iP)
    {
      return m_data[iY * m_nParams + iP];
    }
  private:
    size_t m_nData; ///< size of the data / first dimension
    size_t m_nParams; ///< number of parameters / second dimension
    std::vector<double> m_data; ///< data storage
  };
}

void IFunction1D::function(const FunctionDomain& domain,FunctionValues& values)const
{
  const FunctionDomain1D* d1d = dynamic_cast<const FunctionDomain1D*>(&domain);
  if (!d1d)
  {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }
  function1D(values.getPointerToCalculated(0),d1d->getPointerAt(0),d1d->size());
}

void IFunction1D::functionDeriv(const FunctionDomain& domain, Jacobian& jacobian)
{
  const FunctionDomain1D* d1d = dynamic_cast<const FunctionDomain1D*>(&domain);
  if (!d1d)
  {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }
  functionDeriv1D(&jacobian,d1d->getPointerAt(0),d1d->size());
}

/** Base class implementation of derivative IFunction1D throws error. This is to check if such a function is provided
    by derivative class. In the derived classes this method must return the derivatives of the resuduals function
    (defined in void Fit1D::function(const double*, double*, const double*, const double*, const double*, const int&))
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used.
 */
void IFunction1D::functionDeriv1D(Jacobian*, const double*, const size_t)
{
  throw Kernel::Exception::NotImplementedError("No derivative IFunction1D provided");
}

/** 
 * Creates a workspace containing values calculated with this function. It takes a workspace and ws index
 * of a spectrum which this function may have been fitted to. The output contains the original spectrum 
 * (wi = 0), the calculated values (ws = 1), and the difference between them (ws = 2).
 * @param inWS :: input workspace
 * @param wi :: workspace index
 * @param sd :: optional standard deviations of the parameters for calculating the error bars
 * @return created workspase
 */
//boost::shared_ptr<API::MatrixWorkspace> IFunction1D::createCalculatedWorkspace(
//  boost::shared_ptr<const API::MatrixWorkspace> inWS, 
//  size_t wi,
//  const std::vector<double>& sd
//  )
//{
//      const MantidVec& inputX = inWS->readX(wi);
//      const MantidVec& inputY = inWS->readY(wi);
//      const MantidVec& inputE = inWS->readE(wi);
//      size_t nData = dataSize();
//
//      size_t histN = inWS->isHistogramData() ? 1 : 0;
//      API::MatrixWorkspace_sptr ws =
//        Mantid::API::WorkspaceFactory::Instance().create(
//            "Workspace2D",
//            3,
//            nData + histN,
//            nData);
//      ws->setTitle("");
//      ws->setYUnitLabel(inWS->YUnitLabel());
//      ws->setYUnit(inWS->YUnit());
//      ws->getAxis(0)->unit() = inWS->getAxis(0)->unit();
//      API::TextAxis* tAxis = new API::TextAxis(3);
//      tAxis->setLabel(0,"Data");
//      tAxis->setLabel(1,"Calc");
//      tAxis->setLabel(2,"Diff");
//      ws->replaceAxis(1,tAxis);
//
//      assert(m_xMaxIndex-m_xMinIndex+1 == nData);
//
//      for(size_t i=0;i<3;i++)
//      {
//        ws->dataX(i).assign(inputX.begin()+m_xMinIndex,inputX.begin()+m_xMaxIndex+1+histN);
//      }
//
//      ws->dataY(0).assign(inputY.begin()+m_xMinIndex,inputY.begin()+m_xMaxIndex+1);
//      ws->dataE(0).assign(inputE.begin()+m_xMinIndex,inputE.begin()+m_xMaxIndex+1);
//
//      MantidVec& Ycal = ws->dataY(1);
//      MantidVec& Ecal = ws->dataE(1);
//      MantidVec& E = ws->dataY(2);
//
//      double* lOut = new double[nData];  // to capture output from call to function()
//      function( lOut );
//
//      for(size_t i=0; i<nData; i++)
//      {
//        Ycal[i] = lOut[i]; 
//        E[i] = m_data[i] - Ycal[i];
//      }
//
//      delete [] lOut; 
//
//      if (sd.size() == static_cast<size_t>(this->nParams()))
//      {
//        SimpleJacobian J(nData,this->nParams());
//        try
//        {
//          this->functionDeriv(&J);
//        }
//        catch(...)
//        {
//          this->calNumericalDeriv(&J,&m_xValues[0],nData);
//        }
//        for(size_t i=0; i<nData; i++)
//        {
//          double err = 0.0;
//          for(size_t j=0;j< static_cast<size_t>(nParams());++j)
//          {
//            double d = J.get(i,j) * sd[j];
//            err += d*d;
//          }
//          Ecal[i] = sqrt(err);
//        }
//      }
//
//      return ws;
//}

/** Calculate numerical derivatives.
 * @param out :: Derivatives
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
//void IFunction1D::calNumericalDeriv(FunctionDomain1D& domain, Jacobian& jacobian)
//{
    //const double minDouble = std::numeric_limits<double>::min();
    //const double epsilon = std::numeric_limits<double>::epsilon();
    //double stepPercentage = 0.001; // step percentage
    //double step; // real step
    //double cutoff = 100.0*minDouble/stepPercentage;
    //size_t nParam = nParams();

    //// allocate memory if not already done
    //if (m_tmpFunctionOutputMinusStep.size() != domain.size())
    //{
    //  m_tmpFunctionOutputMinusStep.resize(domain.size());
    //  m_tmpFunctionOutputPlusStep.resize(domain.size());
    //}

    //functionMW(m_tmpFunctionOutputMinusStep.get(), xValues, nData);

    //for (size_t iP = 0; iP < nParam; iP++)
    //{
    //  if ( isActive(iP) )
    //  {
    //    const double& val = getParameter(iP);
    //    if (fabs(val) < cutoff)
    //    {
    //      step = epsilon;
    //    }
    //    else
    //    {
    //      step = val*stepPercentage;
    //    }

    //    //double paramMstep = val - step;
    //    //setParameter(iP, paramMstep);
    //    //function(m_tmpFunctionOutputMinusStep.get(), xValues, nData);

    //    double paramPstep = val + step;
    //    setParameter(iP, paramPstep);
    //    functionMW(m_tmpFunctionOutputPlusStep.get(), xValues, nData);

    //    step = paramPstep - val;
    //    setParameter(iP, val);

    //    for (size_t i = 0; i < nData; i++) {
    //     // out->set(i,iP, 
    //     //   (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/(2.0*step));
    //      out->set(i,iP, 
    //        (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/step);
    //    }
    //  }
    //}
//}

} // namespace API
} // namespace Mantid
