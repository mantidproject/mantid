#ifndef MANTID_CURVEFITTING_FORTRANDEFS_H_
#define MANTID_CURVEFITTING_FORTRANDEFS_H_

#include "MantidCurveFitting/ComplexMatrix.h"
#include "MantidCurveFitting/FortranMatrix.h"
#include "MantidCurveFitting/FortranVector.h"
#include "MantidCurveFitting/GSLMatrix.h"

namespace Mantid {
namespace CurveFitting {

typedef FortranMatrix<ComplexMatrix> ComplexFortranMatrix;
typedef FortranMatrix<GSLMatrix> DoubleFortranMatrix;
typedef FortranVector<ComplexVector> ComplexFortranVector;
typedef FortranVector<GSLVector> DoubleFortranVector;
typedef FortranVector<std::vector<int>> IntFortranVector;

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FORTRANDEFS_H_ */