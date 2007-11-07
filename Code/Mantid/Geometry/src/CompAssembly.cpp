#include "CompAssembly.h" 
#include <algorithm>
#include <stdexcept> 
#include <ostream>
namespace Mantid 
{
namespace Geometry
{
	
/*! Empty constructor
 */
CompAssembly::CompAssembly():Component()
{
}

/*! Valued constructor
 *  @param n :: name of the assembly
 *  @param reference :: the parent Component
 * 
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */	
CompAssembly::CompAssembly(const std::string& n, Component* reference) : Component(n,reference)
{
	if (reference)
	{
	CompAssembly* test=dynamic_cast<CompAssembly*>(reference);
	if (test)
		test->add(this);
	}
}

/*! Copy constructor
 *  @param ass :: assembly to copy
 */
CompAssembly::CompAssembly(const CompAssembly& ass):Component(ass)
{
	group=ass.group;
}

/*! Destructor
 */
CompAssembly::~CompAssembly()
{
	group.empty();
}

/*! Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
Component* CompAssembly::clone() const
{
	return new CompAssembly(*this);
}
	
/*! Add method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 * This becomes the new parent of comp.
 */
int	CompAssembly::add(Component* comp)
{
	if (comp)
	{
	comp->setParent(this);
	group.push_back(comp);
	} 
	return group.size();
}

/*! AddCopy method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 *  Add a copy of a component in the assembly. 
 *  Comp is cloned if valid, then added in the assembly
 *  This becomes the parent of the cloned component
 */
int CompAssembly::addCopy(Component* comp)
{
	if (comp)
	{
	Component* newcomp=comp->clone();
	newcomp->setParent(this);
	group.push_back(newcomp);
	}
	return group.size();
}

/*! AddCopy method
 * @param comp :: component to add 
 * @param m    :: name of the copied component. 
 * @return number of components in the assembly
 * 
 *  Add a copy of a component in the assembly. 
 *  Comp is cloned if valid, then added in the assembly
 *  This becomes the parent of the cloned component
 */
int CompAssembly::addCopy(Component* comp, const std::string& n)
{
	if (comp)
	{
	Component* newcomp=comp->clone();
	newcomp->setParent(this);
	newcomp->setName(n);
	group.push_back(newcomp);
	}
	return group.size();
}

/*! Return the number of components in the assembly
 * @return group.size() 
  */
int CompAssembly::nelements() const
{
	return group.size();
}

/*! Get a pointer to the ith component in the assembly
 * @return group[i] 
 * 
 *  Throws if i is not in range
 */
Component* CompAssembly::operator[](int i) const
{
	if (i<0 || i> static_cast<int>(group.size()-1)) 
		throw std::runtime_error("CompAssembly::operator[] range not valid");
	return group[i];
}

/*! Print information about elements in the assembly to a stream
 * @param os :: output stream 
 * 
 *  Loops through all components in the assembly 
 *  and call printSelf(os). 
 */
void CompAssembly::printChildren(std::ostream& os) const
{
	std::vector<Component*>::const_iterator it;
	int i=0;
	for (it=group.begin();it!=group.end();it++)
	{
		os << "Component " << i++ <<" : **********" <<std::endl;
		(*it)->printSelf(os);
	}
}

void CompAssembly::printTree(std::ostream& os) const
{
	std::vector<Component*>::const_iterator it;
	int i=0;
	for (it=group.begin();it!=group.end();it++)
	{
		const CompAssembly* test=dynamic_cast<CompAssembly*>(*it);
		os << "Element " << i++ << " in the assembly : ";
		if (test)
		{	
			os << test->getName() << std::endl;
			os << "Children :******** " << std::endl;
			test->printTree(os);
		}
		else
		os << (*it)->getName() << std::endl;
	}
}

/*! Print information about elements in the assembly to a stream
 *  Overload the operator <<
 * @param os  :: output stream 
 * @param ass :: component assembly 
 * 
 *  Loops through all components in the assembly 
 *  and call printSelf(os). 
 *  Also output the number of children
 */
std::ostream& operator<<(std::ostream& os, const CompAssembly& ass)
{
	ass.printSelf(os);
	os << "************************" << std::endl;
	os << "Number of children :" << ass.nelements() << std::endl;
	ass.printChildren(os);
	return os;
}

} // Namespace Geometry
} // Namespace Mantid

