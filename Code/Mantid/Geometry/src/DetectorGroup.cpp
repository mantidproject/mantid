//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DetectorGroup.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Geometry
{

// Get a reference to the logger
Kernel::Logger& DetectorGroup::g_log = Kernel::Logger::get("DetectorGroup");

/** Constructor that takes a list of detectors to add
 *  @param dets The vector of IDetector pointers that this virtual detector will hold
 *  @throw std::invalid_argument If an empty vector is passed as argument
 */
DetectorGroup::DetectorGroup(const std::vector<IDetector_sptr>& dets) :
  IDetector(),
  m_id(),
  m_detectors()
{
  if ( dets.empty() )
  {
    g_log.error("Illegal attempt to create an empty DetectorGroup");
    throw std::invalid_argument("Empty DetectorGroup objects are not allowed");
  }
  std::vector<IDetector_sptr>::const_iterator it;
  for (it = dets.begin(); it != dets.end(); ++it)
  {
    addDetector(*it);
  }
}

/// Destructor
DetectorGroup::~DetectorGroup()
{
}

/// Add a detector to the collection
void DetectorGroup::addDetector(IDetector_sptr det)
{
  // Warn if adding a masked detector
  if ( det->isMasked() )
  {
    g_log.warning() << "Adding a detector (ID:" << det->getID() << ") that is flagged as masked." << std::endl;
  }

  // For now at least, the ID is the same as the first detector that is added
  if ( m_detectors.empty() ) m_id = det->getID();

  if ( m_detectors.insert( DetCollection::value_type(det->getID(), det) ).second )
  {
    //g_log.debug() << "Detector with ID " << det->getID() << " added to group." << std::endl;
  }
  else
  {
    g_log.warning() << "Detector with ID " << det->getID() << " is already in group." << std::endl;
  }
}

int DetectorGroup::getID() const
{
  return m_id;
}

/** Returns the position of the DetectorGroup.
 *  In the absence of a full surface/solid angle implementation, this is a simple
 *  average of the component detectors (i.e. there's no weighting for size or if one
 *  or more of the detectors is masked). Also, no regard is made to whether a
 *  constituent detector is itself a DetectorGroup - it's just treated as a single,
 *  pointlike object with the same weight as any other detector.
 */
V3D DetectorGroup::getPos() const
{
  V3D newPos;
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
    newPos += (*it).second->getPos();
  }
  return newPos /= m_detectors.size(); // protection against divide by zero in V3D
}

/// Gives the average distance of a group of detectors from the given component
double DetectorGroup::getDistance(const IComponent& comp) const
{
  double result = 0.0;
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
    result += (*it).second->getDistance(comp);
  }
  return result/m_detectors.size();
}

/// Gives the average angle of a group of detectors from the observation point, relative to the axis given
double DetectorGroup::getTwoTheta(const V3D& observer, const V3D& axis) const
{
  double result = 0.0;
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
    const V3D sampleDetVec = (*it).second->getPos() - observer;
    result += sampleDetVec.angle(axis);
  }
  return result/m_detectors.size();
}

/** Gives the total solid angle subtended by a group of detectors by summing the
 *  contributions from the individual detectors.
 *  @param observer The point from which the detector is being viewed
 *  @return The solid angle in steradians
 *  @throw NullPointerException If geometrical form of any detector has not been provided in the instrument definition file
 */
double DetectorGroup::solidAngle(const V3D& observer) const
{
  double result = 0.0;
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
    result += (*it).second->solidAngle(observer);
  }
  return result;
}

bool DetectorGroup::isMasked() const
{
  bool isMasked = true;
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
    if ( !(*it).second->isMasked() ) isMasked = false;
  }
  return isMasked;
}

/** Indicates whether this is a monitor.
 *  Will return false if even one member of the group is not flagged as a monitor
 */
bool DetectorGroup::isMonitor() const
{
  // Would you ever want to group monitors?
  // For now, treat as NOT a monitor if even one isn't
  bool isMonitor = true;
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
    if ( !(*it).second->isMonitor() ) isMonitor = false;
  }
  return isMonitor;

}

} // namespace Geometry
} // namespace Mantid
