#ifndef MANTID_GEOMETRY_DETECTORSRING_H_
#define MANTID_GEOMETRY_DETECTORSRING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/DetectorGroup.h"

namespace Mantid
{
namespace Geometry
{
    /** Holds a collection of detectors, arranged in a ring.

    
    @author Alex Buts ISIS
    @date 19/05/2011
    
    Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
    
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
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport DetectorsRing : public DetectorGroup
    {
    public:
      DetectorsRing(const std::vector<IDetector_sptr>& dets, bool warnAboutMasked = false);
      virtual ~DetectorsRing();

   //   void addDetector(IDetector_sptr det, bool& warn);

      // IDetector methods; overloaded from DetectorGroup; most other group methods remain the same
      V3D getPos() const{return RingCenter;}
      /** The distance is defined by sqrt(RingRadius^2+(RingCenter-comp_Center)^2)
          to be meaningfull, the component center has to belong to the ring rotation axis, 
          no checks for that are performed at the moment	  */
      double getDistance(const IComponent& comp) const;
      double getTwoTheta(const V3D& observer, const V3D& axis) const;
      double getPhi() const;
      //currently from group; should be different? double solidAngle(const V3D& observer) const; 
      /// can not be a monitor?
      bool isMonitor() const{return false;}
      
      std::map<detid_t, double> getNeighbours(double) { throw Mantid::Kernel::Exception::NotImplementedError("getNeighbours not implemented for DetectorsRings"); };
       ///Try to find a point that lies within (or on) the object ?? meaning is unclear on rings but let's define it as on group
      /// int getPointInObject(V3D& point) const { throw Mantid::Kernel::Exception::NotImplementedError("getNeighbours not implemented for DetectorsRings"); };
      /// Get the bounding box for this component and store it in the given argument
      virtual void getBoundingBox(BoundingBox& boundingBox) const{ UNUSED_ARG(boundingBox); // hopefully it is temporary
          throw(Mantid::Kernel::Exception::NotImplementedError("getNeighbours not implemented for DetectorsRings"));};

    
    private:
      /// Private, unimplemented copy constructor
      DetectorsRing(const DetectorsRing&);
      /// Private, unimplemented copy assignment operator
      DetectorsRing& operator=(const DetectorsRing&);

  /// Static reference to the logger class
      static Kernel::Logger& g_log;

      /// the geometrical center of the detectors ring
      V3D RingCenter;
      /// the radius of the ring
      double RingRadius;
      /// helper function to verify if detecotors ring is correct (no detectors intersect the center) and identify its radius
      void calcRingRadius();
    };
} // endnamespace Geometry
} // end namespace MANTID

#endif