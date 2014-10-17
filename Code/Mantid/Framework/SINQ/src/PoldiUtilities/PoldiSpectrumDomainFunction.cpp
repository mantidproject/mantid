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
    ParamFunction(),
    m_chopperSlitOffsets(),
    m_deltaT(0.0),
    m_timeTransformer()
{

}

/**
 * Sets the workspace and initializes helper data
 *
 * This method calls PoldiSpectrumDomainFunction::initializeParametersFromWorkspace to
 * setup the factors required for calculation of the spectrum with given index.
 *
 * @param ws :: Workspace with valid POLDI instrument and run information
 */
void PoldiSpectrumDomainFunction::setWorkspace(boost::shared_ptr<const Workspace> ws)
{
    Workspace2D_const_sptr workspace2D = boost::dynamic_pointer_cast<const Workspace2D>(ws);

    if(!workspace2D) {
        throw std::invalid_argument("PoldiSpectrumDomainFunction can only work with Workspace2D.");
    }

    initializeParametersFromWorkspace(workspace2D);
}

/**
 * Performs the actual function calculation
 *
 * This method performs the necessary transformations for the parameters and calculates the function
 * values for the spectrum with the index stored in domain.
 *
 * @param domain :: FunctionDomain1DSpectrum, containing a workspace index
 * @param values :: Object to store the calculated function values
 */
void PoldiSpectrumDomainFunction::function1DSpectrum(const API::FunctionDomain1DSpectrum &domain, API::FunctionValues &values) const
{
    values.zeroCalculated();

    size_t index = domain.getWorkspaceIndex();
    int domainSize = static_cast<int>(domain.size());

    /* Parameters are given in d, but need to be processed in arrival time.
     * This section performs the conversions. They depend on several factors
     * terminated by the position in the detector, so the index is stored.
     */
    double fwhm = getParameter("Fwhm");
    double fwhmT = m_timeTransformer->timeTransformedWidth(fwhm, index);
    double fwhmChannel = fwhmT / m_deltaT;
    double sigmaChannel = fwhmChannel / (2.0 * sqrt(2.0 * log(2.0)));

    double centre = getParameter("Centre");
    double centreT = m_timeTransformer->timeTransformedCentre(centre, index);

    double area = getParameter("Area");
    double areaT = m_timeTransformer->timeTransformedIntensity(area, centre, index);

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

/**
 * Initializes function parameters
 */
void PoldiSpectrumDomainFunction::init() {
    declareParameter("Area", 1.0);
    declareParameter("Fwhm", 1.0);
    declareParameter("Centre", 0.0);
}

/**
 * Extracts the time difference as well as instrument information
 *
 * @param workspace2D :: Workspace with valid POLDI instrument and required run information
 */
void PoldiSpectrumDomainFunction::initializeParametersFromWorkspace(const Workspace2D_const_sptr &workspace2D)
{
    m_deltaT = workspace2D->readX(0)[1] - workspace2D->readX(0)[0];

    PoldiInstrumentAdapter_sptr adapter = boost::make_shared<PoldiInstrumentAdapter>(workspace2D->getInstrument(),  workspace2D->run());
    initializeInstrumentParameters(adapter);
 }

/**
 * Initializes chopper offsets and time transformer
 *
 * In this method, the instrument dependent parameter for the calculation are setup, so that a PoldiTimeTransformer is
 * available to transfer parameters to the time domain using correct factors etc.
 *
 * @param poldiInstrument :: PoldiInstrumentAdapter that holds chopper, detector and spectrum
 */
void PoldiSpectrumDomainFunction::initializeInstrumentParameters(const PoldiInstrumentAdapter_sptr &poldiInstrument)
{
    m_timeTransformer = boost::make_shared<PoldiTimeTransformer>(poldiInstrument);
    m_chopperSlitOffsets = getChopperSlitOffsets(poldiInstrument->chopper());

}

/**
 * Adds the zero-offset of the chopper to the slit times
 *
 * @param chopper :: PoldiAbstractChopper with slit times, not corrected with zero-offset
 * @return vector with zero-offset-corrected chopper slit times
 */
std::vector<double> PoldiSpectrumDomainFunction::getChopperSlitOffsets(const PoldiAbstractChopper_sptr &chopper)
{
    const std::vector<double> &chopperSlitTimes = chopper->slitTimes();
    std::vector<double> offsets;
    offsets.reserve(chopperSlitTimes.size());
    for(std::vector<double>::const_iterator time = chopperSlitTimes.begin(); time != chopperSlitTimes.end(); ++time) {
        offsets.push_back(*time + chopper->zeroOffset());
    }

    return offsets;
}

/**
 * Profile function
 *
 * This is the actual profile function. Currently this is a Gaussian.
 *
 * @param x :: x-value for which y is to be calculated, in channel units
 * @param x0 :: Centre of the peak, in channel units
 * @param sigma :: Sigma-parameter of Gaussian distribution, in channel units
 * @param area :: Area parameter
 * @return Function value at position x
 */
double PoldiSpectrumDomainFunction::actualFunction(double x, double x0, double sigma, double area) const
{
    return area / (sqrt(2.0 * M_PI) * sigma) * exp(-0.5 * pow((x - x0) / sigma, 2.0));
}

} // namespace Poldi
} // namespace Mantid
