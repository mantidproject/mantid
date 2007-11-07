#include "Component.h"
#include "Quat.h"

namespace Mantid
{
namespace Geometry
{

/*! Empty constructor
 *  Create a component with null parent
 */
Component::Component()
{
	name="";
	parent=0;
}

/*! Constructor by value
 *  @param n :: Component name
 * 	@param reference :: parent Component (optional)
 */
Component::Component(const std::string& n, Component* reference):name(n),parent(reference)
{
}

/*! Constructor by value 
 * 	@param n :: Component name
 * 	@param v :: position 
 * 	absolute or relative if the reference if defined
 * 	@param reference :: parent Component
 */ 
Component::Component(const std::string& n, const V3D& v, Component* reference):name(n),pos(v),parent(reference)
{
	if (reference) parent=reference;
}

/*! Constructor 
 *  @param n :: Component name
 *  @param v :: position
 *  @param q :: orientation quaternion
 * 	@param reference :: parent Component (optional)
 */
Component::Component(const std::string& n, const V3D& v, const Quat& q, Component* reference):name(n),pos(v),rot(q),parent(reference)
{
	if (reference) parent=reference;
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
Component* Component::clone() const
{
	 return new Component(*this);
}

/*! Set the parent. Previous parenting is lost.
 *  @param comp :: the parent component
 */
void Component::setParent(Component* comp)
{
	parent=comp;
}

/*! Get a pointer to the parent. 
 *  @return this.parent 
 */
const Component* Component::getParent() const
{
	return parent;
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
void Component::copyRot(const Component& comp)
{
	rot=comp.rot;
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
void Component::rotate(const Quat& r)
{
	rot=r*rot;
}
void Component::rotate(double, const V3D& axis) 
{
	
}
V3D Component::getRelativePos() const 
{
	return pos;
}
V3D Component::getPos() const
{
	
	if (!parent)
	return pos;
	else
	{
		V3D temp(pos);
		rot.rotate(temp);
		temp+=parent->getPos();
		return temp;
	}
}
const Quat& Component::getRelativeRot() const
{
	return rot;
}
double Component::getDistance(const Component& comp) const
{
	return pos.distance(comp.pos);
}
void Component::printSelf(std::ostream& os) const
{
	os << "Name : " << name << std::endl;
	os << "Type: " << this->type() << std::endl;
	if (parent)
		os << "Parent: " << parent->name << std::endl;
	else
	os << "Parent: None" << std::endl; 
	
	os << "Position : " << getPos() << std::endl;
	os << "Orientation :" << rot << std::endl;
}
std::ostream& operator<<(std::ostream& os, const Component& comp)
{
	comp.printSelf(os);
	return os;
}


} //Namespace Geometry 
} //Namespace Mantid
