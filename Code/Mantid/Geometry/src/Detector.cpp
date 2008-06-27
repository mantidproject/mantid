#include "MantidGeometry/Detector.h"

namespace Mantid
{
namespace Geometry
{

// Get a reference to the logger
Kernel::Logger& Detector::g_log = Kernel::Logger::get("Detector");

/** Constructor
 *  @param name The name of the component
 *  @param parent The parent component
 */
Detector::Detector(const std::string& name, Component* parent) :
  ObjComponent(name,parent), IDetector(), m_id(0), m_isDead(false)
{
}

/// Copy constructor
Detector::Detector(const Detector& rhs) :
  ObjComponent(rhs), m_id(rhs.m_id), m_isDead(rhs.m_isDead)
{
}

///Destructor
Detector::~Detector()
{
}

/** Sets the detector id
 *  @param det_id the detector id
 */
void Detector::setID(int det_id)
{
	m_id=det_id;
}

/** Gets the detector id
 *  @returns the detector id
 */
int Detector::getID() const
{
	return m_id;
}

// IDetector methods. Just pull in Component implementation
V3D Detector::getPos() const
{
  return ObjComponent::getPos();
}

double Detector::getDistance(const Component& comp) const
{
  return ObjComponent::getDistance(comp);
}

bool Detector::isDead() const
{
  return m_isDead;
}

void Detector::markDead()
{
  if ( !m_isDead ) g_log.warning() << "Detector " << getID() << " is already marked as dead." << std::endl;
  m_isDead = true;
}

} //Namespace Geometry
} //Namespace Mantid
