#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"

#include "MantidSINQ/PoldiUtilities/PoldiDetectorFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiChopperFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"

namespace Mantid
{
namespace Poldi
{
using namespace Mantid::Geometry;
using namespace Mantid::API;

/** Constructor with required arguments
  *
  * With the Instrument and Run data provided to the constructor, the resulting POLDI utility objects are
  * created. Currently this is a detector, a chopper and the neutron source spectrum.
  * When a NULL-instrument pointer is passed in, or the chopperspeed-property is not present in the
  * experiment log, std::runtime_error is thrown.
  *
  * @param mantidInstrument :: Const shared pointer to Mantid::Geometry::Instrument
  * @param runInformation :: Const Reference to Mantid::API::Run object
  */
PoldiInstrumentAdapter::PoldiInstrumentAdapter(Instrument_const_sptr mantidInstrument, const Run &runInformation)
{
    if(!mantidInstrument) {
        throw std::runtime_error("Can not construct POLDI classes from invalid instrument. Aborting.");
    }

    if(!runInformation.hasProperty("chopperspeed")) {
        throw(std::runtime_error("Can not construct instrument without 'chopperspeed' property in log. Aborting."));
    }

    setDetector(mantidInstrument);
    setChopper(mantidInstrument, runInformation);
    setSpectrum(mantidInstrument);
}

PoldiInstrumentAdapter::~PoldiInstrumentAdapter()
{
}

/** Returns the chopper stored in the adapter
  *
  * @return Abstract chopper, configured according to instrument and run (chopperspeed).
  */
PoldiAbstractChopper_sptr PoldiInstrumentAdapter::chopper() const
{
    return m_chopper;
}

/** Returns the detector stored in the adapter
  *
  * @return Abstract detector, configured with the data in instrument.
  */
PoldiAbstractDetector_sptr PoldiInstrumentAdapter::detector() const
{
    return m_detector;
}

/** Returns the spectrum stored in the adapter
  *
  * @return PoldiSourceSpectrum, as given in the insturment configuration.
  */
PoldiSourceSpectrum_sptr PoldiInstrumentAdapter::spectrum() const
{
    return m_spectrum;
}

/** Constructs a detector and stores it
  *
  * A PoldiAbstractDetector is constructed through PoldiDetectorFactory. Currently, the He3-detector
  * is hard-coded at this place, but as soon as the new detector is available and tests
  * have been performed, this will be changed.
  *
  * @param mantidInstrument :: Mantid instrument with POLDI detector setup data.
  */
void PoldiInstrumentAdapter::setDetector(Instrument_const_sptr mantidInstrument)
{
    PoldiDetectorFactory detectorFactory;
    m_detector = PoldiAbstractDetector_sptr(detectorFactory.createDetector(std::string("helium3-detector")));
    m_detector->loadConfiguration(mantidInstrument);
}

/** Constructs a chopper and stores it
  *
  * A PoldiAbstractChopper is constructed by PoldiChopperFactory. The configuration is taken
  * from the passed instrument and the run information (for the chopper speed).
  *
  * @param mantidInstrument :: Mantid instrument with POLDI chopper information.
  * @param runInformation :: Run information that contains a "chopperspeed" property.
  */
void PoldiInstrumentAdapter::setChopper(Instrument_const_sptr mantidInstrument, const Run &runInformation)
{
    double chopperSpeed = runInformation.getPropertyValueAsType<std::vector<double> >("chopperspeed").front();

    PoldiChopperFactory chopperFactory;
    m_chopper = PoldiAbstractChopper_sptr(chopperFactory.createChopper(std::string("default-chopper")));
    m_chopper->loadConfiguration(mantidInstrument);
    m_chopper->setRotationSpeed(chopperSpeed);
}

/** Constructs a spectrum and stores it
  *
  * This method constructs a PoldiSourceSpectrum object with the spectrum data provided
  * by the instrument.
  *
  * @param mantidInstrument :: Mantid instrument containing a lookup table with a neutron wavelength spectrum.
  */
void PoldiInstrumentAdapter::setSpectrum(Instrument_const_sptr mantidInstrument)
{
    m_spectrum = PoldiSourceSpectrum_sptr(new PoldiSourceSpectrum(mantidInstrument));
}

} // namespace Poldi
} // namespace Mantid
