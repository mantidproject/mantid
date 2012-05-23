#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitBMatrix.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h"
#include "MantidMDAlgorithms/Quantification/Observation.h"

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
    void TobyFitBMatrix::recalculate(const Observation & observation,
                                     const QOmegaPoint & qOmega)
    {
      // Compute transformation matrices
      API::ExperimentInfo_const_sptr exptInfo = observation.experimentInfo();
      const Kernel::DblMatrix labToDet = observation.labToDetectorTransform();
      const Geometry::OrientedLattice & lattice = exptInfo->sample().getOrientedLattice();
      const Kernel::DblMatrix & sMat = lattice.getU();
      const Kernel::DblMatrix dMat = labToDet*sMat;

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
      const API::ChopperModel & chopper0 = exptInfo->chopperModel(0);
      const double angvel = chopper0.getAngularVelocity();

      // Moderator tilt angle
      const API::ModeratorModel & moderator = exptInfo->moderatorModel();
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

      Geometry::Instrument_const_sptr instrument = exptInfo->getInstrument();
      boost::shared_ptr<const Geometry::ReferenceFrame> refFrame = instrument->getReferenceFrame();

      // Define rows of matrix that correspond to each direction
      const unsigned int beam = refFrame->pointingAlongBeam();
      const unsigned int up = refFrame->pointingUp();
      const unsigned int horiz = refFrame->pointingHorizontal();

      TobyFitBMatrix & self = *this;

      self[beam][TobyFitYVector::ModeratorTime] =  cp_i;
      self[beam][TobyFitYVector::ApertureWidthCoord] = -cp_i * gg1;
      self[beam][TobyFitYVector::ApertureHeightCoord] =  0.0;
      self[beam][TobyFitYVector::ChopperTime] = -cp_i;
      self[beam][TobyFitYVector::ScatterPointX] =  cp_i * gg2 * sMat[1][0];
      self[beam][TobyFitYVector::ScatterPointY] =  cp_i * gg2 * sMat[1][1];
      self[beam][TobyFitYVector::ScatterPointZ] =  cp_i * gg2 * sMat[1][2];
      self[beam][TobyFitYVector::DetectorDepth] =  0.0;
      self[beam][TobyFitYVector::DetectorWidthCoord] =  0.0;
      self[beam][TobyFitYVector::DetectorHeightCoord] =  0.0;
      self[beam][TobyFitYVector::DetectionTime] =  0.0;

      self[horiz][TobyFitYVector::ModeratorTime] =  0.0;
      self[horiz][TobyFitYVector::ApertureWidthCoord] = -ct_i;
      self[horiz][TobyFitYVector::ApertureHeightCoord] =  0.0;
      self[horiz][TobyFitYVector::ChopperTime] =  0.0;
      self[horiz][TobyFitYVector::ScatterPointX] =  ct_i * sMat[1][0];
      self[horiz][TobyFitYVector::ScatterPointY] =  ct_i * sMat[1][1];
      self[horiz][TobyFitYVector::ScatterPointZ] =  ct_i * sMat[1][2];
      self[horiz][TobyFitYVector::DetectorDepth] =  0.0;
      self[horiz][TobyFitYVector::DetectorWidthCoord] =  0.0;
      self[horiz][TobyFitYVector::DetectorHeightCoord] =  0.0;
      self[horiz][TobyFitYVector::DetectionTime]=  0.0;

      self[up][TobyFitYVector::ModeratorTime] =  0.0;
      self[up][TobyFitYVector::ApertureWidthCoord] =  0.0;
      self[up][TobyFitYVector::ApertureHeightCoord] = -ct_i;
      self[up][TobyFitYVector::ChopperTime] =  0.0;
      self[up][TobyFitYVector::ScatterPointX] =  ct_i * sMat[2][0];
      self[up][TobyFitYVector::ScatterPointY] =  ct_i * sMat[2][1];
      self[up][TobyFitYVector::ScatterPointZ] =  ct_i * sMat[2][2];
      self[up][TobyFitYVector::DetectorDepth] =  0.0;
      self[up][TobyFitYVector::DetectorWidthCoord] =  0.0;
      self[up][TobyFitYVector::DetectorHeightCoord] =  0.0;
      self[up][TobyFitYVector::DetectionTime] =  0.0;
//
      // Output components
      const unsigned int beamf = beam + 3;
      const unsigned int upf = up + 3;
      const unsigned int horizf = horiz + 3;

      const Kernel::DblMatrix ds = dMat*sMat;

      self[beamf][TobyFitYVector::ModeratorTime] =  cp_f * (-x1/x0);
      self[beamf][TobyFitYVector::ApertureWidthCoord] =  cp_f *  ff1;
      self[beamf][TobyFitYVector::ApertureHeightCoord] =  0.0;
      self[beamf][TobyFitYVector::ChopperTime] =  cp_f * (x0+x1)/x0;
      self[beamf][TobyFitYVector::ScatterPointX] =  cp_f * ( sMat[beam][0]/veli - (ds[2][2])/velf - ff2*sMat[horiz][0] );
      self[beamf][TobyFitYVector::ScatterPointY] =  cp_f * ( sMat[beam][1]/veli - (ds[2][0])/velf - ff2*sMat[horiz][1] );
      self[beamf][TobyFitYVector::ScatterPointZ] =  cp_f * ( sMat[beam][2]/veli - (ds[2][1])/velf - ff2*sMat[horiz][2] );
      self[beamf][TobyFitYVector::DetectorDepth] =  cp_f/velf;
      self[beamf][TobyFitYVector::DetectorWidthCoord] =  0.0;
      self[beamf][TobyFitYVector::DetectorHeightCoord] =  0.0;
      self[beamf][TobyFitYVector::DetectionTime]= -cp_f;

      self[horizf][TobyFitYVector::ModeratorTime] =  0.0;
      self[horizf][TobyFitYVector::ApertureWidthCoord] =  0.0;
      self[horizf][TobyFitYVector::ApertureHeightCoord] =  0.0;
      self[horizf][TobyFitYVector::ChopperTime] =  0.0;
      self[horizf][TobyFitYVector::ScatterPointX] = -ct_f * ( ds[0][2] );
      self[horizf][TobyFitYVector::ScatterPointY] = -ct_f * ( ds[0][0] );
      self[horizf][TobyFitYVector::ScatterPointZ] = -ct_f * ( ds[0][1] );
      self[horizf][TobyFitYVector::DetectorDepth]=  0.0;
      self[horizf][TobyFitYVector::DetectorWidthCoord] =  ct_f;
      self[horizf][TobyFitYVector::DetectorHeightCoord] =  0.0;
      self[horizf][TobyFitYVector::DetectionTime]=  0.0;

      self[upf][TobyFitYVector::ModeratorTime] =  0.0;
      self[upf][TobyFitYVector::ApertureWidthCoord] =  0.0;
      self[upf][TobyFitYVector::ApertureHeightCoord] =  0.0;
      self[upf][TobyFitYVector::ChopperTime] =  0.0;
      self[upf][TobyFitYVector::ScatterPointX] = -ct_f * ( ds[1][2] );
      self[upf][TobyFitYVector::ScatterPointY] = -ct_f * ( ds[1][0] );
      self[upf][TobyFitYVector::ScatterPointZ] = -ct_f * ( ds[1][1] );
      self[upf][TobyFitYVector::DetectorDepth] =  0.0;
      self[upf][TobyFitYVector::DetectorWidthCoord] =  0.0;
      self[upf][TobyFitYVector::DetectorHeightCoord] =  ct_f;
      self[upf][TobyFitYVector::DetectionTime]=  0.0;
    }
  }
}
