// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/LeanElasticPeak.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidGeometry/Surfaces/LineIntersectVisit.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"

#include "boost/make_shared.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid {
namespace DataObjects {

//----------------------------------------------------------------------------------------------
/** Default constructor */
LeanElasticPeak::LeanElasticPeak()
    : BasePeak(), m_Qsample(V3D(0, 0, 0)), m_wavelength(0.) {}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the sample frame)
 * and a goniometer rotation matrix.
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in
 *the sample frame (goniometer rotation accounted for).
 */
LeanElasticPeak::LeanElasticPeak(const Mantid::Kernel::V3D &QSampleFrame)
    : BasePeak() {
  this->setQSampleFrame(QSampleFrame);
}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the lab frame).
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space
 * @param goniometer :: a 3x3 rotation matrix
 */
LeanElasticPeak::LeanElasticPeak(
    const Mantid::Kernel::V3D &QSampleFrame,
    const Mantid::Kernel::Matrix<double> &goniometer)
    : BasePeak(goniometer), m_Qsample(QSampleFrame) {}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the sample frame)
 * and a goniometer rotation matrix.
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in
 *the sample frame (goniometer rotation accounted for).
 * @param wavelength :: wavelength in Angstroms.
 */
LeanElasticPeak::LeanElasticPeak(const Mantid::Kernel::V3D &QSampleFrame,
                                 double wavelength)
    : BasePeak(), m_Qsample(QSampleFrame), m_wavelength(wavelength) {}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the lab frame).
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space
 * @param goniometer :: a 3x3 rotation matrix
 * @param wavelength :: wavelength in Angstroms.
 */
LeanElasticPeak::LeanElasticPeak(
    const Mantid::Kernel::V3D &QSampleFrame,
    const Mantid::Kernel::Matrix<double> &goniometer, double wavelength)
    : BasePeak(goniometer), m_Qsample(QSampleFrame), m_wavelength(wavelength) {}

/**
 * @brief Copy constructor
 * @param other : Source
 */
LeanElasticPeak::LeanElasticPeak(const LeanElasticPeak &other)
    : BasePeak(other), m_Qsample(other.m_Qsample),
      m_wavelength(other.m_wavelength) {}

//----------------------------------------------------------------------------------------------
/** Constructor making a LeanElasticPeak from IPeak interface
 *
 * @param ipeak :: const reference to an IPeak object
 */
LeanElasticPeak::LeanElasticPeak(const Geometry::IPeak &ipeak)
    : BasePeak(ipeak), m_Qsample(ipeak.getQSampleFrame()),
      m_wavelength(ipeak.getWavelength()) {}

//----------------------------------------------------------------------------------------------
/** Set the wavelength of the neutron. Assumes elastic scattering.
 *
 * @param wavelength :: wavelength in Angstroms.
 */
void LeanElasticPeak::setWavelength(double wavelength) {
  m_wavelength = wavelength;
}

//----------------------------------------------------------------------------------------------
/** Set the detector ID of the pixel at the centre of the peak and look up and
 * cache values related to it. It also adds it to the list of contributing
 * detectors for this peak but does NOT remove the old centre.
 */
void LeanElasticPeak::setDetectorID(int) {
  throw Exception::NotImplementedError(
      "LeanElasticPeak::setDetectorID(): Can't set detectorID on "
      "LeanElasticPeak");
}

//----------------------------------------------------------------------------------------------
/** Get the ID of the detector at the center of the peak  */
int LeanElasticPeak::getDetectorID() const { return -1; }

//----------------------------------------------------------------------------------------------
/** Set the instrument (and save the source/sample pos).
 * Call setDetectorID AFTER this call.
 *
 */
void LeanElasticPeak::setInstrument(const Geometry::Instrument_const_sptr &) {
  throw Exception::NotImplementedError(
      "LeanElasticPeak::setInstrument(): Can't set instrument on "
      "LeanElasticPeak");
}

//----------------------------------------------------------------------------------------------
/** Return a shared ptr to the detector at center of peak. */
Geometry::IDetector_const_sptr LeanElasticPeak::getDetector() const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak::getDetector(): Has no detector ID");
}

/** Return a shared ptr to the instrument for this peak. */
Geometry::Instrument_const_sptr LeanElasticPeak::getInstrument() const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak::setInstrument(): Has no instrument");
}

// -------------------------------------------------------------------------------------
/** Return the neutron wavelength (in angstroms) */
double LeanElasticPeak::getWavelength() const { return m_wavelength; }

// -------------------------------------------------------------------------------------
/** Calculate the time of flight (in microseconds) of the neutrons for this
 * peak,
 * using the geometry of the detector  */
double LeanElasticPeak::getTOF() const {
  return std::numeric_limits<double>::quiet_NaN();
}

// -------------------------------------------------------------------------------------
/** Calculate the scattering angle of the peak  */
double LeanElasticPeak::getScattering() const {
  return asin(getWavelength() / (2 * getDSpacing())) * 2;
}

// -------------------------------------------------------------------------------------
/** Calculate the azimuthal angle of the peak  */
double LeanElasticPeak::getAzimuthal() const {
  return std::numeric_limits<double>::quiet_NaN();
}

// -------------------------------------------------------------------------------------
/** Calculate the d-spacing of the peak, in 1/Angstroms  */
double LeanElasticPeak::getDSpacing() const {
  return 2 * M_PI / m_Qsample.norm();
}

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Lab frame: the goniometer rotation was NOT taken out.
 *
 * Note: There is a 2*pi factor used, so |Q| = 2*pi/wavelength.
 * */
Mantid::Kernel::V3D LeanElasticPeak::getQLabFrame() const {
  return getGoniometerMatrix() * m_Qsample;
}

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Sample frame: the goniometer rotation WAS taken out. */
Mantid::Kernel::V3D LeanElasticPeak::getQSampleFrame() const {
  return m_Qsample;
}

//----------------------------------------------------------------------------------------------
/** Set the peak using the peak's position in reciprocal space, in the sample
 *frame.
 * The GoniometerMatrix will be used to find the Q in the lab frame, so it
 *should
 * be set beforehand.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space
 *        This is in inelastic convention: momentum transfer of the LATTICE!
 *        Also, q does NOT have a 2pi factor = it is equal to 1/wavelength.
 */
void LeanElasticPeak::setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame,
                                      boost::optional<double>) {
  m_Qsample = QSampleFrame;
}

//----------------------------------------------------------------------------------------------
/** Set the peak using the peak's position in reciprocal space, in the lab
 *frame.
 * The detector position will be determined.
 * DetectorID, row and column will be set to -1 since they are not (necessarily)
 *found.
 * You can call findDetector to look for the detector ID
 *
 * @param qLab :: Q of the center of the peak, in reciprocal space.
 *        This is in inelastic convention: momentum transfer of the LATTICE!
 *        Also, q does have a 2pi factor = it is equal to 2pi/wavelength (in
 *        Angstroms).
 */
void LeanElasticPeak::setQLabFrame(const Mantid::Kernel::V3D &qLab,
                                   boost::optional<double>) {
  this->setQSampleFrame(getInverseGoniometerMatrix() * qLab);
}

//----------------------------------------------------------------------------------------------
/** Get the final neutron energy in meV */
double LeanElasticPeak::getFinalEnergy() const {
  // Velocity of the neutron (non-relativistic)
  const double velocity =
      PhysicalConstants::h /
      (m_wavelength * 1e-10 * PhysicalConstants::NeutronMass);
  // Energy in J of the neutron
  const double energy =
      PhysicalConstants::NeutronMass * velocity * velocity / 2.0;
  // Convert to meV
  return energy / PhysicalConstants::meV;
}

/** Get the initial (incident) neutron energy in meV */
double LeanElasticPeak::getInitialEnergy() const { return getFinalEnergy(); }

/** Get the difference between the initial and final neutron energy in meV,
 * elastic so always 0 */
double LeanElasticPeak::getEnergyTransfer() const { return 0.; }

/** Set sample position */
void LeanElasticPeak::setSamplePos(double, double, double) {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no sample information");
}

/** Set sample position  */
void LeanElasticPeak::setSamplePos(const Mantid::Kernel::V3D &) {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no sample information");
}

/** Set the final energy */
void LeanElasticPeak::setFinalEnergy(double) {
  throw Exception::NotImplementedError("Use LeanElasticPeak::setWavelength");
}

/** Set the initial energy */
void LeanElasticPeak::setInitialEnergy(double) {
  throw Exception::NotImplementedError("Use LeanElasticPeak::setWavelength");
}

// -------------------------------------------------------------------------------------
/** Return the detector position vector */
Mantid::Kernel::V3D LeanElasticPeak::getDetPos() const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no detector information");
}

// -------------------------------------------------------------------------------------
/** Return the sample position vector */
Mantid::Kernel::V3D LeanElasticPeak::getSamplePos() const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no sample information");
}

// -------------------------------------------------------------------------------------
/** Return the L1 flight path length (source to sample), in meters. */
double LeanElasticPeak::getL1() const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no detector information");
}

// -------------------------------------------------------------------------------------
/** Return the L2 flight path length (sample to detector), in meters. */
double LeanElasticPeak::getL2() const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no detector information");
}

/**
 * @brief Assignement operator overload
 * @param other : Other peak object to assign from
 * @return this
 */
LeanElasticPeak &LeanElasticPeak::operator=(const LeanElasticPeak &other) {
  if (&other != this) {
    BasePeak::operator=(other);
    m_Qsample = other.m_Qsample;
    m_wavelength = other.m_wavelength;
  }
  return *this;
}

/** After creating a peak using the Q in the lab frame,
 * the detPos is set to the direction of the detector (but the detector is
 *unknown)
 *
 * Using the instrument set in the peak, perform ray tracing
 * to find the exact detector.
 *
 * @return true if the detector ID was found.
 */
bool LeanElasticPeak::findDetector() {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no detector information");
}

/**
 * Performs the same algorithm as findDetector() but uses a pre-existing
 * InstrumentRayTracer object to be able to take adavtange of its caches.
 * This method should be preferred if findDetector is to be called many times
 * over the same instrument.
 * @return true if the detector ID was found.
 */
bool LeanElasticPeak::findDetector(const InstrumentRayTracer &) {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no detector information");
}

/**
 Forwarding function. Exposes the detector position directly.
 */
Mantid::Kernel::V3D LeanElasticPeak::getDetectorPositionNoCheck() const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no detector information");
}

/**
 Forwarding function. Exposes the detector position directly, but checks that
 the detector is not null before accessing its position. Throws if null.
 */
Mantid::Kernel::V3D LeanElasticPeak::getDetectorPosition() const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak has no detector information");
}

Mantid::Kernel::Logger LeanElasticPeak::g_log("PeakLogger");

} // namespace DataObjects
} // namespace Mantid
