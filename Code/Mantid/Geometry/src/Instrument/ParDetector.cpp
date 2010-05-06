#include "MantidGeometry/Instrument/ParDetector.h"
#include "MantidGeometry/Instrument/Detector.h"

namespace Mantid
{
namespace Geometry
{

/** Constructor
 *  @param base Pointer to a base instrument detector
 *  @param map Pointer to a parameter map
 */
ParDetector::ParDetector(const Detector* base, const ParameterMap& map) :
  ParObjComponent(base,map)
{
}


/// Copy constructor
ParDetector::ParDetector(const ParDetector& rhs) :
  ParObjComponent(rhs)
{
}

///Destructor
ParDetector::~ParDetector()
{
}

/** Sets the ParDetector id
 *  @param det_id the ParDetector id
 */
void ParDetector::setID(int det_id)
{
//	m_id=det_id;
}

/** Gets the ParDetector id
 *  @returns the ParDetector id
 */
int ParDetector::getID() const
{
	return dynamic_cast<const Detector*>(m_base)->getID();
}

// IParDetector methods. Just pull in Component implementation
V3D ParDetector::getPos() const
{
  return ParObjComponent::getPos();
}

/// Does the point given lie within this object component?
bool ParDetector::isValid(const V3D& point) const
{
  return ParObjComponent::isValid(point);
}

double ParDetector::getDistance(const IComponent& comp) const
{
  return ParObjComponent::getDistance(comp);
}

double ParDetector::getTwoTheta(const V3D& observer, const V3D& axis) const
{
  const V3D sampleDetVec = this->getPos() - observer;
  return sampleDetVec.angle(axis);
}

double ParDetector::getPhi() const
{
  double phi = 0.0, dummy;
  this->getPos().getSpherical(dummy,dummy,phi);
  return phi*M_PI/180.0;
}

double ParDetector::solidAngle(const V3D& observer) const
{
  return ParObjComponent::solidAngle(observer);
}

bool ParDetector::isMasked() const
{
  Parameter_sptr par = m_map.get(m_base,"masked");
  return par ? true : false;
}

bool ParDetector::isMonitor() const
{
  return dynamic_cast<const Detector*>(m_base)->isMonitor();
}

/** Sets the flag for whether this ParDetector object is a monitor
 *  @param flag True to mark the ParDetector a monitor (default), false otherwise
 */
void ParDetector::markAsMonitor(const bool flag)
{
  //m_isMonitor = flag;
}

} //Namespace Geometry
} //Namespace Mantid
