#ifndef WORKFLOWALGORITHMHELPERS_H_
#define WORKFLOWALGORITHMHELPERS_H_

#include <string>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/PropertyManager.h"

namespace WorkflowAlgorithmHelpers {
/// Function to get double property or instrument parameter value
double getDblPropOrParam(const std::string &pmProp,
                         Mantid::Kernel::PropertyManager_sptr &pm,
                         const std::string &instParam,
                         Mantid::API::MatrixWorkspace_sptr &ws,
                         const double overrideValue = Mantid::EMPTY_DBL());

/// Function to get int property or instrument parameter value
int getIntPropOrParam(const std::string &pmProp,
                      Mantid::Kernel::PropertyManager_sptr &pm,
                      const std::string &instParam,
                      Mantid::API::MatrixWorkspace_sptr &ws,
                      const int overrideValue = Mantid::EMPTY_INT());

/// Function to get boolean property or instrument parameter value
bool getBoolPropOrParam(const std::string &pmProp,
                        Mantid::Kernel::PropertyManager_sptr &pm,
                        const std::string &instParam,
                        Mantid::API::MatrixWorkspace_sptr &ws,
                        const bool overrideValue = false);
}

#endif /* WORKFLOWALGORITHMHELPERS_H_ */
