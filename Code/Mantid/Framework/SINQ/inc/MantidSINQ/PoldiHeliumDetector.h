#ifndef POLDIHELIUMDETECTOR_H
#define POLDIHELIUMDETECTOR_H

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiAbstractDetector.h"

#include "MantidKernel/V2D.h"

namespace Mantid {
namespace Poldi {

class MANTID_SINQ_DLL PoldiHeliumDetector : public PoldiAbstractDetector
{
public:
    PoldiHeliumDetector();
    ~PoldiHeliumDetector() {}

    void loadConfiguration(DataObjects::TableWorkspace_sptr detectorConfigurationWorkspace);

    double twoTheta(int elementIndex);
    double distanceFromSample(int elementIndex);

    size_t elementCount();

    std::pair<double, double> qLimits(double lambdaMin, double lambdaMax);

protected:
    double phi(int elementIndex);
    double phi(double twoTheta);

    void initializeFixedParameters(double radius, size_t elementCount, double elementWidth);
    void initializeCalibratedParameters(V2D position, double centerTwoTheta);

    /* These detector parameters are fixed and specific to the geometry or result from it directly */
    double m_radius;
    size_t m_elementCount;
    double m_elementWidth;
    double m_angularResolution;
    double m_totalOpeningAngle;

    /* Parameters that are calibrated or depend on calibrated parameters */
    V2D m_calibratedPosition;
    double m_vectorAngle;
    double m_distanceFromSample;

    double m_calibratedCenterTwoTheta;
    double m_phiCenter;
    double m_phiStart;
};

}
}

#endif // POLDIHELIUMDETECTOR_H
