//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/DetectorsRing.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Geometry
{

    // Get a reference to the logger
    Kernel::Logger& DetectorsRing::g_log = Kernel::Logger::get("DetectorsRing");

/** Constructor that takes a list of detectors to add
*  @param dets :: The vector of IDetector pointers that this virtual detector will hold
*  @param warnAboutMasked :: If true a log message at warning level will be generated if a one of the detectors in dets is masked. 
*  @throw std::invalid_argument If an empty vector is passed as argument
*/
DetectorsRing::DetectorsRing(const std::vector<IDetector_sptr>& dets, bool warnAboutMasked) :
DetectorGroup(dets,warnAboutMasked),
RingCenter(0,0,0),
RingRadius(0)
{
  
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
      RingCenter+=it->second->getPos();
  }
  RingCenter/=double(m_detectors.size());

  calcRingRadius();

}
//
void
DetectorsRing::calcRingRadius()
{
    RingRadius=0;

    DetCollection::const_iterator it;
    for (it = m_detectors.begin(); it != m_detectors.end(); ++it){
          if(it->second->isValid(RingCenter)){
              g_log.error()<<" can not build a detectors ring as ring center belongs to one of the detectors\n";
              throw(std::invalid_argument("wrong group to build a detectors ring"));
           }
      V3D rVect       = it->second->getPos()-RingCenter;
      RingRadius += rVect.norm2();
    }
    RingRadius = sqrt(RingRadius/double(m_detectors.size()));
}
/// Destructor
DetectorsRing::~DetectorsRing()
{
}

  

/// Gives the average distance of a group of detectors from the given component
double DetectorsRing::getDistance(const IComponent& comp) const
{
      double result = 0.0;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        result += (*it).second->getDistance(comp);
      }
      return result/static_cast<double>(m_detectors.size());
}

    /// Gives the average angle of a group of detectors from the observation point, relative to the axis given
double DetectorsRing::getTwoTheta(const V3D& observer, const V3D& axis) const
{
   double result = 0.0;
    DetCollection::const_iterator it;
    for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
    {
        const V3D sampleDetVec = (*it).second->getPos() - observer;
        result += sampleDetVec.angle(axis);
    }
    return result/static_cast<double>(m_detectors.size());
}

    /// Gives the average phi of the constituent detectors
    double DetectorsRing::getPhi() const
    {
      double result = 0.0;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        V3D detPos = (*it).second->getPos();
        double phi = 0.0, dummy;
        detPos.getSpherical(dummy,dummy,phi);
        result += phi*M_PI/180.0;;
      }
      return result/static_cast<double>(m_detectors.size()); 
    }

   
 

  
}  // namespace Geometry
} // namespace Mantid