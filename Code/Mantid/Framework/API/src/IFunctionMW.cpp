//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/SpectraDetectorMap.h"
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
#include <muParser.h>

#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  using namespace Geometry;
  
  Kernel::Logger& IFunctionMW::g_log = Kernel::Logger::get("IFunctionMW");

  /** Set the workspace
    * @param ws :: A shared pointer to a workspace. Must be a MatrixWorkspace.
    * @param slicing :: A string identifying the data to be fitted. Format for IFunctionMW:
    *  "WorkspaceIndex=int,StartX=double,EndX=double". StartX and EndX are optional.
  */
  void IFunctionMW::setWorkspace(boost::shared_ptr<Workspace> ws,const std::string& slicing)
  {
    try
    {
      MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
      if (!mws)
      {
        throw std::invalid_argument("Workspace has a wrong type (not a MatrixWorkspace)");
      }

      int index = -1;
      int xMin = -1;
      int xMax = -1;
      double startX = 0;
      double endX = 0;
      int n = int(mws->blocksize()); // length of each Y vector

      Expression expr;
      expr.parse(slicing);
      if (expr.name() == ",")
      {
        for(int i = 0; i < expr.size(); ++i)
        {
          const Expression& e = expr[i];
          if (e.size() == 2 && e.name() == "=")
          {
            if (e[0].name() == "WorkspaceIndex")
            {
              index = boost::lexical_cast<int>(e[1].str());
            }
            else if (e[0].name() == "StartX")
            {
              startX = boost::lexical_cast<double>(e[1].str());
              xMin = 0;
            }
            else if (e[0].name() == "EndX")
            {
              endX = boost::lexical_cast<double>(e[1].str());
              xMax = 0;
            }
          }
        }
      }
      else if (expr.size() == 2 && expr.name() == "=" && expr[0].name() == "WorkspaceIndex")
      {
        index = boost::lexical_cast<int>(expr[1].name());
      }

      if (index < 0)
      {
        g_log.warning("WorkspaceIndex not set, defaulting to 0");
      }
      else if (index >= mws->getNumberHistograms())
      {
        throw std::range_error("WorkspaceIndex outside range");
      }
      const MantidVec& x = mws->readX(index);
      const MantidVec& y = mws->readY(index);
      const MantidVec& e = mws->readE(index);

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
      m_dataSize = xMax - xMin + 1;
      m_data = &y[xMin];
      m_xValues.reset(new double[m_dataSize]);
      m_weights.reset(new double[m_dataSize]);
      bool isHist = x.size() > y.size();

      for (int i = 0; i < m_dataSize; ++i)
      {
        if (isHist)
        {
          m_xValues[i] = 0.5*(x[xMin + i] + x[xMin + i + 1]);
        }
        else
        {
          m_xValues[i] = x[xMin + i];
        }
        if (e[xMin + i] <= 0.0)
          m_weights[i] = 1.0;
        else
          m_weights[i] = 1./e[xMin + i];
      }

      if (mws->hasMaskedBins(index))
      {
        const MatrixWorkspace::MaskList& mlist = mws->maskedBins(index);
        MatrixWorkspace::MaskList::const_iterator it = mlist.begin();
        for(;it!=mlist.end();it++)
        {
          m_weights[it->first - xMin] = 0.;
        }
      }

      setMatrixWorkspace(mws,index,xMin,xMax);
      
    }
    catch(std::exception& e)
    {
      g_log.error() << "IFunctionMW::setWorkspace failed with error: " << e.what() << '\n';
      throw;
    }
  }

  /// Get the workspace
  boost::shared_ptr<const API::Workspace> IFunctionMW::getWorkspace()const
  {
    return m_workspace;
  }

  /// Returns the size of the fitted data (number of double values returned by the function)
  int IFunctionMW::dataSize()const
  {
    return m_dataSize;
  }

  /// Returns a pointer to the fitted data. These data are taken from the workspace set by setWorkspace() method.
  const double* IFunctionMW::getData()const
  {
    return m_data;
  }

  const double* IFunctionMW::getWeights()const
  {
    return &m_weights[0];
  }

  /// Function you want to fit to. 
  /// @param out :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  void IFunctionMW::function(double* out)const
  {
    if (m_dataSize == 0) return;
    function(out,m_xValues.get(),m_dataSize);

  }

  /// Derivatives of function with respect to active parameters
  void IFunctionMW::functionDeriv(Jacobian* out)
  {
    if (out == NULL) 
    {
      functionDeriv(out,m_xValues.get(),0);
      return;
    }
    if (m_dataSize == 0) return;
    functionDeriv(out,m_xValues.get(),m_dataSize);

    if (m_dataSize <= 0) return;

    //std::cerr<<"-------------- Jacobian ---------------\n";
    //for(int i=0;i<nActive();i++)
    //  for(int j=0;j<nData;j++)
    //    std::cerr<<i<<' '<<j<<' '<<gsl_matrix_get(((JacobianImpl1*)out)->m_J,j,i)<<'\n';
  }


/** Base class implementation of derivative IFunctionMW throws error. This is to check if such a function is provided
    by derivative class. In the derived classes this method must return the derivatives of the resuduals function
    (defined in void Fit1D::function(const double*, double*, const double*, const double*, const double*, const int&))
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used.
 * @param out :: Derivatives
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
void IFunctionMW::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
  (void) out; (void) xValues; (void) nData; //Avoid compiler warning
  throw Kernel::Exception::NotImplementedError("No derivative IFunctionMW provided");
}

/** Initialize the function providing it the workspace
 * @param workspace :: The workspace to set
 * @param wi :: The workspace index
 * @param xMin :: The lower bin index
 * @param xMax :: The upper bin index
 */
void IFunctionMW::setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int wi,int xMin,int xMax)
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
 *  @param value ::   assumed to be in unit of workspace
 *  @param outUnit ::  unit to convert to
 *  @param ws ::      workspace
 *  @param wsIndex :: workspace index
 *  @return converted value
 */
double IFunctionMW::convertValue(double value, Kernel::Unit_sptr& outUnit, 
                               boost::shared_ptr<const MatrixWorkspace> ws,
                               int wsIndex)const
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

/** Convert values from unit defined in workspace (ws) to outUnit
 *
 *  @param values ::   As input: assumed to be in unit of workspace. 
 *                  As output: in unit of outUnit
 *  @param outUnit ::  unit to convert to
 *  @param ws ::      workspace
 *  @param wsIndex :: workspace index
 */
void IFunctionMW::convertValue(std::vector<double>& values, Kernel::Unit_sptr& outUnit, 
                               boost::shared_ptr<const MatrixWorkspace> ws,
                               int wsIndex) const
{
  Kernel::Unit_sptr wsUnit = ws->getAxis(0)->unit();

  // if unit required by formula or look-up-table different from ws-unit then 
  if ( outUnit->unitID().compare(wsUnit->unitID()) != 0 )
  {
    // first check if it is possible to do a quick convertion convert
    double factor,power;
    if (wsUnit->quickConversion(*outUnit,factor,power) )
    {
      for (unsigned int i = 0; i < values.size(); i++)
        values[i] = factor * std::pow(values[i],power);
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

      std::vector<double> emptyVec;
      wsUnit->toTOF(values,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
      outUnit->fromTOF(values,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
    }
  }  
}

/**
 * Calculate the Jacobian with respect to parameters actually declared in the IFunctionMW
 * @param out :: The output Jacobian
 * @param xValues :: The x-values
 * @param nData :: The number of data points (and x-values).
 */
void IFunctionMW::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
  this->functionDeriv(out,xValues,nData);
}

/// Called after setMatrixWorkspace if setWorkspace hadn't been called before
void IFunctionMW::setUpNewStuff(boost::shared_array<double> xs,boost::shared_array<double> weights)
{
  m_dataSize = m_xMaxIndex - m_xMinIndex;
  m_data = &m_workspace->readY(m_workspaceIndex)[m_xMinIndex];
  //m_xValues = &m_workspace->readX(m_workspaceIndex)[m_xMinIndex];
  if (weights && xs)
  {
    m_xValues = xs;
    m_weights = weights;
  }
  else
  {
    m_xValues.reset(new double[m_dataSize]);
    m_weights.reset(new double[m_dataSize]);
    const MantidVec& x = m_workspace->readX(m_workspaceIndex);
    const MantidVec& e = m_workspace->readE(m_workspaceIndex);
    bool isHist = m_workspace->isHistogramData();

    for (int i = 0; i < m_dataSize; ++i)
    {
      if (isHist)
      {
        m_xValues[i] = 0.5*(x[m_xMinIndex + i] + x[m_xMinIndex + i + 1]);
      }
      else
      {
        m_xValues[i] = x[m_xMinIndex + i];
      }
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

}

boost::shared_ptr<API::MatrixWorkspace> IFunctionMW::createCalculatedWorkspace(boost::shared_ptr<const API::MatrixWorkspace> inWS,int wi)const
{
      const MantidVec& inputX = inWS->readX(wi);
      const MantidVec& inputY = inWS->readY(wi);
      int nData = dataSize();

      int histN = inWS->isHistogramData() ? 1 : 0;
      API::MatrixWorkspace_sptr ws =
        Mantid::API::WorkspaceFactory::Instance().create(
            "Workspace2D",
            3,
            nData + histN,
            nData);
      ws->setTitle("");
      ws->setYUnitLabel(inWS->YUnitLabel());
      ws->setYUnit(inWS->YUnit());
      ws->getAxis(0)->unit() = inWS->getAxis(0)->unit();
      API::TextAxis* tAxis = new API::TextAxis(3);
      tAxis->setLabel(0,"Data");
      tAxis->setLabel(1,"Calc");
      tAxis->setLabel(2,"Diff");
      ws->replaceAxis(1,tAxis);

      assert(m_xMaxIndex-m_xMinIndex+1 == nData);

      for(int i=0;i<3;i++)
      {
        ws->dataX(i).assign(inputX.begin()+m_xMinIndex,inputX.begin()+m_xMaxIndex+1+histN);
      }

      ws->dataY(0).assign(inputY.begin()+m_xMinIndex,inputY.begin()+m_xMaxIndex+1);

      MantidVec& Ycal = ws->dataY(1);
      MantidVec& E = ws->dataY(2);

      double* lOut = new double[nData];  // to capture output from call to function()
      function( lOut );

      for(int i=0; i<nData; i++)
      {
        Ycal[i] = lOut[i]; 
        E[i] = m_data[i] - Ycal[i];
      }

      delete [] lOut; 

      return ws;
}

/** Calculate numerical derivatives.
 * @param out :: Derivatives
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
void IFunctionMW::calNumericalDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    double stepPercentage = 0.001; // step percentage
    double step; // real step
    double minDouble = std::numeric_limits<double>::min();
    double cutoff = 100.0*minDouble/stepPercentage;
    int nParam = nParams();

    // allocate memory if not already done
    if (!m_tmpFunctionOutputMinusStep && nData>0)
    {
      m_tmpFunctionOutputMinusStep.reset(new double[nData]);
      m_tmpFunctionOutputPlusStep.reset(new double[nData]);
    }

    function(m_tmpFunctionOutputMinusStep.get(), xValues, nData);

    for (int iP = 0; iP < nParam; iP++)
    {
      if ( isActive(iP) )
      {
        const double& val = getParameter(iP);
        if (val < cutoff)
        {
          step = cutoff;
        }
        else
        {
          step = val*stepPercentage;
        }

        //double paramMstep = val - step;
        //setParameter(iP, paramMstep);
        //function(m_tmpFunctionOutputMinusStep.get(), xValues, nData);

        double paramPstep = val + step;
        setParameter(iP, paramPstep);
        function(m_tmpFunctionOutputPlusStep.get(), xValues, nData);

        step = paramPstep - val;
        setParameter(iP, val);

        for (int i = 0; i < nData; i++) {
         // out->set(i,iP, 
         //   (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/(2.0*step));
          out->set(i,iP, 
            (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/step);
        }
      }
    }
}


} // namespace API
} // namespace Mantid
