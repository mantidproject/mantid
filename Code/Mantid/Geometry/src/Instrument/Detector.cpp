#include "MantidGeometry/Instrument/Detector.h"

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
Detector::Detector(const std::string& name, IComponent* parent) :
  IDetector(), ObjComponent(name,parent), m_id(0), m_isMonitor(false)
{
}

/** Constructor
 *  @param name The name of the component
 *  @param shape  A pointer to the object describing the shape of this component
 *  @param parent The parent component
 */
Detector::Detector(const std::string& name, boost::shared_ptr<Object> shape, IComponent* parent) :
  IDetector(), ObjComponent(name,shape,parent), m_id(0), m_isMonitor(false)
{
}

/// Copy constructor
Detector::Detector(const Detector& rhs) :
  ObjComponent(rhs), m_id(rhs.m_id), m_isMonitor(rhs.m_isMonitor)
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

///Get the distance between the detector and another component
///@param comp The other component
///@return The distance
double Detector::getDistance(const IComponent& comp) const
{
  return ObjComponent::getDistance(comp);
}

///Get the twotheta angle between the detector and an observer
///@param observer The observer position
///@param axis The axis
///@return The angle
double Detector::getTwoTheta(const V3D& observer, const V3D& axis) const
{
  const V3D sampleDetVec = this->getPos() - observer;
  return sampleDetVec.angle(axis);
}

///Get the phi angle between the detector with reference to the origin
///@return The angle
double Detector::getPhi() const
{
  double phi = 0.0, dummy;
  this->getPos().getSpherical(dummy,dummy,phi);
  return phi*M_PI/180.0;
}

///Get the solid angle between the detector and an observer
///@param observer The observer position
///@return The solid angle
double Detector::solidAngle(const V3D& observer) const
{
	return ObjComponent::solidAngle(observer);
}

/** This method will only be called if this detector is not part of a parameterized instrument,
 *  in which case there are no masked detectors.
 *  @return false
 */
bool Detector::isMasked() const
{
  // If you get to here, instead of the ParDetector method, then it isn't masked
  return false;
}

/// Is the detector a monitor?
///@return true if it is a monitor
bool Detector::isMonitor() const
{
  return m_isMonitor;
}

/** Sets the flag for whether this detector object is a monitor
 *  @param flag True to mark the detector a monitor (default), false otherwise
 */
void Detector::markAsMonitor(const bool flag)
{
  m_isMonitor = flag;
  return;
}

} //Namespace Geometry
} //Namespace Mantid
