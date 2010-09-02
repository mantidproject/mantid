#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Geometry
{

/*! Empty constructor
 *  Create a component with null parent
 */
Component::Component() : name(), pos(), rot(), parent(NULL)
{
}

/*! Constructor by value
 *  @param name :: Component name
 * 	@param parent :: parent Component (optional)
 */
Component::Component(const std::string& name, Component* parent) :
  name(name), pos(), rot(), parent(parent)
{
}

/*! Constructor by value
 * 	@param name :: Component name
 * 	@param position :: position
 * 	absolute or relative if the parent is defined
 * 	@param parent :: parent Component
 */
Component::Component(const std::string& name, const V3D& position, Component* parent) :
  name(name), pos(position), rot(), parent(parent)
{
}

/*! Constructor
 *  @param name :: Component name
 *  @param position :: position (relative to parent, if present)
 *  @param rotation :: orientation quaternion (relative to parent, if present)
 * 	@param parent :: parent Component (optional)
 */
Component::Component(const std::string& name, const V3D& position, const Quat& rotation, Component* parent) :
  name(name),pos(position),rot(rotation),parent(parent)
{
}

Component::~Component(){}

/*! Copy constructor
 * 	@param comp :: Component to copy
 *
 *  All properties of comp are copied including the parent Component
 */
Component::Component(const Component& comp)
{
  name=comp.name;
  parent=comp.parent;
  pos=comp.pos;
  rot=comp.rot;
}

/*! Clone method
 *  Make a copy of the component
 *  @return new(*this)
 */
IComponent* Component::clone() const
{
  return new Component(*this);
}

/*!  Get the component's ID
 *   @return ID
*/
ComponentID Component::getComponentID()const
{
  return ComponentID((const IComponent*)(this));
}

/*! Set the parent. Previous parenting is lost.
 *  @param comp :: the parent component
 */
void Component::setParent(IComponent* comp)
{
  parent=comp;
}

/*! Get a pointer to the parent.
 *  @return this.parent
 */
boost::shared_ptr<const IComponent> Component::getParent() const
{
    return boost::shared_ptr<const IComponent>(parent,NoDeleting());
}

/*! Returns an array of all ancestors of this component,
 *  starting with the direct parent and moving up
 *  @return An array of pointers to ancestor components
 */
std::vector<boost::shared_ptr<const IComponent> > Component::getAncestors() const
{
  std::vector<boost::shared_ptr<const IComponent> > ancs;
  boost::shared_ptr<const IComponent> current = this->getParent();
  while (current)
  {
    ancs.push_back(current);
    current = current->getParent();
  }
  return ancs;
}

/*! Set the name of the component
 *  @param s :: name string
 */
void Component::setName(const std::string& s)
{
  name=s;
}

/*! Get the name of the component
 *  @return this.name
 */
std::string Component::getName() const
{
  return name;
}

/*! Set the position of the component
 *  The position is with respect to the parent component
 *  @param x :: x position
 *  @param y :: y position
 * 	@param z :: z position
 */
void Component::setPos(double x, double y, double z)
{
  pos(x,y,z);
}

/*! Set the position of the component
 *  The position is with respect to the parent component
 *  @param v :: vector position
 */
void Component::setPos(const V3D& v)
{
  pos=v;
}

/*! Set the orientation of the component
 *  The position is with respect to the parent component
 *  @param q :: rotation quaternion
 */
void Component::setRot(const Quat& q)
{
  rot=q;
}

/*! Copy the orientationmatrix from another component
 *  @param comp :: component to copy rotation from
 */
void Component::copyRot(const IComponent& comp)
{
  rot = comp.getRotation();
}

/*! Translate the component relative to the parent component
 *  @param x :: translation on the x-axis
 * 	@param y :: translation on the y-axis
 *  @param z :: translation on the z-axis
 */
void Component::translate(double x, double y, double z)

{
  pos[0]+=x;
  pos[1]+=y;
  pos[2]+=z;
  return;
}

/*! Translate the component relative to the parent component
 *  @param v :: translation vector
 */
void Component::translate(const V3D& v)
{
  pos+=v;
  return;
}

/*! Rotate the component relative to the parent component
 *  @param r :: translation vector
 */
void Component::rotate(const Quat& r)
{
  rot=rot*r;
}

/*! Rotate the component by an angle in degrees with respect to an axis.
 * @param angle the number of degrees to rotate
 * @param axis The Vector to rotate around
 */
void Component::rotate(double angle, const V3D& axis)
{
  throw Kernel::Exception::NotImplementedError("Rotate(double angle, const V3D& axis) has not been implemented");
}

/** Gets the position relative to the parent
 * @returns A vector of the relative position
 */
V3D Component::getRelativePos() const
{
  return pos;
}

/** Gets the absolute position of the component
 * @returns A vector of the absolute position
 */
V3D Component::getPos() const
{
  if (!parent)
  {
    return pos;
  }
  else
  {
    V3D temp(pos);
    // RJT: I think that the parent's rotation should be used here instead of the child's
    // @todo Have a discussion with Laurent to clarify this
//    rot.rotate(temp);

    parent->getRotation().rotate(temp);
    temp+=parent->getPos();
    return temp;
  }
}

/** Gets the rotation relative to the parent
 * @returns A quaternion of the relative rotation
 */
const Quat& Component::getRelativeRot() const
{
  return rot;
}

/** Returns the absolute rotation of the component
 *  @return A quaternion representing the total rotation
 */
const Quat Component::getRotation() const
{
  if (!parent)
  {
    return rot;
  }
  else
  {
    return parent->getRotation()*rot;
  }
}

/** Gets the distance between two components
 *  @param comp The Component to measure against
 *  @returns The distance
 */
double Component::getDistance(const IComponent& comp) const
{
  return getPos().distance(comp.getPos());
}

/**
* Get the names of the parameters for this component.
* @param recursive If true, the parameters for all of the parent components are also included
* @returns A set of strings giving the parameter names for this component
*/
std::set<std::string> Component::getParameterNames(bool recursive) const
{
  return std::set<std::string>();
}

/**
* Returns a boolean indicating if the component has the named parameter
* @param name The name of the parameter
* @param recursive If true the parent components will also be searched (Default: true)
* @returns A boolean indicating if the search was successful or not. Always false as this is not
* parameterized
*/
bool Component::hasParameter(const std::string & name, bool recursive) const
{
  return false;
}

/// Default implementation
std::vector<double> Component::getNumberParameter(const std::string&) const
{
  return std::vector<double>(0);
}

/// Default implementation
std::vector<V3D> Component::getPositionParameter(const std::string&) const
{
  return std::vector<V3D>(0);
}

/// Default implementation  
std::vector<Quat> Component::getRotationParameter(const std::string&) const
{
  return std::vector<Quat>(0);
}

/// Default implementation  
std::vector<std::string> Component::getStringParameter(const std::string&) const
{
  return std::vector<std::string>(0);
}



/** Prints a text representation of itself
 * @param os The ouput stream to write to
 */
void Component::printSelf(std::ostream& os) const
{
  os << "Name : " << name << std::endl;
  os << "Type: " << this->type() << std::endl;
  if (parent)
    os << "Parent: " << parent->getName() << std::endl;
  else
    os << "Parent: None" << std::endl;

  os << "Position : " << getPos() << std::endl;
  os << "Orientation :" << rot << std::endl;
}

/** Prints a text representation
 * @param os The ouput stream to write to
 * @param comp The component to output
 * @returns The ouput stream
 */
std::ostream& operator<<(std::ostream& os, const Component& comp)
{
  comp.printSelf(os);
  return os;
}

} //Namespace Geometry
} //Namespace Mantid
