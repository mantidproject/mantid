#include "MantidAPI/Column.h"
#include "MantidKernel/Logger.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace Mantid
{
namespace API
{

namespace
{
  /// static logger object
  Kernel::Logger g_log("Column");
}

template<>
bool Column::isType<bool>()const
{
    return isBool();
}

/// Set plot type where
/// None = 0 (means it has specifically been set to 'no plot type')
/// NotSet = -1000 (this is the default and means plot style has not been set)
/// X = 1, Y = 2, Z = 3, xErr = 4, yErr = 5, Label = 6
/// @param t plot type as defined above
void Column::setPlotType(int t)
{
  if ( t == -1000 || t == 0 || t == 1 || t == 2 || t == 3 || t == 4 ||
       t == 5 || t == 6 )
    m_plotType = t;
  else
  {
    g_log.error() << "Cannot set plot of column to " << t
      << " . Ignore this attempt." << std::endl;
  }
}

/**
 * No implementation by default.
 */
void Column::sortIndex( bool, size_t, size_t, std::vector<size_t>&, std::vector<std::pair<size_t,size_t>>& ) const
{
  throw std::runtime_error("Cannot sort column of type " + m_type);
}

/**
 * No implementation by default.
 */
void Column::sortValues( const std::vector<size_t>& )
{
  throw std::runtime_error("Cannot sort column of type " + m_type);
}

std::ostream& operator<<(std::ostream& s,const API::Boolean& b)
{
    s << (b.value?"true":"false");
    return s;
}

/**
 * Translate text into a Boolean.
 */
std::istream& operator>>(std::istream& istr, API::Boolean& b)
{
  std::string buff;
  istr >> buff;
  std::transform(buff.begin(),buff.end(),buff.begin(),toupper);
  if (buff == "TRUE" || buff == "1" || buff == "OK" || buff == "YES" || buff == "ON")
  {
    b = true;
  }
  else
  {
    b = false;
  }
  return istr;
}

} // namespace API
} // namespace Mantid

