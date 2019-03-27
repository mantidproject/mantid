// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"

#include "boost/make_shared.hpp"
#include <boost/math/special_functions/round.hpp>

#include <algorithm>
#include <cctype>
#include <string>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid {
namespace DataObjects {

//----------------------------------------------------------------------------------------------
/** Default constructor */
Peak::Peak()
    : m_detectorID(-1), m_H(0), m_K(0), m_L(0), m_intensity(0),
      m_sigmaIntensity(0), m_binCount(0), m_initialEnergy(0.),
      m_finalEnergy(0.), m_GoniometerMatrix(3, 3, true),
      m_InverseGoniometerMatrix(3, 3, true), m_runNumber(0), m_monitorCount(0),
      m_row(-1), m_col(-1), m_orig_H(0), m_orig_K(0), m_orig_L(0),
      m_peakNumber(0), m_IntHKL(V3D(0, 0, 0)), m_IntMNP(V3D(0, 0, 0)),
      m_peakShape(boost::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the lab frame).
 * No detector ID is set.
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param QLabFrame :: Q of the center of the peak, in reciprocal space
 * @param detectorDistance :: Optional distance between the sample and the
 *detector. Calculated if not explicitly provided.
 *        Used to give a valid TOF. Default 1.0 meters.
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst,
           const Mantid::Kernel::V3D &QLabFrame,
           boost::optional<double> detectorDistance)
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_GoniometerMatrix(3, 3, true),
      m_InverseGoniometerMatrix(3, 3, true), m_runNumber(0), m_monitorCount(0),
      m_orig_H(0), m_orig_K(0), m_orig_L(0), m_peakNumber(0),
      m_IntHKL(V3D(0, 0, 0)), m_IntMNP(V3D(0, 0, 0)),
      m_peakShape(boost::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  this->setInstrument(m_inst);
  this->setQLabFrame(QLabFrame, detectorDistance);
}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the sample frame)
 * and a goniometer rotation matrix.
 * No detector ID is set.
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in
 *the sample frame (goniometer rotation accounted for).
 * @param goniometer :: a 3x3 rotation matrix
 * @param detectorDistance :: Optional distance between the sample and the
 *detector. Calculated if not explicitly provided.
 *        Used to give a valid TOF. Default 1.0 meters.
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst,
           const Mantid::Kernel::V3D &QSampleFrame,
           const Mantid::Kernel::Matrix<double> &goniometer,
           boost::optional<double> detectorDistance)
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_GoniometerMatrix(goniometer),
      m_InverseGoniometerMatrix(goniometer), m_runNumber(0), m_monitorCount(0),
      m_orig_H(0), m_orig_K(0), m_orig_L(0), m_peakNumber(0),
      m_IntHKL(V3D(0, 0, 0)), m_IntMNP(V3D(0, 0, 0)),
      m_peakShape(boost::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument(
        "Peak::ctor(): Goniometer matrix must non-singular.");
  this->setInstrument(m_inst);
  this->setQSampleFrame(QSampleFrame, detectorDistance);
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param m_detectorID :: ID to the detector of the center of the peak
 * @param m_Wavelength :: incident neutron wavelength, in Angstroms
 * @return
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID,
           double m_Wavelength)
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_GoniometerMatrix(3, 3, true),
      m_InverseGoniometerMatrix(3, 3, true), m_runNumber(0), m_monitorCount(0),
      m_orig_H(0), m_orig_K(0), m_orig_L(0), m_peakNumber(0),
      m_IntHKL(V3D(0, 0, 0)), m_IntMNP(V3D(0, 0, 0)),
      m_peakShape(boost::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  this->setInstrument(m_inst);
  this->setDetectorID(m_detectorID);
  this->setWavelength(m_Wavelength);
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param m_detectorID :: ID to the detector of the center of the peak
 * @param m_Wavelength :: incident neutron wavelength, in Angstroms
 * @param HKL :: vector with H,K,L position of the peak
 * @return
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID,
           double m_Wavelength, const Mantid::Kernel::V3D &HKL)
    : m_H(HKL[0]), m_K(HKL[1]), m_L(HKL[2]), m_intensity(0),
      m_sigmaIntensity(0), m_binCount(0), m_GoniometerMatrix(3, 3, true),
      m_InverseGoniometerMatrix(3, 3, true), m_runNumber(0), m_monitorCount(0),
      m_orig_H(0), m_orig_K(0), m_orig_L(0), m_peakNumber(0),
      m_IntHKL(V3D(0, 0, 0)), m_IntMNP(V3D(0, 0, 0)),
      m_peakShape(boost::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  this->setInstrument(m_inst);
  this->setDetectorID(m_detectorID);
  this->setWavelength(m_Wavelength);
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param m_detectorID :: ID to the detector of the center of the peak
 * @param m_Wavelength :: incident neutron wavelength, in Angstroms
 * @param HKL :: vector with H,K,L position of the peak
 * @param goniometer :: a 3x3 rotation matrix
 * @return
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID,
           double m_Wavelength, const Mantid::Kernel::V3D &HKL,
           const Mantid::Kernel::Matrix<double> &goniometer)
    : m_H(HKL[0]), m_K(HKL[1]), m_L(HKL[2]), m_intensity(0),
      m_sigmaIntensity(0), m_binCount(0), m_GoniometerMatrix(goniometer),
      m_InverseGoniometerMatrix(goniometer), m_runNumber(0), m_monitorCount(0),
      m_orig_H(0), m_orig_K(0), m_orig_L(0), m_peakNumber(0),
      m_IntHKL(V3D(0, 0, 0)), m_IntMNP(V3D(0, 0, 0)),
      m_peakShape(boost::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument(
        "Peak::ctor(): Goniometer matrix must non-singular.");
  this->setInstrument(m_inst);
  this->setDetectorID(m_detectorID);
  this->setWavelength(m_Wavelength);
}
//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param scattering :: fake detector position using scattering angle
 * @param m_Wavelength :: incident neutron wavelength, in Angstroms
 * @return
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, double scattering,
           double m_Wavelength)
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_GoniometerMatrix(3, 3, true),
      m_InverseGoniometerMatrix(3, 3, true), m_runNumber(0), m_monitorCount(0),
      m_row(-1), m_col(-1), m_orig_H(0), m_orig_K(0), m_orig_L(0),
      m_IntHKL(V3D(0, 0, 0)), m_IntMNP(V3D(0, 0, 0)),
      m_peakShape(boost::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  this->setInstrument(m_inst);
  this->setWavelength(m_Wavelength);
  m_detectorID = -1;
  // get the approximate location of the detector
  const auto detectorDir = V3D(sin(scattering), 0.0, cos(scattering));
  detPos = getVirtualDetectorPosition(detectorDir);
}

/**
 * @brief Copy constructor
 * @param other : Source
 * @return
 */
Peak::Peak(const Peak &other)
    : m_inst(other.m_inst), m_det(other.m_det), m_bankName(other.m_bankName),
      m_detectorID(other.m_detectorID), m_H(other.m_H), m_K(other.m_K),
      m_L(other.m_L), m_intensity(other.m_intensity),
      m_sigmaIntensity(other.m_sigmaIntensity), m_binCount(other.m_binCount),
      m_initialEnergy(other.m_initialEnergy),
      m_finalEnergy(other.m_finalEnergy),
      m_GoniometerMatrix(other.m_GoniometerMatrix),
      m_InverseGoniometerMatrix(other.m_InverseGoniometerMatrix),
      m_runNumber(other.m_runNumber), m_monitorCount(other.m_monitorCount),
      m_row(other.m_row), m_col(other.m_col), sourcePos(other.sourcePos),
      samplePos(other.samplePos), detPos(other.detPos),
      m_orig_H(other.m_orig_H), m_orig_K(other.m_orig_K),
      m_orig_L(other.m_orig_L), m_peakNumber(other.m_peakNumber),
      m_IntHKL(other.m_IntHKL), m_IntMNP(other.m_IntMNP),
      m_detIDs(other.m_detIDs), m_peakShape(other.m_peakShape->clone()),
      convention(other.convention) {}

//----------------------------------------------------------------------------------------------
/** Constructor making a Peak from IPeak interface
 *
 * @param ipeak :: const reference to an IPeak object
 * @return
 */
Peak::Peak(const Geometry::IPeak &ipeak)
    : IPeak(ipeak), m_detectorID(ipeak.getDetectorID()), m_H(ipeak.getH()),
      m_K(ipeak.getK()), m_L(ipeak.getL()), m_intensity(ipeak.getIntensity()),
      m_sigmaIntensity(ipeak.getSigmaIntensity()),
      m_binCount(ipeak.getBinCount()),
      m_initialEnergy(ipeak.getInitialEnergy()),
      m_finalEnergy(ipeak.getFinalEnergy()),
      m_GoniometerMatrix(ipeak.getGoniometerMatrix()),
      m_InverseGoniometerMatrix(ipeak.getGoniometerMatrix()),
      m_runNumber(ipeak.getRunNumber()),
      m_monitorCount(ipeak.getMonitorCount()), m_row(ipeak.getRow()),
      m_col(ipeak.getCol()), m_orig_H(0.), m_orig_K(0.), m_orig_L(0.),
      m_peakNumber(ipeak.getPeakNumber()), m_IntHKL(ipeak.getIntHKL()),
      m_IntMNP(ipeak.getIntMNP()), m_peakShape(boost::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument(
        "Peak::ctor(): Goniometer matrix must non-singular.");
  setInstrument(ipeak.getInstrument());
  detid_t id = ipeak.getDetectorID();
  if (id >= 0) {
    setDetectorID(id);
  }
  if (const Peak *peak = dynamic_cast<const Peak *>(&ipeak)) {
    this->m_detIDs = peak->m_detIDs;
  }
}

//----------------------------------------------------------------------------------------------
/** Set the incident wavelength of the neutron. Calculates the energy from this.
 * Assumes elastic scattering.
 *
 * @param wavelength :: wavelength in Angstroms.
 */
void Peak::setWavelength(double wavelength) {
  // Velocity of the neutron (non-relativistic)
  double velocity = PhysicalConstants::h /
                    (wavelength * 1e-10 * PhysicalConstants::NeutronMass);
  // Energy in J of the neutron
  double energy = PhysicalConstants::NeutronMass * velocity * velocity / 2.0;
  // Convert to meV
  m_initialEnergy = energy / PhysicalConstants::meV;
  m_finalEnergy = m_initialEnergy;
}

//----------------------------------------------------------------------------------------------
/** Set the detector ID of the pixel at the centre of the peak and look up and
 * cache
 *  values related to it. It also adds it to the list of contributing detectors
 * for this peak but
 *  does NOT remove the old centre.
 * @param id :: ID of detector at the centre of the peak.
 */
void Peak::setDetectorID(int id) {
  if (!m_inst)
    throw std::runtime_error("Peak::setInstrument(): No instrument is set!");
  this->m_det = m_inst->getDetector(id);
  if (!m_det)
    throw std::runtime_error("Peak::setInstrument(): No detector was found!");

  this->m_detectorID = id;
  addContributingDetID(id);

  detPos = m_det->getPos();

  // We now look for the row/column. -1 if not found.
  m_row = -1;
  m_col = -1;

  // Go up 2 parents to find the bank/rectangular detector
  IComponent_const_sptr parent = m_det->getParent();

  // Find the ROW by looking at the string name of the pixel. E.g. "pixel12"
  // gives row 12.
  m_row = Strings::endsWithInt(m_det->getName());

  if (!parent)
    return;
  m_bankName = parent->getName();

  // Find the COLUMN by looking at the string name of the parent. E.g. "tube003"
  // gives column 3.
  m_col = Strings::endsWithInt(parent->getName());

  parent = parent->getParent();
  // Use the parent if there is no grandparent.
  if (!parent)
    return;

  // Use the parent if the grandparent is the instrument
  Instrument_const_sptr instrument =
      boost::dynamic_pointer_cast<const Instrument>(parent);
  if (instrument)
    return;
  // Use the grand-parent whenever possible
  m_bankName = parent->getName();
  // For CORELLI, one level above sixteenpack
  if (m_bankName == "sixteenpack") {
    parent = parent->getParent();
    m_bankName = parent->getName();
  }

  // Special for rectangular detectors: find the row and column.
  RectangularDetector_const_sptr retDet =
      boost::dynamic_pointer_cast<const RectangularDetector>(parent);
  if (!retDet)
    return;
  std::pair<int, int> xy = retDet->getXYForDetectorID(m_detectorID);
  m_row = xy.second;
  m_col = xy.first;
}

//----------------------------------------------------------------------------------------------
/** Get the ID of the detector at the center of the peak  */
int Peak::getDetectorID() const { return m_detectorID; }

//----------------------------------------------------------------------------------------------
/**
 * Add a detector ID that contributed to this peak
 * @param id :: The ID of a detector that contributed to this peak
 */
void Peak::addContributingDetID(const int id) { m_detIDs.insert(id); }

//-------------------------------------------------------------------------------------
/**
 * Removes an ID from the list of contributing detectors
 * @param id :: This ID is removed from the list.
 */
void Peak::removeContributingDetector(const int id) { m_detIDs.erase(id); }

//----------------------------------------------------------------------------------------------
/**
 * Return the set of detector IDs that contribute to this peak
 * @returns A set of unique detector IDs that form this peak
 */
const std::set<int> &Peak::getContributingDetIDs() const { return m_detIDs; }

//----------------------------------------------------------------------------------------------
/** Set the instrument (and save the source/sample pos).
 * Call setDetectorID AFTER this call.
 *
 * @param inst :: Instrument sptr to use
 */
void Peak::setInstrument(const Geometry::Instrument_const_sptr &inst) {
  m_inst = inst;
  if (!inst)
    throw std::runtime_error("Peak::setInstrument(): No instrument is set!");

  // Cache some positions
  const Geometry::IComponent_const_sptr sourceObj = m_inst->getSource();
  if (sourceObj == nullptr)
    throw Exception::InstrumentDefinitionError("Peak::setInstrument(): Failed "
                                               "to get source component from "
                                               "instrument");
  const Geometry::IComponent_const_sptr sampleObj = m_inst->getSample();
  if (sampleObj == nullptr)
    throw Exception::InstrumentDefinitionError("Peak::setInstrument(): Failed "
                                               "to get sample component from "
                                               "instrument");

  sourcePos = sourceObj->getPos();
  samplePos = sampleObj->getPos();
}

//----------------------------------------------------------------------------------------------
/** Return a shared ptr to the detector at center of peak. */
Geometry::IDetector_const_sptr Peak::getDetector() const { return m_det; }

/** Return a shared ptr to the instrument for this peak. */
Geometry::Instrument_const_sptr Peak::getInstrument() const { return m_inst; }

// -------------------------------------------------------------------------------------
/** Calculate the neutron wavelength (in angstroms) at the peak
 * (Note for inelastic scattering - it is the wavelength corresponding to the
 * final energy)*/
double Peak::getWavelength() const {
  // Energy in J of the neutron
  double energy = PhysicalConstants::meV * m_finalEnergy;
  // v = sqrt(2.0 * E / m)
  double velocity = sqrt(2.0 * energy / PhysicalConstants::NeutronMass);
  // wavelength = h / mv
  double wavelength =
      PhysicalConstants::h / (PhysicalConstants::NeutronMass * velocity);
  // Return it in angstroms
  return wavelength * 1e10;
}

// -------------------------------------------------------------------------------------
/** Calculate the time of flight (in microseconds) of the neutrons for this
 * peak,
 * using the geometry of the detector  */
double Peak::getTOF() const {
  // First, get the neutron traveling distances
  double L1 = this->getL1();
  double L2 = this->getL2();
  // Energy in J of the neutron
  double Ei = PhysicalConstants::meV * m_initialEnergy;
  double Ef = PhysicalConstants::meV * m_finalEnergy;
  // v = sqrt(2 * E / m)
  double vi = sqrt(2.0 * Ei / PhysicalConstants::NeutronMass);
  double vf = sqrt(2.0 * Ef / PhysicalConstants::NeutronMass);
  // Time of flight in seconds = distance / speed
  double tof = L1 / vi + L2 / vf;
  // Return in microsecond units
  return tof * 1e6;
}

// -------------------------------------------------------------------------------------
/** Calculate the scattering angle of the peak  */
double Peak::getScattering() const {
  // The detector is at 2 theta scattering angle
  V3D beamDir = samplePos - sourcePos;
  V3D detDir = detPos - samplePos;

  return detDir.angle(beamDir);
}

// -------------------------------------------------------------------------------------
/** Calculate the azimuthal angle of the peak  */
double Peak::getAzimuthal() const {
  // The detector is at 2 theta scattering angle
  V3D detDir = detPos - samplePos;

  return atan2(detDir.Y(), detDir.X());
}

// -------------------------------------------------------------------------------------
/** Calculate the d-spacing of the peak, in 1/Angstroms  */
double Peak::getDSpacing() const {
  // The detector is at 2 theta scattering angle
  V3D beamDir = samplePos - sourcePos;
  V3D detDir = detPos - samplePos;

  double two_theta;
  try {
    two_theta = detDir.angle(beamDir);
  } catch (std::runtime_error &) {
    two_theta = 0.;
  }

  // In general case (2*pi/d)^2=k_i^2+k_f^2-2*k_i*k_f*cos(two_theta)
  // E_i,f=k_i,f^2*hbar^2/(2 m)
  return 1e10 * PhysicalConstants::h /
         sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV) /
         sqrt(m_initialEnergy + m_finalEnergy -
              2.0 * sqrt(m_initialEnergy * m_finalEnergy) * cos(two_theta));
}

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Lab frame: the goniometer rotation was NOT taken out.
 *
 * Note: There is a 2*pi factor used, so |Q| = 2*pi/wavelength.
 * */
Mantid::Kernel::V3D Peak::getQLabFrame() const {
  // Normalized beam direction
  V3D beamDir = samplePos - sourcePos;
  beamDir /= beamDir.norm();
  // Normalized detector direction
  V3D detDir = (detPos - samplePos);
  detDir /= detDir.norm();

  // Energy in J of the neutron
  double ei = PhysicalConstants::meV * m_initialEnergy;
  // v = sqrt(2.0 * E / m)
  double vi = sqrt(2.0 * ei / PhysicalConstants::NeutronMass);
  // wavenumber = h_bar / mv
  double wi = PhysicalConstants::h_bar / (PhysicalConstants::NeutronMass * vi);
  // in angstroms
  wi *= 1e10;
  // wavevector=1/wavenumber = 2pi/wavelength
  double wvi = 1.0 / wi;
  // Now calculate the wavevector of the scattered neutron
  double wvf = (2.0 * M_PI) / this->getWavelength();
  // And Q in the lab frame
  // Default for ki-kf is positive
  double qSign = 1.0;
  if (convention == "Crystallography")
    qSign = -1.0;
  return (beamDir * wvi - detDir * wvf) * qSign;
}

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Sample frame: the goniometer rotation WAS taken out. */
Mantid::Kernel::V3D Peak::getQSampleFrame() const {
  V3D Qlab = this->getQLabFrame();
  // Multiply by the inverse of the goniometer matrix to get the sample frame
  V3D Qsample = m_InverseGoniometerMatrix * Qlab;
  return Qsample;
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
 * @param detectorDistance :: distance between the sample and the detector.
 *        Used to give a valid TOF. You do NOT need to explicitly set this.
 */
void Peak::setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame,
                           boost::optional<double> detectorDistance) {
  V3D Qlab = m_GoniometerMatrix * QSampleFrame;
  this->setQLabFrame(Qlab, detectorDistance);
}

//----------------------------------------------------------------------------------------------
/** Set the peak using the peak's position in reciprocal space, in the lab
 *frame.
 * The detector position will be determined.
 * DetectorID, row and column will be set to -1 since they are not (necessarily)
 *found.
 * You can call findDetector to look for the detector ID
 *
 * @param QLabFrame :: Q of the center of the peak, in reciprocal space.
 *        This is in inelastic convention: momentum transfer of the LATTICE!
 *        Also, q does have a 2pi factor = it is equal to 2pi/wavelength (in
 *Angstroms).
 * @param detectorDistance :: distance between the sample and the detector. If
 *this is provided. Then we do not
 * ray trace to find the intersecing detector.
 */
void Peak::setQLabFrame(const Mantid::Kernel::V3D &QLabFrame,
                        boost::optional<double> detectorDistance) {
  // Clear out the detector = we can't know them
  m_detectorID = -1;
  detPos = V3D();
  m_det = IDetector_sptr();
  m_row = -1;
  m_col = -1;
  m_bankName = "None";

  // The q-vector direction of the peak is = goniometer * ub * hkl_vector
  V3D q = QLabFrame;

  /* The incident neutron wavevector is along the beam direction, ki = 1/wl
   * (usually z, but referenceframe is definitive).
   * In the inelastic convention, q = ki - kf.
   * The final neutron wavector kf = -qx in x; -qy in y; and (-q.beam_dir+1/wl)
   * in beam direction.
   * AND: norm(kf) = norm(ki) = 2*pi/wavelength
   * THEREFORE: 1/wl = norm(q)^2 / (2*q.beam_dir)
   */
  double norm_q = q.norm();
  if (!this->m_inst) {
    throw std::invalid_argument("Setting QLab without an instrument would lead "
                                "to an inconsistent state for the Peak");
  }
  boost::shared_ptr<const ReferenceFrame> refFrame =
      this->m_inst->getReferenceFrame();
  const V3D refBeamDir = refFrame->vecPointingAlongBeam();
  // Default for ki-kf has -q
  double qSign = 1.0;
  if (convention == "Crystallography")
    qSign = -1.0;
  const double qBeam = q.scalar_prod(refBeamDir) * qSign;

  if (norm_q == 0.0)
    throw std::invalid_argument("Peak::setQLabFrame(): Q cannot be 0,0,0.");
  if (qBeam == 0.0)
    throw std::invalid_argument(
        "Peak::setQLabFrame(): Q cannot be 0 in the beam direction.");

  double one_over_wl = (norm_q * norm_q) / (2.0 * qBeam);
  double wl = (2.0 * M_PI) / one_over_wl;
  if (wl < 0.0) {
    std::ostringstream mess;
    mess << "Peak::setQLabFrame(): Wavelength found was negative (" << wl
         << " Ang)! This Q is not physical.";
    throw std::invalid_argument(mess.str());
  }

  // Save the wavelength
  this->setWavelength(wl);

  // Default for ki-kf has -q
  qSign = -1.0;
  if (convention == "Crystallography")
    qSign = 1.0;

  V3D detectorDir = q * qSign;
  detectorDir[refFrame->pointingAlongBeam()] = one_over_wl - qBeam;
  detectorDir.normalize();

  // Use the given detector distance to find the detector position.
  if (detectorDistance.is_initialized()) {
    detPos = samplePos + detectorDir * detectorDistance.get();
    // We do not-update the detector as by manually setting the distance the
    // client seems to know better.
  } else {
    // Find the detector
    InstrumentRayTracer tracer(m_inst);
    const bool found = findDetector(detectorDir, tracer);
    if (!found) {
      // This is important, so we ought to log when this fails to happen.
      g_log.debug("Could not find detector after setting qLab via setQLab with "
                  "QLab : " +
                  q.toString());

      detPos = getVirtualDetectorPosition(detectorDir);
    }
  }
}

V3D Peak::getVirtualDetectorPosition(const V3D &detectorDir) const {
  const auto component =
      getInstrument()->getComponentByName("extended-detector-space");
  if (!component) {
    return detectorDir; // the best idea we have is just the direction
  }

  const auto object =
      boost::dynamic_pointer_cast<const ObjComponent>(component);
  Geometry::Track track(samplePos, detectorDir);
  object->shape()->interceptSurface(track);
  return track.back().exitPoint;
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
bool Peak::findDetector() {
  InstrumentRayTracer tracer(m_inst);
  return findDetector(tracer);
}

/**
 * Performs the same algorithm as findDetector() but uses a pre-existing
 * InstrumentRayTracer object to be able to take adavtange of its caches.
 * This method should be preferred if findDetector is to be called many times
 * over the same instrument.
 * @param tracer A reference to an existing InstrumentRayTracer object.
 * @return true if the detector ID was found.
 */
bool Peak::findDetector(const InstrumentRayTracer &tracer) {
  // Scattered beam direction
  V3D beam = detPos - samplePos;
  beam.normalize();

  return findDetector(beam, tracer);
}

/**
 * @brief Peak::findDetector : Find the detector along the beam location. sets
 * the detector, and detector position if found
 * @param beam : Detector direction from the sample as V3D
 * @param tracer : Ray tracer to use for detector finding
 * @return True if a detector has been found
 */
bool Peak::findDetector(const Mantid::Kernel::V3D &beam,
                        const InstrumentRayTracer &tracer) {
  bool found = false;
  // Create a ray tracer
  tracer.traceFromSample(beam);
  IDetector_const_sptr det = tracer.getDetectorResult();
  if (det) {
    // Set the detector ID, the row, col, etc.
    this->setDetectorID(det->getID());
    // The old detector position is not more precise if it comes from
    // FindPeaksMD
    detPos = det->getPos();
    found = true;
  }
  // Use tube-gap parameter in instrument parameter file  to find peaks with
  // center in gaps between tubes
  else if (m_inst->hasParameter("tube-gap")) {
    std::vector<double> gaps = m_inst->getNumberParameter("tube-gap", true);
    if (!gaps.empty()) {
      const double gap = static_cast<double>(gaps.front());
      // try adding and subtracting tube-gap in 3 q dimensions to see if you can
      // find detectors on each side of tube gap
      for (int i = 0; i < 3; i++) {
        V3D gapDir = V3D(0., 0., 0.);
        gapDir[i] = gap;
        V3D beam1 = beam + gapDir;
        tracer.traceFromSample(beam1);
        IDetector_const_sptr det1 = tracer.getDetectorResult();
        V3D beam2 = beam - gapDir;
        tracer.traceFromSample(beam2);
        IDetector_const_sptr det2 = tracer.getDetectorResult();
        if (det1 && det2) {
          // Set the detector ID to one of the neighboring pixels
          this->setDetectorID(static_cast<int>(det1->getID()));
          ;
          detPos = det1->getPos();
          found = true;
          break;
        }
      }
    }
  }
  return found;
}

//----------------------------------------------------------------------------------------------
/** Return the run number this peak was measured at. */
int Peak::getRunNumber() const { return m_runNumber; }

/** Set the run number that measured this peak
 * @param m_runNumber :: the run number   */
void Peak::setRunNumber(int m_runNumber) { this->m_runNumber = m_runNumber; }

//----------------------------------------------------------------------------------------------
/** Return the monitor count stored in this peak. */
double Peak::getMonitorCount() const { return m_monitorCount; }

/** Set the monitor count for this peak
 * @param m_monitorCount :: the monitor count */
void Peak::setMonitorCount(double m_monitorCount) {
  this->m_monitorCount = m_monitorCount;
}

//----------------------------------------------------------------------------------------------
/** Get the final neutron energy in meV */
double Peak::getFinalEnergy() const { return m_finalEnergy; }

/** Get the initial (incident) neutron energy in meV */
double Peak::getInitialEnergy() const { return m_initialEnergy; }

/** Get the difference between the initial and final neutron energy in meV */
double Peak::getEnergyTransfer() const {
  return getInitialEnergy() - getFinalEnergy();
}

//----------------------------------------------------------------------------------------------
/** Get the H index of the peak */
double Peak::getH() const { return m_H; }

/** Get the K index of the peak */
double Peak::getK() const { return m_K; }

/** Get the L index of the peak */
double Peak::getL() const { return m_L; }

/** Return the HKL vector */
Mantid::Kernel::V3D Peak::getHKL() const { return V3D(m_H, m_K, m_L); }

/** Return the int HKL vector */
Mantid::Kernel::V3D Peak::getIntHKL() const { return m_IntHKL; }

//----------------------------------------------------------------------------------------------
/** Set the H index of this peak
 * @param m_H :: index to set   */
void Peak::setH(double m_H) { this->m_H = m_H; }

/** Set the K index of this peak
 * @param m_K :: index to set   */
void Peak::setK(double m_K) { this->m_K = m_K; }

/** Set the L index of this peak
 * @param m_L :: index to set   */
void Peak::setL(double m_L) { this->m_L = m_L; }

/** Set the BankName of this peak
 * @param m_bankName :: index to set   */
void Peak::setBankName(std::string m_bankName) {
  this->m_bankName = m_bankName;
}

/** Set all three H,K,L indices of the peak */
void Peak::setHKL(double H, double K, double L) {
  if (m_orig_H == 0 && m_orig_K == 0 && m_orig_L == 0) {
    m_orig_H = m_H;
    m_orig_K = m_K;
    m_orig_L = m_L;
  }
  m_H = H;
  m_K = K;
  m_L = L;
}

/** Reset all three H,K,L indices of the peak to values before setHKL */
void Peak::resetHKL() {
  if (m_orig_H == 0 && m_orig_K == 0 && m_orig_L == 0)
    return;
  m_H = m_orig_H;
  m_K = m_orig_K;
  m_L = m_orig_L;
}

/** Set all HKL
 *
 * @param HKL :: vector with x,y,z -> h,k,l
 */
void Peak::setHKL(const Mantid::Kernel::V3D &HKL) {
  if (m_orig_H == 0 && m_orig_K == 0 && m_orig_L == 0) {
    m_orig_H = m_H;
    m_orig_K = m_K;
    m_orig_L = m_L;
  }
  m_H = HKL.X();
  m_K = HKL.Y();
  m_L = HKL.Z();
}

/** Set int HKL
 *
 * @param HKL :: vector with integer x,y,z -> h,k,l
 */
void Peak::setIntHKL(V3D HKL) {
  m_IntHKL = V3D(boost::math::iround(HKL[0]), boost::math::iround(HKL[1]),
                 boost::math::iround(HKL[2]));
}

/** Set sample position
 *
 * @ doubles x,y,z-> samplePos(x), samplePos(y), samplePos(z)
 */
void Peak::setSamplePos(double samX, double samY, double samZ) {

  this->samplePos[0] = samX;
  this->samplePos[1] = samY;
  this->samplePos[2] = samZ;
}

/** Set sample position
 *
 * @param XYZ :: vector x,y,z-> samplePos(x), samplePos(y), samplePos(z)
 */
void Peak::setSamplePos(const Mantid::Kernel::V3D &XYZ) {

  this->samplePos[0] = XYZ[0];
  this->samplePos[1] = XYZ[1];
  this->samplePos[2] = XYZ[2];
}
//----------------------------------------------------------------------------------------------
/** Return the # of counts in the bin at its peak*/
double Peak::getBinCount() const { return m_binCount; }

/** Return the integrated peak intensity */
double Peak::getIntensity() const { return m_intensity; }

/** Return the error on the integrated peak intensity */
double Peak::getSigmaIntensity() const { return m_sigmaIntensity; }

/** Return the peak intensity divided by the error in the intensity */
double Peak::getIntensityOverSigma() const {
  const auto result = m_intensity / m_sigmaIntensity;
  return (std::isinf(result)) ? 0.0 : result;
}

/** Set the integrated peak intensity
 * @param m_intensity :: intensity value   */
void Peak::setIntensity(double m_intensity) { this->m_intensity = m_intensity; }

/** Set the # of counts in the bin at its peak
 * @param m_binCount :: counts  */
void Peak::setBinCount(double m_binCount) { this->m_binCount = m_binCount; }

/** Set the error on the integrated peak intensity
 * @param m_sigmaIntensity :: intensity error value   */
void Peak::setSigmaIntensity(double m_sigmaIntensity) {
  this->m_sigmaIntensity = m_sigmaIntensity;
}

/** Set the final energy
 * @param m_finalEnergy :: final energy in meV   */
void Peak::setFinalEnergy(double m_finalEnergy) {
  this->m_finalEnergy = m_finalEnergy;
}

/** Set the initial energy
 * @param m_initialEnergy :: initial energy in meV   */
void Peak::setInitialEnergy(double m_initialEnergy) {
  this->m_initialEnergy = m_initialEnergy;
}

// -------------------------------------------------------------------------------------
/** Get the goniometer rotation matrix at which this peak was measured. */
Mantid::Kernel::Matrix<double> Peak::getGoniometerMatrix() const {
  return this->m_GoniometerMatrix;
}

/** Set the goniometer rotation matrix at which this peak was measured.
 * @param goniometerMatrix :: 3x3 matrix that represents the rotation matrix of
 * the goniometer
 * @throw std::invalid_argument if matrix is not 3x3*/
void Peak::setGoniometerMatrix(
    const Mantid::Kernel::Matrix<double> &goniometerMatrix) {
  if ((goniometerMatrix.numCols() != 3) || (goniometerMatrix.numRows() != 3))
    throw std::invalid_argument(
        "Peak::setGoniometerMatrix(): Goniometer matrix must be 3x3.");
  this->m_GoniometerMatrix = goniometerMatrix;
  // Calc the inverse rotation matrix
  m_InverseGoniometerMatrix = m_GoniometerMatrix;
  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument(
        "Peak::setGoniometerMatrix(): Goniometer matrix must be non-singular.");
}

// -------------------------------------------------------------------------------------
/** Find the name of the bank that is the parent of the detector. This works
 * best for RectangularDetector instruments (goes up two levels)
 * @return name of the bank.
 */
std::string Peak::getBankName() const { return m_bankName; }

// -------------------------------------------------------------------------------------
/** For RectangularDetectors only, returns the row (y) of the pixel of the
 * detector.
 * Returns -1 if it could not find it. */
int Peak::getRow() const { return m_row; }

// -------------------------------------------------------------------------------------
/** For RectangularDetectors only, returns the column (x) of the pixel of the
 * detector.
 * Returns -1 if it could not find it. */
int Peak::getCol() const { return m_col; }

// -------------------------------------------------------------------------------------
/**Returns the unique peak number
 * Returns -1 if it could not find it. */
int Peak::getPeakNumber() const { return m_peakNumber; }

// -------------------------------------------------------------------------------------
/**Returns the unique peak number
 * Returns -1 if it could not find it. */
V3D Peak::getIntMNP() const { return m_IntMNP; }

// -------------------------------------------------------------------------------------
/** For RectangularDetectors only, sets the row (y) of the pixel of the
 * detector.
 * @param m_row :: row value   */
void Peak::setRow(int m_row) { this->m_row = m_row; }

// -------------------------------------------------------------------------------------
/** For RectangularDetectors only, sets the column (x) of the pixel of the
 * detector.
 * @param m_col :: col value   */
void Peak::setCol(int m_col) { this->m_col = m_col; }

// -------------------------------------------------------------------------------------
/** Sets the unique peak number
 * @param m_peakNumber :: unique peak number value   */
void Peak::setPeakNumber(int m_peakNumber) {
  this->m_peakNumber = m_peakNumber;
}

// -------------------------------------------------------------------------------------
/** Sets the modulated peak structure number
 * @param MNP :: modulated peak structure value   */
void Peak::setIntMNP(V3D MNP) {
  m_IntMNP = V3D(boost::math::iround(MNP[0]), boost::math::iround(MNP[1]),
                 boost::math::iround(MNP[2]));
}

// -------------------------------------------------------------------------------------
/** Return the detector position vector */
Mantid::Kernel::V3D Peak::getDetPos() const { return detPos; }

// -------------------------------------------------------------------------------------
/** Return the sample position vector */
Mantid::Kernel::V3D Peak::getSamplePos() const { return samplePos; }

// -------------------------------------------------------------------------------------
/** Return the L1 flight path length (source to sample), in meters. */
double Peak::getL1() const { return (samplePos - sourcePos).norm(); }

// -------------------------------------------------------------------------------------
/** Return the L2 flight path length (sample to detector), in meters. */
double Peak::getL2() const { return (detPos - samplePos).norm(); }

// -------------------------------------------------------------------------------------
/** Helper function for displaying/sorting peaks in MantidPlot
 *
 * @param name_in :: name of the column in the table workspace
 * @return a double representing that value (if that's possible)
 * @throw std::runtime_error if you asked for a column that can't convert to
 *double.
 */
double Peak::getValueByColName(const std::string &name_in) const {
  std::string name = name_in;
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  if (name == "runnumber")
    return double(this->getRunNumber());
  else if (name == "detid")
    return double(this->getDetectorID());
  else if (name == "h")
    return this->getH();
  else if (name == "k")
    return this->getK();
  else if (name == "l")
    return this->getL();
  else if (name == "wavelength")
    return this->getWavelength();
  else if (name == "energy")
    return this->getInitialEnergy();
  else if (name == "tof")
    return this->getTOF();
  else if (name == "dspacing")
    return this->getDSpacing();
  else if (name == "intens")
    return this->getIntensity();
  else if (name == "sigint")
    return this->getSigmaIntensity();
  else if (name == "bincount")
    return this->getBinCount();
  else if (name == "row")
    return this->getRow();
  else if (name == "col")
    return this->getCol();
  else if (name == "peaknumber")
    return double(this->getPeakNumber());
  else
    throw std::runtime_error(
        "Peak::getValueByColName() unknown column or column is not a number: " +
        name);
}

/**
 * @brief Get the peak shape
 * @return : const ref to current peak shape.
 */
const PeakShape &Peak::getPeakShape() const { return *this->m_peakShape; }

/**
 * @brief Set the peak shape
 * @param shape : Desired shape
 */
void Peak::setPeakShape(Mantid::Geometry::PeakShape *shape) {
  this->m_peakShape = PeakShape_const_sptr(shape);
}

/**
 * @brief Set the peak shape
 * @param shape : Desired shape
 */
void Peak::setPeakShape(Mantid::Geometry::PeakShape_const_sptr shape) {
  this->m_peakShape = std::move(shape);
}

/**
 * @brief Assignement operator overload
 * @param other : Other peak object to assign from
 * @return this
 */
Peak &Peak::operator=(const Peak &other) {
  if (&other != this) {
    m_inst = other.m_inst;
    m_det = other.m_det;
    m_bankName = other.m_bankName;
    m_detectorID = other.m_detectorID;
    m_H = other.m_H;
    m_K = other.m_K;
    m_L = other.m_L;
    m_intensity = other.m_intensity;
    m_sigmaIntensity = other.m_sigmaIntensity;
    m_binCount = other.m_binCount;
    m_initialEnergy = other.m_initialEnergy;
    m_finalEnergy = other.m_finalEnergy;
    m_GoniometerMatrix = other.m_GoniometerMatrix;
    m_InverseGoniometerMatrix = other.m_InverseGoniometerMatrix;
    m_runNumber = other.m_runNumber;
    m_monitorCount = other.m_monitorCount;
    m_row = other.m_row;
    m_col = other.m_col;
    sourcePos = other.sourcePos;
    samplePos = other.samplePos;
    detPos = other.detPos;
    m_orig_H = other.m_orig_H;
    m_orig_K = other.m_orig_K;
    m_orig_L = other.m_orig_L;
    m_detIDs = other.m_detIDs;
    m_IntHKL = other.m_IntHKL;
    m_IntMNP = other.m_IntMNP;
    convention = other.convention;
    m_peakShape.reset(other.m_peakShape->clone());
  }
  return *this;
}

/**
 Forwarding function. Exposes the detector position directly.
 */
Mantid::Kernel::V3D Peak::getDetectorPositionNoCheck() const {
  return getDetector()->getPos();
}

/**
 Forwarding function. Exposes the detector position directly, but checks that
 the detector is not null before accessing its position. Throws if null.
 */
Mantid::Kernel::V3D Peak::getDetectorPosition() const {
  auto det = getDetector();
  if (det == nullptr) {
    throw Mantid::Kernel::Exception::NullPointerException("Peak", "Detector");
  }
  return getDetector()->getPos();
}

Mantid::Kernel::Logger Peak::g_log("PeakLogger");

} // namespace DataObjects
} // namespace Mantid
