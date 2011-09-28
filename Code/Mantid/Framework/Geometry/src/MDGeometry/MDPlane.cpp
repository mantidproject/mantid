#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidKernel/System.h"
#include <stdexcept>
#include "MantidKernel/VMD.h"
#include <gsl/gsl_linalg.h>

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Geometry
{

  //----------------------------------------------------------------------------------------------
  /** Constructor with normal and point
   *
   * @param normal :: vector of length nd of coefficients for the linear equation.
   * @param point :: any point that is on the plane
   */
  MDPlane::MDPlane(const std::vector<coord_t> & normal, const std::vector<coord_t> & point)
  {
    m_nd = normal.size();
    if ((m_nd < 1) || (m_nd > 100)) throw std::invalid_argument("MDPlane::ctor(): Invalid number of dimensions in the normal vector !");
    if (point.size() != normal.size()) throw std::invalid_argument("MDPlane::ctor(): Inconsistent number of dimensions in the normal/point vectors!");
    construct(normal, point);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor with normal and point
   *
   * @param normal :: vector of length nd of coefficients for the linear equation.
   * @param point :: any point that is on the plane
   */
  MDPlane::MDPlane(const Mantid::Kernel::VMD & normal, const Mantid::Kernel::VMD & point)
  {
    m_nd = normal.getNumDims();
    if ((m_nd < 1) || (m_nd > 100)) throw std::invalid_argument("MDPlane::ctor(): Invalid number of dimensions in the normal vector !");
    if (point.getNumDims() != normal.getNumDims()) throw std::invalid_argument("MDPlane::ctor(): Inconsistent number of dimensions in the normal/point vectors!");
    construct(normal, point);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor with normal and point
   *
   * @param nd :: number of dimensions
 * @param normal :: vector of length nd of coefficients for the linear equation.
 * @param point :: any point that is on the plane
   */
  MDPlane::MDPlane(const size_t nd, const coord_t * normal, const coord_t * point)
  : m_nd(nd)
  {
    if ((m_nd < 1) || (m_nd > 100)) throw std::invalid_argument("MDPlane::ctor(): Invalid number of dimensions in the workspace!");
    construct(normal, point);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor with N points on the plane's surface.
   *
   * A hyperplane is defined as:
   *  a1*x1 + a2*x2 + ... = b
   *
   * We have N "x" points describing points on the surface
   *  a1*x11 + a2*x12 + ... = b
   *  a1*x21 + a2*x22 + ... = b
   *
   * Let us reset the origin by offsetting all points by - x1 so that the first point is 0,0,0...
   * This allows us to set b = 0
   *  0 + 0 = 0
   *  a1 * (x21-x11) + a2 * (x22-x12) + ... = 0
   *  a1 * (x31-x11) + a2 * (x32-x12) + ... = 0
   *
   * We can divide by a1 to reduce the problems to N-1 equations with N-1 variables
   *  1 * (x21-x11) + a2/a1 * (x22-x12) + ... = 0
   *  1 * (x31-x11) + a2/a1 * (x32-x12) + ... = 0
   *      ...
   *
   * Or, expressed as a matrix A x = B
   *
   * A x = | x22 x23 | | a2/a1 | = B = | -x21 |
   *       | x32 x33 | | a3/a1 |       | -x31 |
   *
   * (where i've offset x22 etc. already)
   * We can take a1 = 1 since it doesn't matter: we'll calculate b later.
   *
   * @param points :: vector of N points with N dimensions.
   * @param insidePoint :: coordinate of a point that is known to be inside the plane described
   * @throws GSL error if the matrix cannot be solved (singular, etc.)
   */
  MDPlane::MDPlane(const std::vector<Mantid::Kernel::VMD> & points, const VMD & insidePoint)
  {
    if (points.size() <= 0)
      throw std::invalid_argument("MDPlane::ctor(): Must give at least 1 point");
    VMD origin = points[0];
    m_nd = origin.getNumDims();
    if (insidePoint.getNumDims() != m_nd)
      throw std::invalid_argument("MDPlane::ctor(): The insidePoint parameter must match the dimensions of the other points!");
    if (m_nd < 1)
      throw std::invalid_argument("MDPlane::ctor(): Must have at least 1 dimension!");
    if (points.size() != m_nd)
      throw std::invalid_argument("MDPlane::ctor(): Must have as many points as there are dimensions!");
    for (size_t i=0; i < points.size(); i++)
      if (points[i].getNumDims() != m_nd)
        throw std::invalid_argument("MDPlane::ctor(): Inconsistent number of dimensions in the points given!");

    // Special case for 1D (no need to use the linear equation system)
    if (m_nd == 1)
    {
      VMD origin = points[0];
      VMD normal(1);
      normal[0] = (insidePoint[0] > origin[0]) ? +1.0 : -1.0;
      construct(normal, origin);
      return;
    }

    // Make the A and B matrices as described above
    double * a_data = new double[(m_nd-1) * (m_nd-1)];;
    double * b_data = new double[(m_nd-1)];
    for (size_t row=0; row<(m_nd-1); row++)
    {
      // Offset the point
      VMD point = points[row+1] - origin;
      // Put the 1th+ coords
      for (size_t col=0; col<(m_nd-1); col++)
        a_data[row*(m_nd-1) + col] = point[col+1];
      // The row in B = - the 0th coordinate of the offset point.
      b_data[row] = -point[0];
    }

    gsl_matrix_view m = gsl_matrix_view_array (a_data, m_nd-1, m_nd-1);
    gsl_vector_view b = gsl_vector_view_array (b_data, m_nd-1);
    gsl_vector *x = gsl_vector_alloc (m_nd-1);
    int s;

    // Use GSL to solve the linear algebra problem
    gsl_permutation * p;
    try
    {
      p = gsl_permutation_alloc (m_nd-1);
      gsl_linalg_LU_decomp (&m.matrix, p, &s);
      gsl_linalg_LU_solve (&m.matrix, p, &b.vector, x);
    }
    catch (...)
    {
      // Rethrow with useful message
      throw std::runtime_error("MDPlane::ctor(): the points given did not form a plane (may be collinear), meaning the plane cannot be constructed.\n"
          "GSL error: " /*+ std::string(e) */);
    }

    // The normal vector
    VMD normal(m_nd);

    // Recall that we fixed a1 to 1.0
    normal[0] = 1.0;
    for (size_t d=0; d<(m_nd-1); d++)
      normal[d+1] = x->data[d];

    gsl_permutation_free (p);
    gsl_vector_free (x);
    delete [] a_data;
    delete [] b_data;

    // Whew. We have a normal, and a point on the plane. We can construct
    construct(normal, origin);

    // Did we get the wrong normal?
    if (!this->isPointBounded(insidePoint))
    {
      // Flip the normal over
      delete [] this->m_normal;
      for (size_t d=0; d<normal.getNumDims(); d++)
        normal[d] = -1.0 * normal[d];
      // And re-construct
      construct(normal, origin);
    }
  }



  //----------------------------------------------------------------------------------------------
  /** Copy constructor
   *
   * @param other :: MDPlane to copy */
  MDPlane::MDPlane(const MDPlane & other)
  : m_nd(other.m_nd), m_inequality(other.m_inequality)
  {
    m_normal = new coord_t[m_nd];
    for (size_t d=0; d<m_nd; d++)
      m_normal[d] = other.m_normal[d];
  }


  //----------------------------------------------------------------------------------------------
  /** Assignment operator
   *
   * @param other :: MDPlane to copy */
  MDPlane & MDPlane::operator=(const MDPlane & other)
  {
    if (this != &other) // protect against invalid self-assignment
    {
      m_nd = other.m_nd;
      m_inequality = other.m_inequality;
      // Replace the array
      delete [] m_normal;
      m_normal = new coord_t[m_nd];
      for (size_t d=0; d<m_nd; d++)
        m_normal[d] = other.m_normal[d];
    }
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDPlane::~MDPlane()
  {
    delete [] m_normal;
  }
  



} // namespace Mantid
} // namespace Geometry

