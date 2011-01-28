#ifndef MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_
#define MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** A helper class for calculating neutron's gravitional drop. Only works for
    SANS because Mantid has no convention on which was is up or down. For this
    to work y must increase with distance from the earth
    @author Steve Williams, ISIS Rutherford Appleton Laboratory 
    @date 26/01/2009
     
    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
     
    This file is part of Mantid.
     
    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
     
    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
     
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
     
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
 */
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
      *  @param wav :: the wave length in Angstrom
      *  @return the drop in meters
      */
      double gravitationalDrop(const double wav) const {return m_dropPerAngstrom2*wav*wav;}
      double calcSinTheta() const;
    };

  }
}
#endif /*#define MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_*/