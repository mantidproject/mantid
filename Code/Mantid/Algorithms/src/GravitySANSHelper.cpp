#include "MantidAlgorithms/GravitySANSHelper.h"
#include <math.h>

namespace Mantid
{
  namespace Algorithms
  {
    using namespace Geometry;
    /** sets up the object with workspace data and calculates cached values ready to calculate gravitional
    *  effects across a spectrum
    *  @param ws the workspace that contains the neutron counts
    *  @param det the detector for which the calculations will be for
    */
    GravitySANSHelper::GravitySANSHelper(API::MatrixWorkspace_const_sptr ws, Geometry::IDetector_const_sptr det) :
          m_works(ws), m_beamLineNorm(-1), m_det(det), m_cachedDrop(0)
     {
       m_samplePos = m_works->getInstrument()->getSample()->getPos();
       const V3D sourcePos = m_works->getInstrument()->getSource()->getPos();
       m_beamLine = m_samplePos-sourcePos;

       m_beamLineNorm = 2.0*(m_samplePos-sourcePos).norm();

       //this is the LineOfSight assuming no drop, the drop is added (and subtracted) later in the code when required
       m_cachedLineOfSight = m_det->getPos()-m_samplePos;
     }

    /** Caclulates the sin of the that the neutron left the sample at, before the effect of gravity
    *  @param wavAngstroms the neutrons' wave length in Angstoms
    *  @return the sin of theta
    */
    double GravitySANSHelper::calcSinTheta(const double wavAngstroms) const
    {
      getDetLoc(wavAngstroms);
      return calcSinTheta();
    }
    /** Calculate the sins and cosins of angles as required to calculate Q is 2 dimensions
    *  @param[in] wavAngstroms wavelength of the neutron in Angstrom
    *  @param[out] xFrac the component in the x direction
    *  @param[out] yFrac component in the y direction
    *  @return sin of the angle theta to the detector
    */
    double GravitySANSHelper::calcComponents(const double wavAngstroms, double & xFrac, double & yFrac) const
    {
      const V3D & detPos = getDetLoc(wavAngstroms);

      const double phi = atan2(detPos.Y(),detPos.X());
      xFrac = cos(phi);
      yFrac = sin(phi);
      return calcSinTheta();
    }
    /** Finds the location of the detector the neutron would have entered if it followed a
    *  straight line path from the sample
    *  @param wav wavelength of the neutron in Angstrom
    *  @return a reference to the cached location
    */
    const V3D & GravitySANSHelper::getDetLoc(const double wav) const
    {
      // Calculate the drop
      const double drop = m_works->gravitationalDrop(m_det, wav);
      // I'm fairly confident that Y is up! Using the previous drop to allow the same V3D to be modified many times
      m_cachedLineOfSight[1] += drop   -     m_cachedDrop;
      m_cachedDrop = drop;
      // must return a member variable, not a local!
      return m_cachedLineOfSight;
    }
    /** getDetLoc must have been called before this is used to calculate the sin of the angle
    *  @return the sin of theta
    */
    double GravitySANSHelper::calcSinTheta() const
    {
      // This is 0.5*cos(2theta)
      double halfcosTheta =
        m_cachedLineOfSight.scalar_prod(m_beamLine)/m_cachedLineOfSight.norm()/m_beamLineNorm;
      // This is sin(theta)
      return sqrt(0.5-halfcosTheta);
    }

  }
}