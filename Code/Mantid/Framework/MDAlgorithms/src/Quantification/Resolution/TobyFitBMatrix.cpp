#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitBMatrix.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h"
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/ModeratorModel.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /**
     * Construct a B matrix. The size is (3*ki+3*kf,TobyFitYVector::variableCount())
     * The number of rows is 3 d.o.f. for ki plus 3 for kf. The number of columns
     * must match the number of monte carlo sample variables so that the
     * two can be multiplied to find the integration variables
     */
    TobyFitBMatrix::TobyFitBMatrix()
      : Kernel::DblMatrix(6, TobyFitYVector::variableCount())
    {
    }

    /**
     * Calculate the values for this observation & QDeltaE point
     * @param observation :: The current observation
     * @param qOmega :: The current point in Q-DeltaE space
     */
    void TobyFitBMatrix::recalculate(const CachedExperimentInfo & observation,
                                     const QOmegaPoint & qOmega)
    {
      // Compute transformation matrices
      const API::ExperimentInfo & exptInfo = observation.experimentInfo();
      const Geometry::OrientedLattice & lattice = exptInfo.sample().getOrientedLattice();
      const Kernel::DblMatrix & sMat = lattice.getU();

      // Wavevectors (1/Angstrom)
      const double efixed = observation.getEFixed();
      const double wi = std::sqrt(efixed/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double wf = std::sqrt((efixed - qOmega.deltaE)/PhysicalConstants::E_mev_toNeutronWavenumberSq);

      // Velocities (m/s)
      static const double perAngstromToMetresPerSec = 1e10*PhysicalConstants::h_bar/PhysicalConstants::NeutronMass;
      const double veli = wi * perAngstromToMetresPerSec;
      const double velf = wf * perAngstromToMetresPerSec;

      // Distances
      const double x0 = observation.moderatorToFirstChopperDistance();
      const double xa = observation.firstApertureToFirstChopperDistance();
      const double x1 = observation.firstChopperToSampleDistance();
      const double x2 = observation.sampleToDetectorDistance();

      // Travel times
      const double ti = x0/veli;
      const double tf = x2/velf;

      // Chopper frequency
      const API::ChopperModel & chopper0 = exptInfo.chopperModel(0);
      const double angvel = chopper0.getAngularVelocity();

      // Moderator tilt angle
      const API::ModeratorModel & moderator = exptInfo.moderatorModel();
      const double thetam = moderator.getTiltAngleInRadians();

      const double g1 = (1.0 - angvel*(x0 + x1)*tan(thetam)/veli );
      const double g2 = (1.0 - angvel*(x0 - xa)*tan(thetam)/veli );
      const double f1 =  1.0 + (x1/x0)*g1;
      const double f2 =  1.0 + (x1/x0)*g2;
      const double gg1 = g1 / ( angvel*(xa+x1) );
      const double gg2 = g2 / ( angvel*(xa+x1) );
      const double ff1 = f1 / ( angvel*(xa+x1) );
      const double ff2 = f2 / ( angvel*(xa+x1) );

      const double cp_i = wi/ti;
      const double ct_i = wi/(xa+x1);
      const double cp_f = wf/tf;
      const double ct_f = wf/x2;

      // Define rows of matrix that correspond to each direction
      const unsigned int beam = 2;
      const unsigned int up = 1;
      const unsigned int horiz = 0;

      TobyFitBMatrix & self = *this;
      double *beamVec = self[beam];
      double *horizVec = self[horiz];
      double *upVec = self[up];

      beamVec[TobyFitYVector::ModeratorTime] =  cp_i;
      beamVec[TobyFitYVector::ApertureWidthCoord] = -cp_i * gg1;
      beamVec[TobyFitYVector::ApertureHeightCoord] =  0.0;
      beamVec[TobyFitYVector::ChopperTime] = -cp_i;
      beamVec[TobyFitYVector::ScatterPointX] =  cp_i * gg2 * sMat[1][0];
      beamVec[TobyFitYVector::ScatterPointY] =  cp_i * gg2 * sMat[1][1];
      beamVec[TobyFitYVector::ScatterPointZ] =  cp_i * gg2 * sMat[1][2];
      beamVec[TobyFitYVector::DetectorDepth] =  0.0;
      beamVec[TobyFitYVector::DetectorWidthCoord] =  0.0;
      beamVec[TobyFitYVector::DetectorHeightCoord] =  0.0;
      beamVec[TobyFitYVector::DetectionTime] =  0.0;

      horizVec[TobyFitYVector::ModeratorTime] =  0.0;
      horizVec[TobyFitYVector::ApertureWidthCoord] = -ct_i;
      horizVec[TobyFitYVector::ApertureHeightCoord] =  0.0;
      horizVec[TobyFitYVector::ChopperTime] =  0.0;
      horizVec[TobyFitYVector::ScatterPointX] =  ct_i * sMat[1][0];
      horizVec[TobyFitYVector::ScatterPointY] =  ct_i * sMat[1][1];
      horizVec[TobyFitYVector::ScatterPointZ] =  ct_i * sMat[1][2];
      horizVec[TobyFitYVector::DetectorDepth] =  0.0;
      horizVec[TobyFitYVector::DetectorWidthCoord] =  0.0;
      horizVec[TobyFitYVector::DetectorHeightCoord] =  0.0;
      horizVec[TobyFitYVector::DetectionTime]=  0.0;

      upVec[TobyFitYVector::ModeratorTime] =  0.0;
      upVec[TobyFitYVector::ApertureWidthCoord] =  0.0;
      upVec[TobyFitYVector::ApertureHeightCoord] = -ct_i;
      upVec[TobyFitYVector::ChopperTime] =  0.0;
      upVec[TobyFitYVector::ScatterPointX] =  ct_i * sMat[2][0];
      upVec[TobyFitYVector::ScatterPointY] =  ct_i * sMat[2][1];
      upVec[TobyFitYVector::ScatterPointZ] =  ct_i * sMat[2][2];
      upVec[TobyFitYVector::DetectorDepth] =  0.0;
      upVec[TobyFitYVector::DetectorWidthCoord] =  0.0;
      upVec[TobyFitYVector::DetectorHeightCoord] =  0.0;
      upVec[TobyFitYVector::DetectionTime] =  0.0;

      // Output components
      const unsigned int beamf = beam + 3;
      const unsigned int upf = up + 3;
      const unsigned int horizf = horiz + 3;

      double *beamOutVec = self[beamf];
      double *horizOutVec = self[horizf];
      double *upOutVec = self[upf];
      const Kernel::DblMatrix & ds = observation.sampleToDetectorTransform();

      beamOutVec[TobyFitYVector::ModeratorTime] =  cp_f * (-x1/x0);
      beamOutVec[TobyFitYVector::ApertureWidthCoord] =  cp_f *  ff1;
      beamOutVec[TobyFitYVector::ApertureHeightCoord] =  0.0;
      beamOutVec[TobyFitYVector::ChopperTime] =  cp_f * (x0+x1)/x0;
      beamOutVec[TobyFitYVector::ScatterPointX] =  cp_f * ( sMat[beam][0]/veli - (ds[2][2])/velf - ff2*sMat[horiz][0] );
      beamOutVec[TobyFitYVector::ScatterPointY] =  cp_f * ( sMat[beam][1]/veli - (ds[2][0])/velf - ff2*sMat[horiz][1] );
      beamOutVec[TobyFitYVector::ScatterPointZ] =  cp_f * ( sMat[beam][2]/veli - (ds[2][1])/velf - ff2*sMat[horiz][2] );
      beamOutVec[TobyFitYVector::DetectorDepth] =  cp_f/velf;
      beamOutVec[TobyFitYVector::DetectorWidthCoord] =  0.0;
      beamOutVec[TobyFitYVector::DetectorHeightCoord] =  0.0;
      beamOutVec[TobyFitYVector::DetectionTime]= -cp_f;

      horizOutVec[TobyFitYVector::ModeratorTime] =  0.0;
      horizOutVec[TobyFitYVector::ApertureWidthCoord] =  0.0;
      horizOutVec[TobyFitYVector::ApertureHeightCoord] =  0.0;
      horizOutVec[TobyFitYVector::ChopperTime] =  0.0;
      horizOutVec[TobyFitYVector::ScatterPointX] = -ct_f * ( ds[0][2] );
      horizOutVec[TobyFitYVector::ScatterPointY] = -ct_f * ( ds[0][0] );
      horizOutVec[TobyFitYVector::ScatterPointZ] = -ct_f * ( ds[0][1] );
      horizOutVec[TobyFitYVector::DetectorDepth]=  0.0;
      horizOutVec[TobyFitYVector::DetectorWidthCoord] =  ct_f;
      horizOutVec[TobyFitYVector::DetectorHeightCoord] =  0.0;
      horizOutVec[TobyFitYVector::DetectionTime]=  0.0;

      upOutVec[TobyFitYVector::ModeratorTime] =  0.0;
      upOutVec[TobyFitYVector::ApertureWidthCoord] =  0.0;
      upOutVec[TobyFitYVector::ApertureHeightCoord] =  0.0;
      upOutVec[TobyFitYVector::ChopperTime] =  0.0;
      upOutVec[TobyFitYVector::ScatterPointX] = -ct_f * ( ds[1][2] );
      upOutVec[TobyFitYVector::ScatterPointY] = -ct_f * ( ds[1][0] );
      upOutVec[TobyFitYVector::ScatterPointZ] = -ct_f * ( ds[1][1] );
      upOutVec[TobyFitYVector::DetectorDepth] =  0.0;
      upOutVec[TobyFitYVector::DetectorWidthCoord] =  0.0;
      upOutVec[TobyFitYVector::DetectorHeightCoord] =  ct_f;
      upOutVec[TobyFitYVector::DetectionTime]=  0.0;
    }
  }
}
