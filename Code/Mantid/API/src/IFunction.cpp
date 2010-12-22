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

  std::vector<double> IFunction::g_empty(0);

  /// Set the workspace
  /// @param wsIDString A string identifying the data to be fitted, e.g. workspace name and spectrum index separated by a comma
  void IFunction::setWorkspace(const std::string& wsIDString)
  {
  }

  /// Get the workspace
  boost::shared_ptr<const API::Workspace> IFunction::getWorkspace()const
  {
    return boost::shared_ptr<const API::Workspace>();
  }

  /// Returns the size of the fitted data (number of double values returned by the function)
  int IFunction::dataSize()const
  {
    return 0;
  }

  /// Returns a reference to the fitted data. These data are taken from the workspace set by setWorkspace() method.
  /// Must be true: getData().size() == dataSize()
  const std::vector<double>& IFunction::getData()const
  {
    return g_empty;
  }

  /// Function you want to fit to. 
  /// @param out The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  void IFunction::function(double* out)const
  {
  }

  /// Derivatives of function with respect to active parameters
  void IFunction::functionDeriv(Jacobian* out)
  {
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

} // namespace API
} // namespace Mantid
