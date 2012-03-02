//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/ParameterTie.h"
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
  
  Kernel::Logger& IFunctionMW::g_log = Kernel::Logger::get("IFunctionMW");

/** Initialize the function providing it the workspace
 * @param workspace :: The workspace to set
 * @param wi :: The workspace index
 * @param startX :: The lower bin index
 * @param endX :: The upper bin index
 */
void IFunctionMW::setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,size_t wi,double startX, double endX)
{
  m_workspace = workspace;
  m_workspaceIndex = wi;

  if (!workspace) return; // unset the workspace

  try
  {

    // check if parameter are specified in instrument definition file

    const Geometry::ParameterMap& paramMap = workspace->instrumentParameters();


    Geometry::IDetector_const_sptr det;
    size_t numDetectors = workspace->getSpectrum(wi)->getDetectorIDs().size() ;
    if (numDetectors > 1)
    {
      // If several detectors are on this workspace index, just use the ID of the first detector
      // Note JZ oct 2011 - I'm not sure why the code uses the first detector and not the group. Ask Roman.
      Instrument_const_sptr inst = workspace->getInstrument();
      det = inst->getDetector( *workspace->getSpectrum(wi)->getDetectorIDs().begin() );
    }
    else
      // Get the detector (single) at this workspace index
      det = workspace->getDetector(wi);;

    for (size_t i = 0; i < nParams(); i++)
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
              {
                if ( !fitParam.getFormulaUnit().empty() )
                {
                  try
                  {
                    centreUnit = Kernel::UnitFactory::Instance().create(fitParam.getFormulaUnit());  // from formula
                  }
                  catch (...)
                  {
                    g_log.warning() << fitParam.getFormulaUnit() << " Is not an recognised formula unit for parameter " 
                                    << fitParam.getName() << "\n";
                  }
                }
              }

              // if unit specified convert centre value to unit required by formula or look-up-table
              if (centreUnit)
                centreValue = convertValue(centreValue, centreUnit, workspace, wi);

              double paramValue = fitParam.getValue(centreValue);

              // this returned param value by say a formula or a look-up-table may have
              // a unit of its own. If this is specified, try the following
              if ( fitParam.getFormula().compare("") == 0 )
              {
                // so from look up table
                Kernel::Unit_sptr resultUnit = fitParam.getLookUpTable().getYUnit();  // from table
                paramValue /= convertValue(1.0, resultUnit, workspace, wi);
              }
              else
              {
                // so from formula 

                std::string resultUnitStr = fitParam.getResultUnit();

                if ( !resultUnitStr.empty() )
                {
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
                        convertValue(1.0, unt, workspace, wi);
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
                } // end if
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

/**
 * Get a shared pointer to the saved matrix workspace.
 */
boost::shared_ptr<const API::MatrixWorkspace> IFunctionMW::getMatrixWorkspace() const
{
  return m_workspace.lock();
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
                               size_t wsIndex)const
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
      Instrument_const_sptr instrument = ws->getInstrument();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if (sample == NULL)
      {
        g_log.error() << "No sample defined instrument. Cannot convert units for function\n"
                      << "Ignore convertion."; 
        return value; 
      }
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
                               size_t wsIndex) const
{
  Kernel::Unit_sptr wsUnit = ws->getAxis(0)->unit();

  // if unit required by formula or look-up-table different from ws-unit then 
  if ( outUnit->unitID().compare(wsUnit->unitID()) != 0 )
  {
    // first check if it is possible to do a quick convertion convert
    double factor,power;
    if (wsUnit->quickConversion(*outUnit,factor,power) )
    {
      for (size_t i = 0; i < values.size(); i++)
        values[i] = factor * std::pow(values[i],power);
    }
    else
    {
      double l1,l2,twoTheta;

      // Get l1, l2 and theta  (see also RemoveBins.calculateDetectorPosition())
      Instrument_const_sptr instrument = ws->getInstrument();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if (sample == NULL)
      {
        g_log.error() << "No sample defined instrument. Cannot convert units for function\n"
                      << "Ignore convertion."; 
        return; 
      }
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

} // namespace API
} // namespace Mantid
