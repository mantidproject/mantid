#include "MantidKernel/Interpolation.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/UnitFactory.h"
#include <Poco/StringTokenizer.h>

namespace Mantid
{
namespace Kernel
{
  namespace
  {
    /// static logger
    Logger  g_log("Interpolation");
  }

  /** Constructor default to linear interpolation and x-unit set to TOF
   */
  Interpolation::Interpolation() : m_method("linear")
  {
    m_xUnit = UnitFactory::Instance().create("TOF");
    m_yUnit = UnitFactory::Instance().create("TOF");
  }

  size_t Interpolation::findIndexOfNextLargerValue(const std::vector<double> &data, double key, size_t range_start, size_t range_end) const
  {
      if(range_end < range_start)
      {
          throw std::range_error("Value is outside array range.");
      }

      size_t center = range_start + (range_end - range_start) / 2;

      if(data[center] > key && data[center - 1] <= key) {
          return center;
      }

      if(data[center] <= key) {
          return findIndexOfNextLargerValue(data, key, center + 1, range_end);
      } else {
          return findIndexOfNextLargerValue(data, key, range_start, center - 1);
      }
  }

  void Interpolation::setXUnit(const std::string& unit) 
  { 
    m_xUnit = UnitFactory::Instance().create(unit);
  }

  void Interpolation::setYUnit(const std::string& unit) 
  { 
    m_yUnit = UnitFactory::Instance().create(unit);
  }


  /** Get interpolated value at location at
  * @param at :: Location where to get interpolated value
  * @return the value
  */
  double Interpolation::value(const double& at) const
  {
    size_t N = m_x.size();

    if ( N == 0 )
    {
      g_log.error() << "Need at least one value for interpolation. Return interpolation value zero.";
      return 0.0;
    }

    if ( N == 1 )
    {
      return m_y[0];
    }
    
    // check first if at is within the limits of interpolation interval

    if ( at < m_x[0] )
    {
      return m_y[0]-(m_x[0]-at)*(m_y[1]-m_y[0])/(m_x[1]-m_x[0]);
    }

    if ( at >= m_x[N-1] )
    {
      return m_y[N-1]+(at-m_x[N-1])*(m_y[N-1]-m_y[N-2])/(m_x[N-1]-m_x[N-2]);
    }

    try {
        // otherwise
        // General case. Find index of next largest value by binary search.
        size_t idx = findIndexOfNextLargerValue(m_x, at, 1, N - 1);
        return m_y[idx-1] + (at - m_x[idx-1])*(m_y[idx]-m_y[idx-1])/(m_x[idx]-m_x[idx-1]);
    } catch(std::range_error) {
        return 0.0;
    }
  }

  /** Add point in the interpolation.
  *
  * @param xx :: x-value
  * @param yy :: y-value
  */
  void Interpolation::addPoint(const double& xx, const double& yy)
  { 
    size_t N = m_x.size();
    std::vector<double>::iterator it;

    if ( N == 0)
    {
      m_x.push_back(xx); m_y.push_back(yy);
      return;
    }

    // check first if xx is within the limits of interpolation interval

    if ( xx <= m_x[0] )
    {
      it = m_x.begin();
      it = m_x.insert ( it , xx );
      it = m_y.begin();
      it = m_y.insert ( it , yy ); 
      return;
    }

    if ( xx >= m_x[N-1] )
    {
      m_x.push_back(xx); m_y.push_back(yy);
      return;
    }

    // otherwise

    for (unsigned int i = 1; i < N; i++)
    {
      if ( m_x[i] > xx )
      {
        it = m_x.begin();
        it = m_x.insert ( it+i , xx );
        it = m_y.begin();
        it = m_y.insert ( it+i , yy ); 
        return;
      }
    }
  }

  /**
    Prints object to stream
    @param os :: the Stream to output to
  */
  void Interpolation::printSelf(std::ostream& os) const
  {
    os << m_method << " ; " << m_xUnit->unitID() << " ; " << m_yUnit->unitID();

    for ( unsigned int i = 0; i < m_x.size(); i++)
    {
      os << " ; " << m_x[i] << " " << m_y[i];
    }
  }

  /**
    Resets interpolation data by clearing the internal storage for x- and y-values
  */
  void Interpolation::resetData()
  {
      m_x.clear();
      m_y.clear();
  }

  /**
    Prints the value of parameter
    @param os :: the Stream to output to
    @param f :: the FitParameter to output
    @return the output stream
    */
  std::ostream& operator<<(std::ostream& os, const Interpolation& f)
  {
    f.printSelf(os);
    return os;
  }

  /**
    Reads in parameter value
    @param in :: Input Stream
    @param f :: FitParameter to write to
    @return Current state of stream
  */
  std::istream& operator>>(std::istream& in, Interpolation& f)
  {

    typedef Poco::StringTokenizer tokenizer;
    std::string str;
    getline(in, str);
    tokenizer values(str, ";", tokenizer::TOK_TRIM);

    f.setMethod(values[0]);
    f.setXUnit(values[1]);
    f.setYUnit(values[2]);
    f.resetData(); // Reset data, in case the interpolation table is not empty

    for ( unsigned int i = 3; i < values.count(); i++)
    {
      std::stringstream str(values[i]);
      double x, y;

      str >> x >> y;

      f.addPoint(x,y);
    }

    return in;
  }

} // namespace Kernel
} // namespace Mantid
