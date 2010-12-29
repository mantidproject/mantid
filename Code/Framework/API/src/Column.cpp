#include "MantidAPI/Column.h"

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

} // namespace API
} // namespace Mantid

