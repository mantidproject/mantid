#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/MDUnit.h"

namespace Mantid {
namespace Geometry {

const std::string QSample::QSampleName = "QSample";


//----------------------------------------------------------------------------------------------
/** Constructor
 */
QSample::QSample() : m_unit(new Mantid::Kernel::InverseAngstromsUnit) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
QSample::~QSample() {}

Kernel::UnitLabel QSample::getUnitLabel() const
{
    return m_unit->getUnitLabel();
}

const Kernel::MDUnit &QSample::getMDUnit() const
{
    return *m_unit;
}

bool QSample::canConvertTo(const Kernel::MDUnit &otherUnit) const
{
    return this->getMDUnit() == otherUnit;
}

std::string QSample::name() const
{
    return QSampleName;
}

QSample *QSample::clone() const
{
    return new QSample;
}

} // namespace Geometry
} // namespace Mantid
