#include "MantidKernel/Interpolation.h"
#include <Poco/StringTokenizer.h>
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace Kernel
{

  Logger& Interpolation::g_log = Logger::get("Interpolation");

  /** Constructor default to linear interpolation and x-unit set to TOF
   */
  Interpolation::Interpolation() : m_name("linear")
  {
    m_xUnit = UnitFactory::Instance().create("TOF");
    m_yUnit = UnitFactory::Instance().create("TOF");
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
      g_log.error() << "No data in Interpolation. Return interpolation value zero.";
      return 0.0;
    }
    
    // check first if at is within the limits of interpolation interval

    if ( at <= m_x[0] )
    {
      return m_y[0]-(m_x[0]-at)*(m_y[1]-m_y[0])/(m_x[1]-m_x[0]);
    }

    if ( at >= m_x[N-1] )
    {
      return m_y[N-1]+(at-m_y[N-1])*(m_y[N-1]-m_y[N-2])/(m_x[N-1]-m_x[N-2]);
    }

    // otherwise

    for (unsigned int i = 1; i < N; i++)
    {
      if ( m_x[i] > at )
      {
        return m_y[i-1] + (at-m_x[i-1])*(m_y[i]-m_y[i-1])/(m_x[i]-m_x[i-1]);
      }
    }
    return 0.0;
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
    os << m_name << " ; " << m_xUnit->unitID() << " ; " << m_yUnit->unitID();

    for ( unsigned int i = 0; i < m_x.size(); i++)
    {
      os << " ; " << m_x[i] << " " << m_y[i];
    }
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
