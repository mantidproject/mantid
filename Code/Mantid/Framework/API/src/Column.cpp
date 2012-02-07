#include "MantidAPI/Column.h"
#include <algorithm>

namespace Mantid
{
namespace API
{

// Get a reference to the logger
Kernel::Logger& Column::g_log = Kernel::Logger::get("Column");

template<>
bool Column::isType<bool>()const
{
    return isBool();
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

