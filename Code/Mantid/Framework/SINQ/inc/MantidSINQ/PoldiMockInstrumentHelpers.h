#ifndef POLDIMOCKINSTRUMENTHELPERS_H
#define POLDIMOCKINSTRUMENTHELPERS_H

#include "MantidSINQ/DllConfig.h"
#include <gmock/gmock.h>
#include "MantidSINQ/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiAbstractChopper.h"

#include "MantidSINQ/PoldiHeliumDetector.h"

using namespace Mantid;
using namespace Mantid::Poldi;

namespace Mantid {
namespace Poldi {

typedef std::pair<double, double> DoublePair;

class MANTID_SINQ_DLL MockDetector : public PoldiAbstractDetector
{
public:
    ~MockDetector() { }

    void loadConfiguration(DataObjects::TableWorkspace_sptr detectorConfigurationWorkspace)
    {
        UNUSED_ARG(detectorConfigurationWorkspace);
    }

    MOCK_METHOD1(twoTheta, double(int elementIndex));
    MOCK_METHOD1(distanceFromSample, double(int elementIndex));
    MOCK_METHOD0(elementCount, size_t());
    MOCK_METHOD0(centralElement, size_t());
    MOCK_METHOD2(qLimits, DoublePair(double lambdaMin, double lambdaMax));

    std::vector<int> availableElements()
    {
        std::vector<int> availableElements(400);

        int n = 0;
        std::generate(availableElements.begin(), availableElements.end(), [&n] { return n++; });

        return availableElements;
    }
};

class MANTID_SINQ_DLL ConfiguredHeliumDetector : public PoldiHeliumDetector
{
public:
    ConfiguredHeliumDetector() :
        PoldiHeliumDetector()
    {
        loadConfiguration(DataObjects::TableWorkspace_sptr(0));
    }

    void loadConfiguration(DataObjects::TableWorkspace_sptr detectorConfigurationWorkspace)
    {
        UNUSED_ARG(detectorConfigurationWorkspace);

        initializeFixedParameters(3000.0, static_cast<size_t>(400), 2.5);
        initializeCalibratedParameters(Mantid::Kernel::V2D(-931.47, -860.0), 90.41 / 180.0 * M_PI);
    }
};


class MANTID_SINQ_DLL MockChopper : public PoldiAbstractChopper
{
public:
    ~MockChopper() { }

    void loadConfiguration(DataObjects::TableWorkspace_sptr chopperConfigurationWorkspace, DataObjects::TableWorkspace_sptr chopperSlitWorkspace, DataObjects::TableWorkspace_sptr chopperSpeedWorkspace)
    {
        UNUSED_ARG(chopperConfigurationWorkspace);
        UNUSED_ARG(chopperSlitWorkspace);
        UNUSED_ARG(chopperSpeedWorkspace);
    }

    MOCK_METHOD0(rotationSpeed, double());
    MOCK_METHOD0(cycleTime, double());
    MOCK_METHOD0(zeroOffset, double());
    MOCK_METHOD0(distanceFromSample, double());

    MOCK_METHOD1(setRotationSpeed, void(double rotationSpeed));

    std::vector<double> slitPositions() {
        double slits [] = {0.000000, 0.162156};

        return std::vector<double>(slits, slits + sizeof(slits) / sizeof(slits[0]));
    }
    std::vector<double> slitTimes() {
        double slits [] = {0.000000, 243.234};

        return std::vector<double>(slits, slits + sizeof(slits) / sizeof(slits[0]));
    }
};
}
}
#endif // POLDIMOCKINSTRUMENTHELPERS_H
