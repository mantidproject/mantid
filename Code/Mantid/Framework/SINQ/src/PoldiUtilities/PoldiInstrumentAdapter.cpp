#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"

#include "MantidSINQ/PoldiUtilities/PoldiDetectorFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiChopperFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"

#include "boost/assign.hpp"
#include "boost/make_shared.hpp"

namespace Mantid {
namespace Poldi {
using namespace Mantid::Geometry;
using namespace Mantid::API;

// Initializing static variables for DoubleValueExtractors
const std::string PoldiInstrumentAdapter::m_chopperSpeedPropertyName =
    "chopperspeed";
const std::string PoldiInstrumentAdapter::m_chopperSpeedTargetPropertyName =
    "ChopperSpeedTarget";

std::map<std::string, AbstractDoubleValueExtractor_sptr>
    PoldiInstrumentAdapter::m_extractors = boost::assign::map_list_of(
        "dbl list", boost::static_pointer_cast<AbstractDoubleValueExtractor>(
                        boost::make_shared<VectorDoubleValueExtractor>()))(
        "int list", boost::static_pointer_cast<AbstractDoubleValueExtractor>(
                        boost::make_shared<VectorIntValueExtractor>()))(
        "number", boost::static_pointer_cast<AbstractDoubleValueExtractor>(
                      boost::make_shared<NumberDoubleValueExtractor>()));

/** Constructor with workspace argument
  *
  * This constructor directly takes a matrix workspace and extracts instrument
  *and run information.
  *
  * @param matrixWorkspace :: Workspace with a valid POLDI instrument and run
  *information
  */
PoldiInstrumentAdapter::PoldiInstrumentAdapter(
    const MatrixWorkspace_const_sptr &matrixWorkspace) {
  initializeFromInstrumentAndRun(matrixWorkspace->getInstrument(),
                                 matrixWorkspace->run());
}

/** Constructor with instrument and run information arguments
  *
  * This constructor internall calls
  *PoldiInstrumentAdapter::initializeFromInstrumentAndRun.
  *
  * @param mantidInstrument :: Const shared pointer to
  *Mantid::Geometry::Instrument
  * @param runInformation :: Const Reference to Mantid::API::Run object
  */
PoldiInstrumentAdapter::PoldiInstrumentAdapter(
    const Instrument_const_sptr &mantidInstrument, const Run &runInformation) {
  initializeFromInstrumentAndRun(mantidInstrument, runInformation);
}

PoldiInstrumentAdapter::~PoldiInstrumentAdapter() {}

/** Returns the chopper stored in the adapter
  *
  * @return Abstract chopper, configured according to instrument and run
  *(chopperspeed).
  */
PoldiAbstractChopper_sptr PoldiInstrumentAdapter::chopper() const {
  return m_chopper;
}

/** Returns the detector stored in the adapter
  *
  * @return Abstract detector, configured with the data in instrument.
  */
PoldiAbstractDetector_sptr PoldiInstrumentAdapter::detector() const {
  return m_detector;
}

/** Returns the spectrum stored in the adapter
  *
  * @return PoldiSourceSpectrum, as given in the insturment configuration.
  */
PoldiSourceSpectrum_sptr PoldiInstrumentAdapter::spectrum() const {
  return m_spectrum;
}

/** Initializes object from POLDI instrument definition and run information
  *
  * With the Instrument and Run data provided in the arguments, the resulting
  *POLDI utility objects are
  * created. Currently this is a detector, a chopper and the neutron source
  *spectrum.
  * When a NULL-instrument pointer is passed in, or the chopperspeed-property is
  *not present in the
  * experiment log, std::runtime_error is thrown.
  *
  * @param mantidInstrument :: Const shared pointer to
  *Mantid::Geometry::Instrument
  * @param runInformation :: Const Reference to Mantid::API::Run object
  */
void PoldiInstrumentAdapter::initializeFromInstrumentAndRun(
    const Instrument_const_sptr &mantidInstrument, const Run &runInformation) {
  if (!mantidInstrument) {
    throw std::runtime_error(
        "Can not construct POLDI classes from invalid instrument. Aborting.");
  }

  setDetector(mantidInstrument);
  setChopper(mantidInstrument, runInformation);
  setSpectrum(mantidInstrument);
}

/** Constructs a detector and stores it
  *
  * A PoldiAbstractDetector is constructed through PoldiDetectorFactory.
  *Currently, the He3-detector
  * is hard-coded at this place, but as soon as the new detector is available
  *and tests
  * have been performed, this will be changed.
  *
  * @param mantidInstrument :: Mantid instrument with POLDI detector setup data.
  */
void PoldiInstrumentAdapter::setDetector(
    const Instrument_const_sptr &mantidInstrument) {
  PoldiDetectorFactory detectorFactory;
  m_detector = PoldiAbstractDetector_sptr(
      detectorFactory.createDetector(std::string("helium3-detector")));
  m_detector->loadConfiguration(mantidInstrument);
}

/** Constructs a chopper and stores it
  *
  * A PoldiAbstractChopper is constructed by PoldiChopperFactory. The
  *configuration is taken
  * from the passed instrument and the run information (for the chopper speed).
  *
  * @param mantidInstrument :: Mantid instrument with POLDI chopper information.
  * @param runInformation :: Run information that contains a "chopperspeed"
  *property.
  */
void PoldiInstrumentAdapter::setChopper(
    const Instrument_const_sptr &mantidInstrument, const Run &runInformation) {
  double rawChopperSpeed = getChopperSpeedFromRun(runInformation);
  double chopperSpeed = getCleanChopperSpeed(rawChopperSpeed);

  if (!chopperSpeedMatchesTarget(runInformation, chopperSpeed)) {
    throw std::invalid_argument("Chopper speed deviates from target speed.");
  }

  PoldiChopperFactory chopperFactory;
  m_chopper = PoldiAbstractChopper_sptr(
      chopperFactory.createChopper(std::string("default-chopper")));
  m_chopper->loadConfiguration(mantidInstrument);
  m_chopper->setRotationSpeed(chopperSpeed);
}

/**
 * Returns a plausible chopper speed
 *
 * Sometimes the measured chopper speed is off by a few ms, which is a result of
 * the measuring method. Since only multiples of 500 are allowed, the speed is
 * rounded to the next multiple of 500.
 *
 * @param rawChopperSpeed :: Raw chopper rotation speed as found in the HDF-file
 * @return Next multiple of 500.
 */
double
PoldiInstrumentAdapter::getCleanChopperSpeed(double rawChopperSpeed) const {
  return floor((rawChopperSpeed + 250.0) / 500.0) * 500.0;
}

/**
 * Extracts the chopper speed from run information
 *
 * This method tries to extract the chopper rotation speed from the run
 *information,
 * using extractPropertyFromRun with the correct property name.
 *
 * @param runInformation :: Run information that contains a property with the
 *chopper speed.
 * @return Chopper speed as stored in run information
 */
double PoldiInstrumentAdapter::getChopperSpeedFromRun(
    const Run &runInformation) const {
  return extractPropertyFromRun(runInformation, m_chopperSpeedPropertyName);
}

/**
 * Extracts the target chopper speed from run information
 *
 * Tries to extract the target chopper speed from run information.
 *
 * @param runInformation :: Run information that contains a property with the
 *target chopper speed.
 * @return Target chopper speed as stored in run information
 */
double PoldiInstrumentAdapter::getChopperSpeedTargetFromRun(
    const Run &runInformation) const {
  return extractPropertyFromRun(runInformation,
                                m_chopperSpeedTargetPropertyName);
}

/**
 * Checks if the chopper speed matches the target value
 *
 * If runInformation contains the chopper target speed, this method checks
 *whether the actural speed
 * agrees with the target.
 *
 * If the information is not present, it's most likely an older data file, where
 *this information
 * is not available. In this case it is not possible to compare anything, so the
 *function returns
 * true for any value of chopperSpeed.
 *
 * @param runInformation :: Run information that may or may not contain
 *information about chopper target speed.
 * @param chopperSpeed :: Chopper speed, "cleaned" by
 *PoldiInstrumentAdapter::getCleanChopperSpeed.
 * @return True if speed matches target or information is absent, false
 *otherwise.
 */
bool
PoldiInstrumentAdapter::chopperSpeedMatchesTarget(const Run &runInformation,
                                                  double chopperSpeed) const {
  try {
    double targetChopperSpeed = getChopperSpeedTargetFromRun(runInformation);

    if (fabs(targetChopperSpeed - chopperSpeed) > 1e-4) {
      return false;
    }

    return true;
  } catch (std::runtime_error) {
    // Old data files don't have information on target chopper speed. Do
    // nothing.
    return true;
  }
}

/**
 * Extracts a property from run information
 *
 * This method extracts the property with the supplied name from the run
 *information,
 * using the appropriate AbstractDoubleValueExtractor. If the property is not
 *found,
 * an std::runtime_error exception is thrown. If there is no extractor for the
 *requested
 * data type, an std::invalid_argument exception is thrown.
 *
 * @param runInformation :: Run information that cotains the property with
 *propertyName
 * @param propertyName :: Property name that should be extracted
 * @return Value of property as double
 */
double PoldiInstrumentAdapter::extractPropertyFromRun(
    const Run &runInformation, const std::string &propertyName) const {
  if (!runInformation.hasProperty(propertyName)) {
    throw std::runtime_error("Cannot construct instrument without " +
                             propertyName + "-property in log. Aborting.");
  }

  Kernel::Property *property = runInformation.getProperty(propertyName);

  AbstractDoubleValueExtractor_sptr extractor =
      getExtractorForProperty(property);

  if (!extractor) {
    throw std::invalid_argument(
        "Cannot extract chopper speed from run information.");
  }

  return (*extractor)(runInformation, propertyName);
}

/**
 * Returns appropriate extractor for supplied property
 *
 * This method checks the property's type and gets the appropriate functor to
 *extract the value.
 * If a null pointer is supplied, the method throws an std::runtime_error
 *exception.
 *
 * @param chopperSpeedProperty :: Property containing the chopper speed
 * @return Functor of type AbstractDoubleValueExtractor
 */
AbstractDoubleValueExtractor_sptr
PoldiInstrumentAdapter::getExtractorForProperty(
    Kernel::Property *chopperSpeedProperty) const {
  if (!chopperSpeedProperty) {
    throw std::invalid_argument("Cannot process null-Property.");
  }

  std::string propertyType = chopperSpeedProperty->type();

  return m_extractors[propertyType];
}

/** Constructs a spectrum and stores it
  *
  * This method constructs a PoldiSourceSpectrum object with the spectrum data
  *provided
  * by the instrument.
  *
  * @param mantidInstrument :: Mantid instrument containing a lookup table with
  *a neutron wavelength spectrum.
  */
void PoldiInstrumentAdapter::setSpectrum(
    const Instrument_const_sptr &mantidInstrument) {
  m_spectrum = boost::make_shared<PoldiSourceSpectrum>(mantidInstrument);
}

} // namespace Poldi
} // namespace Mantid
