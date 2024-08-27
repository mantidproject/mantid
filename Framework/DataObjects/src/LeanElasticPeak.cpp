// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/LeanElasticPeak.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
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

namespace Mantid::DataObjects {

//----------------------------------------------------------------------------------------------
/** Default constructor */
LeanElasticPeak::LeanElasticPeak() : BasePeak(), m_Qsample(V3D(0, 0, 0)), m_wavelength(0.) {}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the sample frame)
 * and a goniometer rotation matrix.
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in
 *the sample frame (goniometer rotation accounted for).
 */
LeanElasticPeak::LeanElasticPeak(const Mantid::Kernel::V3D &QSampleFrame)
    : BasePeak(), m_Qsample(QSampleFrame), m_wavelength(0.) {}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the lab frame).
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space
 * @param goniometer :: a 3x3 rotation matrix
 * @param refFrame :: optional reference frame, will default to beam along +Z
 */
LeanElasticPeak::LeanElasticPeak(const Mantid::Kernel::V3D &QSampleFrame,
                                 const Mantid::Kernel::Matrix<double> &goniometer,
                                 std::optional<std::shared_ptr<const Geometry::ReferenceFrame>> refFrame)
    : BasePeak() {
  if (refFrame.has_value())
    setReferenceFrame(refFrame.value());
  setQSampleFrame(QSampleFrame, goniometer);
}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the sample frame)
 * and a goniometer rotation matrix.
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in
 *the sample frame (goniometer rotation accounted for).
 * @param wavelength :: wavelength in Angstroms.
 */
LeanElasticPeak::LeanElasticPeak(const Mantid::Kernel::V3D &QSampleFrame, double wavelength)
    : BasePeak(), m_Qsample(QSampleFrame), m_wavelength(wavelength) {}

/**
 * @brief Copy constructor
 * @param other : Source
 */
LeanElasticPeak::LeanElasticPeak(const LeanElasticPeak &other) = default;

//----------------------------------------------------------------------------------------------
/** Constructor making a LeanElasticPeak from IPeak interface
 *
 * @param ipeak :: const reference to an IPeak object
 */
LeanElasticPeak::LeanElasticPeak(const Geometry::IPeak &ipeak)
    : BasePeak(ipeak), m_Qsample(ipeak.getQSampleFrame()), m_wavelength(ipeak.getWavelength()) {}

//----------------------------------------------------------------------------------------------
/** Set the wavelength of the neutron. Assumes elastic scattering.
 *
 * @param wavelength :: wavelength in Angstroms.
 */
void LeanElasticPeak::setWavelength(double wavelength) { m_wavelength = wavelength; }

/** Return a shared ptr to the reference frame for this peak. */
std::shared_ptr<const Geometry::ReferenceFrame> LeanElasticPeak::getReferenceFrame() const {
  return (m_refFrame) ? m_refFrame : std::make_shared<const ReferenceFrame>();
}

/**
Setter for the reference frame.
@param frame : reference frame object to use.
*/
void LeanElasticPeak::setReferenceFrame(std::shared_ptr<const ReferenceFrame> frame) { m_refFrame = std::move(frame); }

// -------------------------------------------------------------------------------------
/** Return the neutron wavelength (in angstroms) */
double LeanElasticPeak::getWavelength() const { return m_wavelength; }

// -------------------------------------------------------------------------------------
/** Calculate the time of flight (in microseconds) of the neutrons for this
 * peak,
 * using the geometry of the detector  */
double LeanElasticPeak::getTOF() const {
  throw Exception::NotImplementedError("LeanElasticPeak::getTOF(): no detector infomation in LeanElasticPeak");
}

// returns the detectorID of the pixel, throws NotImplementedError for LeanElasticPeak
int LeanElasticPeak::getDetectorID() const {
  throw Exception::NotImplementedError("LeanElasticPeak::getDetectorID(): no detector infomation in LeanElasticPeak");
}

/// returns the row (y) of the pixel of the detector, throws NotImplementedError for LeanElasticPeak
int LeanElasticPeak::getRow() const {
  throw Exception::NotImplementedError("LeanElasticPeak::getRow(): no detector infomation in LeanElasticPeak");
}

/// returns the column (x) of the pixel of the detector, throws NotImplementedError for LeanElasticPeak
int LeanElasticPeak::getCol() const {
  throw Exception::NotImplementedError("LeanElasticPeak::getCol(): no detector infomation in LeanElasticPeak");
}

// -------------------------------------------------------------------------------------
/** Calculate the scattering angle of the peak  */
double LeanElasticPeak::getScattering() const { return asin(getWavelength() / (2 * getDSpacing())) * 2; }

// -------------------------------------------------------------------------------------
/** Calculate the azimuthal angle of the peak  */
double LeanElasticPeak::getAzimuthal() const {
  const V3D qLab = getQLabFrame();
  std::shared_ptr<const ReferenceFrame> refFrame = getReferenceFrame();
  const double qSign = (m_convention != "Crystallography") ? 1.0 : -1.0;
  const V3D detectorDir = -qLab * qSign;
  if (refFrame)
    return atan2(detectorDir[refFrame->pointingUp()], detectorDir[refFrame->pointingHorizontal()]);
  else
    return atan2(detectorDir[1], detectorDir[0]);
}

// -------------------------------------------------------------------------------------
/** Calculate the scattered beam direction in the sample frame  */
Mantid::Kernel::V3D LeanElasticPeak::getDetectorDirectionSampleFrame() const {
  const V3D qSample = getQSampleFrame();
  const double qSign = (m_convention != "Crystallography") ? 1.0 : -1.0;
  return getSourceDirectionSampleFrame() * -1.0 - qSample * qSign * getWavelength() / (2 * M_PI);
}

// -------------------------------------------------------------------------------------
/** Calculate the reverse incident beam direction in the sample frame  */
Mantid::Kernel::V3D LeanElasticPeak::getSourceDirectionSampleFrame() const {
  std::shared_ptr<const ReferenceFrame> refFrame = getReferenceFrame();
  if (refFrame)
    return getInverseGoniometerMatrix() * refFrame->vecPointingAlongBeam() * -1.0;
  else
    return getInverseGoniometerMatrix() * V3D(0, 0, -1);
}

// -------------------------------------------------------------------------------------
/** Calculate the d-spacing of the peak, in 1/Angstroms  */
double LeanElasticPeak::getDSpacing() const { return 2 * M_PI / m_Qsample.norm(); }

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Lab frame: the goniometer rotation was NOT taken out.
 *
 * Note: There is a 2*pi factor used, so |Q| = 2*pi/wavelength.
 * */
Mantid::Kernel::V3D LeanElasticPeak::getQLabFrame() const { return getGoniometerMatrix() * m_Qsample; }

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Sample frame: the goniometer rotation WAS taken out. */
Mantid::Kernel::V3D LeanElasticPeak::getQSampleFrame() const { return m_Qsample; }

//----------------------------------------------------------------------------------------------
/** Set the peak using the peak's position in reciprocal space, in the sample
 *frame.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space
 *        This is in inelastic convention: momentum transfer of the LATTICE!
 *        Also, q does NOT have a 2pi factor = it is equal to 1/wavelength.
 */
void LeanElasticPeak::setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame, std::optional<double>) {
  m_Qsample = QSampleFrame;
}

void LeanElasticPeak::setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame,
                                      const Mantid::Kernel::Matrix<double> &goniometer) {
  m_Qsample = QSampleFrame;
  setGoniometerMatrix(goniometer);

  const V3D qLab = getQLabFrame();

  try {
    double wl = calculateWavelengthFromQLab(qLab);
    setWavelength(wl);
  } catch (std::exception &e) {
    g_log.information() << "Unable to automatically determine wavelength from q-lab\n"
                        << e.what() << ", goniometer is likely not correct\n";
  }
}

//----------------------------------------------------------------------------------------------
/** Set the peak using the peak's position in reciprocal space, in the lab
 *frame.
 *
 * @param qLab :: Q of the center of the peak, in reciprocal space.
 *        This is in inelastic convention: momentum transfer of the LATTICE!
 *        Also, q does have a 2pi factor = it is equal to 2pi/wavelength (in
 *        Angstroms).
 */
void LeanElasticPeak::setQLabFrame(const Mantid::Kernel::V3D &qLab, std::optional<double>) {
  this->setQSampleFrame(getInverseGoniometerMatrix() * qLab);
}

//----------------------------------------------------------------------------------------------
/** Get the final neutron energy in meV */
double LeanElasticPeak::getFinalEnergy() const {
  // Velocity of the neutron (non-relativistic)
  const double velocity = PhysicalConstants::h / (m_wavelength * 1e-10 * PhysicalConstants::NeutronMass);
  // Energy in J of the neutron
  const double energy = PhysicalConstants::NeutronMass * velocity * velocity / 2.0;
  // Convert to meV
  return energy / PhysicalConstants::meV;
}

/** Get the initial (incident) neutron energy in meV */
double LeanElasticPeak::getInitialEnergy() const { return getFinalEnergy(); }

/** Get the difference between the initial and final neutron energy in meV,
 * elastic so always 0 */
double LeanElasticPeak::getEnergyTransfer() const { return 0.; }

/** Set the final energy */
void LeanElasticPeak::setFinalEnergy(double) {
  throw Exception::NotImplementedError("Use LeanElasticPeak::setWavelength");
}

/** Set the initial energy */
void LeanElasticPeak::setInitialEnergy(double) {
  throw Exception::NotImplementedError("Use LeanElasticPeak::setWavelength");
}

// -------------------------------------------------------------------------------------
/** Return the L1 flight path length (source to sample), in meters. */
double LeanElasticPeak::getL1() const {
  throw Exception::NotImplementedError("LeanElasticPeak has no detector information");
}

// -------------------------------------------------------------------------------------
/** Return the L2 flight path length (sample to detector), in meters. */
double LeanElasticPeak::getL2() const {
  throw Exception::NotImplementedError("LeanElasticPeak has no detector information");
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

Mantid::Kernel::Logger LeanElasticPeak::g_log("PeakLogger");

} // namespace Mantid::DataObjects
