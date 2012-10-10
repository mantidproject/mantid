#ifndef WORKFLOWALGORITHMHELPERS_H_
#define WORKFLOWALGORITHMHELPERS_H_

#include <string>
#include "MantidAPI/Algorithm.h"

namespace WorkflowAlgorithmHelpers
{
  /// Function to get algorithm property or instrument parameter value
  double getPropOrParam(Mantid::API::Algorithm_sptr alg,
      const std::string algProp, Mantid::API::MatrixWorkspace_sptr ws,
      const std::string instParam);
}

#endif /* WORKFLOWALGORITHMHELPERS_H_ */
