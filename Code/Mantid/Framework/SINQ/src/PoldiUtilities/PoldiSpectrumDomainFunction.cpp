#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include <stdexcept>

#include "MantidAPI/FunctionDomain1D.h"

namespace Mantid {
namespace Poldi {

using namespace DataObjects;
using namespace API;

DECLARE_FUNCTION(PoldiSpectrumDomainFunction)

PoldiSpectrumDomainFunction::PoldiSpectrumDomainFunction()
    : ParamFunction(), m_chopperSlitOffsets(), m_deltaT(0.0),
      m_timeTransformer(), m_2dHelpers() {}

/**
 * Sets the workspace and initializes helper data
 *
 * This method calls
 *PoldiSpectrumDomainFunction::initializeParametersFromWorkspace to
 * setup the factors required for calculation of the spectrum with given index.
 *
 * @param ws :: Workspace with valid POLDI instrument and run information
 */
void PoldiSpectrumDomainFunction::setWorkspace(
    boost::shared_ptr<const Workspace> ws) {
  Workspace2D_const_sptr workspace2D =
      boost::dynamic_pointer_cast<const Workspace2D>(ws);

  if (!workspace2D) {
    throw std::invalid_argument(
        "PoldiSpectrumDomainFunction can only work with Workspace2D.");
  }

  initializeParametersFromWorkspace(workspace2D);
}

/**
 * Performs the actual function calculation
 *
 * This method performs the necessary transformations for the parameters and
 *calculates the function
 * values for the spectrum with the index stored in domain.
 *
 * @param domain :: FunctionDomain1DSpectrum, containing a workspace index
 * @param values :: Object to store the calculated function values
 */
void PoldiSpectrumDomainFunction::function1DSpectrum(
    const API::FunctionDomain1DSpectrum &domain,
    API::FunctionValues &values) const {
  values.zeroCalculated();

  size_t index = domain.getWorkspaceIndex();
  int domainSize = static_cast<int>(domain.size());

  /* Parameters are given in d, but need to be processed in arrival time.
   * This section performs the conversions. They depend on several factors
   * terminated by the position in the detector, so the index is stored.
   */
  double fwhm = getParameter("Fwhm");
  double sigma = fwhm / (2.0 * sqrt(2.0 * log(2.0)));

  double centre = getParameter("Centre");

  Poldi2DHelper_sptr helper = m_2dHelpers[index];

  if (helper) {
    double area = getParameter("Area");

    double dWidth = 2.0 * fwhm;
    double dCalcMin = centre - dWidth;
    int dWidthN = std::min(
        50, std::max(10, 2 * static_cast<int>(dWidth / helper->deltaD) + 1));

    const double *domainBeginPointer = 0;
    int pos = 0;

    for (size_t i = 0; i < helper->domain->size(); ++i) {
      if ((*(helper->domain))[i] >= dCalcMin) {
        pos = static_cast<int>(i + 1);
        domainBeginPointer = helper->domain->getPointerAt(i + 1);
        break;
      }
    }

    if (domainBeginPointer == 0) {
      throw std::runtime_error("Cannot find d-value in domain.");
    }

    if (dWidthN < 0) {
      throw std::runtime_error("dWidth < 0");
    }

    std::vector<double> smallDomain(dWidthN);
    for (size_t i = 0; i < dWidthN; ++i) {
      smallDomain[i] = (*(helper->domain))[pos + i];
    }

    std::vector<double> factors(dWidthN);

    for (size_t i = 0; i < factors.size(); ++i) {
      factors[i] =
          m_timeTransformer->detectorElementIntensity(smallDomain[i], index);
    }

    size_t baseOffset = static_cast<size_t>(pos + helper->minTOFN);

    for (size_t i = 0; i < helper->dOffsets.size(); ++i) {
      double newD = centre + helper->dFractionalOffsets[i];
      size_t offset = static_cast<size_t>(helper->dOffsets[i]) + baseOffset;

      for (size_t j = 0; j < smallDomain.size(); ++j) {
        values.addToCalculated(
            (offset + j) % domainSize,
            actualFunction(smallDomain[j], newD, sigma, area) * factors[j]);
      }
    }
  }
}

void
PoldiSpectrumDomainFunction::poldiFunction1D(const std::vector<int> &indices,
                                             const FunctionDomain1D &domain,
                                             FunctionValues &values) const {

  double deltaD = domain[1] - domain[0];

  double fwhm = getParameter("Fwhm");

  double centre = getParameter("Centre");
  double area = getParameter("Area");

  double centreTOffsetChannel = centre / deltaD;
  int centreChannel = static_cast<int>(centreTOffsetChannel);

  int offset = static_cast<int>(domain[0] / deltaD + 0.5);

  for (auto index = indices.begin(); index != indices.end(); ++index) {
    std::vector<double> factors(domain.size());

    for (size_t i = 0; i < factors.size(); ++i) {
      factors[i] = m_timeTransformer->detectorElementIntensity(
          domain[i], static_cast<size_t>(*index));
    }
    double fwhmT = m_timeTransformer->timeTransformedWidth(fwhm, *index);
    double fwhmChannel = fwhmT / m_deltaT;
    double sigma = fwhm / (2.0 * sqrt(2.0 * log(2.0)));
    int widthChannels = std::min(
        50, std::max(10, 2 * static_cast<int>(fwhmChannel) + 1));

    for (int i = centreChannel - widthChannels;
         i <= centreChannel + widthChannels; ++i) {

      values.addToCalculated(
          i - offset,
          8.0 * actualFunction(domain[i - offset], centre, sigma, area) *
              factors[i - offset]);
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
 * @param workspace2D :: Workspace with valid POLDI instrument and required run
 *information
 */
void PoldiSpectrumDomainFunction::initializeParametersFromWorkspace(
    const Workspace2D_const_sptr &workspace2D) {
  m_deltaT = workspace2D->readX(0)[1] - workspace2D->readX(0)[0];

  PoldiInstrumentAdapter_sptr adapter =
      boost::make_shared<PoldiInstrumentAdapter>(workspace2D->getInstrument(),
                                                 workspace2D->run());
  initializeInstrumentParameters(adapter);
}

/**
 * Initializes chopper offsets and time transformer
 *
 * In this method, the instrument dependent parameter for the calculation are
 *setup, so that a PoldiTimeTransformer is
 * available to transfer parameters to the time domain using correct factors
 *etc.
 *
 * @param poldiInstrument :: PoldiInstrumentAdapter that holds chopper, detector
 *and spectrum
 */
void PoldiSpectrumDomainFunction::initializeInstrumentParameters(
    const PoldiInstrumentAdapter_sptr &poldiInstrument) {
  m_timeTransformer = boost::make_shared<PoldiTimeTransformer>(poldiInstrument);
  m_chopperSlitOffsets = getChopperSlitOffsets(poldiInstrument->chopper());

  if (!poldiInstrument) {
    throw std::runtime_error("No valid POLDI instrument.");
  }

  m_2dHelpers.clear();

  PoldiAbstractDetector_sptr detector = poldiInstrument->detector();
  PoldiAbstractChopper_sptr chopper = poldiInstrument->chopper();

  std::pair<double, double> qLimits = detector->qLimits(1.1, 5.0);

  double dMin = Conversions::qToD(qLimits.second);
  double dMax = Conversions::dToQ(qLimits.first);

  for (int i = 0; i < static_cast<int>(detector->elementCount()); ++i) {
    double sinTheta = sin(detector->twoTheta(i) / 2.0);
    double distance =
        detector->distanceFromSample(i) + chopper->distanceFromSample();
    double deltaD = Conversions::TOFtoD(m_deltaT, distance, sinTheta);

    Poldi2DHelper_sptr curr = boost::make_shared<Poldi2DHelper>();
    curr->setChopperSlitOffsets(distance, sinTheta, deltaD,
                                m_chopperSlitOffsets);
    curr->setDomain(dMin, dMax, deltaD);
    curr->deltaD = deltaD;
    curr->minTOFN = static_cast<int>(
        Conversions::dtoTOF(dMin, distance, sinTheta) / m_deltaT);

    m_2dHelpers.push_back(curr);
  }
}

/**
 * Adds the zero-offset of the chopper to the slit times
 *
 * @param chopper :: PoldiAbstractChopper with slit times, not corrected with
 *zero-offset
 * @return vector with zero-offset-corrected chopper slit times
 */
std::vector<double> PoldiSpectrumDomainFunction::getChopperSlitOffsets(
    const PoldiAbstractChopper_sptr &chopper) {
  const std::vector<double> &chopperSlitTimes = chopper->slitTimes();
  std::vector<double> offsets;
  offsets.reserve(chopperSlitTimes.size());
  for (std::vector<double>::const_iterator time = chopperSlitTimes.begin();
       time != chopperSlitTimes.end(); ++time) {
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
double PoldiSpectrumDomainFunction::actualFunction(double x, double x0,
                                                   double sigma,
                                                   double area) const {
  return area / (sqrt(2.0 * M_PI) * sigma) *
         exp(-0.5 * pow((x - x0) / sigma, 2.0));
}

} // namespace Poldi
} // namespace Mantid
