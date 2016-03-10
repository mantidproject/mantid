#ifndef MANTID_CURVEFITTING_CRYSTALELECTRICFIELD_H_
#define MANTID_CURVEFITTING_CRYSTALELECTRICFIELD_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/ComplexMatrix.h"
#include "MantidCurveFitting/FortranMatrix.h"
#include "MantidCurveFitting/FortranVector.h"
#include "MantidCurveFitting/GSLMatrix.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

typedef FortranMatrix<ComplexMatrix> ComplexFortranMatrix;
typedef FortranMatrix<GSLMatrix> DoubleFortranMatrix;
typedef FortranVector<ComplexVector> ComplexFortranVector;
typedef FortranVector<GSLVector> DoubleFortranVector;

std::tuple<GSLVector, ComplexMatrix> MANTID_CURVEFITTING_DLL sc_crystal_field(
    int nre, const std::string &type, int symmetry,
    const DoubleFortranMatrix &sbkq, DoubleFortranVector &bmol,
    DoubleFortranVector &bext, ComplexFortranMatrix &bkq, double temp);

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CRYSTALELECTRICFIELD_H_*/
