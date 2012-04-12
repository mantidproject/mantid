//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/FuncMinimizerFactory.h"
#include "MantidCurveFitting/IFuncMinimizer.h"
#include "MantidCurveFitting/CostFuncFitting.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunctionMW.h"

#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_errno.h>
#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

namespace
{
  /**
   * A simple implementation of Jacobian.
   */
  class SimpleJacobian: public API::Jacobian
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

  bool greaterIsLess(double x1, double x2)
  {
    return x1 > x2;
  }
}

  using namespace Kernel;
  using API::WorkspaceProperty;
  using API::Workspace;
  using API::Axis;
  using API::MatrixWorkspace;
  using API::Algorithm;
  using API::Progress;
  using API::Jacobian;

  /**
   * Declare properties that specify the dataset within the workspace to fit to.
   * @param domainIndex :: Index of created domain in a composite domain or 0 in single domain case
   */
  void FitMW::declareDatasetProperties(const std::string& suffix,bool addProp)
  {
    m_workspaceIndexPropertyName = "WorkspaceIndex" + suffix;
    m_startXPropertyName = "StartX" + suffix;
    m_endXPropertyName = "EndX" + suffix;

    if (addProp && !m_fit->existsProperty(m_workspaceIndexPropertyName))
    {
      auto mustBePositive = boost::shared_ptr< BoundedValidator<int> >( new BoundedValidator<int>() );
      mustBePositive->setLower(0);
      declareProperty(new PropertyWithValue<int>(m_workspaceIndexPropertyName,0, mustBePositive),
                      "The Workspace Index to fit in the input workspace");
      declareProperty(new PropertyWithValue<double>(m_startXPropertyName, EMPTY_DBL()),
        "A value of x in, or on the low x boundary of, the first bin to include in\n"
        "the fit (default lowest value of x)" );
      declareProperty(new PropertyWithValue<double>(m_endXPropertyName, EMPTY_DBL()),
        "A value in, or on the high x boundary of, the last bin the fitting range\n"
        "(default the highest value of x)" );
    }
  }

  /// Create a domain from the input workspace
  void FitMW::createDomain(
    const std::vector<std::string>& workspacePropetyNames,
    boost::shared_ptr<API::FunctionDomain>& domain, 
    boost::shared_ptr<API::IFunctionValues>& ivalues, size_t i0)
  {
    if (workspacePropetyNames.empty())
    {
      throw std::runtime_error("Cannot create FunctionDomain1DVector: no workspace given");
    }
    m_workspacePropertyName = workspacePropetyNames[0];
    // get the function
    m_function = m_fit->getProperty("Function");
    // get the workspace 
    API::Workspace_sptr ws = m_fit->getProperty(m_workspacePropertyName);
    m_matrixWorkspace = boost::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
    if (!m_matrixWorkspace)
    {
      throw std::invalid_argument("InputWorkspace must be a MatrixWorkspace.");
    }
    //m_function->setWorkspace(ws);
    int index = m_fit->getProperty(m_workspaceIndexPropertyName);
    m_workspaceIndex = static_cast<size_t>(index);

    const Mantid::MantidVec& X = m_matrixWorkspace->readX(m_workspaceIndex);
    double startX = m_fit->getProperty(m_startXPropertyName);
    double endX = m_fit->getProperty(m_endXPropertyName);

    if (X.empty())
    {
      throw std::runtime_error("Workspace contains no data.");
    }

    // find the fitting interval: from -> to
    Mantid::MantidVec::const_iterator from;
    Mantid::MantidVec::const_iterator to;

    bool isXAscending = X.front() < X.back();

    if (isXAscending)
    {
      if (startX == EMPTY_DBL() && endX == EMPTY_DBL())
      {
        startX = X.front();
        from = X.begin();
        endX = X.back();
        to = X.end();
      }
      else if (startX == EMPTY_DBL() || endX == EMPTY_DBL())
      {
        throw std::invalid_argument("Both StartX and EndX must be given to set fitting interval.");
      }
      else
      {
        if (startX > endX)
        {
          std::swap(startX,endX);
        }
        from = std::lower_bound(X.begin(),X.end(),startX);
        to = std::upper_bound(from,X.end(),endX);
      }
    }
    else // x is descending
    {
      if (startX == EMPTY_DBL() && endX == EMPTY_DBL())
      {
        startX = X.front();
        from = X.begin();
        endX = X.back();
        to = X.end();
      }
      else if (startX == EMPTY_DBL() || endX == EMPTY_DBL())
      {
        throw std::invalid_argument("Both StartX and EndX must be given to set fitting interval.");
      }
      else
      {
        if (startX < endX)
        {
          std::swap(startX,endX);
        }
        //from = std::lower_bound(X.begin(),X.end(),startX,([](double x1,double x2)->bool{return x1 > x2;}));
        //to = std::upper_bound(from,X.end(),endX,([](double x1,double x2)->bool{return x1 > x2;}));
        from = std::lower_bound(X.begin(),X.end(),startX,greaterIsLess);
        to = std::upper_bound(from,X.end(),endX,greaterIsLess);
      }
    }


    API::IFunctionMW* funMW = dynamic_cast<API::IFunctionMW*>(m_function.get());
    if (funMW)
    {
      funMW->setMatrixWorkspace(m_matrixWorkspace,m_workspaceIndex,startX,endX);
    }

    // set function domain
    if (m_matrixWorkspace->isHistogramData())
    {
      if ( X.end() == to ) to = X.end() - 1;
      std::vector<double> x( static_cast<size_t>(to - from) );
      Mantid::MantidVec::const_iterator it = from;
      for(size_t i = 0; it != to; ++it,++i)
      {
        x[i] = (*it + *(it+1)) / 2;
      }
      domain.reset(new API::FunctionDomain1DVector(x));
      x.clear();
    }
    else
    {
      domain.reset(new API::FunctionDomain1DVector(from,to));
    }

    auto values = ivalues ? dynamic_cast<API::FunctionValues*>(ivalues.get()) : new API::FunctionValues(*domain);
    if (!ivalues)
    {
      ivalues.reset(values);
    }
    else
    {
      values->expand(i0 + domain->size());
    }

    // set the data to fit to
    m_startIndex = static_cast<size_t>( from - X.begin() );
    size_t n = domain->size();
    size_t ito = m_startIndex + n;
    const Mantid::MantidVec& Y = m_matrixWorkspace->readY( m_workspaceIndex );
    const Mantid::MantidVec& E = m_matrixWorkspace->readE( m_workspaceIndex );
    if (ito > Y.size())
    {
      throw std::runtime_error("FitMW: Inconsistent MatrixWorkspace");
    }
    bool foundZeroOrNegativeError = false;
    for(size_t i = m_startIndex; i < ito; ++i)
    {
      size_t j = i - m_startIndex + i0;
      values->setFitData( j, Y[i] );
      double error = E[i];
      if (error <= 0)
      {
        error = 1.0;
        foundZeroOrNegativeError = true;
      }
      values->setFitWeight( j, 1.0 / error );
    }

    if (foundZeroOrNegativeError)
    {
      log().warning() << "Zero or negative errors are replaced with 1.0\n";
    }

  }

  /**
   * Create an output workspace with the calculated values.
   * @param inWS :: The input workspace to the algorithm
   * @param wi :: The input workspace index.
   * @param startIndex :: The starting index in the X array where the fit data start
   * @param domain :: The domain
   * @param values :: The values
   */
  void FitMW::createOutputWorkspace(
        const std::string& baseName,
        boost::shared_ptr<API::FunctionDomain> domain,
        boost::shared_ptr<API::IFunctionValues> ivalues
    )
  {
    auto values = boost::dynamic_pointer_cast<API::FunctionValues>(ivalues);
    if (!values)
    {
      throw std::invalid_argument("Unsupported Function Values found in FitMW");
    }

    // calculate the values
    m_function->function(*domain,*values);
    const MantidVec& inputX = m_matrixWorkspace->readX(m_workspaceIndex);
    const MantidVec& inputY = m_matrixWorkspace->readY(m_workspaceIndex);
    const MantidVec& inputE = m_matrixWorkspace->readE(m_workspaceIndex);
    size_t nData = ivalues->size();

      size_t histN = m_matrixWorkspace->isHistogramData() ? 1 : 0;
      API::MatrixWorkspace_sptr ws =
        Mantid::API::WorkspaceFactory::Instance().create(
            "Workspace2D",
            3,
            nData + histN,
            nData);
      ws->setTitle("");
      ws->setYUnitLabel(m_matrixWorkspace->YUnitLabel());
      ws->setYUnit(m_matrixWorkspace->YUnit());
      ws->getAxis(0)->unit() = m_matrixWorkspace->getAxis(0)->unit();
      API::TextAxis* tAxis = new API::TextAxis(3);
      tAxis->setLabel(0,"Data");
      tAxis->setLabel(1,"Calc");
      tAxis->setLabel(2,"Diff");
      ws->replaceAxis(1,tAxis);

      for(size_t i=0;i<3;i++)
      {
        ws->dataX(i).assign( inputX.begin() + m_startIndex, inputX.begin() + m_startIndex + nData + histN );
      }

      ws->dataY(0).assign( inputY.begin() + m_startIndex, inputY.begin() + m_startIndex + nData);
      ws->dataE(0).assign( inputE.begin() + m_startIndex, inputE.begin() + m_startIndex + nData );

      MantidVec& Ycal = ws->dataY(1);
      MantidVec& Ecal = ws->dataE(1);
      MantidVec& Diff = ws->dataY(2);

      for(size_t i = 0; i < nData; ++i)
      {
        Ycal[i] = values->getCalculated(i);
        Diff[i] = values->getFitData(i) - Ycal[i];
      }

      SimpleJacobian J(nData,m_function->nParams());
      try
      {
        m_function->functionDeriv(*domain,J);
      }
      catch(...)
      {
        m_function->calNumericalDeriv(*domain,J);
      }
      for(size_t i=0; i<nData; i++)
      {
        double err = 0.0;
        for(size_t j=0;j< m_function->nParams();++j)
        {
          double d = J.get(i,j) * m_function->getError(j);
          err += d*d;
        }
        Ecal[i] = sqrt(err);
      }

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output Workspace holding resulting simulated spectrum");
      m_fit->setPropertyValue("OutputWorkspace",baseName+"Workspace");
      m_fit->setProperty("OutputWorkspace",ws);

  }


} // namespace Algorithm
} // namespace Mantid
