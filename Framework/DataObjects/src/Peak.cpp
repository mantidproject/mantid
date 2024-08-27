// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
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
Peak::Peak() : BasePeak(), m_row(-1), m_col(-1), m_detectorID(-1), m_initialEnergy(0.), m_finalEnergy(0.) {}

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
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, const Mantid::Kernel::V3D &QLabFrame,
           std::optional<double> detectorDistance)
    : BasePeak() {
  // Initialization of m_inst, sourcePos, m_samplePos
  setInstrument(m_inst);
  // Initialization of m_detectorID, detPos, m_det, m_row, m_col, m_bankName, m_initialEnergy, m_finalEnergy
  setQLabFrame(QLabFrame, std::move(detectorDistance));
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
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, const Mantid::Kernel::V3D &QSampleFrame,
           const Mantid::Kernel::Matrix<double> &goniometer, std::optional<double> detectorDistance)
    : BasePeak(goniometer) {
  // Initialization of m_inst, sourcePos, m_samplePos
  this->setInstrument(m_inst);
  // Initialization of m_detectorID, detPos, m_det, m_row, m_col, m_bankName, m_initialEnergy, m_finalEnergy
  this->setQSampleFrame(QSampleFrame, std::move(detectorDistance));
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param m_detectorID :: ID to the detector of the center of the peak
 * @param m_Wavelength :: incident neutron wavelength, in Angstroms
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID, double m_Wavelength) : BasePeak() {
  // Initialization of m_inst, sourcePos, m_samplePos
  this->setInstrument(m_inst);
  // Initialization of m_detectorID, detPos, m_det, m_row, m_col, m_bankName
  this->setDetectorID(m_detectorID);
  // Initialization of m_initialEnergy, m_finalEnergy
  this->setWavelength(m_Wavelength);
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param m_detectorID :: ID to the detector of the center of the peak
 * @param m_Wavelength :: incident neutron wavelength, in Angstroms
 * @param HKL :: vector with H,K,L position of the peak
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID, double m_Wavelength,
           const Mantid::Kernel::V3D &HKL)
    : BasePeak() {
  // Initialization of m_inst, sourcePos, m_samplePos
  this->setInstrument(m_inst);
  // Initialization of m_detectorID, detPos, m_det, m_row, m_col, m_bankName
  this->setDetectorID(m_detectorID);
  // Initialization of m_initialEnergy, m_finalEnergy
  this->setWavelength(m_Wavelength);
  this->setHKL(HKL);
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param m_detectorID :: ID to the detector of the center of the peak
 * @param m_Wavelength :: incident neutron wavelength, in Angstroms
 * @param HKL :: vector with H,K,L position of the peak
 * @param goniometer :: a 3x3 rotation matrix
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID, double m_Wavelength,
           const Mantid::Kernel::V3D &HKL, const Mantid::Kernel::Matrix<double> &goniometer)
    : BasePeak(goniometer) {
  // Initialization of m_inst, sourcePos, m_samplePos
  this->setInstrument(m_inst);
  // Initialization of m_detectorID, detPos, m_det, m_row, m_col, m_bankName
  this->setDetectorID(m_detectorID);
  // Initialization of m_initialEnergy, m_finalEnergy
  this->setWavelength(m_Wavelength);
  this->setHKL(HKL);
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param scattering :: fake detector position using scattering angle
 * @param m_Wavelength :: incident neutron wavelength, in Angstroms
 */
Peak::Peak(const Geometry::Instrument_const_sptr &m_inst, double scattering, double m_Wavelength)
    : BasePeak(), m_row(-1), m_col(-1) {
  // Initialization of m_inst, sourcePos, m_samplePos
  this->setInstrument(m_inst);
  // Initialization of m_initialEnergy, m_finalEnergy
  this->setWavelength(m_Wavelength);
  m_detectorID = -1;
  // get the approximate location of the detector
  const auto detectorDir = V3D(sin(scattering), 0.0, cos(scattering));
  detPos = getVirtualDetectorPosition(detectorDir);
}

/**
 * @brief Copy constructor
 * @param other : Source
 */
Peak::Peak(const Peak &other) = default;

//----------------------------------------------------------------------------------------------
/** Constructor making a Peak from IPeak interface
 *
 * @param ipeak :: const reference to an IPeak object though actually
 * referencing a Peak object.
 */
Peak::Peak(const Geometry::IPeak &ipeak)
    : BasePeak(ipeak), m_initialEnergy(ipeak.getInitialEnergy()), m_finalEnergy(ipeak.getFinalEnergy()) {
  const auto *peak = dynamic_cast<const Peak *>(&ipeak);
  if (!peak)
    throw std::invalid_argument("Cannot construct a Peak from this non-Peak object");
  setInstrument(peak->getInstrument());
  detid_t id = peak->getDetectorID();
  if (id >= 0)
    setDetectorID(id);
  this->m_detIDs = peak->m_detIDs;
}

//----------------------------------------------------------------------------------------------
Peak::Peak(const Mantid::DataObjects::LeanElasticPeak &lpeak, const Geometry::Instrument_const_sptr &inst,
           std::optional<double> detectorDistance)
    : BasePeak(lpeak) {
  this->setInstrument(inst);
  this->setQLabFrame(lpeak.getQLabFrame(), std::move(detectorDistance));
}

//----------------------------------------------------------------------------------------------
/** Set the incident wavelength of the neutron. Calculates the energy from this.
 * Assumes elastic scattering.
 *
 * @param wavelength :: wavelength in Angstroms.
 */
void Peak::setWavelength(double wavelength) {
  // Velocity of the neutron (non-relativistic)
  double velocity = PhysicalConstants::h / (wavelength * 1e-10 * PhysicalConstants::NeutronMass);
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
  setRow(-1);
  setCol(-1);

  // Go up 2 parents to find the bank/rectangular detector
  IComponent_const_sptr parent = m_det->getParent();

  // Find the ROW by looking at the string name of the pixel. E.g. "pixel12"
  // gives row 12.
  setRow(Strings::endsWithInt(m_det->getName()));

  if (!parent)
    return;
  setBankName(parent->getName());

  // Find the COLUMN by looking at the string name of the parent. E.g. "tube003"
  // gives column 3.
  setCol(Strings::endsWithInt(parent->getName()));

  parent = parent->getParent();
  // Use the parent if there is no grandparent.
  if (!parent)
    return;

  // Use the parent if the grandparent is the instrument
  Instrument_const_sptr instrument = std::dynamic_pointer_cast<const Instrument>(parent);
  if (instrument)
    return;
  // Use the grand-parent whenever possible
  setBankName(parent->getName());
  // For CORELLI, one level above sixteenpack
  if (getBankName() == "sixteenpack") {
    parent = parent->getParent();
    setBankName(parent->getName());
  }

  // Special for rectangular detectors: find the row and column.
  RectangularDetector_const_sptr retDet = std::dynamic_pointer_cast<const RectangularDetector>(parent);
  if (!retDet)
    return;
  std::pair<int, int> xy = retDet->getXYForDetectorID(m_detectorID);
  setRow(xy.second);
  setCol(xy.first);
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
  m_samplePos = sampleObj->getPos();
}

//----------------------------------------------------------------------------------------------
/** Return a shared ptr to the detector at center of peak. */
Geometry::IDetector_const_sptr Peak::getDetector() const { return m_det; }

/** Return a shared ptr to the instrument for this peak. */
Geometry::Instrument_const_sptr Peak::getInstrument() const { return m_inst; }

/** Return a shared ptr to the reference frame from the instrument for this peak. */
std::shared_ptr<const Geometry::ReferenceFrame> Peak::getReferenceFrame() const { return m_inst->getReferenceFrame(); }

/// For RectangularDetectors only, returns the row (y) of the pixel of the detector or -1 if not found
int Peak::getRow() const { return m_row; }

/**
 * @brief For RectangularDetectors only, sets the row (y) of the pixel of the detector
 * @param row :: row value
 */
void Peak::setRow(int row) { m_row = row; }

/// For RectangularDetectors only, returns the column (x) of the pixel of the detector or -1 if not found
int Peak::getCol() const { return m_col; }

/**
 * @brief For RectangularDetectors only, sets the column (x) of the pixel of the detector
 * @param col :: col value
 */
void Peak::setCol(int col) { m_col = col; }

/** Find the name of the bank that is the parent of the detector. This works
 * best for RectangularDetector instruments (goes up two levels)
 * @return name of the bank.
 */
std::string Peak::getBankName() const { return m_bankName; }

/** Set the BankName of this peak
 * @param bankName :: index to set
 */
void Peak::setBankName(std::string bankName) { this->m_bankName = std::move(bankName); }

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
  double wavelength = PhysicalConstants::h / (PhysicalConstants::NeutronMass * velocity);
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
  double Ei = PhysicalConstants::meV * getInitialEnergy();
  double Ef = PhysicalConstants::meV * getFinalEnergy();
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
  V3D beamDir = m_samplePos - sourcePos;
  V3D detDir = detPos - m_samplePos;

  return detDir.angle(beamDir);
}

// -------------------------------------------------------------------------------------
/** Calculate the azimuthal angle of the peak  */
double Peak::getAzimuthal() const {
  // The detector is at 2 theta scattering angle
  V3D detDir = detPos - m_samplePos;

  return atan2(detDir.Y(), detDir.X());
}

// -------------------------------------------------------------------------------------
/** Calculate the scattered beam direction in the sample frame  */
Mantid::Kernel::V3D Peak::getDetectorDirectionSampleFrame() const {
  V3D detDir = detPos - m_samplePos;
  detDir /= detDir.norm();
  return getInverseGoniometerMatrix() * detDir;
}

// -------------------------------------------------------------------------------------
/** Calculate the reverse incident beam direction in the sample frame  */
Mantid::Kernel::V3D Peak::getSourceDirectionSampleFrame() const {
  V3D beamDir = m_samplePos - sourcePos;
  beamDir /= beamDir.norm();
  return getInverseGoniometerMatrix() * beamDir * -1.0;
}

// -------------------------------------------------------------------------------------
/** Calculate the d-spacing of the peak, in 1/Angstroms  */
double Peak::getDSpacing() const {
  // The detector is at 2 theta scattering angle
  V3D beamDir = m_samplePos - sourcePos;
  V3D detDir = detPos - m_samplePos;

  double two_theta;
  try {
    two_theta = detDir.angle(beamDir);
  } catch (std::runtime_error &) {
    two_theta = 0.;
  }

  // In general case (2*pi/d)^2=k_i^2+k_f^2-2*k_i*k_f*cos(two_theta)
  // E_i,f=k_i,f^2*hbar^2/(2 m)
  return 1e10 * PhysicalConstants::h / sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV) /
         sqrt(getInitialEnergy() + getFinalEnergy() -
              2.0 * sqrt(getInitialEnergy() * getFinalEnergy()) * cos(two_theta));
}

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Lab frame: the goniometer rotation was NOT taken out.
 *
 * Note: There is a 2*pi factor used, so |Q| = 2*pi/wavelength.
 * */
Mantid::Kernel::V3D Peak::getQLabFrame() const {
  // Normalized beam direction
  V3D beamDir = m_samplePos - sourcePos;
  beamDir /= beamDir.norm();
  // Normalized detector direction
  V3D detDir = (detPos - m_samplePos);
  detDir /= detDir.norm();

  // Energy in J of the neutron
  double ei = PhysicalConstants::meV * getInitialEnergy();
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
  if (m_convention == "Crystallography")
    qSign = -1.0;
  return (beamDir * wvi - detDir * wvf) * qSign;
}

//----------------------------------------------------------------------------------------------
/** Return the Q change (of the lattice, k_i - k_f) for this peak.
 * The Q is in the Sample frame: the goniometer rotation WAS taken out. */
Mantid::Kernel::V3D Peak::getQSampleFrame() const {
  V3D Qlab = this->getQLabFrame();
  // Multiply by the inverse of the goniometer matrix to get the sample frame
  V3D Qsample = getInverseGoniometerMatrix() * Qlab;
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
void Peak::setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame, std::optional<double> detectorDistance) {
  V3D Qlab = getGoniometerMatrix() * QSampleFrame;
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
 * @param qLab :: Q of the center of the peak, in reciprocal space.
 *        This is in inelastic convention: momentum transfer of the LATTICE!
 *        Also, q does have a 2pi factor = it is equal to 2pi/wavelength (in
 *Angstroms).
 * @param detectorDistance :: distance between the sample and the detector. If
 *this is provided. Then we do not
 * ray trace to find the intersecing detector.
 */
void Peak::setQLabFrame(const Mantid::Kernel::V3D &qLab, std::optional<double> detectorDistance) {
  if (!this->m_inst) {
    throw std::invalid_argument("Setting QLab without an instrument would lead "
                                "to an inconsistent state for the Peak");
  }
  // Clear out the detector = we can't know them
  m_detectorID = -1;
  detPos = V3D();
  m_det = IDetector_sptr();
  m_row = -1;
  m_col = -1;
  setBankName("None");

  /* The q-vector direction of the peak is = goniometer * ub * hkl_vector
   * The incident neutron wavevector is along the beam direction, ki = 1/wl
   * (usually z, but referenceframe is definitive).
   * In the inelastic convention, q = ki - kf.
   * The final neutron wavector kf = -qx in x; -qy in y; and (-q.beam_dir+1/wl)
   * in beam direction.
   * AND: norm(kf) = norm(ki) = 2*pi/wavelength
   * THEREFORE: 1/wl = norm(q)^2 / (2*q.beam_dir)
   */
  const double norm_q = qLab.norm();
  if (norm_q == 0.0)
    throw std::invalid_argument("Peak::setQLabFrame(): Q cannot be 0,0,0.");

  std::shared_ptr<const ReferenceFrame> refFrame = this->m_inst->getReferenceFrame();
  const V3D refBeamDir = refFrame->vecPointingAlongBeam();
  // Default for ki-kf has -q
  const double qSign = (m_convention != "Crystallography") ? 1.0 : -1.0;
  const double qBeam = qLab.scalar_prod(refBeamDir) * qSign;

  if (qBeam == 0.0)
    throw std::invalid_argument("Peak::setQLabFrame(): Q cannot be 0 in the beam direction.");

  const double one_over_wl = (norm_q * norm_q) / (2.0 * qBeam);
  const double wl = (2.0 * M_PI) / one_over_wl;
  if (wl < 0.0) {
    std::ostringstream mess;
    mess << "Peak::setQLabFrame(): Wavelength found was negative (" << wl << " Ang)! This Q is not physical.";
    throw std::invalid_argument(mess.str());
  }

  // Save the wavelength
  this->setWavelength(wl);

  V3D detectorDir = -qLab * qSign;
  detectorDir[refFrame->pointingAlongBeam()] = one_over_wl - qBeam;
  detectorDir.normalize();

  // Use the given detector distance to find the detector position.
  if (detectorDistance.has_value()) {
    detPos = m_samplePos + detectorDir * detectorDistance.value();
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
                  qLab.toString());

      detPos = getVirtualDetectorPosition(detectorDir);
    }
  }
}

V3D Peak::getVirtualDetectorPosition(const V3D &detectorDir) const {
  const auto component = getInstrument()->getComponentByName("extended-detector-space");
  if (!component) {
    return detectorDir; // the best idea we have is just the direction
  }
  const auto object = std::dynamic_pointer_cast<const ObjComponent>(component);
  const auto distance = object->shape()->distance(Geometry::Track(m_samplePos, detectorDir));
  return detectorDir * distance;
}

double Peak::getValueByColName(std::string name) const {
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  std::map<std::string, double> colVals = {
      {"detid", double(m_detectorID)}, {"row", double(m_row)}, {"col", double(m_col)}};
  auto it = colVals.find(name);
  if (it != colVals.end())
    return it->second;
  else
    return BasePeak::getValueByColName(name);
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
  const V3D beam = normalize(detPos - m_samplePos);

  return findDetector(beam, tracer);
}

/**
 * @brief Peak::findDetector : Find the detector along the beam location. sets
 * the detector, and detector position if found
 * @param beam : Detector direction from the sample as V3D
 * @param tracer : Ray tracer to use for detector finding
 * @return True if a detector has been found
 */
bool Peak::findDetector(const Mantid::Kernel::V3D &beam, const InstrumentRayTracer &tracer) {
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
      const auto gap = static_cast<double>(gaps.front());
      // try adding and subtracting tube-gap in 3 q dimensions to see if you can
      // find detectors on each side of tube gap
      for (int i = 0; i < 3; i++) {
        V3D gapDir;
        gapDir[i] = gap;
        V3D beam1 = beam + gapDir;
        tracer.traceFromSample(normalize(beam1));
        IDetector_const_sptr det1 = tracer.getDetectorResult();
        V3D beam2 = beam - gapDir;
        tracer.traceFromSample(normalize(beam2));
        IDetector_const_sptr det2 = tracer.getDetectorResult();
        if (det1 && det2) {
          // compute the cosAngle to select the detector closes to beam
          if (beam1.cosAngle(beam) > beam2.cosAngle(beam)) {
            // det1 is closer, use det1
            this->setDetectorID(static_cast<int>(det1->getID()));
            detPos = det1->getPos();
          } else {
            // det2 is closer, let's use det2
            this->setDetectorID(static_cast<int>(det2->getID()));
            detPos = det2->getPos();
          }
          found = true;
          break;
        }
      }
    }
  }
  return found;
}

//----------------------------------------------------------------------------------------------
/** Get the final neutron energy in meV */
double Peak::getFinalEnergy() const { return m_finalEnergy; }

/** Get the initial (incident) neutron energy in meV */
double Peak::getInitialEnergy() const { return m_initialEnergy; }

/** Get the difference between the initial and final neutron energy in meV */
double Peak::getEnergyTransfer() const { return getInitialEnergy() - getFinalEnergy(); }

/** Set the final energy
 * @param m_finalEnergy :: final energy in meV   */
void Peak::setFinalEnergy(double m_finalEnergy) { this->m_finalEnergy = m_finalEnergy; }

/** Set the initial energy
 * @param m_initialEnergy :: initial energy in meV   */
void Peak::setInitialEnergy(double m_initialEnergy) { this->m_initialEnergy = m_initialEnergy; }

// -------------------------------------------------------------------------------------
/** Return the detector position vector */
Mantid::Kernel::V3D Peak::getDetPos() const { return detPos; }

// -------------------------------------------------------------------------------------
/** Return the L1 flight path length (source to sample), in meters. */
double Peak::getL1() const { return (m_samplePos - sourcePos).norm(); }

// -------------------------------------------------------------------------------------
/** Return the L2 flight path length (sample to detector), in meters. */
double Peak::getL2() const { return (detPos - m_samplePos).norm(); }

/**
 * @brief Assignement operator overload
 * @param other : Other peak object to assign from
 * @return this
 */
Peak &Peak::operator=(const Peak &other) {
  if (&other != this) {
    BasePeak::operator=(other);
    m_inst = other.m_inst;
    m_det = other.m_det;
    m_bankName = other.m_bankName;
    m_row = other.m_row;
    m_col = other.m_col;
    m_detectorID = other.m_detectorID;
    m_initialEnergy = other.m_initialEnergy;
    m_finalEnergy = other.m_finalEnergy;
    sourcePos = other.sourcePos;
    m_samplePos = other.m_samplePos;
    detPos = other.detPos;
    m_detIDs = other.m_detIDs;
  }
  return *this;
}

/**
 Forwarding function. Exposes the detector position directly.
 */
Mantid::Kernel::V3D Peak::getDetectorPositionNoCheck() const { return getDetector()->getPos(); }

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

} // namespace Mantid::DataObjects
