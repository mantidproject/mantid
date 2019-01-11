// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_FORTRANDEFS_H_
#define MANTID_CURVEFITTING_FORTRANDEFS_H_

#include "MantidCurveFitting/ComplexMatrix.h"
#include "MantidCurveFitting/FortranMatrix.h"
#include "MantidCurveFitting/FortranVector.h"
#include "MantidCurveFitting/GSLMatrix.h"

namespace Mantid {
namespace CurveFitting {

using ComplexFortranMatrix = FortranMatrix<ComplexMatrix>;
using DoubleFortranMatrix = FortranMatrix<GSLMatrix>;
using ComplexFortranVector = FortranVector<ComplexVector>;
using DoubleFortranVector = FortranVector<GSLVector>;
using IntFortranVector = FortranVector<std::vector<int>>;

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FORTRANDEFS_H_ */