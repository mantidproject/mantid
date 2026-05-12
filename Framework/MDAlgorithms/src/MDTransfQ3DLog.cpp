// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MDTransfQ3DLog.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"

using Mantid::Kernel::DblMatrix;

namespace Mantid::MDAlgorithms {
// register the class, whith conversion factory under namd "Q3DLog"
DECLARE_MD_TRANSFID(MDTransfQ3DLog, Q3DLog)

/** Initializes the class (calls parent and extract rotation logs)
 * @param ConvParams - a MDWSDescription of the input workspace
 * */
void MDTransfQ3DLog::initialize(const MDWSDescription &ConvParams) {
  MDTransfQ3D::initialize(ConvParams);
  // Get the rotation log name from the Gonio (only first Gonio supported)
  m_Wtransf = ConvParams.m_Wtransf; // This is the Bmatrix, converting to Q
  Mantid::Geometry::Goniometer gon = ConvParams.getInWS()->run().getGoniometer();
  Mantid::Geometry::GoniometerAxis a0 = gon.getAxis(0);
  m_rotAxis = a0.rotationaxis;
  if (a0.sense < 0) {
    m_rotAxis = -m_rotAxis;
  }
  // TODO: here we assume that the log is in degrees - need to check this
  m_rotLog = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(
      ConvParams.getInWS()->run().getLogData(a0.name)->clone());
  // TODO: subsequent axes can only be constants (we also assume they have the same axis as the first)
  size_t nmot = gon.getNumberAxes();
  m_angZero = 0;
  if (nmot > 1) {
    for (size_t ix = 1; ix < nmot; ix++) {
      Mantid::Geometry::GoniometerAxis a1 = gon.getAxis(ix);
      if (!a1.name.ends_with("FixedValue")) {
        throw std::invalid_argument("Multiple goniometers with log values not implemented");
      }
      m_angZero += a1.angle * a1.sense;
    }
  }
}

/** Calculates 3D transformation of the variable coordinates depending on 3D coordinates
 * and rotation angle for a given neutron even pulse time.
 * Follows signature of parent calcMatrixCoord() method
 */
bool MDTransfQ3DLog::calcMatrixCoordTime(const double &deltaEOrK0, std::vector<coord_t> &Coord,
                                         const DateAndTime &pulsetime, double &s, double &err) const {
  if (m_Emode == Kernel::DeltaEMode::Elastic) {
    return calcMatrixCoord3DElasticTime(deltaEOrK0, Coord, pulsetime, s, err);
  } else {
    return calcMatrixCoord3DInelasticTime(deltaEOrK0, Coord, pulsetime);
  }
}

/** method calculates workspace-dependent coordinates in inelastic case
 * for particular neutron event. Follows signature of parent calcMatrixCoord3DInelastic()
 */
bool MDTransfQ3DLog::calcMatrixCoord3DInelasticTime(const double &deltaE, std::vector<coord_t> &Coord,
                                                    const DateAndTime &pulseTime) const {
  Coord[3] = static_cast<coord_t>(deltaE);
  if (Coord[3] < m_DimMin[3] || Coord[3] >= m_DimMax[3])
    return false;

  // x,y,z refer to internal coordinate system where Z is the beam direction
  double qx{0.0}, qy{0.0}, qz{0.0};
  if (m_Emode == Kernel::DeltaEMode::Direct) {
    const double kFinal = sqrt((m_eFixed - deltaE) / PhysicalConstants::E_mev_toNeutronWavenumberSq);
    qx = -m_ex * kFinal;
    qy = -m_ey * kFinal;
    qz = m_kFixed - m_ez * kFinal;
  } else {
    qx = -m_ex * m_kFixed;
    qy = -m_ey * m_kFixed;
    const double kInitial = sqrt((m_eFixed + deltaE) / PhysicalConstants::E_mev_toNeutronWavenumberSq);
    qz = kInitial - m_ez * m_kFixed;
  }

  if (convention == "Crystallography") {
    qx = -qx;
    qy = -qy;
    qz = -qz;
  }

  // Get rotmat from the current rotation angle
  double ang = m_rotLog->getSingleValue(pulseTime) + m_angZero;
  DblMatrix mat = DblMatrix(Mantid::Kernel::Quat(ang, m_rotAxis).getRotation()) * m_Wtransf;
  mat.Invert();
  std::vector<double> RotMat = mat.getVector();

  Coord[0] = static_cast<coord_t>(RotMat[0] * qx + RotMat[1] * qy + RotMat[2] * qz);

  if (Coord[0] < m_DimMin[0] || Coord[0] >= m_DimMax[0])
    return false;
  Coord[1] = static_cast<coord_t>(RotMat[3] * qx + RotMat[4] * qy + RotMat[5] * qz);
  if (Coord[1] < m_DimMin[1] || Coord[1] >= m_DimMax[1])
    return false;
  Coord[2] = static_cast<coord_t>(RotMat[6] * qx + RotMat[7] * qy + RotMat[8] * qz);
  if (Coord[2] < m_DimMin[2] || Coord[2] >= m_DimMax[2])
    return false;

  if (std::sqrt(Coord[0] * Coord[0] + Coord[1] * Coord[1] + Coord[2] * Coord[2]) < m_AbsMin)
    return false;

  return true;
}

/** function calculates workspace-dependent coordinates in elastic case.
 * for particular neutron event. Follows signature of parent calcMatrixCoord3DElastic()
 */
bool MDTransfQ3DLog::calcMatrixCoord3DElasticTime(const double &k0, std::vector<coord_t> &Coord,
                                                  const DateAndTime &pulseTime, double &signal, double &errSq) const {

  double qx = -m_ex * k0;
  double qy = -m_ey * k0;
  double qz = (1 - m_ez) * k0;
  if (convention == "Crystallography") {
    qx = -qx;
    qy = -qy;
    qz = -qz;
  }

  // Get rotmat from the current rotation angle
  double ang = m_rotLog->getSingleValue(pulseTime) + m_angZero;
  DblMatrix mat = DblMatrix(Mantid::Kernel::Quat(ang, m_rotAxis).getRotation()) * m_Wtransf;
  mat.Invert();
  std::vector<double> RotMat = mat.getVector();

  // Dimension limits have to be converted to coord_t, otherwise floating point
  // error will cause valid events to be discarded.
  Coord[0] = static_cast<coord_t>(RotMat[0] * qx + RotMat[1] * qy + RotMat[2] * qz);
  if (Coord[0] < static_cast<coord_t>(m_DimMin[0]) || Coord[0] >= static_cast<coord_t>(m_DimMax[0]))
    return false;

  Coord[1] = static_cast<coord_t>(RotMat[3] * qx + RotMat[4] * qy + RotMat[5] * qz);
  if (Coord[1] < static_cast<coord_t>(m_DimMin[1]) || Coord[1] >= static_cast<coord_t>(m_DimMax[1]))
    return false;

  Coord[2] = static_cast<coord_t>(RotMat[6] * qx + RotMat[7] * qy + RotMat[8] * qz);
  if (Coord[2] < static_cast<coord_t>(m_DimMin[2]) || Coord[2] >= static_cast<coord_t>(m_DimMax[2]))
    return false;

  if (std::sqrt(Coord[0] * Coord[0] + Coord[1] * Coord[1] + Coord[2] * Coord[2]) < m_AbsMin)
    return false;

  /*Apply Lorentz corrections if necessary */
  if (m_isLorentzCorrected) {
    double kdash = k0 / (2 * M_PI);
    double correct = m_SinThetaSq * kdash * kdash * kdash * kdash;
    signal *= correct;
    errSq *= (correct * correct);
  }

  return true;
}

} // namespace Mantid::MDAlgorithms
