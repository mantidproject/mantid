#ifndef MANTID_PARAVIEW_VIEWFRUSTUM
#define MANTID_PARAVIEW_VIEWFRUSTUM

#include "MantidKernel/System.h"
#include "MantidKernel/Matrix.h"
#include <stdexcept>
#include <cmath>
#include <cfloat>
#include <vector>

namespace Mantid
{
namespace VATES
{

enum PLANELOCATION{LEFTPLANE, RIGHTPLANE, BOTTOMPLANE, TOPPLANE, FARPLANE, NEARPLANE};

template<PLANELOCATION I, typename T>
class DLLExport FrustumPlane
{
  public:
    explicit FrustumPlane(const T& paramA, const T& paramB, const T& paramC, const T& paramD) : m_paramA(paramA), 
                                                                                                m_paramB(paramB),
                                                                                                m_paramC(paramC), 
                                                                                                m_paramD(paramD){}
    FrustumPlane(const FrustumPlane<I, T>& other) : m_paramA(other.m_paramA), 
                                                    m_paramB(other.m_paramB),
                                                    m_paramC(other.m_paramC), 
                                                    m_paramD(other.m_paramD){}
    T A() {return m_paramA;}
    T B() {return m_paramB;}
    T C() {return m_paramC;}
    T D() {return m_paramD;}

    std::vector<T> getPlaneCoefficients()
    {
      std::vector<T> coefficients;
      coefficients.push_back(m_paramA);
      coefficients.push_back(m_paramB);
      coefficients.push_back(m_paramC);
      coefficients.push_back(m_paramD);

      return coefficients;
    }

private:
    T m_paramA;
    T m_paramB;
    T m_paramC;
    T m_paramD;
    enum{m_location = I};
};

typedef FrustumPlane<LEFTPLANE, double> LeftPlane;
typedef FrustumPlane<RIGHTPLANE, double> RightPlane;
typedef FrustumPlane<BOTTOMPLANE, double> BottomPlane;
typedef FrustumPlane<TOPPLANE, double> TopPlane;
typedef FrustumPlane<FARPLANE, double> FarPlane;
typedef FrustumPlane<NEARPLANE, double> NearPlane;


class DLLExport ViewFrustum
{
  public:
    ViewFrustum(const LeftPlane leftPlane,
                const RightPlane rightPlane,
                const BottomPlane bottomPlane,
                const TopPlane topPlane,
                const FarPlane farPlane, 
                const NearPlane nearPlane);
    ViewFrustum(const ViewFrustum& other);
    ~ViewFrustum();
    ViewFrustum& operator=(const ViewFrustum& other);
    std::vector<std::pair<double, double>> toExtents() const;
    std::string toExtentsAsString() const;
    bool pointLiesInsideViewFrustum(std::vector<double> point) const;

  private:
    mutable LeftPlane m_leftPlane;
    mutable RightPlane m_rightPlane;
    mutable TopPlane m_topPlane;
    mutable BottomPlane m_bottomPlane;
    mutable FarPlane m_farPlane;
    mutable NearPlane m_nearPlane;

    template<PLANELOCATION p1, PLANELOCATION p2, PLANELOCATION p3, typename T>
    std::vector<T> getIntersectionPointThreePlanes(FrustumPlane<p1, T> plane1, FrustumPlane<p2, T> plane2, FrustumPlane<p3, T> plane3) const;

    template<typename T>
    void initializeMatrix(Mantid::Kernel::Matrix<T>& matrix, std::vector<T> vec0, std::vector<T> vec1, std::vector<T> vec2) const;
};
  /**
   * Get the intersection point of three planes using Cramer's rule.
   * @param plane1 The first frustum plane
   * @param plane2 The second frustum plane
   * @param plane3 The third frustum plane
   */
  template<PLANELOCATION p1, PLANELOCATION p2, PLANELOCATION p3, typename T>
  std::vector<T> ViewFrustum::getIntersectionPointThreePlanes(FrustumPlane<p1, T> plane1, FrustumPlane<p2, T> plane2, FrustumPlane<p3, T> plane3) const
  {
    const size_t dim = 3;

    std::vector<T> aVec;
    aVec.push_back(plane1.A());
    aVec.push_back(plane2.A());
    aVec.push_back(plane3.A());

    std::vector<T> bVec;
    bVec.push_back(plane1.B());
    bVec.push_back(plane2.B());
    bVec.push_back(plane3.B());

    std::vector<T> cVec;
    cVec.push_back(plane1.C());
    cVec.push_back(plane2.C());
    cVec.push_back(plane3.C());

    // The input is Ax+By+Cz+D=0 but we need the form Ax+By+Cz=D
    std::vector<T> dVec;
    const T factor = -1;
    dVec.push_back(factor*plane1.D());
    dVec.push_back(factor*plane2.D());
    dVec.push_back(factor*plane3.D());

    // Get the different matrix permutations
    Mantid::Kernel::Matrix<T> abcMatrix(dim, dim);
    Mantid::Kernel::Matrix<T>  dbcMatrix(dim, dim);
    Mantid::Kernel::Matrix<T>  adcMatrix(dim, dim);
    Mantid::Kernel::Matrix<T>  abdMatrix(dim, dim);
    
    initializeMatrix<T>(abcMatrix, aVec, bVec, cVec);
    T abcDet = abcMatrix.determinant();
    if (abcDet == 0)
    {
      throw std::runtime_error("Determinant for view frustum is 0.");
    }

    initializeMatrix<T>(dbcMatrix, dVec, bVec, cVec);
    initializeMatrix<T>(adcMatrix, aVec, dVec, cVec);
    initializeMatrix<T>(abdMatrix, aVec, bVec, dVec);

    T dbcDet = dbcMatrix.determinant();
    T adcDet = adcMatrix.determinant();
    T abdDet = abdMatrix.determinant();

    std::vector<T> intersection;
    intersection.push_back(dbcDet/abcDet);
    intersection.push_back(adcDet/abcDet);
    intersection.push_back(abdDet/abcDet);

    return intersection;
  }

  /**
   * Initialize the matrix with the plane coefficient vectors.
   * @param matrix The matrix to initialze.
   * @param vec0 The first vector.
   * @param vec1 The second vector.
   * @param vec2 The third vector.
   */
  template<typename T>
  void ViewFrustum::initializeMatrix(Mantid::Kernel::Matrix<T>& matrix, std::vector<T> vec0, std::vector<T> vec1, std::vector<T> vec2) const
  {
    std::pair<size_t, size_t> size = matrix.size();

    if (size.first != 3 || size.second != 3)
    {
      throw std::runtime_error("Matrix for view frustum calculation has the wrong dimensionality.");
    }
    matrix.setColumn(0, vec0);
    matrix.setColumn(1, vec1);
    matrix.setColumn(2, vec2);
  }
}
}
#endif