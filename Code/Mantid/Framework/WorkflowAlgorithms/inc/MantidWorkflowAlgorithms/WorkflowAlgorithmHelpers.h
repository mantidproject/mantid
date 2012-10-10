#ifndef WORKFLOWALGORITHMHELPERS_H_
#define WORKFLOWALGORITHMHELPERS_H_

#include <string>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/PropertyManager.h"

namespace WorkflowAlgorithmHelpers
{
  using namespace Mantid;

  /// Function to get property or instrument parameter value
  double getDblPropOrParam(const std::string algProp,
      Kernel::PropertyManager_sptr pm, const std::string instParam,
      API::MatrixWorkspace_sptr ws);
}

#endif /* WORKFLOWALGORITHMHELPERS_H_ */
