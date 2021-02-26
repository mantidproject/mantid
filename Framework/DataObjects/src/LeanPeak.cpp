// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/LeanPeak.h"
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
LeanPeak::LeanPeak() : BasePeak(), m_Qsample(V3D(0, 0, 0)) {}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the sample frame)
 * and a goniometer rotation matrix.
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in
 *the sample frame (goniometer rotation accounted for).
 */
LeanPeak::LeanPeak(const Mantid::Kernel::V3D &QSampleFrame) : BasePeak() {
  this->setQSampleFrame(QSampleFrame);
}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the lab frame).
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space
 * @param goniometer :: a 3x3 rotation matrix
 */
LeanPeak::LeanPeak(const Mantid::Kernel::V3D &QSampleFrame,
                   const Mantid::Kernel::Matrix<double> &goniometer)
    : BasePeak(goniometer) {
  this->setQSampleFrame(QSampleFrame);
}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the sample frame)
 * and a goniometer rotation matrix.
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in
 *the sample frame (goniometer rotation accounted for).
 */
LeanPeak::LeanPeak(const Mantid::Kernel::V3D &QSampleFrame, double wavelength)
    : BasePeak() {
  this->setQSampleFrame(QSampleFrame);
  this->setWavelength(wavelength);
}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the lab frame).
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space
 * @param goniometer :: a 3x3 rotation matrix
 */
LeanPeak::LeanPeak(const Mantid::Kernel::V3D &QSampleFrame,
                   const Mantid::Kernel::Matrix<double> &goniometer,
                   double wavelength)
    : BasePeak(goniometer) {
  this->setQSampleFrame(QSampleFrame);
  this->setWavelength(wavelength);
}

/**
 * @brief Copy constructor
 * @param other : Source
 * @return
 */
LeanPeak::LeanPeak(const LeanPeak &other)
    : BasePeak(other), m_Qsample(other.m_Qsample) {}

//----------------------------------------------------------------------------------------------
/** Constructor making a LeanPeak from IPeak interface
 *
 * @param ipeak :: const reference to an IPeak object
 * @return
 */
LeanPeak::LeanPeak(const Geometry::IPeak &ipeak)
    : BasePeak(ipeak), m_Qsample(ipeak.getQSampleFrame()) {}

//----------------------------------------------------------------------------------------------
/** Set the detector ID of the pixel at the centre of the peak and look up and
 * cache
 *  values related to it. It also adds it to the list of contributing detectors
 * for this peak but
 *  does NOT remove the old centre.
 * @param id :: ID of detector at the centre of the peak.
 */
void LeanPeak::setDetectorID([[maybe_unused]] int id) {
  throw std::runtime_error(
      "LeanPeak::setDetectorID(): Can't set detectorID on LeanPeak");
}

//----------------------------------------------------------------------------------------------
/** Get the ID of the detector at the center of the peak  */
int LeanPeak::getDetectorID() const { return -1; }

//----------------------------------------------------------------------------------------------
/** Set the instrument (and save the source/sample pos).
 * Call setDetectorID AFTER this call.
 *
 * @param inst :: Instrument sptr to use
 */
void LeanPeak::setInstrument([
    [maybe_unused]] const Geometry::Instrument_const_sptr &inst) {
  throw std::runtime_error(
      "LeanPeak::setInstrument(): Can't set instrument on LeanPeak");
}

//----------------------------------------------------------------------------------------------
/** Return a shared ptr to the detector at center of peak. */
Geometry::IDetector_const_sptr LeanPeak::getDetector() const {
  throw std::runtime_error("LeanPeak::getDetector(): Has no detector ID");
}

/** Return a shared ptr to the instrument for this peak. */
Geometry::Instrument_const_sptr LeanPeak::getInstrument() const {
  throw std::runtime_error("LeanPeak::setInstrument(): Has no instrument");
}

// -------------------------------------------------------------------------------------
/** Calculate the time of flight (in microseconds) of the neutrons for this
 * peak,
 * using the geometry of the detector  */
double LeanPeak::getTOF() const {
  return std::numeric_limits<double>::quiet_NaN();
}

// -------------------------------------------------------------------------------------
/** Calculate the scattering angle of the peak  */
double LeanPeak::getScattering() const {
  return asin(getWavelength() / (2 * getDSpacing())) * 2;
}

// -------------------------------------------------------------------------------------
/** Calculate the azimuthal angle of the peak  */
double LeanPeak::getAzimuthal() const {
  return std::numeric_limits<double>::quiet_NaN();
}

// -------------------------------------------------------------------------------------
/** Calculate the d-spacing of the peak, in 1/Angstroms  */
double LeanPeak::getDSpacing() const { return 2 * M_PI / m_Qsample.norm(); }

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Lab frame: the goniometer rotation was NOT taken out.
 *
 * Note: There is a 2*pi factor used, so |Q| = 2*pi/wavelength.
 * */
Mantid::Kernel::V3D LeanPeak::getQLabFrame() const {
  return m_GoniometerMatrix * m_Qsample;
}

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Sample frame: the goniometer rotation WAS taken out. */
Mantid::Kernel::V3D LeanPeak::getQSampleFrame() const { return m_Qsample; }

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
void LeanPeak::setQSampleFrame(
    const Mantid::Kernel::V3D &QSampleFrame,
    [[maybe_unused]] boost::optional<double> detectorDistance) {
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
 *Angstroms).
 * @param detectorDistance :: distance between the sample and the detector. If
 *this is provided. Then we do not
 * ray trace to find the intersecing detector.
 */
void LeanPeak::setQLabFrame(
    const Mantid::Kernel::V3D &qLab,
    [[maybe_unused]] boost::optional<double> detectorDistance) {
  this->setQSampleFrame(m_InverseGoniometerMatrix * qLab);
}

/** Set sample position
 *
 * @ doubles x,y,z-> samplePos(x), samplePos(y), samplePos(z)
 */
void LeanPeak::setSamplePos([[maybe_unused]] double samX,
                            [[maybe_unused]] double samY,
                            [[maybe_unused]] double samZ) {
  throw std::runtime_error("not implemented");
}

/** Set sample position
 *
 * @param XYZ :: vector x,y,z-> samplePos(x), samplePos(y), samplePos(z)
 */
void LeanPeak::setSamplePos([[maybe_unused]] const Mantid::Kernel::V3D &XYZ) {
  throw std::runtime_error("not implemented");
}

// -------------------------------------------------------------------------------------
/** Return the detector position vector */
Mantid::Kernel::V3D LeanPeak::getDetPos() const {
  throw std::runtime_error("not implemented");
}

// -------------------------------------------------------------------------------------
/** Return the sample position vector */
Mantid::Kernel::V3D LeanPeak::getSamplePos() const {
  throw std::runtime_error("not implemented");
}

// -------------------------------------------------------------------------------------
/** Return the L1 flight path length (source to sample), in meters. */
double LeanPeak::getL1() const {
  return std::numeric_limits<double>::quiet_NaN();
}

// -------------------------------------------------------------------------------------
/** Return the L2 flight path length (sample to detector), in meters. */
double LeanPeak::getL2() const {
  return std::numeric_limits<double>::quiet_NaN();
}

/**
 * @brief Assignement operator overload
 * @param other : Other peak object to assign from
 * @return this
 */
LeanPeak &LeanPeak::operator=(const LeanPeak &other) {
  if (&other != this) {
    BasePeak::operator=(other);
    m_Qsample = other.m_Qsample;
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
bool LeanPeak::findDetector() { throw std::runtime_error("not implemented"); }

/**
 * Performs the same algorithm as findDetector() but uses a pre-existing
 * InstrumentRayTracer object to be able to take adavtange of its caches.
 * This method should be preferred if findDetector is to be called many times
 * over the same instrument.
 * @param tracer A reference to an existing InstrumentRayTracer object.
 * @return true if the detector ID was found.
 */
bool LeanPeak::findDetector([
    [maybe_unused]] const InstrumentRayTracer &tracer) {
  throw std::runtime_error("not implemented");
}

/**
 Forwarding function. Exposes the detector position directly.
 */
Mantid::Kernel::V3D LeanPeak::getDetectorPositionNoCheck() const {
  return getDetector()->getPos();
}

/**
 Forwarding function. Exposes the detector position directly, but checks that
 the detector is not null before accessing its position. Throws if null.
 */
Mantid::Kernel::V3D LeanPeak::getDetectorPosition() const {
  auto det = getDetector();
  if (det == nullptr) {
    throw Mantid::Kernel::Exception::NullPointerException("LeanPeak",
                                                          "Detector");
  }
  return getDetector()->getPos();
}

Mantid::Kernel::Logger LeanPeak::g_log("PeakLogger");

} // namespace DataObjects
} // namespace Mantid
