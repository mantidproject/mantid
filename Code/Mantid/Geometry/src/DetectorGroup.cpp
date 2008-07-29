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
DetectorGroup::DetectorGroup(const std::vector<IDetector*>& dets) :
  IDetector(),
  m_id(),
  m_detectors()
{
  if ( dets.empty() )
  {
    g_log.error("Illegal attempt to create and empty DetectorGroup");
    throw std::invalid_argument("Empty DetectorGroup objects are not allowed");
  }
  std::vector<IDetector*>::const_iterator it;
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
void DetectorGroup::addDetector(IDetector* det)
{
  // Warn if adding a dead detector
  if ( det->isDead() )
  {
    g_log.warning() << "Adding a detector (ID:" << det->getID() << ") that is flagged as dead." << std::endl;
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
 *  or more of the detectors is dead). Also, no regard is made to whether a
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

double DetectorGroup::getDistance(const Component& comp) const
{
  return getPos().distance(comp.getPos());
}

bool DetectorGroup::isDead() const
{
  bool isDead = true;
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
    if ( !(*it).second->isDead() ) isDead = false;
  }
  return isDead;
}

void DetectorGroup::markDead()
{
  DetCollection::const_iterator it;
  for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
  {
    (*it).second->markDead();
  }
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
