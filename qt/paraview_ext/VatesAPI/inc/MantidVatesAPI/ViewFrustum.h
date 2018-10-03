// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARAVIEW_VIEWFRUSTUM
#define MANTID_PARAVIEW_VIEWFRUSTUM

#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <cfloat>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace VATES {

enum PLANELOCATION {
  LEFTPLANE,
  RIGHTPLANE,
  BOTTOMPLANE,
  TOPPLANE,
  FARPLANE,
  NEARPLANE
};

template <PLANELOCATION I, typename T> class DLLExport FrustumPlane {
public:
  explicit FrustumPlane(const T &paramA, const T &paramB, const T &paramC,
                        const T &paramD)
      : m_paramA(paramA), m_paramB(paramB), m_paramC(paramC), m_paramD(paramD) {
  }
  FrustumPlane(const FrustumPlane<I, T> &other)
      : m_paramA(other.m_paramA), m_paramB(other.m_paramB),
        m_paramC(other.m_paramC), m_paramD(other.m_paramD) {}
  T A() { return m_paramA; }
  T B() { return m_paramB; }
  T C() { return m_paramC; }
  T D() { return m_paramD; }

  std::vector<T> getPlaneCoefficients() {
    return {m_paramA, m_paramB, m_paramC, m_paramD};
  }

private:
  T m_paramA;
  T m_paramB;
  T m_paramC;
  T m_paramD;
  enum { m_location = I };
};

using LeftPlane = FrustumPlane<LEFTPLANE, double>;
using RightPlane = FrustumPlane<RIGHTPLANE, double>;
using BottomPlane = FrustumPlane<BOTTOMPLANE, double>;
using TopPlane = FrustumPlane<TOPPLANE, double>;
using FarPlane = FrustumPlane<FARPLANE, double>;
using NearPlane = FrustumPlane<NEARPLANE, double>;

class DLLExport ViewFrustum {
public:
  ViewFrustum(const LeftPlane &leftPlane, const RightPlane &rightPlane,
              const BottomPlane &bottomPlane, const TopPlane &topPlane,
              const FarPlane &farPlane, const NearPlane &nearPlane);
  ViewFrustum(const ViewFrustum &other);
  ~ViewFrustum();
  ViewFrustum &operator=(const ViewFrustum &other);
  std::vector<std::pair<double, double>> toExtents() const;
  std::string toExtentsAsString() const;

private:
  mutable LeftPlane m_leftPlane;
  mutable RightPlane m_rightPlane;
  mutable TopPlane m_topPlane;
  mutable BottomPlane m_bottomPlane;
  mutable FarPlane m_farPlane;
  mutable NearPlane m_nearPlane;

  template <PLANELOCATION p1, PLANELOCATION p2, PLANELOCATION p3, typename T>
  std::vector<T>
  getIntersectionPointThreePlanes(FrustumPlane<p1, T> plane1,
                                  FrustumPlane<p2, T> plane2,
                                  FrustumPlane<p3, T> plane3) const;

  template <typename T>
  void initializeMatrix(Mantid::Kernel::Matrix<T> &matrix, std::vector<T> vec0,
                        std::vector<T> vec1, std::vector<T> vec2) const;
};
/**
 * Get the intersection point of three planes using Cramer's rule.
 * @param plane1 The first frustum plane
 * @param plane2 The second frustum plane
 * @param plane3 The third frustum plane
 */
template <PLANELOCATION p1, PLANELOCATION p2, PLANELOCATION p3, typename T>
std::vector<T>
ViewFrustum::getIntersectionPointThreePlanes(FrustumPlane<p1, T> plane1,
                                             FrustumPlane<p2, T> plane2,
                                             FrustumPlane<p3, T> plane3) const {
  const size_t dim = 3;

  std::vector<T> aVec{plane1.A(), plane2.A(), plane3.A()};

  std::vector<T> bVec{plane1.B(), plane2.B(), plane3.B()};

  std::vector<T> cVec{plane1.C(), plane2.C(), plane3.C()};

  // The input is Ax+By+Cz+D=0 but we need the form Ax+By+Cz=D
  const T factor = -1;
  std::vector<T> dVec{factor * plane1.D(), factor * plane2.D(),
                      factor * plane3.D()};

  // Get the different matrix permutations
  Mantid::Kernel::Matrix<T> abcMatrix(dim, dim);
  Mantid::Kernel::Matrix<T> dbcMatrix(dim, dim);
  Mantid::Kernel::Matrix<T> adcMatrix(dim, dim);
  Mantid::Kernel::Matrix<T> abdMatrix(dim, dim);

  initializeMatrix<T>(abcMatrix, aVec, bVec, cVec);
  T abcDet = abcMatrix.determinant();
  if (abcDet == 0) {
    throw std::runtime_error("Determinant for view frustum is 0.");
  }

  initializeMatrix<T>(dbcMatrix, dVec, bVec, cVec);
  initializeMatrix<T>(adcMatrix, aVec, dVec, cVec);
  initializeMatrix<T>(abdMatrix, aVec, bVec, dVec);

  T dbcDet = dbcMatrix.determinant();
  T adcDet = adcMatrix.determinant();
  T abdDet = abdMatrix.determinant();

  std::vector<T> intersection{dbcDet / abcDet, adcDet / abcDet,
                              abdDet / abcDet};

  return intersection;
}

/**
 * Initialize the matrix with the plane coefficient vectors.
 * @param matrix The matrix to initialze.
 * @param vec0 The first vector.
 * @param vec1 The second vector.
 * @param vec2 The third vector.
 */
template <typename T>
void ViewFrustum::initializeMatrix(Mantid::Kernel::Matrix<T> &matrix,
                                   std::vector<T> vec0, std::vector<T> vec1,
                                   std::vector<T> vec2) const {
  std::pair<size_t, size_t> size = matrix.size();

  if (size.first != 3 || size.second != 3) {
    throw std::runtime_error(
        "Matrix for view frustum calculation has the wrong dimensionality.");
  }
  matrix.setColumn(0, vec0);
  matrix.setColumn(1, vec1);
  matrix.setColumn(2, vec2);
}

/// shared pointer to the view frustum
using ViewFrustum_sptr = boost::shared_ptr<Mantid::VATES::ViewFrustum>;
using ViewFrustum_const_sptr =
    boost::shared_ptr<const Mantid::VATES::ViewFrustum>;
} // namespace VATES
} // namespace Mantid
#endif
