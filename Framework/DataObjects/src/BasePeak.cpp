// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/BasePeak.h"
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
BasePeak::BasePeak()
    : m_samplePos(V3D(0, 0, 0)), m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0), m_binCount(0),
      m_absorptionWeightedPathLength(0), m_GoniometerMatrix(3, 3, true), m_InverseGoniometerMatrix(3, 3, true),
      m_runNumber(0), m_monitorCount(0), m_peakNumber(0), m_intHKL(V3D(0, 0, 0)), m_intMNP(V3D(0, 0, 0)),
      m_peakShape(std::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
}

//----------------------------------------------------------------------------------------------
/** Constructor including goniometer
 *
 * @param goniometer :: a 3x3 rotation matrix
 */
BasePeak::BasePeak(const Mantid::Kernel::Matrix<double> &goniometer)
    : m_H(0), m_K(0), m_L(0), m_intensity(0), m_sigmaIntensity(0), m_binCount(0), m_absorptionWeightedPathLength(0),
      m_GoniometerMatrix(goniometer), m_InverseGoniometerMatrix(goniometer), m_runNumber(0), m_monitorCount(0),
      m_peakNumber(0), m_intHKL(V3D(0, 0, 0)), m_intMNP(V3D(0, 0, 0)), m_peakShape(std::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");

  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument("BasePeak::ctor(): Goniometer matrix must non-singular.");
}

BasePeak::BasePeak(const BasePeak &other)
    : convention(other.convention), m_samplePos(other.m_samplePos), m_H(other.m_H), m_K(other.m_K), m_L(other.m_L),
      m_intensity(other.m_intensity), m_sigmaIntensity(other.m_sigmaIntensity), m_binCount(other.m_binCount),
      m_absorptionWeightedPathLength(other.m_absorptionWeightedPathLength),
      m_GoniometerMatrix(other.m_GoniometerMatrix), m_InverseGoniometerMatrix(other.m_InverseGoniometerMatrix),
      m_runNumber(other.m_runNumber), m_monitorCount(other.m_monitorCount), m_peakNumber(other.m_peakNumber),
      m_intHKL(other.m_intHKL), m_intMNP(other.m_intMNP), m_peakShape(other.m_peakShape->clone()) {}

//----------------------------------------------------------------------------------------------
/** Constructor making a LeanPeak from IPeak interface
 *
 * @param ipeak :: const reference to an IPeak object
 */
BasePeak::BasePeak(const Geometry::IPeak &ipeak)
    : IPeak(ipeak), m_H(ipeak.getH()), m_K(ipeak.getK()), m_L(ipeak.getL()), m_intensity(ipeak.getIntensity()),
      m_sigmaIntensity(ipeak.getSigmaIntensity()), m_binCount(ipeak.getBinCount()),
      m_absorptionWeightedPathLength(ipeak.getAbsorptionWeightedPathLength()),
      m_GoniometerMatrix(ipeak.getGoniometerMatrix()), m_InverseGoniometerMatrix(ipeak.getGoniometerMatrix()),
      m_runNumber(ipeak.getRunNumber()), m_monitorCount(ipeak.getMonitorCount()), m_peakNumber(ipeak.getPeakNumber()),
      m_intHKL(ipeak.getIntHKL()), m_intMNP(ipeak.getIntMNP()), m_peakShape(std::make_shared<NoShape>()) {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");

  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument("Peak::ctor(): Goniometer matrix must non-singular.");
}

//----------------------------------------------------------------------------------------------
/** Return the run number this peak was measured at. */
int BasePeak::getRunNumber() const { return m_runNumber; }

/** Set the run number that measured this peak
 * @param m_runNumber :: the run number   */
void BasePeak::setRunNumber(int m_runNumber) { this->m_runNumber = m_runNumber; }

//----------------------------------------------------------------------------------------------
/** Return the monitor count stored in this peak. */
double BasePeak::getMonitorCount() const { return m_monitorCount; }

/** Set the monitor count for this peak
 * @param m_monitorCount :: the monitor count */
void BasePeak::setMonitorCount(double m_monitorCount) { this->m_monitorCount = m_monitorCount; }

//----------------------------------------------------------------------------------------------
/** Get the H index of the peak */
double BasePeak::getH() const { return m_H; }

/** Get the K index of the peak */
double BasePeak::getK() const { return m_K; }

/** Get the L index of the peak */
double BasePeak::getL() const { return m_L; }

/** Return the HKL vector */
Mantid::Kernel::V3D BasePeak::getHKL() const { return V3D(m_H, m_K, m_L); }

/** Return True if the peak has been indexed */
bool BasePeak::isIndexed() const {
  if (m_H == 0. && m_K == 0. && m_L == 0.)
    return false;
  return true;
}

/** Return the int HKL vector */
Mantid::Kernel::V3D BasePeak::getIntHKL() const { return m_intHKL; }

/** Return the int MNP vector */
V3D BasePeak::getIntMNP() const { return m_intMNP; }

//----------------------------------------------------------------------------------------------
/** Set the H index of this peak
 * @param m_H :: index to set   */
void BasePeak::setH(double m_H) { this->m_H = m_H; }

/** Set the K index of this peak
 * @param m_K :: index to set   */
void BasePeak::setK(double m_K) { this->m_K = m_K; }

/** Set the L index of this peak
 * @param m_L :: index to set   */
void BasePeak::setL(double m_L) { this->m_L = m_L; }

/** Set all three H,K,L indices of the peak */
void BasePeak::setHKL(double H, double K, double L) {
  m_H = H;
  m_K = K;
  m_L = L;
}

/** Set all HKL
 *
 * @param HKL :: vector with x,y,z -> h,k,l
 */
void BasePeak::setHKL(const Mantid::Kernel::V3D &HKL) { this->setHKL(HKL.X(), HKL.Y(), HKL.Z()); }

/** Set int HKL
 *
 * @param HKL :: vector with integer x,y,z -> h,k,l
 */
void BasePeak::setIntHKL(const V3D &HKL) { m_intHKL = V3D(std::round(HKL[0]), std::round(HKL[1]), std::round(HKL[2])); }

/** Sets the modulated peak structure number
 * @param MNP :: modulated peak structure value
 */
void BasePeak::setIntMNP(const V3D &MNP) { m_intMNP = V3D(std::round(MNP[0]), std::round(MNP[1]), std::round(MNP[2])); }

/** Return the sample position vector */
Mantid::Kernel::V3D BasePeak::getSamplePos() const { return m_samplePos; }

/** Set sample position
 *
 * @ doubles x,y,z-> m_samplePos(x), m_samplePos(y), m_samplePos(z)
 */
void BasePeak::setSamplePos(double samX, double samY, double samZ) {

  this->m_samplePos[0] = samX;
  this->m_samplePos[1] = samY;
  this->m_samplePos[2] = samZ;
}

/** Set sample position
 *
 * @param XYZ :: vector x,y,z-> m_samplePos(x), m_samplePos(y), m_samplePos(z)
 */
void BasePeak::setSamplePos(const Mantid::Kernel::V3D &XYZ) {

  this->m_samplePos[0] = XYZ[0];
  this->m_samplePos[1] = XYZ[1];
  this->m_samplePos[2] = XYZ[2];
}

//----------------------------------------------------------------------------------------------
/** Return the # of counts in the bin at its peak*/
double BasePeak::getBinCount() const { return m_binCount; }

/** Return the integrated peak intensity */
double BasePeak::getIntensity() const { return m_intensity; }

/** Return the error on the integrated peak intensity */
double BasePeak::getSigmaIntensity() const { return m_sigmaIntensity; }

/** Return the peak intensity divided by the error in the intensity */
double BasePeak::getIntensityOverSigma() const {
  const auto result = m_intensity / m_sigmaIntensity;
  return (std::isfinite(result)) ? result : 0.0;
}

/** Set the integrated peak intensity
 * @param m_intensity :: intensity value   */
void BasePeak::setIntensity(double m_intensity) { this->m_intensity = m_intensity; }

/** Set the # of counts in the bin at its peak
 * @param m_binCount :: counts  */
void BasePeak::setBinCount(double m_binCount) { this->m_binCount = m_binCount; }

/** Set the error on the integrated peak intensity
 * @param m_sigmaIntensity :: intensity error value   */
void BasePeak::setSigmaIntensity(double m_sigmaIntensity) { this->m_sigmaIntensity = m_sigmaIntensity; }

// -------------------------------------------------------------------------------------
/** Get the goniometer rotation matrix at which this peak was measured. */
Mantid::Kernel::Matrix<double> BasePeak::getGoniometerMatrix() const { return this->m_GoniometerMatrix; }

// -------------------------------------------------------------------------------------
/** Get the goniometer rotation matrix at which this peak was measured. */
Mantid::Kernel::Matrix<double> BasePeak::getInverseGoniometerMatrix() const { return this->m_InverseGoniometerMatrix; }

/** Set the goniometer rotation matrix at which this peak was measured.
 * @param goniometerMatrix :: 3x3 matrix that represents the rotation matrix of
 * the goniometer
 * @throw std::invalid_argument if matrix is not 3x3*/
void BasePeak::setGoniometerMatrix(const Mantid::Kernel::Matrix<double> &goniometerMatrix) {
  if ((goniometerMatrix.numCols() != 3) || (goniometerMatrix.numRows() != 3))
    throw std::invalid_argument("BasePeak::setGoniometerMatrix(): Goniometer matrix must be 3x3.");
  this->m_GoniometerMatrix = goniometerMatrix;
  // Calc the inverse rotation matrix
  m_InverseGoniometerMatrix = m_GoniometerMatrix;
  if (fabs(m_InverseGoniometerMatrix.Invert()) < 1e-8)
    throw std::invalid_argument("BasePeak::setGoniometerMatrix(): Goniometer "
                                "matrix must be non-singular.");
}

// -------------------------------------------------------------------------------------
/**Returns the unique peak number
 * Returns -1 if it could not find it. */
int BasePeak::getPeakNumber() const { return m_peakNumber; }

// -------------------------------------------------------------------------------------
/** Sets the unique peak number
 * @param m_peakNumber :: unique peak number value   */
void BasePeak::setPeakNumber(int m_peakNumber) { this->m_peakNumber = m_peakNumber; }

// -------------------------------------------------------------------------------------
/** Helper function for displaying/sorting peaks
 *
 * @param name :: name of the column in the table workspace. The matching is
 * case-insensitive.
 * @return a double representing that value (if that's possible)
 * @throw std::runtime_error if you asked for a column that can't convert to
 *double.
 */
double BasePeak::getValueByColName(std::string name) const {
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  if (name == "runnumber")
    return double(this->getRunNumber());
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
  else if (name == "peaknumber")
    return double(this->getPeakNumber());
  else if (name == "tbar")
    return this->getAbsorptionWeightedPathLength();
  else
    throw std::runtime_error("BasePeak::getValueByColName() unknown column or "
                             "column is not a number: " +
                             name);
}

/**
 * @brief Get the peak shape
 * @return : const ref to current peak shape.
 */
const PeakShape &BasePeak::getPeakShape() const { return *this->m_peakShape; }

/**
 * @brief Set the peak shape
 * @param shape : Desired shape
 */
void BasePeak::setPeakShape(Mantid::Geometry::PeakShape *shape) { this->m_peakShape = PeakShape_const_sptr(shape); }

/**
 * @brief Set the peak shape
 * @param shape : Desired shape
 */
void BasePeak::setPeakShape(Mantid::Geometry::PeakShape_const_sptr shape) { this->m_peakShape = std::move(shape); }

/**
 * @brief Assignement operator overload
 * @param other : Other peak object to assign from
 * @return this
 */
BasePeak &BasePeak::operator=(const BasePeak &other) {
  if (&other != this) {
    m_H = other.m_H;
    m_K = other.m_K;
    m_L = other.m_L;
    m_intensity = other.m_intensity;
    m_sigmaIntensity = other.m_sigmaIntensity;
    m_binCount = other.m_binCount;
    m_GoniometerMatrix = other.m_GoniometerMatrix;
    m_InverseGoniometerMatrix = other.m_InverseGoniometerMatrix;
    m_runNumber = other.m_runNumber;
    m_monitorCount = other.m_monitorCount;
    m_peakNumber = other.m_peakNumber;
    m_intHKL = other.m_intHKL;
    m_intMNP = other.m_intMNP;
    m_peakShape.reset(other.m_peakShape->clone());
    m_absorptionWeightedPathLength = other.m_absorptionWeightedPathLength;
    convention = other.convention;
  }
  return *this;
}

/**
 * @brief Set the absorption weighted path length
 * @param pathLength : Desired path length
 */
void BasePeak::setAbsorptionWeightedPathLength(double pathLength) { m_absorptionWeightedPathLength = pathLength; }

/**
 * Gets the absorption weighted path length
 */
double BasePeak::getAbsorptionWeightedPathLength() const { return m_absorptionWeightedPathLength; }

double BasePeak::calculateWavelengthFromQLab(const V3D qLab) {
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
    throw std::invalid_argument("BasePeak::setQLabFrame(): Q cannot be 0,0,0.");

  std::shared_ptr<const ReferenceFrame> refFrame = getReferenceFrame();
  V3D refBeamDir(0, 0, 1); // default beam direction +Z
  if (refFrame)
    refBeamDir = refFrame->vecPointingAlongBeam();

  // Default for ki-kf has -q
  const double qSign = (convention != "Crystallography") ? 1.0 : -1.0;
  const double qBeam = qLab.scalar_prod(refBeamDir) * qSign;

  if (qBeam == 0.0)
    throw std::invalid_argument("BasePeak::setQLabFrame(): Q cannot be 0 in the beam direction.");

  const double one_over_wl = (norm_q * norm_q) / (2.0 * qBeam);
  const double wl = (2.0 * M_PI) / one_over_wl;
  if (wl < 0.0) {
    std::ostringstream mess;
    mess << "BasePeak::setQLabFrame(): Wavelength found was negative (" << wl << " Ang)! This Q is not physical.";
    throw std::invalid_argument(mess.str());
  }

  return wl;
}

Mantid::Kernel::Logger BasePeak::g_log("PeakLogger");

} // namespace DataObjects
} // namespace Mantid
