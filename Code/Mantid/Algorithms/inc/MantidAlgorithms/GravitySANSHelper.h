#ifndef MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_
#define MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_
#include "MantidAPI/MatrixWorkspace.h"
//#include "MantidGeometry/Instrument/IDetector.h"
namespace Mantid
{
  namespace Algorithms
  {
    /// A helper class for calculating neutron's gravitional drop. Only works for
    /// SANS because Mantid has no convention on which was is up or down. For this
    /// to work y must increase with distance from the earth
    class GravitySANSHelper
    {
    public:
      GravitySANSHelper() : m_beamLineNorm(-1), m_dropPerAngstrom2(-1), m_cachedDrop(-1) {}
      GravitySANSHelper(API::MatrixWorkspace_const_sptr ws, Geometry::IDetector_const_sptr det);
      double calcSinTheta(const double wavAngstroms) const;
      double calcComponents(const double wavAngstroms, double & xFrac, double & yFrac) const;

    private:
      /// coordinates of the sample
      Geometry::V3D m_samplePos;
      /// the displacement from the source to the sample
      Geometry::V3D m_beamLine;
      /// twice the distance from the source to the sample
      double m_beamLineNorm;

      /// the detector whose neutrons we are doing the calcualations for
      Geometry::IDetector_const_sptr m_det;
      /// the drop is proportional to the wavelength squared, storing this drop increases calculation speed a lot
      double m_dropPerAngstrom2;
      /// the location that the neutron would have been detected at if it continued in a straight line, without gravity
      mutable Geometry::V3D m_cachedLineOfSight;
      /// the drop that was last caclulated, this allows m_cachedDetLoc to be recalculated each time without its original value being stored
      mutable double m_cachedDrop;

      const Geometry::V3D & getDetLoc(const double wav) const;
      /** Calculates the drop very much faster than running the workspace's gravitationalDrop function
      *  @param wav the wave length in Angstrom
      *  @return the drop in meters
      */
      double gravitationalDrop(const double wav) const {return m_dropPerAngstrom2*wav*wav;}
      double calcSinTheta() const;
    };

  }
}
#endif /*#define MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_*/