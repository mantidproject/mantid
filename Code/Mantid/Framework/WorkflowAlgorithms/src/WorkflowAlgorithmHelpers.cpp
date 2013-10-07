#include "MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

namespace WorkflowAlgorithmHelpers
{
  /**
   * This function tries to get a double value from a particular algorithm
   * property. If that property is the default (EMPTY_DBL()), then the function
   * tries to look up the parameter on the instrument in the given workspace
   * for a given parameter name. If found, that value is returned. If not found,
   * the default value (EMPTY_DBL()) is returned.
   *
   * @param pmProp : The name of the algorithm property to retrieve
   * @param pm : The property manager pointer to retrieve a property value from
   * @param instParam : The name of the instrument parameter to fetch from the workspace
   * @param ws : A workspace that should house the alternate parameter
   * @param overrideValue : A provided override value to hand back if nothing is found
   * @return : Either the algorithm property or an instrument parameter.
   */
  double getDblPropOrParam(const std::string &pmProp, Kernel::PropertyManager_sptr &pm,
      const std::string &instParam, API::MatrixWorkspace_sptr &ws,
      const double overrideValue)
  {
    double defaultValue = EMPTY_DBL();
    double param = defaultValue;
    if (pm->existsProperty(pmProp))
    {
      param = pm->getProperty(pmProp);
      if (defaultValue == param)
      {
        std::vector<double> params = ws->getInstrument()->getNumberParameter(instParam);
        if (!params.empty())
        {
          param = params[0];
        }
      }
    }
    else
    {
      std::vector<double> params = ws->getInstrument()->getNumberParameter(instParam);
      if (!params.empty())
      {
        param = params[0];
      }
    }
    if(defaultValue != overrideValue)
    {
      param = overrideValue;
    }
    return param;
  }

  /**
   * This function tries to get a boolean value from a particular algorithm
   * property. If that property does not exist, then the function tries to look
   * up the parameter on the instrument in the given workspace for a given
   * parameter name. If found, that value is returned. If not found,
   * the default value (false) is returned. If the property exists, just use
   * the value provided.
   *
   * @param pmProp : The name of the algorithm property to retrieve
   * @param pm : The property manager pointer to retrieve a property value from
   * @param instParam : The name of the instrument parameter to fetch from the workspace
   * @param ws : A workspace that should house the alternate parameter
   * @param overrideValue : A provided override value to hand back if nothing is found
   * @return : Either the algorithm property or an instrument parameter.
   */
  bool getBoolPropOrParam(const std::string &pmProp, Kernel::PropertyManager_sptr &pm,
      const std::string &instParam, API::MatrixWorkspace_sptr &ws,
      const bool overrideValue)
  {
    bool defaultValue = false;
    bool param = defaultValue;
    if (pm->existsProperty(pmProp))
    {
      param = pm->getProperty(pmProp);
    }
    else
    {
        try
        {
           std::vector<bool> params = ws->getInstrument()->getBoolParameter(instParam);
           if (!params.empty())
                param = params[0];
           else
               param = false;
        }
        catch(std::runtime_error &) // Old style bool parameter expressed as double
        {
           std::vector<double> params = ws->getInstrument()->getNumberParameter(instParam);
           if (!params.empty())
           {
                param = (params[0] != 0.0);
           }
        }


    }
    if(defaultValue != overrideValue)
    {
      param = overrideValue;
    }
    return param;
  }
}
