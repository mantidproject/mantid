#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidKernel/System.h"
#include <stdexcept>
#include "MantidKernel/VMD.h"
#include <gsl/gsl_linalg.h>
#include "MantidKernel/Matrix.h"
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Geometry {

//----------------------------------------------------------------------------------------------
/** Constructor with normal and point
 *
 * @param normal :: normal to the plane. Points that are in the direction of the
 *normal of the plane are considered to be bounded by it.
 * @param point :: any point that is on the plane
 */
MDPlane::MDPlane(const std::vector<coord_t> &normal,
                 const std::vector<coord_t> &point) {
  m_nd = normal.size();
  if ((m_nd < 1) || (m_nd > 100))
    throw std::invalid_argument(
        "MDPlane::ctor(): Invalid number of dimensions in the normal vector !");
  if (point.size() != normal.size())
    throw std::invalid_argument("MDPlane::ctor(): Inconsistent number of "
                                "dimensions in the normal/point vectors!");
  construct(normal, point);
}

//----------------------------------------------------------------------------------------------
/** Constructor with normal and point
 *
 * @param normal :: normal to the plane. Points that are in the direction of the
 *normal of the plane are considered to be bounded by it.
 * @param point :: any point that is on the plane
 */
MDPlane::MDPlane(const Mantid::Kernel::VMD &normal,
                 const Mantid::Kernel::VMD &point) {
  m_nd = normal.getNumDims();
  if ((m_nd < 1) || (m_nd > 100))
    throw std::invalid_argument(
        "MDPlane::ctor(): Invalid number of dimensions in the normal vector !");
  if (point.getNumDims() != normal.getNumDims())
    throw std::invalid_argument("MDPlane::ctor(): Inconsistent number of "
                                "dimensions in the normal/point vectors!");
  construct(normal, point);
}

//----------------------------------------------------------------------------------------------
/** Constructor with normal and point
 *
 * @param nd :: number of dimensions
 * @param normal :: normal to the plane. Points that are in the direction of the
 *normal of the plane are considered to be bounded by it.
 * @param point :: any point that is on the plane
 */
MDPlane::MDPlane(const size_t nd, const float *normal, const float *point)
    : m_nd(nd) {
  if ((m_nd < 1) || (m_nd > 100))
    throw std::invalid_argument(
        "MDPlane::ctor(): Invalid number of dimensions in the workspace!");
  construct(normal, point);
}

//----------------------------------------------------------------------------------------------
/** Constructor with normal and point
 *
 * @param nd :: number of dimensions
 * @param normal :: normal to the plane. Points that are in the direction of the
 *normal of the plane are considered to be bounded by it.
 * @param point :: any point that is on the plane
 */
MDPlane::MDPlane(const size_t nd, const double *normal, const double *point)
    : m_nd(nd) {
  if ((m_nd < 1) || (m_nd > 100))
    throw std::invalid_argument(
        "MDPlane::ctor(): Invalid number of dimensions in the workspace!");
  construct(normal, point);
}

//----------------------------------------------------------------------------------------------
/** Constructor with N-1 vectors on the plane's surface.
 *
 * @param vectors :: vector of N-1 vectors with N dimensions.
 * @param origin :: any point on the plane
 * @param insidePoint :: coordinate of a point that is known to be inside the
 *plane described
 * @throws if the vectors are collinear
 */
MDPlane::MDPlane(const std::vector<Mantid::Kernel::VMD> &vectors,
                 const Mantid::Kernel::VMD &origin,
                 const Mantid::Kernel::VMD &insidePoint) {
  // Get the normal vector by the determinant method
  VMD normal = VMD::getNormalVector(vectors);

  // The dimensionality of the plane
  m_nd = normal.getNumDims();

  // Whew. We have a normal, and a point on the plane. We can construct
  construct(normal, origin);

  // Did we get the wrong sign of the normal?
  if (!this->isPointBounded(insidePoint)) {
    // Flip the normal over
    delete[] this->m_normal;
    for (size_t d = 0; d < normal.getNumDims(); d++)
      normal[d] = -1.0f * normal[d];
    // And re-construct
    construct(normal, origin);
  }
}

//  //----------------------------------------------------------------------------------------------
//  MDPlane::MDPlane(const std::vector<Mantid::Kernel::VMD> & points, const VMD
//  & insidePoint)
//  {
//    if (points.size() <= 0)
//      throw std::invalid_argument("MDPlane::ctor(): Must give at least 1
//      point");
//    VMD origin = points[0];
//    m_nd = origin.getNumDims();
//    if (insidePoint.getNumDims() != m_nd)
//      throw std::invalid_argument("MDPlane::ctor(): The insidePoint parameter
//      must match the dimensions of the other points!");
//    if (m_nd < 1)
//      throw std::invalid_argument("MDPlane::ctor(): Must have at least 1
//      dimension!");
//    if (points.size() != m_nd)
//      throw std::invalid_argument("MDPlane::ctor(): Must have as many points
//      as there are dimensions!");
//    for (size_t i=0; i < points.size(); i++)
//      if (points[i].getNumDims() != m_nd)
//        throw std::invalid_argument("MDPlane::ctor(): Inconsistent number of
//        dimensions in the points given!");
//
//    // Special case for 1D (no need to use the linear equation system)
//    if (m_nd == 1)
//    {
//      VMD origin = points[0];
//      VMD normal(1);
//      normal[0] = (insidePoint[0] > origin[0]) ? +1.0 : -1.0;
//      construct(normal, origin);
//      return;
//    }
//
//    // Make the A and B matrices as described above
//    double * a_data = new double[(m_nd-1) * (m_nd-1)];;
//    double * b_data = new double[(m_nd-1)];
//    for (size_t row=0; row<(m_nd-1); row++)
//    {
//      // Offset the point
//      VMD point = points[row+1] - origin;
//      // Put the 1th+ coords
//      for (size_t col=0; col<(m_nd-1); col++)
//        a_data[row*(m_nd-1) + col] = point[col+1];
//      // The row in B = - the 0th coordinate of the offset point.
//      b_data[row] = -point[0];
//    }
//    // To avoid aborting
//    gsl_set_error_handler_off();
//
//    int ret = 0;
//
//    gsl_matrix_view m = gsl_matrix_view_array (a_data, m_nd-1, m_nd-1);
//    gsl_vector_view b = gsl_vector_view_array (b_data, m_nd-1);
//    gsl_vector *x = gsl_vector_alloc (m_nd-1);
//
//    // Use GSL to solve the linear algebra problem
////    gsl_permutation * p = gsl_permutation_alloc (m_nd-1);
////    int s;
////    ret = gsl_linalg_LU_decomp (&m.matrix, p, &s);
////    if (ret == 0)
////      ret = gsl_linalg_LU_solve (&m.matrix, p, &b.vector, x);
//
//    gsl_matrix * V = gsl_matrix_alloc(m_nd-1, m_nd-1);
//    gsl_vector * S = gsl_vector_alloc(m_nd-1);
//    gsl_vector * work = gsl_vector_alloc(m_nd-1);
//    ret = gsl_linalg_SV_decomp(&m.matrix, V, S, work);
//    if (ret == 0)
//      ret = gsl_linalg_SV_solve(&m.matrix, V, S, &b.vector, x);
//
//    if (ret != 0)
//    {
//      std::string pointsStr;
//      for (size_t i=0; i < points.size(); i++)
//        pointsStr += points[i].toString(",") + ((i != points.size()-1) ? "; "
//        : "");
//      // Something failed
//      throw std::runtime_error("MDPlane::ctor(): unable to solve the system of
//      equations.\n"
//          "The points given likely do not form a plane (some may be
//          collinear), meaning the plane cannot be constructed.\n"
//          "Points were: " + pointsStr);
//    }
//
//    // The normal vector
//    VMD normal(m_nd);
//
//    // Recall that we fixed a1 to 1.0
//    normal[0] = 1.0;
//    for (size_t d=0; d<(m_nd-1); d++)
//      normal[d+1] = x->data[d];
//
//    gsl_vector_free (x);
//    gsl_vector_free (S);
//    gsl_matrix_free (V);
//    delete [] a_data;
//    delete [] b_data;
//
//    // Whew. We have a normal, and a point on the plane. We can construct
//    construct(normal, origin);
//
//    // Did we get the wrong normal?
//    if (!this->isPointBounded(insidePoint))
//    {
//      // Flip the normal over
//      delete [] this->m_normal;
//      for (size_t d=0; d<normal.getNumDims(); d++)
//        normal[d] = -1.0 * normal[d];
//      // And re-construct
//      construct(normal, origin);
//    }
//  }

//----------------------------------------------------------------------------------------------
/** Copy constructor
 *
 * @param other :: MDPlane to copy
 */
MDPlane::MDPlane(const MDPlane &other)
    : m_nd(other.m_nd), m_inequality(other.m_inequality) {
  m_normal = new coord_t[m_nd];
  for (size_t d = 0; d < m_nd; d++)
    m_normal[d] = other.m_normal[d];
}

//----------------------------------------------------------------------------------------------
/** Assignment operator
 *
 * @param other :: MDPlane to copy
 */
MDPlane &MDPlane::operator=(const MDPlane &other) {
  if (this != &other) // protect against invalid self-assignment
  {
    m_nd = other.m_nd;
    m_inequality = other.m_inequality;
    // Replace the array
    delete[] m_normal;
    m_normal = new coord_t[m_nd];
    for (size_t d = 0; d < m_nd; d++)
      m_normal[d] = other.m_normal[d];
  }
  return *this;
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MDPlane::~MDPlane() { delete[] m_normal; }

} // namespace Mantid
} // namespace Geometry
