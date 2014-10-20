#include "MantidGeometry/Crystal/IScatterer.h"

namespace Mantid
{
namespace Geometry
{

/// Default constructor.
IScatterer::IScatterer(const Kernel::V3D &position) :
    m_position(position)
{
}

/// Sets the position of the scatterer to the supplied coordinates.
void IScatterer::setPosition(const Kernel::V3D &position)
{
    m_position = position;
}

/// Returns the position of the scatterer.
Kernel::V3D IScatterer::getPosition() const
{
    return m_position;
}



} // namespace Geometry
} // namespace Mantid
