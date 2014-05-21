#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include <stdexcept>

#include "MantidAPI/FunctionDomain1D.h"

namespace Mantid
{
namespace Poldi
{

using namespace DataObjects;
using namespace API;

DECLARE_FUNCTION(PoldiSpectrumDomainFunction)

PoldiSpectrumDomainFunction::PoldiSpectrumDomainFunction() :
    ParamFunction()
{

}

void PoldiSpectrumDomainFunction::initializeParametersFromWorkspace(Workspace2D_const_sptr workspace2D)
{
    PoldiInstrumentAdapter adapter(workspace2D->getInstrument(),  workspace2D->run());
    initializeFromInstrument(adapter.detector(), adapter.chopper());

    m_spectrum = boost::const_pointer_cast<const PoldiSourceSpectrum>(adapter.spectrum());
    m_deltaT = workspace2D->readX(0)[1] - workspace2D->readX(0)[0];
}

void PoldiSpectrumDomainFunction::initializeFromInstrument(PoldiAbstractDetector_sptr detector, PoldiAbstractChopper_sptr chopper)
{
    m_chopperSlitOffsets = getChopperSlitOffsets(chopper);
    //m_chopperCycleTime = chopper->cycleTime();

    m_detectorCenter = getDetectorCenterCharacteristics(detector, chopper);
    m_detectorElementData = getDetectorElementData(detector, chopper);
    m_detectorEfficiency = 0.88;
}

void PoldiSpectrumDomainFunction::setWorkspace(boost::shared_ptr<const Workspace> ws)
{
    Workspace2D_const_sptr workspace2D = boost::dynamic_pointer_cast<const Workspace2D>(ws);

    if(!workspace2D) {
        throw std::invalid_argument("PoldiSpectrumDomainFunction can only work with Workspace2D.");
    }

    initializeParametersFromWorkspace(workspace2D);
}

void PoldiSpectrumDomainFunction::function(const FunctionDomain &domain, FunctionValues &values) const
{    
    const FunctionDomain1DSpectrum &spectrumDomain = dynamic_cast<const FunctionDomain1DSpectrum &>(domain);

    try{
        spectrumDomain.getWorkspaceIndex();
    } catch(...) {
        throw std::invalid_argument("PoldiSpectrumDomainFunction can only work with FunctionDomain1DSpectrum.");
    }

    size_t index = spectrumDomain.getWorkspaceIndex();
    int domainSize = static_cast<int>(spectrumDomain.size());

    /* Parameters are given in d, but need to be processed in arrival time.
     * This section performs the conversions. They depend on several factors
     * terminated by the position in the detector, so the index is stored.
     */
    double fwhm = getParameter("Fwhm");
    double fwhmT = timeTransformedWidth(dToTOF(fwhm), index);
    double fwhmChannel = fwhmT / m_deltaT;
    double sigmaChannel = fwhmChannel / (2.0 * sqrt(2.0 * log(2.0)));

    double centre = getParameter("Centre");
    double centreTRaw = dToTOF(centre);
    double centreT = timeTransformedCentre(centreTRaw, index);

    double area = getParameter("Area");
    double areaT = timeTransformedIntensity(area, centreTRaw, index);

    /* Once all the factors are all there, the calculation needs to be
     * performed with one offset per chopper slit.
     */
    for(size_t o = 0; o < m_chopperSlitOffsets.size(); ++o) {
        double centreTOffset = centreT + m_chopperSlitOffsets[o];
        double centreTOffsetChannel = centreTOffset / m_deltaT;

        /* Calculations are performed in channel units
         * Needs to be signed integer, because the profile can extend beyond the left edge,
         * which results in negative indices. Since the spectrum "wraps around" the
         * indices have to be transformed to the right edge.
         */
        int centreChannel = static_cast<int>(centreTOffsetChannel);
        int widthChannels = std::max(2, static_cast<int>(fwhmChannel * 2.0));

        for(int i = centreChannel - widthChannels; i <= centreChannel + widthChannels; ++i) {
            /* Since the POLDI spectra "wrap around" on the time axis, the x-value of the
             * domain can not be used, because if the profile extends to x < 0, it should appear
             * at 500 - x. The same for the other side.
             */
            int cleanChannel = i % domainSize;
            if(cleanChannel < 0) {
                cleanChannel += domainSize;
            }

            double xValue = static_cast<double>(i) + 0.5;

            /* This is a workaround for later, when "actualFunction" will be replaced with
             * an arbitrary profile.
             */
            values.addToCalculated(cleanChannel, actualFunction(xValue, centreTOffsetChannel, sigmaChannel, areaT));
        }
    }
}

void PoldiSpectrumDomainFunction::init() {
    declareParameter("Area", 1.0);
    declareParameter("Fwhm", 1.0);
    declareParameter("Centre", 0.0);
}

std::vector<double> PoldiSpectrumDomainFunction::getChopperSlitOffsets(PoldiAbstractChopper_sptr chopper)
{
    const std::vector<double> &chopperSlitTimes = chopper->slitTimes();
    std::vector<double> offsets;
    offsets.reserve(chopperSlitTimes.size());
    for(std::vector<double>::const_iterator time = chopperSlitTimes.begin(); time != chopperSlitTimes.end(); ++time) {
        offsets.push_back(*time + chopper->zeroOffset());
    }

    return offsets;
}

std::vector<DetectorElementData_const_sptr> PoldiSpectrumDomainFunction::getDetectorElementData(PoldiAbstractDetector_sptr detector, PoldiAbstractChopper_sptr chopper)
{
    std::vector<DetectorElementData_const_sptr> data(detector->elementCount());

    DetectorElementCharacteristics center = getDetectorCenterCharacteristics(detector, chopper);

    for(int i = 0; i < static_cast<int>(detector->elementCount()); ++i) {
        data[i] = DetectorElementData_const_sptr(new DetectorElementData(i, center, detector, chopper));
    }

    return data;
}

DetectorElementCharacteristics PoldiSpectrumDomainFunction::getDetectorCenterCharacteristics(PoldiAbstractDetector_sptr detector, PoldiAbstractChopper_sptr chopper)
{
    return DetectorElementCharacteristics(static_cast<int>(detector->centralElement()), detector, chopper);
}

double PoldiSpectrumDomainFunction::dToTOF(double d) const
{
    return m_detectorCenter.tof1A * d;
}

double PoldiSpectrumDomainFunction::timeTransformedWidth(double widthT, size_t detectorIndex) const
{
    return widthT;// + m_detectorElementData[detectorIndex]->widthFactor() * 0.0;
}

double PoldiSpectrumDomainFunction::timeTransformedCentre(double centreT, size_t detectorIndex) const
{
    return centreT * m_detectorElementData[detectorIndex]->timeFactor();
}

double PoldiSpectrumDomainFunction::timeTransformedIntensity(double areaD, double centreT, size_t detectorIndex) const
{
    return areaD * detectorElementIntensity(centreT, detectorIndex);
}

double PoldiSpectrumDomainFunction::detectorElementIntensity(double centreT, size_t detectorIndex) const
{
    double lambda = centreT * m_detectorElementData[detectorIndex]->lambdaFactor();
    double intensity = m_spectrum->intensity(lambda) * m_detectorElementData[detectorIndex]->intensityFactor();

    return intensity * (1.0 - exp(-m_detectorEfficiency * lambda));
}

double PoldiSpectrumDomainFunction::actualFunction(double x, double x0, double sigma, double area) const
{
    return area / (sqrt(2.0 * M_PI) * sigma) * exp(-0.5 * pow((x - x0) / sigma, 2.0));
}

} // namespace Poldi
} // namespace Mantid
