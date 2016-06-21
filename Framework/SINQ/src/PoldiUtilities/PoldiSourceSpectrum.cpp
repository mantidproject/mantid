#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/FitParameter.h"

namespace Mantid {
namespace Poldi {

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

PoldiSourceSpectrum::PoldiSourceSpectrum(Interpolation spectrum)
    : m_spectrum(spectrum) {}

PoldiSourceSpectrum::PoldiSourceSpectrum(Instrument_const_sptr poldiInstrument)
    : m_spectrum() {
  setSpectrumFromInstrument(poldiInstrument);
}

/** Returns the interpolated intensity at the given wavelength
  *
  * @param wavelength :: Wavelength for which the intensity is required
  * @return Intensity at supplied wavelength.
  */
double PoldiSourceSpectrum::intensity(double wavelength) const {
  return std::max(0.0, m_spectrum.value(wavelength));
}

/** Extracts the source spectrum from an Instrument
  *
  * The spectrum is extracted from an instrument pointer and stored internally
  *as an
  * Interpolation object. In case the spectrum is not present or the
  *Interpolation-object
  * can not be constructed properly, the method throws std::runtime_error.
  *
  * @param poldiInstrument :: Pointer to valid POLDI instrument with configured
  *spectrum.
  */
void PoldiSourceSpectrum::setSpectrumFromInstrument(
    Instrument_const_sptr poldiInstrument) {
  IComponent_const_sptr source = getSourceComponent(poldiInstrument);

  Parameter_sptr spectrumParameter =
      getSpectrumParameter(source, poldiInstrument->getParameterMap());

  setSpectrum(spectrumParameter);
}

/** Returns the "source" component from the instrument.
  *
  * Tries to extract the source from the given instrument. If it's not present,
  *the method throws
  * std::runtime_error.
  *
  * @param poldiInstrument :: Instrument with valid POLDI definition
  * @return Shared pointer to source component
  */
IComponent_const_sptr
PoldiSourceSpectrum::getSourceComponent(Instrument_const_sptr poldiInstrument) {
  IComponent_const_sptr source = poldiInstrument->getComponentByName("source");

  if (!source) {
    throw std::runtime_error(
        "Instrument does not contain a neutron source definition.");
  }

  return source;
}

/** Extracts WavelengthDistribution parameter from source component, given the
  *parameter map of the corresponding instrument.
  *
  * This method extracts the wavelength spectrum of the source and returns the
  *corresponding parameter. If the parameter
  * is not found, the method throws std::runtime_error.
  *
  * @param source :: Shared pointer to source component
  * @param instrumentParameterMap :: Shared pointer to instrument's parameter
  *map
  * @return Shared pointer to Parameter that contains the spectrum.
  */
Parameter_sptr PoldiSourceSpectrum::getSpectrumParameter(
    IComponent_const_sptr source, ParameterMap_sptr instrumentParameterMap) {
  Parameter_sptr spectrumParameter = instrumentParameterMap->getRecursive(
      &(*source), "WavelengthDistribution", "fitting");

  if (!spectrumParameter) {
    throw std::runtime_error(
        "WavelengthDistribution could not be extracted from source component.");
  }

  return spectrumParameter;
}

/** Sets the spectrum given a parameter
  *
  * This methods actually sets the spectrum from a given fitting parameter that
  *contains a lookup table. If there is no lookup table,
  * the method throws std::runtime_error.
  *
  * @param spectrumParameter :: Shared pointer to fitting parameter with lookup
  *table
  */
void PoldiSourceSpectrum::setSpectrum(Parameter_sptr spectrumParameter) {
  if (!spectrumParameter) {
    throw std::runtime_error("Spectrum parameter pointer is null");
  }

  try {
    const FitParameter &spectrum = spectrumParameter->value<FitParameter>();

    m_spectrum = spectrum.getLookUpTable();
  } catch (...) {
    throw std::runtime_error(
        "PoldiSourceSpectrum could not be initialized properly.");
  }
}
}
}
