#include "MantidMDAlgorithms/MDPlane.h"
#include "MantidKernel/System.h"
#include <stdexcept>

namespace Mantid
{
namespace MDAlgorithms
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param coeff :: vector of length nd of coefficients for the linear equation.
   * @param inequality :: RHS of the linear equation
   */
  MDPlane::MDPlane(const std::vector<coord_t> & normal, const std::vector<coord_t> & point)
  {
    m_nd = normal.size();
    if ((m_nd < 1) || (m_nd > 100)) throw std::invalid_argument("Invalid number of dimensions in the normal vector !");
    if (point.size() != normal.size()) throw std::invalid_argument("Inconsistent number of dimensions in the normal/point vectors!");
    construct(normal, point);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param nd :: number of dimensions
   * @param coeff :: vector of length nd of coefficients for the linear equation.
   * @param inequality :: RHS of the linear equation
   */
  MDPlane::MDPlane(const size_t nd, const coord_t * normal, const coord_t * point)
  : m_nd(nd)
  {
    if ((m_nd < 1) || (m_nd > 100)) throw std::invalid_argument("Invalid number of dimensions in the workspace!");
    construct(normal, point);
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
} // namespace MDAlgorithms

