#include "MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h"
#include "MantidKernel/EmptyValues.h"

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
   * @return : Either the algorithm property or an instrument parameter.
   */
  double getDblPropOrParam(const std::string pmProp, PropertyManager_sptr pm,
      const std::string instParam, MatrixWorkspace_sptr ws)
  {
    double param = EMPTY_DBL();
    if (pm->existsProperty(pmProp))
    {
      param = pm->getProperty(pmProp);
      if (EMPTY_DBL() == param)
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
    return param;
  }
}
