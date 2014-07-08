#include "MantidSINQ/PoldiUtilities/PoldiTimeTransformer.h"
#include "boost/make_shared.hpp"

namespace Mantid
{
namespace Poldi
{

PoldiTimeTransformer::PoldiTimeTransformer() :
    m_detectorCenter(),
    m_detectorElementData(),
    m_detectorEfficiency(0.0),
    m_chopperSlits(0),
    m_spectrum()
{
}

PoldiTimeTransformer::PoldiTimeTransformer(const PoldiInstrumentAdapter_sptr &poldiInstrument)
{
    initializeFromPoldiInstrument(poldiInstrument);
}

void PoldiTimeTransformer::initializeFromPoldiInstrument(const PoldiInstrumentAdapter_sptr &poldiInstrument)
{
    if(!poldiInstrument) {
        throw std::invalid_argument("Cannot initialize PoldiTimeTransformer from null-instrument.");
    }

    PoldiAbstractDetector_sptr detector = poldiInstrument->detector();
    PoldiAbstractChopper_sptr chopper = poldiInstrument->chopper();

    m_spectrum = boost::const_pointer_cast<const PoldiSourceSpectrum>(poldiInstrument->spectrum());

    m_detectorCenter = getDetectorCenterCharacteristics(detector, chopper);
    m_detectorElementData = getDetectorElementData(detector, chopper);
    m_detectorEfficiency = detector->efficiency();

    m_chopperSlits = chopper->slitPositions().size();
}

double PoldiTimeTransformer::dToTOF(double d) const
{
    return m_detectorCenter.tof1A * d;
}

double PoldiTimeTransformer::adjustedWidth(double widthT, size_t detectorIndex) const
{
    UNUSED_ARG(detectorIndex);

    return widthT;// + m_detectorElementData[detectorIndex]->widthFactor() * 0.0;
}

double PoldiTimeTransformer::adjustedCentre(double centreT, size_t detectorIndex) const
{
    return centreT * m_detectorElementData[detectorIndex]->timeFactor();
}

double PoldiTimeTransformer::adjustedIntensity(double area, double centreT, size_t detectorIndex) const
{
    return area * detectorElementIntensity(centreT, detectorIndex);
}

double PoldiTimeTransformer::detectorElementIntensity(double centreT, size_t detectorIndex) const
{
    double lambda = centreT * m_detectorElementData[detectorIndex]->lambdaFactor();
    double intensity = m_spectrum->intensity(lambda) * m_detectorElementData[detectorIndex]->intensityFactor();

    return intensity * (1.0 - exp(-m_detectorEfficiency * lambda));
}

double PoldiTimeTransformer::calculatedTotalIntensity(double centreT) const
{
    double sum = 0.0;
    double chopperSlitFactor = static_cast<double>(m_chopperSlits);

    for(size_t i = 0; i < m_detectorElementData.size(); ++i) {
        sum += chopperSlitFactor * detectorElementIntensity(centreT, i);
    }

    return sum;
}

size_t PoldiTimeTransformer::detectorElementCount() const
{
    return m_detectorElementData.size();
}

std::vector<DetectorElementData_const_sptr> PoldiTimeTransformer::getDetectorElementData(const PoldiAbstractDetector_sptr &detector, const PoldiAbstractChopper_sptr &chopper)
{
    std::vector<DetectorElementData_const_sptr> data(detector->elementCount());

    DetectorElementCharacteristics center = getDetectorCenterCharacteristics(detector, chopper);

    for(int i = 0; i < static_cast<int>(detector->elementCount()); ++i) {
        data[i] = boost::make_shared<DetectorElementData>(i, center, detector, chopper);
    }

    return data;
}

DetectorElementCharacteristics PoldiTimeTransformer::getDetectorCenterCharacteristics(const PoldiAbstractDetector_sptr &detector, const PoldiAbstractChopper_sptr &chopper)
{
    return DetectorElementCharacteristics(static_cast<int>(detector->centralElement()), detector, chopper);
}


} // namespace Poldi
} // namespace Mantid
