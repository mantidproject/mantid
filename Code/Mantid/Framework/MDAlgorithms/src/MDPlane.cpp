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
  MDPlane::MDPlane(const std::vector<coord_t> & coeff, const coord_t inequality)
  : m_inequality(inequality)
  {
    m_nd = coeff.size();
    if ((m_nd < 1) || (m_nd > 100)) throw std::invalid_argument("Invalid number of dimensions in the workspace!");
    m_coeff = new coord_t[m_nd];
    for (size_t d=0; d<m_nd; d++)
      m_coeff[d] = coeff[d];
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param nd :: number of dimensions
   * @param coeff :: vector of length nd of coefficients for the linear equation.
   * @param inequality :: RHS of the linear equation
   */
  MDPlane::MDPlane(const size_t nd, const coord_t * coeff, const coord_t inequality)
  : m_nd(nd), m_inequality(inequality)
  {
    if ((m_nd < 1) || (m_nd > 100)) throw std::invalid_argument("Invalid number of dimensions in the workspace!");
    m_coeff = new coord_t[m_nd];
    for (size_t d=0; d<m_nd; d++)
      m_coeff[d] = coeff[d];
  }

  //----------------------------------------------------------------------------------------------
  /** Copy constructor
   *
   * @param other :: MDPlane to copy */
  MDPlane::MDPlane(const MDPlane & other)
  : m_nd(other.m_nd), m_inequality(other.m_inequality)
  {
    m_coeff = new coord_t[m_nd];
    for (size_t d=0; d<m_nd; d++)
      m_coeff[d] = other.m_coeff[d];
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
      delete [] m_coeff;
      m_coeff = new coord_t[m_nd];
      for (size_t d=0; d<m_nd; d++)
        m_coeff[d] = other.m_coeff[d];
    }
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDPlane::~MDPlane()
  {
    delete [] m_coeff;
  }
  



} // namespace Mantid
} // namespace MDAlgorithms

