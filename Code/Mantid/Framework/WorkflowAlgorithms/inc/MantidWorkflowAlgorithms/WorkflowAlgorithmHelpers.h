#ifndef WORKFLOWALGORITHMHELPERS_H_
#define WORKFLOWALGORITHMHELPERS_H_

#include <string>
#include "MantidKernel/PropertyManager.h"

namespace WorkflowAlgorithmHelpers
{
  using namespace Mantid;

  /// Function to get property or instrument parameter value
  double getDblPropOrParam(Kernel::PropertyManager_sptr pm,
      const std::string algProp, API::MatrixWorkspace_sptr ws,
      const std::string instParam);
}

#endif /* WORKFLOWALGORITHMHELPERS_H_ */
