//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DetectorGroup.h"

namespace Mantid
{
namespace Geometry
{

// Get a reference to the logger
Kernel::Logger& DetectorGroup::g_log = Kernel::Logger::get("DetectorGroup");

/// Default Constructor
DetectorGroup::DetectorGroup() : IDetector(), m_id(0), m_detectors(), m_isDead(false)
{
}

/** Constructor that takes a list of detectors to add
 *  @param dets The vector of IDetector pointers that this virtual detector will hold
 */
DetectorGroup::DetectorGroup(const std::vector<IDetector*>& dets) : 
  IDetector(),
  m_id(),
  m_detectors(),
  m_isDead(false)
{
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
    g_log.information() << "Detector with ID " << det->getID() << " added to group." << std::endl;
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
  // Perhaps this should check whether all children are dead
  return m_isDead;
}

void DetectorGroup::markDead()
{
  if ( !m_isDead ) g_log.warning() << "Detector Group" << getID() << " is already marked as dead." << std::endl;
  // Marks only overall effective detector as dead - leaves underlying flags unchanged
  m_isDead = true;
}

} // namespace Geometry
} // namespace Mantid
