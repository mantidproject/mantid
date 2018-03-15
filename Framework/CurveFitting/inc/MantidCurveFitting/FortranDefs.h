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