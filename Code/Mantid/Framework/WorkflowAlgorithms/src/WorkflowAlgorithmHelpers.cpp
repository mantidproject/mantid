#include "MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidAPI/MatrixWorkspace.h"

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
   * @param pm : The property manager pointer to retrieve a property value from
   * @param algProp : The name of the algorithm property to retrieve
   * @param ws : A workspace that should house the alternate parameter
   * @param instParam : The name of the instrument parameter to fetch from the workspace
   * @return : Either the algorithm property or an instrument parameter.
   */
  double getDblPropOrParam(PropertyManager_sptr pm, const std::string pmProp,
      MatrixWorkspace_sptr ws, const std::string instParam)
  {
    double param = EMPTY_DBL();
    if (pm->existsProperty(pmProp))
    {
      param = pm->getProperty(pmProp);
      if (EMPTY_DBL() == param)
      {
        std::vector<double> params = ws->getInstrument()->getNumberParameter(instParam);
        if (params.empty())
        {
          param = EMPTY_DBL();
        }
        else
        {
          param = params[0];
        }
      }
    }
    return param;
  }
}
