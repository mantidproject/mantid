// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/PropertyManager.h"
#include <string>

namespace WorkflowAlgorithmHelpers {
/// Function to get double property or instrument parameter value
double getDblPropOrParam(const std::string &pmProp, Mantid::Kernel::PropertyManager_sptr &pm,
                         const std::string &instParam, Mantid::API::MatrixWorkspace_sptr &ws,
                         const double overrideValue = Mantid::EMPTY_DBL());

/// Function to get int property or instrument parameter value
int getIntPropOrParam(const std::string &pmProp, Mantid::Kernel::PropertyManager_sptr &pm, const std::string &instParam,
                      Mantid::API::MatrixWorkspace_sptr &ws, const int overrideValue = Mantid::EMPTY_INT());

/// Function to get boolean property or instrument parameter value
bool getBoolPropOrParam(const std::string &pmProp, Mantid::Kernel::PropertyManager_sptr &pm,
                        const std::string &instParam, Mantid::API::MatrixWorkspace_sptr &ws,
                        const bool overrideValue = false);
} // namespace WorkflowAlgorithmHelpers
