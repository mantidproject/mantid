// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/EigenComplexMatrix.h"
#include "MantidCurveFitting/EigenFortranMatrix.h"
#include "MantidCurveFitting/EigenFortranVector.h"
#include "MantidCurveFitting/EigenMatrix.h"

namespace Mantid {
namespace CurveFitting {

using ComplexFortranMatrix = FortranMatrix<ComplexMatrix>;
using DoubleFortranMatrix = FortranMatrix<EigenMatrix>;
using ComplexFortranVector = FortranVector<ComplexVector>;
using DoubleFortranVector = FortranVector<EigenVector>;
using IntFortranVector = FortranVector<std::vector<int>>;

} // namespace CurveFitting
} // namespace Mantid
