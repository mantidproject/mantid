#ifndef POLDIABSTRACTDETECTOR_H
#define POLDIABSTRACTDETECTOR_H

#include "MantidSINQ/DllConfig.h"

#include "MantidDataObjects/TableWorkspace.h"

#include "MantidKernel/V2D.h"

#include <utility>

namespace Mantid
{
namespace Poldi
{

using namespace Kernel;
using namespace API;

class MANTID_SINQ_DLL PoldiAbstractDetector
{
public:
    virtual ~PoldiAbstractDetector() {}

    virtual void loadConfiguration(DataObjects::TableWorkspace_sptr detectorConfigurationWorkspace) = 0;

    virtual double twoTheta(int elementIndex) = 0;
    virtual double distanceFromSample(int elementIndex) = 0;

    virtual size_t elementCount() = 0;

    virtual std::pair<double, double> qLimits(double lambdaMin, double lambdaMax) = 0;

protected:
    PoldiAbstractDetector() {}

};
}
}
#endif // POLDIABSTRACTDETECTOR_H
