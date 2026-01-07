// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/EigenMatrix.h"

namespace Mantid::CurveFitting {

Eigen::MatrixXd covar_from_jacobian(const map_type &J, double epsrel);

} // namespace Mantid::CurveFitting
