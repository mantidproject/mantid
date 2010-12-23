//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Expression.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include <muParser.h>

#include "boost/lexical_cast.hpp"

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  using namespace Geometry;
  
  Kernel::Logger& IFunction::g_log = Kernel::Logger::get("IFunction");

  /// Set the workspace
  /// @param wsIDString A string identifying the data to be fitted. Format for IFunction:
  ///  "WorkspaceName,WorkspaceIndex=int,StartX=double,EndX=double"
  void IFunction::setWorkspace(const std::string& wsIDString)
  {
    try
    {
      Expression expr;
      expr.parse(wsIDString);
      if (expr.name() != "," || expr.size() < 2 ) // TODO: Expression needs pattern matching
      {
        throw std::invalid_argument("Syntax error in argument");
      }
      std::string wsName = expr[0].name();
      MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
      if (!ws)
      {
        throw std::invalid_argument("Workspace has wrong type (not a MatrixWorkspace)");
      }

      int index = -1;
      int xMin = -1;
      int xMax = -1;
      double startX,endX;
      for(int i = 1; i < expr.size(); ++i)
      {
        const Expression& e = expr[i];
        if (e.size() == 2 && e.name() == "=")
        {
          if (e[0].name() == "WorkspaceIndex")
          {
            index = boost::lexical_cast<int>(e[1].name());
          }
          else if (e[0].name() == "StartX")
          {
            startX = boost::lexical_cast<double>(e[1].name());
            xMin = 0;
          }
          else if (e[0].name() == "EndX")
          {
            endX = boost::lexical_cast<double>(e[1].name());
            xMax = 0;
          }
        }
      }

      if (index < 0)
      {
        g_log.warning("WorkspaceIndex not set, defaulting to 0");
      }
      else if (index >= ws->getNumberHistograms())
      {
        throw std::range_error("WorkspaceIndex outside range");
      }
      const MantidVec& x = ws->readX(index);
      const MantidVec& y = ws->readY(index);
      const MantidVec& e = ws->readE(index);
      int n = int(y.size());
      if (xMin < 0)
      {
        xMin = 0;
      }
      else
      {
        for(; xMin < n - 1; ++xMin)
        {
          if (x[xMin] > startX)
          {
            xMin--;
            break;
          }
        }
        if (xMin < 0) xMin = 0;
      }

      if (xMax < 0)
      {
        xMax = n - 1;
      }
      else
      {
        for(; xMax < n - 1; ++xMax)
        {
          if (x[xMax] > endX)
          {
            xMax--;
            break;
          }
        }
        if (xMax < 0) xMax = 0;
      }

      if (xMin > xMax)
      {
        int tmp = xMin;
        xMin = xMax;
        xMax = tmp;
      }
      m_dataSize = xMax - xMin;
      m_data = &y[xMin];
      m_xValues = &x[xMin];
      m_weights.resize(m_dataSize);

      for (int i = 0; i < m_dataSize; ++i)
      {
        if (e[xMin + i] <= 0.0)
          m_weights[i] = 1.0;
        else
          m_weights[i] = 1./e[xMin + i];
      }

      if (ws->hasMaskedBins(index))
      {
        const MatrixWorkspace::MaskList& mlist = ws->maskedBins(index);
        MatrixWorkspace::MaskList::const_iterator it = mlist.begin();
        for(;it!=mlist.end();it++)
        {
          m_weights[it->first - xMin] = 0.;
        }
      }

      setMatrixWorkspace(ws,index,xMin,xMax);
      
    }
    catch(std::exception& e)
    {
      g_log.error() << "IFunction::setWorkspace failed with error: " << e.what() << '\n';
      throw;
    }
  }

  /// Get the workspace
  boost::shared_ptr<const API::Workspace> IFunction::getWorkspace()const
  {
    return m_workspace;
  }

  /// Returns the size of the fitted data (number of double values returned by the function)
  int IFunction::dataSize()const
  {
    return m_dataSize;
  }

  /// Returns a pointer to the fitted data. These data are taken from the workspace set by setWorkspace() method.
  const double* IFunction::getData()const
  {
    return m_data;
  }

  const double* IFunction::getWeights()const
  {
    return &m_weights[0];
  }

  /// Function you want to fit to. 
  /// @param out The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  void IFunction::function(double* out)const
  {
    if (m_weights.empty()) return;
    function(out,m_xValues,m_dataSize);
    // Add penalty factor to function if any constraint is violated

    double penalty = 0.;
    for(int i=0;i<nParams();++i)
    {
      API::IConstraint* c = getConstraint(i);
      if (c)
      {
        penalty += c->check();
      }
    }

    // add penalty to first and last point and every 10th point in between
    if ( penalty != 0.0 )
    {
      out[0] += penalty;
      out[m_dataSize - 1] += penalty;

      for (int i = 9; i < m_dataSize - 1; i+=10)
      {
        out[i] += penalty;
      }
    }

  }

  /// Derivatives of function with respect to active parameters
  void IFunction::functionDeriv(Jacobian* out)
  {
    if (m_weights.empty()) return;
    functionDeriv(out,m_xValues,m_dataSize);

    if (m_dataSize <= 0) return;

    for(int i=0;i<nParams();++i)
    {  
      API::IConstraint* c = getConstraint(i);
      if (c)
      {
        double penalty = c->checkDeriv();
        out->addNumberToColumn(penalty, activeIndex(i));
      }
    }

    //std::cerr<<"-------------- Jacobian ---------------\n";
    //for(int i=0;i<nActive();i++)
    //  for(int j=0;j<nData;j++)
    //    std::cerr<<i<<' '<<j<<' '<<gsl_matrix_get(((JacobianImpl1*)out)->m_J,j,i)<<'\n';
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
  (void) out; (void) xValues; (void) nData; //Avoid compiler warning
  throw Kernel::Exception::NotImplementedError("No derivative IFunction provided");
}

/** Initialize the function providing it the workspace
 * @param workspace The workspace to set
 * @param wi The workspace index
 * @param xMin The lower bin index
 * @param xMax The upper bin index
 */
void IFunction::setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int wi,int xMin,int xMax)
{
  m_workspaceIndex = wi;
  m_xMinIndex = xMin;
  m_xMaxIndex = xMax;

  if (m_workspace.get() == workspace.get()) return;
  m_workspace = workspace;

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
      IInstrument_sptr inst = m_workspace->getInstrument();
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
              Kernel::Unit_sptr centreUnit;  // unit of value used in formula or to look up value in lookup table
              if ( fitParam.getFormula().compare("") == 0 )
                centreUnit = fitParam.getLookUpTable().getXUnit();  // from table
              else
                centreUnit =  Kernel::UnitFactory::Instance().create(fitParam.getFormulaUnit());  // from formula

              centreValue = convertValue(centreValue, centreUnit, m_workspace, wi);

              double paramValue = fitParam.getValue(centreValue);

              // this returned param value by say a formula or a look-up-table may have
              // a unit of its own. If this is specified, try the following
              if ( fitParam.getFormula().compare("") == 0 )
              {
                // so from look up table
                Kernel::Unit_sptr resultUnit = fitParam.getLookUpTable().getYUnit();  // from table
                paramValue /= convertValue(1.0, resultUnit, m_workspace, wi);
              }
              else
              {
                // so from formula 

                std::string resultUnitStr = fitParam.getResultUnit();

                std::vector<std::string> allUnitStr = Kernel::UnitFactory::Instance().getKeys();
                for (unsigned iUnit = 0; iUnit < allUnitStr.size(); iUnit++)
                {
                  size_t found = resultUnitStr.find(allUnitStr[iUnit]);
                  if ( found != std::string::npos )
                  {
                    size_t len = allUnitStr[iUnit].size();
                    std::stringstream readDouble;
                    Kernel::Unit_sptr unt = Kernel::UnitFactory::Instance().create(allUnitStr[iUnit]);
                    readDouble << 1.0 / 
                      convertValue(1.0, unt, m_workspace, wi);
                    resultUnitStr.replace(found, len, readDouble.str());
                  }
                }  // end for

                try
                {
                  mu::Parser p;
                  p.SetExpr(resultUnitStr);
                  paramValue *= p.Eval();
                }
                catch (mu::Parser::exception_type &e)
                {
                  g_log.error() << "Cannot convert formula unit to workspace unit"
                    << " Formula unit which cannot be passed is " << resultUnitStr 
                    << ". Muparser error message is: " << e.GetMsg() << std::endl;
                }
              } // end else
              

              setParameter(i, paramValue);
            }  // end of update parameter value

            // add tie if specified for this parameter in instrument definition file
            if ( fitParam.getTie().compare("") )
            {  
              std::ostringstream str;
              str << getParameter(i);
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

/** Convert a value from unit defined in workspace (ws) to outUnit
 *
 *  @param value   assumed to be in unit of workspace
 *  @param outUnit  unit to convert to
 *  @param ws      workspace
 *  @param wsIndex workspace index
 *  @return converted value
 */
double IFunction::convertValue(double value, Kernel::Unit_sptr& outUnit, 
                               boost::shared_ptr<const MatrixWorkspace> ws,
                               int wsIndex)
{
  double retVal = value;
  Kernel::Unit_sptr wsUnit = ws->getAxis(0)->unit();

  // if unit required by formula or look-up-table different from ws-unit then 
  if ( outUnit->unitID().compare(wsUnit->unitID()) != 0 )
  {
    // first check if it is possible to do a quick convertion convert
    double factor,power;
    if (wsUnit->quickConversion(*outUnit,factor,power) )
    {
      retVal = factor * std::pow(retVal,power);
    }
    else
    {
      double l1,l2,twoTheta;

      // Get l1, l2 and theta  (see also RemoveBins.calculateDetectorPosition())
      IInstrument_const_sptr instrument = ws->getInstrument();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      l1 = instrument->getSource()->getDistance(*sample);
      Geometry::IDetector_const_sptr det = ws->getDetector(wsIndex);
      if ( ! det->isMonitor() )
      {
        l2 = det->getDistance(*sample);
        twoTheta = ws->detectorTwoTheta(det);
      }
      else  // If this is a monitor then make l1+l2 = source-detector distance and twoTheta=0
      {
        l2 = det->getDistance(*(instrument->getSource()));
        l2 = l2 - l1;
        twoTheta = 0.0;
      }

      std::vector<double> endPoint;
      endPoint.push_back(retVal);
      std::vector<double> emptyVec;
      wsUnit->toTOF(endPoint,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
      outUnit->fromTOF(endPoint,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
      retVal = endPoint[0];
    }
  }  
  return retVal;
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

/// Called after setMatrixWorkspace if setWorkspace hadn't been called before
void IFunction::setUpNewStuff()
{
  m_dataSize = m_xMaxIndex - m_xMinIndex;
  m_data = &m_workspace->readY(m_workspaceIndex)[m_xMinIndex];
  m_xValues = &m_workspace->readX(m_workspaceIndex)[m_xMinIndex];
  m_weights.resize(m_dataSize);
  const MantidVec& e = m_workspace->readE(m_workspaceIndex);

  for (int i = 0; i < m_dataSize; ++i)
  {
    if (e[m_xMinIndex + i] <= 0.0)
      m_weights[i] = 1.0;
    else
      m_weights[i] = 1./e[m_xMinIndex + i];
  }

  if (m_workspace->hasMaskedBins(m_workspaceIndex))
  {
    const MatrixWorkspace::MaskList& mlist = m_workspace->maskedBins(m_workspaceIndex);
    MatrixWorkspace::MaskList::const_iterator it = mlist.begin();
    for(;it!=mlist.end();it++)
    {
      m_weights[it->first - m_xMinIndex] = 0.;
    }
  }

}

} // namespace API
} // namespace Mantid
