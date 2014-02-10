#include "MantidSINQ/PoldiDetectorFactory.h"

#include "MantidSINQ/PoldiHeliumDetector.h"

namespace Mantid
{
namespace Poldi
{

PoldiDetectorFactory::PoldiDetectorFactory() :
    m_newDetectorDate(from_string(std::string("2016/01/01")))
{
}

PoldiAbstractDetector *PoldiDetectorFactory::createDetector(std::string detectorType)
{
    return new PoldiHeliumDetector();
}

PoldiAbstractDetector *PoldiDetectorFactory::createDetector(date experimentDate)
{
    if(experimentDate < m_newDetectorDate) {
        return new PoldiHeliumDetector();
    }

    return 0;
}


} // namespace Poldi
} // namespace Mantid
