#include "MantidKernel/Interpolation.h"

namespace Mantid
{
namespace Kernel
{

  Logger& Interpolation::g_log = Logger::get("Interpolation");

  /** Constructor
   *  @param name The name of interpolation method
   */
  Interpolation::Interpolation(const std::string &name) : m_name(name)
  {
    if (m_name.compare("linear") != 0)
      g_log.warning() << "Interpolation class only support linear interpolation for now. Default to linear interpolation";
  }

  /** Get interpolated value at location at
  * @param at Location where to get interpolated value
  */
  double Interpolation::value (const double& at)
  {
    unsigned int N = m_x.size();

    
    // check first if at is within the limits of interpolation interval

    if ( at <= m_x[0] )
      return m_y[0];

    if ( at >= m_x[N-1] )
      return m_y[N-1];


    // otherwise

    for (unsigned int i = 1; i < N; i++)
    {
      if ( m_x[i] > at )
      {
        return m_y[i-1] + (at-m_x[i-1])*(m_y[i]-m_y[i-1])/(m_x[i]-m_x[i-1]);
      }
    }
  }

  /** Add point
  *
  * @param xx x-value
  * @param yy y-value
  */
  void Interpolation::addPoint(const double& xx, const double& yy)
  { 
    m_x.push_back(xx); m_y.push_back(yy); 
  }

} // namespace Kernel
} // namespace Mantid
