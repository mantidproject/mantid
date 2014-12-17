#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitBMatrix.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h"
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/ModeratorModel.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace Mantid {
namespace MDAlgorithms {
/**
 * Construct a B matrix. The size is (3*ki+3*kf,TobyFitYVector::variableCount())
 * The number of rows is 3 d.o.f. for ki plus 3 for kf. The number of columns
 * must match the number of monte carlo sample variables so that the
 * two can be multiplied to find the integration variables
 */
TobyFitBMatrix::TobyFitBMatrix()
    : Kernel::DblMatrix(6, TobyFitYVector::length()) {}

/**
 * Calculate the values for this observation & QDeltaE point
 * @param observation :: The current observation
 * @param qOmega :: The current point in Q-DeltaE space
 */
void TobyFitBMatrix::recalculate(const CachedExperimentInfo &observation,
                                 const QOmegaPoint &qOmega) {
  // Compute transformation matrices
  const API::ExperimentInfo &exptInfo = observation.experimentInfo();
  const Geometry::OrientedLattice &lattice =
      exptInfo.sample().getOrientedLattice();
  const Kernel::DblMatrix &sMat = lattice.getU();

  // Wavevectors (1/Angstrom)
  const double efixed = observation.getEFixed();
  const double wi =
      std::sqrt(efixed / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  const double wf = std::sqrt((efixed - qOmega.deltaE) /
                              PhysicalConstants::E_mev_toNeutronWavenumberSq);

  // Velocities (m/s)
  static const double perAngstromToMetresPerSec =
      1e10 * PhysicalConstants::h_bar / PhysicalConstants::NeutronMass;
  const double veli = wi * perAngstromToMetresPerSec;
  const double velf = wf * perAngstromToMetresPerSec;

  // Distances
  const double x0 = observation.moderatorToFirstChopperDistance();
  const double xa = observation.firstApertureToFirstChopperDistance();
  const double x1 = observation.firstChopperToSampleDistance();
  const double x2 = observation.sampleToDetectorDistance();

  // Travel times
  const double ti = x0 / veli;
  const double tf = x2 / velf;

  // Chopper frequency
  const API::ChopperModel &chopper0 = exptInfo.chopperModel(0);
  const double angvel = chopper0.getAngularVelocity();

  // Moderator tilt angle
  const API::ModeratorModel &moderator = exptInfo.moderatorModel();
  const double thetam = moderator.getTiltAngleInRadians();

  const double g1 = (1.0 - angvel * (x0 + x1) * tan(thetam) / veli);
  const double g2 = (1.0 - angvel * (x0 - xa) * tan(thetam) / veli);
  const double f1 = 1.0 + (x1 / x0) * g1;
  const double f2 = 1.0 + (x1 / x0) * g2;
  const double gg1 = g1 / (angvel * (xa + x1));
  const double gg2 = g2 / (angvel * (xa + x1));
  const double ff1 = f1 / (angvel * (xa + x1));
  const double ff2 = f2 / (angvel * (xa + x1));

  const double cp_i = wi / ti;
  const double ct_i = wi / (xa + x1);
  const double cp_f = wf / tf;
  const double ct_f = wf / x2;

  // Define rows of matrix that correspond to each direction
  const unsigned int beam(0), perp(1), up(2);
  TobyFitBMatrix &self = *this;
  double *beamVec = self[beam];
  double *perpVec = self[perp];
  double *upVec = self[up];

  beamVec[TobyFitYVector::ModeratorTime] = cp_i;
  beamVec[TobyFitYVector::ApertureWidthCoord] = -cp_i * gg1;
  beamVec[TobyFitYVector::ApertureHeightCoord] = 0.0;
  beamVec[TobyFitYVector::ChopperTime] = -cp_i;
  beamVec[TobyFitYVector::ScatterPointBeam] = cp_i * gg2 * sMat[1][1];
  beamVec[TobyFitYVector::ScatterPointPerp] = cp_i * gg2 * sMat[1][2];
  beamVec[TobyFitYVector::ScatterPointUp] = cp_i * gg2 * sMat[1][0];
  beamVec[TobyFitYVector::DetectorDepth] = 0.0;
  beamVec[TobyFitYVector::DetectorWidthCoord] = 0.0;
  beamVec[TobyFitYVector::DetectorHeightCoord] = 0.0;
  beamVec[TobyFitYVector::DetectionTime] = 0.0;

  perpVec[TobyFitYVector::ModeratorTime] = 0.0;
  perpVec[TobyFitYVector::ApertureWidthCoord] = -ct_i;
  perpVec[TobyFitYVector::ApertureHeightCoord] = 0.0;
  perpVec[TobyFitYVector::ChopperTime] = 0.0;
  perpVec[TobyFitYVector::ScatterPointBeam] = ct_i * sMat[1][1];
  perpVec[TobyFitYVector::ScatterPointPerp] = ct_i * sMat[1][2];
  perpVec[TobyFitYVector::ScatterPointUp] = ct_i * sMat[1][0];
  perpVec[TobyFitYVector::DetectorDepth] = 0.0;
  perpVec[TobyFitYVector::DetectorWidthCoord] = 0.0;
  perpVec[TobyFitYVector::DetectorHeightCoord] = 0.0;
  perpVec[TobyFitYVector::DetectionTime] = 0.0;

  upVec[TobyFitYVector::ModeratorTime] = 0.0;
  upVec[TobyFitYVector::ApertureWidthCoord] = 0.0;
  upVec[TobyFitYVector::ApertureHeightCoord] = -ct_i;
  upVec[TobyFitYVector::ChopperTime] = 0.0;
  upVec[TobyFitYVector::ScatterPointBeam] = ct_i * sMat[1][0];
  upVec[TobyFitYVector::ScatterPointPerp] = ct_i * sMat[1][2];
  upVec[TobyFitYVector::ScatterPointUp] = -ct_i * sMat[1][1];
  upVec[TobyFitYVector::DetectorDepth] = 0.0;
  upVec[TobyFitYVector::DetectorWidthCoord] = 0.0;
  upVec[TobyFitYVector::DetectorHeightCoord] = 0.0;
  upVec[TobyFitYVector::DetectionTime] = 0.0;

  // ---------- Output components ----------------------------------------
  const unsigned int beamf(3), perpf(4), upf(5);
  double *beamOutVec = self[beamf];
  double *perpOutVec = self[perpf];
  double *upOutVec = self[upf];
  const Kernel::DblMatrix &ds = observation.sampleToDetectorTransform();

  beamOutVec[TobyFitYVector::ModeratorTime] = cp_f * (-x1 / x0);
  beamOutVec[TobyFitYVector::ApertureWidthCoord] = cp_f * ff1;
  beamOutVec[TobyFitYVector::ApertureHeightCoord] = 0.0;
  beamOutVec[TobyFitYVector::ChopperTime] = cp_f * (x0 + x1) / x0;
  beamOutVec[TobyFitYVector::ScatterPointBeam] =
      cp_f * (sMat[2][0] / veli - (ds[0][2]) / velf - ff2 * sMat[1][1]);
  beamOutVec[TobyFitYVector::ScatterPointPerp] =
      cp_f * (sMat[2][2] / veli - (ds[0][1]) / velf - ff2 * sMat[1][2]);
  beamOutVec[TobyFitYVector::ScatterPointUp] =
      -cp_f * (sMat[2][1] / veli - (ds[0][0]) / velf - ff2 * sMat[1][0]);
  beamOutVec[TobyFitYVector::DetectorDepth] = cp_f / velf;
  beamOutVec[TobyFitYVector::DetectorWidthCoord] = 0.0;
  beamOutVec[TobyFitYVector::DetectorHeightCoord] = 0.0;
  beamOutVec[TobyFitYVector::DetectionTime] = -cp_f;

  perpOutVec[TobyFitYVector::ModeratorTime] = 0.0;
  perpOutVec[TobyFitYVector::ApertureWidthCoord] = 0.0;
  perpOutVec[TobyFitYVector::ApertureHeightCoord] = 0.0;
  perpOutVec[TobyFitYVector::ChopperTime] = 0.0;
  perpOutVec[TobyFitYVector::ScatterPointBeam] = -ct_f * (ds[1][2]);
  perpOutVec[TobyFitYVector::ScatterPointPerp] = -ct_f * (ds[1][1]);
  perpOutVec[TobyFitYVector::ScatterPointUp] = -ct_f * (-ds[1][0]);
  perpOutVec[TobyFitYVector::DetectorDepth] = 0.0;
  perpOutVec[TobyFitYVector::DetectorWidthCoord] = 0.0;
  perpOutVec[TobyFitYVector::DetectorHeightCoord] = ct_f;
  perpOutVec[TobyFitYVector::DetectionTime] = 0.0;

  upOutVec[TobyFitYVector::ModeratorTime] = 0.0;
  upOutVec[TobyFitYVector::ApertureWidthCoord] = 0.0;
  upOutVec[TobyFitYVector::ApertureHeightCoord] = 0.0;
  upOutVec[TobyFitYVector::ChopperTime] = 0.0;
  upOutVec[TobyFitYVector::ScatterPointBeam] = -ct_f * (ds[2][2]);
  upOutVec[TobyFitYVector::ScatterPointPerp] = -ct_f * (-ds[2][1]);
  upOutVec[TobyFitYVector::ScatterPointUp] = -ct_f * (-ds[2][0]);
  upOutVec[TobyFitYVector::DetectorDepth] = 0.0;
  upOutVec[TobyFitYVector::DetectorWidthCoord] = ct_f;
  upOutVec[TobyFitYVector::DetectorHeightCoord] = 0.0;
  upOutVec[TobyFitYVector::DetectionTime] = 0.0;
}
}
}
