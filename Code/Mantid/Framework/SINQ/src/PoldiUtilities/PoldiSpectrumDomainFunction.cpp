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
      m_timeTransformer(), m_2dHelpers(), m_profileFunction() {}

/**
 * Sets the workspace and initializes helper data
 *
 * This method calls
 * PoldiSpectrumDomainFunction::initializeParametersFromWorkspace to
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
 * This method calculates a peak profile and transforms the resulting function
 * values from the d-based domain into the desired arrival time based domain,
 * using Poldi2DHelper.
 *
 * @param domain :: FunctionDomain1DSpectrum, containing a workspace index
 * @param values :: Object to store the calculated function values
 */
void PoldiSpectrumDomainFunction::function1DSpectrum(
    const API::FunctionDomain1DSpectrum &domain,
    API::FunctionValues &values) const {
  values.zeroCalculated();

  size_t index = domain.getWorkspaceIndex();
  Poldi2DHelper_sptr helper = m_2dHelpers[index];

  if (helper) {
    int domainSize = static_cast<int>(domain.size());

    double fwhm = m_profileFunction->fwhm();
    double centre = m_profileFunction->centre();

    double dWidth = 2.0 * fwhm;
    double dCalcMin = centre - dWidth;
    size_t dWidthN = static_cast<size_t>(std::min(
        50, std::max(10, 2 * static_cast<int>(dWidth / helper->deltaD) + 1)));

    int pos = 0;

    for (size_t i = 0; i < helper->domain->size(); ++i) {
      if ((*(helper->domain))[i] >= dCalcMin) {
        pos = static_cast<int>(i + 1);
        break;
      }
    }

    std::vector<double> localOut(dWidthN, 0.0);

    size_t baseOffset = static_cast<size_t>(pos + helper->minTOFN);

    for (size_t i = 0; i < helper->dOffsets.size(); ++i) {
      double newD = centre + helper->dFractionalOffsets[i];
      size_t offset = static_cast<size_t>(helper->dOffsets[i]) + baseOffset;

      m_profileFunction->setCentre(newD);
      m_profileFunction->functionLocal(
          &localOut[0], helper->domain->getPointerAt(pos), dWidthN);

      for (size_t j = 0; j < dWidthN; ++j) {
        values.addToCalculated((offset + j) % domainSize,
                               localOut[j] * helper->factors[pos + j]);
      }
    }

    m_profileFunction->setCentre(centre);
  }
}

/**
 * Calculates derivatives
 *
 * The method calculates derivatives of the wrapped profile function and
 * transforms them into the correct domain.
 *
 * @param domain :: FunctionDomain1DSpectrum, containing a workspace index
 * @param jacobian :: Jacobian matrix.
 */
void PoldiSpectrumDomainFunction::functionDeriv1DSpectrum(
    const FunctionDomain1DSpectrum &domain, Jacobian &jacobian) {
  size_t index = domain.getWorkspaceIndex();
  Poldi2DHelper_sptr helper = m_2dHelpers[index];

  if (helper) {
    int domainSize = static_cast<int>(domain.size());

    double fwhm = m_profileFunction->fwhm();
    double centre = m_profileFunction->centre();

    double dWidth = 2.0 * fwhm;
    double dCalcMin = centre - dWidth;
    size_t dWidthN = static_cast<size_t>(std::min(
        50, std::max(10, 2 * static_cast<int>(dWidth / helper->deltaD) + 1)));

    int pos = 0;

    for (size_t i = 0; i < helper->domain->size(); ++i) {
      if ((*(helper->domain))[i] >= dCalcMin) {
        pos = static_cast<int>(i + 1);
        break;
      }
    }

    size_t np = m_profileFunction->nParams();

    size_t baseOffset = static_cast<size_t>(pos + helper->minTOFN);

    for (size_t i = 0; i < helper->dOffsets.size(); ++i) {
      LocalJacobian smallJ(dWidthN, np);

      double newD = centre + helper->dFractionalOffsets[i];
      size_t offset = static_cast<size_t>(helper->dOffsets[i]) + baseOffset;

      m_profileFunction->setCentre(newD);

      m_profileFunction->functionDerivLocal(
          &smallJ, helper->domain->getPointerAt(pos), dWidthN);

      for (size_t j = 0; j < dWidthN; ++j) {
        size_t off = (offset + j) % domainSize;
        for (size_t p = 0; p < np; ++p) {
          jacobian.set(off, p,
                       jacobian.get(off, p) +
                           smallJ.getRaw(j, p) * helper->factors[pos + j]);
        }
      }
    }

    m_profileFunction->setCentre(centre);
  }
}

void
PoldiSpectrumDomainFunction::poldiFunction1D(const std::vector<int> &indices,
                                             const FunctionDomain1D &domain,
                                             FunctionValues &values) const {

  FunctionValues localValues(domain);

  m_profileFunction->functionLocal(localValues.getPointerToCalculated(0),
                                   domain.getPointerAt(0), domain.size());

  double chopperSlitCount = static_cast<double>(m_chopperSlitOffsets.size());

  for (auto index = indices.begin(); index != indices.end(); ++index) {
    std::vector<double> factors(domain.size());

    for (size_t i = 0; i < factors.size(); ++i) {
      values.addToCalculated(i,
                             chopperSlitCount * localValues[i] *
                                 m_timeTransformer->detectorElementIntensity(
                                     domain[i], static_cast<size_t>(*index)));
    }
  }
}

/// Sets the active parameter i on the wrapped IPeakFunction.
void PoldiSpectrumDomainFunction::setActiveParameter(size_t i, double value) {
  m_profileFunction->setActiveParameter(i, value);
}

/// Returns the value of the active parameter i of the wrapped IPeakFunction.
double PoldiSpectrumDomainFunction::activeParameter(size_t i) const {
  return m_profileFunction->activeParameter(i);
}

/// Sets the i-th parameter of the wrapped IPeakFunction.
void PoldiSpectrumDomainFunction::setParameter(size_t i, const double &value,
                                               bool explicitlySet) {
  m_profileFunction->setParameter(i, value, explicitlySet);
}

/// Sets the value of the named parameter of the wrapped IPeakFunction.
void PoldiSpectrumDomainFunction::setParameter(const std::string &name,
                                               const double &value,
                                               bool explicitlySet) {
  ParamFunction::setParameter(name, value, explicitlySet);
}

/// Returns the value of the i-th parameter of the wrapped IPeakFunction.
double PoldiSpectrumDomainFunction::getParameter(size_t i) const {
  return m_profileFunction->getParameter(i);
}

/// Returns the value of the named parameter of the wrapped IPeakFunction.
double
PoldiSpectrumDomainFunction::getParameter(const std::string &name) const {
  return getParameter(parameterIndex(name));
}

/**
 * @brief Sets the attribute value and constructs a wrapped profile function
 *
 * If the "ProfileFunction"-attribute is set, all parameters of this function
 * are cleared and the new profile function is created. If successfull, its
 * parameters are exposed.
 *
 * @param attName :: Name of the attribute.
 * @param attValue :: Value of the attribute.
 */
void PoldiSpectrumDomainFunction::setAttribute(
    const std::string &attName, const IFunction::Attribute &attValue) {
  ParamFunction::setAttribute(attName, attValue);

  if (attName == "ProfileFunction") {
    clearAllParameters();

    setProfileFunction(attValue.asString());
    exposeFunctionParameters(getProfileFunction());
  }
}

/// Sets the wrapped profile function and constructs an instance of it.
void PoldiSpectrumDomainFunction::setProfileFunction(
    const std::string &profileFunctionName) {
  try {
    m_profileFunction = boost::dynamic_pointer_cast<IPeakFunction>(
        FunctionFactory::Instance().createFunction(profileFunctionName));
  }
  catch (std::runtime_error) {
    throw std::invalid_argument("Could not construct function from name " +
                                profileFunctionName + ".");
  }
}

/// Returns a smart pointer to the wrapped profile function.
IPeakFunction_sptr PoldiSpectrumDomainFunction::getProfileFunction() const {
  return m_profileFunction;
}

/// Exposes the parameters of the supplied function as if they were declared
/// on this function.
void PoldiSpectrumDomainFunction::exposeFunctionParameters(
    const IFunction_sptr &function) {

  if (!function) {
    throw std::invalid_argument(
        "Cannot expose parameters of null function pointer.");
  }

  for (size_t i = 0; i < function->nParams(); ++i) {
    declareParameter(function->parameterName(i));
  }
}

/// Initialize ProfileFunction attribute.
void PoldiSpectrumDomainFunction::init() {
  declareAttribute("ProfileFunction", Attribute(""));
}

/**
 * Extracts the time difference as well as instrument information
 *
 * @param workspace2D :: Workspace with valid POLDI instrument and required
 *                       run information
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
 * setup, so that a PoldiTimeTransformer is available to transfer parameters to
 * the time domain using correct factors etc.
 *
 * @param poldiInstrument :: Valid PoldiInstrumentAdapter
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
    curr->setFactors(m_timeTransformer, static_cast<size_t>(i));

    m_2dHelpers.push_back(curr);
  }
}

/**
 * Adds the zero-offset of the chopper to the slit times
 *
 * @param chopper :: PoldiAbstractChopper with slit times, not corrected with
 *                   zero-offset
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

} // namespace Poldi
} // namespace Mantid
