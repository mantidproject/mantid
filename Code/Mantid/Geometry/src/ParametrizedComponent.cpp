#include "MantidGeometry/ParametrizedComponent.h"

namespace Mantid
{
namespace Geometry
{

ParametrizedComponent::ParametrizedComponent(const IComponent* base,  const ParameterMap* map)
:m_base(base),m_map(map),m_parent(0)
{
}

ParametrizedComponent::ParametrizedComponent(const ParametrizedComponent& comp)
{
    m_base = comp.m_base;
    m_map = comp.m_map;
    m_parent = 0;
}

ParametrizedComponent::~ParametrizedComponent()
{
    //std::cerr<<"PDeleted\n";
    if (m_parent) delete m_parent;
}

/*! Clone method
 *  Make a copy of the ParametrizedComponent
 *  @return new(*this)
 */
IComponent* ParametrizedComponent::clone() const
{
  return new ParametrizedComponent(*this);
}

/*! Set the parent. Previous parenting is lost.
 *  @param comp :: the parent ParametrizedComponent
 */
void ParametrizedComponent::setParent(IComponent* comp)
{
//  parent=comp;
}

/*! Get a pointer to the parent.
 *  @return this.parent
 */
const IComponent* ParametrizedComponent::getParent() const
{
    const IComponent* parent = m_base->getParent();
    if (parent)
    {
        //std::cerr<<"New parent\n";
        m_parent = new ParametrizedComponent(parent,m_map);
        return m_parent;
    }
    return 0;
}

/*! Set the name of the ParametrizedComponent
 *  @param s :: name string
 */
void ParametrizedComponent::setName(const std::string& s)
{
//  name=s;
}

/*! Get the name of the ParametrizedComponent
 *  @return this.name
 */
std::string ParametrizedComponent::getName() const
{
    return m_base->getName();
}

/*! Set the position of the ParametrizedComponent
 *  The position is with respect to the parent ParametrizedComponent
 *  @param x :: x position
 *  @param y :: y position
 * 	@param z :: z position
 */
void ParametrizedComponent::setPos(double x, double y, double z)
{
//  pos(x,y,z);
}

/*! Set the position of the ParametrizedComponent
 *  The position is with respect to the parent ParametrizedComponent
 *  @param v :: vector position
 */
void ParametrizedComponent::setPos(const V3D& v)
{
//  pos=v;
}

/*! Set the orientation of the ParametrizedComponent
 *  The position is with respect to the parent ParametrizedComponent
 *  @param q :: rotation quaternion
 */
void ParametrizedComponent::setRot(const Quat& q)
{
//  rot=q;
}

/*! Copy the orientationmatrix from another ParametrizedComponent
 *  @param comp :: ParametrizedComponent to copy rotation from
 */
void ParametrizedComponent::copyRot(const IComponent& comp)
{
//  rot=comp.rot;
}

/*! Translate the ParametrizedComponent relative to the parent ParametrizedComponent
 *  @param x :: translation on the x-axis
 * 	@param y :: translation on the y-axis
 *  @param z :: translation on the z-axis
 */
void ParametrizedComponent::translate(double x, double y, double z)

{
//  pos[0]+=x;
//  pos[1]+=y;
//  pos[2]+=z;
  return;
}

/*! Translate the ParametrizedComponent relative to the parent ParametrizedComponent
 *  @param v :: translation vector
 */
void ParametrizedComponent::translate(const V3D& v)
{
//  pos+=v;
  return;
}

/*! Rotate the ParametrizedComponent relative to the parent ParametrizedComponent
 *  @param r :: translation vector
 */
void ParametrizedComponent::rotate(const Quat& r)
{
//  rot=r*rot;
}

/*! Rotate the ParametrizedComponent by an angle in degrees with respect to an axis.
 * @param angle the number of degrees to rotate
 * @param axis The Vector to rotate around
 */
void ParametrizedComponent::rotate(double angle, const V3D& axis)
{
//  throw Kernel::Exception::NotImplementedError("Rotate(double angle, const V3D& axis) has not been implemented");
}

/** Gets the position relative to the parent
 * @returns A vector of the relative position
 */
V3D ParametrizedComponent::getRelativePos() const
{
    Parameter * par = m_map->get(m_base,"pos");
    if (par)
    {
        //std::cerr<<static_cast<ParameterV3D*>(par)->value()<<'\n';
        return static_cast<ParameterV3D*>(par)->value();
    }
    return m_base->getRelativePos();
}

/** Gets the absolute position of the ParametrizedComponent
 * @returns A vector of the absolute position
 */
V3D ParametrizedComponent::getPos() const
{
  const IComponent *parent = getParent();
  if (!parent)
  {
    return getRelativePos();
  }
  else
  {
    V3D temp(getRelativePos());
    // RJT: I think that the parent's rotation should be used here instead of the child's
    // @todo Have a discussion with Laurent to clarify this
//    rot.rotate(temp);
    parent->getRelativeRot().rotate(temp);
    temp+=parent->getPos();
    return temp;
  }
}

/** Gets the rotation relative to the parent
 * @returns A quaternion of the relative rotation
 */
const Quat& ParametrizedComponent::getRelativeRot() const
{
    Parameter * par = m_map->get(m_base,"rot");
    if (par)
    {
        //std::cerr<<static_cast<ParameterQuat*>(par)->value()<<'\n';
        return static_cast<ParameterQuat*>(par)->value();
    }
    return m_base->getRelativeRot();
}

/** Returns the absolute rotation of the ParametrizedComponent
 *  @return A quaternion representing the total rotation
 */
const Quat ParametrizedComponent::getRotation() const
{
  const IComponent* parent = getParent();
  if (!parent)
  {
    return getRelativeRot();
  }
  else
  {
    return getRelativeRot()*parent->getRotation();
  }
}

/** Gets the distance between two ParametrizedComponents
 *  @param comp The ParametrizedComponent to measure against
 *  @returns The distance
 */
double ParametrizedComponent::getDistance(const IComponent& comp) const
{
  return getPos().distance(comp.getPos());
}

/** Prints a text representation of itself
 * @param os The ouput stream to write to
 */
void ParametrizedComponent::printSelf(std::ostream& os) const
{
    const IComponent* parent = getParent();
  os << "Name : " << getName() << std::endl;
  os << "Type: " << this->type() << std::endl;
  if (parent)
    os << "Parent: " << parent->getName() << std::endl;
  else
    os << "Parent: None" << std::endl;

  os << "Position : " << getPos() << std::endl;
  os << "Orientation :" << getRelativeRot() << std::endl;
}

/** Prints a text representation
 * @param os The ouput stream to write to
 * @param comp The ParametrizedComponent to output
 * @returns The ouput stream
 */
std::ostream& operator<<(std::ostream& os, const ParametrizedComponent& comp)
{
  comp.printSelf(os);
  return os;
}

} // Namespace Geometry

} // Namespace Mantid

