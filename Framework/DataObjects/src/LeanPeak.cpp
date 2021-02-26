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
LeanPeak::LeanPeak()
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_initialEnergy(0.), m_finalEnergy(0.),
      m_absorptionWeightedPathLength(0), m_GoniometerMatrix(3, 3, true),
      m_InverseGoniometerMatrix(3, 3, true), m_runNumber(0), m_monitorCount(0),
      m_row(-1), m_col(-1), m_Qsample(V3D(0, 0, 0)), m_peakNumber(0),
      m_intHKL(V3D(0, 0, 0)), m_intMNP(V3D(0, 0, 0)),
      m_peakShape(std::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
}

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the sample frame)
 * and a goniometer rotation matrix.
 * No detector ID is set.
 *
 * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in
 *the sample frame (goniometer rotation accounted for).
 */
LeanPeak::LeanPeak(const Mantid::Kernel::V3D &QSampleFrame)
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_initialEnergy(0.0), m_finalEnergy(0.0),
      m_absorptionWeightedPathLength(0), m_GoniometerMatrix(3, 3, true),
      m_InverseGoniometerMatrix(3, 3, true), m_runNumber(0), m_monitorCount(0),
      m_peakNumber(0), m_intHKL(V3D(0, 0, 0)), m_intMNP(V3D(0, 0, 0)),
      m_peakShape(std::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
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
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_initialEnergy(0.), m_finalEnergy(0.),
      m_absorptionWeightedPathLength(0), m_GoniometerMatrix(goniometer),
      m_InverseGoniometerMatrix(goniometer), m_runNumber(0), m_monitorCount(0),
      m_peakNumber(0), m_intHKL(V3D(0, 0, 0)), m_intMNP(V3D(0, 0, 0)),
      m_peakShape(std::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument(
        "Peak::ctor(): Goniometer matrix must non-singular.");
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
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_absorptionWeightedPathLength(0),
      m_GoniometerMatrix(3, 3, true), m_InverseGoniometerMatrix(3, 3, true),
      m_runNumber(0), m_monitorCount(0), m_peakNumber(0),
      m_intHKL(V3D(0, 0, 0)), m_intMNP(V3D(0, 0, 0)),
      m_peakShape(std::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
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
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0),
      m_binCount(0), m_absorptionWeightedPathLength(0),
      m_GoniometerMatrix(goniometer), m_InverseGoniometerMatrix(goniometer),
      m_runNumber(0), m_monitorCount(0), m_peakNumber(0),
      m_intHKL(V3D(0, 0, 0)), m_intMNP(V3D(0, 0, 0)),
      m_peakShape(std::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument(
        "Peak::ctor(): Goniometer matrix must non-singular.");
  this->setQSampleFrame(QSampleFrame);
  this->setWavelength(wavelength);
}

/**
 * @brief Copy constructor
 * @param other : Source
 * @return
 */
LeanPeak::LeanPeak(const LeanPeak &other)
    : m_bankName(other.m_bankName), m_H(other.m_H), m_K(other.m_K),
      m_L(other.m_L), m_intensity(other.m_intensity),
      m_sigmaIntensity(other.m_sigmaIntensity), m_binCount(other.m_binCount),
      m_initialEnergy(other.m_initialEnergy),
      m_finalEnergy(other.m_finalEnergy),
      m_absorptionWeightedPathLength(other.m_absorptionWeightedPathLength),
      m_GoniometerMatrix(other.m_GoniometerMatrix),
      m_InverseGoniometerMatrix(other.m_InverseGoniometerMatrix),
      m_runNumber(other.m_runNumber), m_monitorCount(other.m_monitorCount),
      m_row(other.m_row), m_col(other.m_col), m_Qsample(other.m_Qsample),
      m_peakNumber(other.m_peakNumber), m_intHKL(other.m_intHKL),
      m_intMNP(other.m_intMNP), m_peakShape(other.m_peakShape->clone()),
      convention(other.convention) {}
//----------------------------------------------------------------------------------------------
/** Set the incident wavelength of the neutron. Calculates the energy from this.
 * Assumes elastic scattering.
 *
 * @param wavelength :: wavelength in Angstroms.
 */
void LeanPeak::setWavelength(double wavelength) {
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
/** Calculate the neutron wavelength (in angstroms) at the peak
 * (Note for inelastic scattering - it is the wavelength corresponding to the
 * final energy)*/
double LeanPeak::getWavelength() const {
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

//----------------------------------------------------------------------------------------------
/** Return the run number this peak was measured at. */
int LeanPeak::getRunNumber() const { return m_runNumber; }

/** Set the run number that measured this peak
 * @param m_runNumber :: the run number   */
void LeanPeak::setRunNumber(int m_runNumber) {
  this->m_runNumber = m_runNumber;
}

//----------------------------------------------------------------------------------------------
/** Return the monitor count stored in this peak. */
double LeanPeak::getMonitorCount() const { return m_monitorCount; }

/** Set the monitor count for this peak
 * @param m_monitorCount :: the monitor count */
void LeanPeak::setMonitorCount(double m_monitorCount) {
  this->m_monitorCount = m_monitorCount;
}

//----------------------------------------------------------------------------------------------
/** Get the final neutron energy in meV */
double LeanPeak::getFinalEnergy() const { return m_finalEnergy; }

/** Get the initial (incident) neutron energy in meV */
double LeanPeak::getInitialEnergy() const { return m_initialEnergy; }

/** Get the difference between the initial and final neutron energy in meV */
double LeanPeak::getEnergyTransfer() const {
  return getInitialEnergy() - getFinalEnergy();
}

//----------------------------------------------------------------------------------------------
/** Get the H index of the peak */
double LeanPeak::getH() const { return m_H; }

/** Get the K index of the peak */
double LeanPeak::getK() const { return m_K; }

/** Get the L index of the peak */
double LeanPeak::getL() const { return m_L; }

/** Return the HKL vector */
Mantid::Kernel::V3D LeanPeak::getHKL() const { return V3D(m_H, m_K, m_L); }

/** Return True if the peak has been indexed */
bool LeanPeak::isIndexed() const {
  if (m_H == 0. && m_K == 0. && m_L == 0.)
    return false;
  return true;
}

/** Return the int HKL vector */
Mantid::Kernel::V3D LeanPeak::getIntHKL() const { return m_intHKL; }

/** Return the int MNP vector */
V3D LeanPeak::getIntMNP() const { return m_intMNP; }

//----------------------------------------------------------------------------------------------
/** Set the H index of this peak
 * @param m_H :: index to set   */
void LeanPeak::setH(double m_H) { this->m_H = m_H; }

/** Set the K index of this peak
 * @param m_K :: index to set   */
void LeanPeak::setK(double m_K) { this->m_K = m_K; }

/** Set the L index of this peak
 * @param m_L :: index to set   */
void LeanPeak::setL(double m_L) { this->m_L = m_L; }

/** Set the BankName of this peak
 * @param m_bankName :: index to set   */
void LeanPeak::setBankName(std::string m_bankName) {
  this->m_bankName = std::move(m_bankName);
}

/** Set all three H,K,L indices of the peak */
void LeanPeak::setHKL(double H, double K, double L) {
  m_H = H;
  m_K = K;
  m_L = L;
}

/** Set all HKL
 *
 * @param HKL :: vector with x,y,z -> h,k,l
 */
void LeanPeak::setHKL(const Mantid::Kernel::V3D &HKL) {
  m_H = HKL.X();
  m_K = HKL.Y();
  m_L = HKL.Z();
}

/** Set int HKL
 *
 * @param HKL :: vector with integer x,y,z -> h,k,l
 */
void LeanPeak::setIntHKL(const V3D &HKL) {
  m_intHKL = V3D(std::round(HKL[0]), std::round(HKL[1]), std::round(HKL[2]));
}

/** Sets the modulated peak structure number
 * @param MNP :: modulated peak structure value
 */
void LeanPeak::setIntMNP(const V3D &MNP) {
  m_intMNP = V3D(std::round(MNP[0]), std::round(MNP[1]), std::round(MNP[2]));
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
//----------------------------------------------------------------------------------------------
/** Return the # of counts in the bin at its peak*/
double LeanPeak::getBinCount() const { return m_binCount; }

/** Return the integrated peak intensity */
double LeanPeak::getIntensity() const { return m_intensity; }

/** Return the error on the integrated peak intensity */
double LeanPeak::getSigmaIntensity() const { return m_sigmaIntensity; }

/** Return the peak intensity divided by the error in the intensity */
double LeanPeak::getIntensityOverSigma() const {
  const auto result = m_intensity / m_sigmaIntensity;
  return (std::isinf(result)) ? 0.0 : result;
}

/** Set the integrated peak intensity
 * @param m_intensity :: intensity value   */
void LeanPeak::setIntensity(double m_intensity) {
  this->m_intensity = m_intensity;
}

/** Set the # of counts in the bin at its peak
 * @param m_binCount :: counts  */
void LeanPeak::setBinCount(double m_binCount) { this->m_binCount = m_binCount; }

/** Set the error on the integrated peak intensity
 * @param m_sigmaIntensity :: intensity error value   */
void LeanPeak::setSigmaIntensity(double m_sigmaIntensity) {
  this->m_sigmaIntensity = m_sigmaIntensity;
}

/** Set the final energy
 * @param m_finalEnergy :: final energy in meV   */
void LeanPeak::setFinalEnergy(double m_finalEnergy) {
  this->m_finalEnergy = m_finalEnergy;
}

/** Set the initial energy
 * @param m_initialEnergy :: initial energy in meV   */
void LeanPeak::setInitialEnergy(double m_initialEnergy) {
  this->m_initialEnergy = m_initialEnergy;
}

// -------------------------------------------------------------------------------------
/** Get the goniometer rotation matrix at which this peak was measured. */
Mantid::Kernel::Matrix<double> LeanPeak::getGoniometerMatrix() const {
  return this->m_GoniometerMatrix;
}

/** Set the goniometer rotation matrix at which this peak was measured.
 * @param goniometerMatrix :: 3x3 matrix that represents the rotation matrix of
 * the goniometer
 * @throw std::invalid_argument if matrix is not 3x3*/
void LeanPeak::setGoniometerMatrix(
    const Mantid::Kernel::Matrix<double> &goniometerMatrix) {
  if ((goniometerMatrix.numCols() != 3) || (goniometerMatrix.numRows() != 3))
    throw std::invalid_argument(
        "LeanPeak::setGoniometerMatrix(): Goniometer matrix must be 3x3.");
  this->m_GoniometerMatrix = goniometerMatrix;
  // Calc the inverse rotation matrix
  m_InverseGoniometerMatrix = m_GoniometerMatrix;
  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument("LeanPeak::setGoniometerMatrix(): Goniometer "
                                "matrix must be non-singular.");
}

// -------------------------------------------------------------------------------------
/** Find the name of the bank that is the parent of the detector. This works
 * best for RectangularDetector instruments (goes up two levels)
 * @return name of the bank.
 */
std::string LeanPeak::getBankName() const { return m_bankName; }

// -------------------------------------------------------------------------------------
/** For RectangularDetectors only, returns the row (y) of the pixel of the
 * detector.
 * Returns -1 if it could not find it. */
int LeanPeak::getRow() const { return m_row; }

// -------------------------------------------------------------------------------------
/** For RectangularDetectors only, returns the column (x) of the pixel of the
 * detector.
 * Returns -1 if it could not find it. */
int LeanPeak::getCol() const { return m_col; }

// -------------------------------------------------------------------------------------
/**Returns the unique peak number
 * Returns -1 if it could not find it. */
int LeanPeak::getPeakNumber() const { return m_peakNumber; }

// -------------------------------------------------------------------------------------
/** For RectangularDetectors only, sets the row (y) of the pixel of the
 * detector.
 * @param m_row :: row value   */
void LeanPeak::setRow(int m_row) { this->m_row = m_row; }

// -------------------------------------------------------------------------------------
/** For RectangularDetectors only, sets the column (x) of the pixel of the
 * detector.
 * @param m_col :: col value   */
void LeanPeak::setCol(int m_col) { this->m_col = m_col; }

// -------------------------------------------------------------------------------------
/** Sets the unique peak number
 * @param m_peakNumber :: unique peak number value   */
void LeanPeak::setPeakNumber(int m_peakNumber) {
  this->m_peakNumber = m_peakNumber;
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

// -------------------------------------------------------------------------------------
/** Helper function for displaying/sorting peaks
 *
 * @param name :: name of the column in the table workspace. The matching is
 * case-insensitive.
 * @return a double representing that value (if that's possible)
 * @throw std::runtime_error if you asked for a column that can't convert to
 *double.
 */
double LeanPeak::getValueByColName(std::string name) const {
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
  else if (name == "intens/sigint")
    return this->getIntensityOverSigma();
  else if (name == "bincount")
    return this->getBinCount();
  else if (name == "row")
    return this->getRow();
  else if (name == "col")
    return this->getCol();
  else if (name == "peaknumber")
    return double(this->getPeakNumber());
  else if (name == "tbar")
    return this->getAbsorptionWeightedPathLength();
  else
    throw std::runtime_error("LeanPeak::getValueByColName() unknown column or "
                             "column is not a number: " +
                             name);
}

/**
 * @brief Get the peak shape
 * @return : const ref to current peak shape.
 */
const PeakShape &LeanPeak::getPeakShape() const { return *this->m_peakShape; }

/**
 * @brief Set the peak shape
 * @param shape : Desired shape
 */
void LeanPeak::setPeakShape(Mantid::Geometry::PeakShape *shape) {
  this->m_peakShape = PeakShape_const_sptr(shape);
}

/**
 * @brief Set the peak shape
 * @param shape : Desired shape
 */
void LeanPeak::setPeakShape(Mantid::Geometry::PeakShape_const_sptr shape) {
  this->m_peakShape = std::move(shape);
}

/**
 * @brief Assignement operator overload
 * @param other : Other peak object to assign from
 * @return this
 */
LeanPeak &LeanPeak::operator=(const LeanPeak &other) {
  if (&other != this) {
    m_bankName = other.m_bankName;
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
    m_intHKL = other.m_intHKL;
    m_intMNP = other.m_intMNP;
    convention = other.convention;
    m_peakShape.reset(other.m_peakShape->clone());
    m_absorptionWeightedPathLength = other.m_absorptionWeightedPathLength;
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

/**
 * @brief Set the absorption weighted path length
 * @param pathLength : Desired path length
 */
void LeanPeak::setAbsorptionWeightedPathLength(double pathLength) {
  m_absorptionWeightedPathLength = pathLength;
}

/**
 * Gets the absorption weighted path length
 */
double LeanPeak::getAbsorptionWeightedPathLength() const {
  return m_absorptionWeightedPathLength;
}

Mantid::Kernel::Logger LeanPeak::g_log("PeakLogger");

} // namespace DataObjects
} // namespace Mantid
