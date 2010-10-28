#include "MantidGeometry/Instrument/ParObjCompAssembly.h" 
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include <algorithm>
#include <stdexcept> 
#include <ostream>

namespace Mantid
{
namespace Geometry
{

ParObjCompAssembly::ParObjCompAssembly(const ObjCompAssembly* base, const ParameterMap& map)
      :ParObjComponent(base,map)
{
}

/**
 * Copy constructor
 */
ParObjCompAssembly::ParObjCompAssembly(const ParObjCompAssembly& A):
ParObjComponent(A)
{
}

/*! Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent* ParObjCompAssembly::clone() const
{
  return new ParObjCompAssembly(*this);
}


/*! Return the number of components in the assembly
 * @return group.size() 
 */
int ParObjCompAssembly::nelements() const
{
  return dynamic_cast<const ObjCompAssembly*>(m_base)->nelements();
}

/*! Add method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 * This becomes the new parent of comp.
 */
int ParObjCompAssembly::add(IComponent* comp)
{
  return nelements();
}

/*! AddCopy method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 *  Add a copy of a component in the assembly. 
 *  Comp is cloned if valid, then added in the assembly
 *  This becomes the parent of the cloned component
 */
int ParObjCompAssembly::addCopy(IComponent* comp)
{
  return nelements();
}

/*! AddCopy method
 * @param comp :: component to add 
 * @param n    :: name of the copied component. 
 * @return number of components in the assembly
 * 
 *  Add a copy of a component in the assembly. 
 *  Comp is cloned if valid, then added in the assembly
 *  This becomes the parent of the cloned component
 */
int ParObjCompAssembly::addCopy(IComponent* comp, const std::string& n)
{
  return nelements();
}

/*! Get a pointer to the ith component in the assembly. Note standard C/C++
 *  array notation used, that is, i most be an integer i = 0,1,..., N-1, where
 *  N is the number of component in the assembly.
 *
 * @param i The index of the component you want
 * @return group[i] 
 * 
 *  Throws if i is not in range
 */
boost::shared_ptr<IComponent> ParObjCompAssembly::operator[](int i) const
{
  if (i<0 || i> nelements()-1)
  {
      throw std::runtime_error("ParObjCompAssembly::operator[] range not valid");
  }
  boost::shared_ptr<IComponent> child_base = dynamic_cast<const ObjCompAssembly*>(m_base)->operator[](i);

  return ParComponentFactory::create(child_base,m_map);
}

/*! Print information about elements in the assembly to a stream
 * @param os :: output stream 
 * 
 *  Loops through all components in the assembly 
 *  and call printSelf(os). 
 */
void ParObjCompAssembly::printChildren(std::ostream& os) const
{
  std::vector<IComponent*>::const_iterator it;
  for (int i=0;i<nelements();i++)
  {
      boost::shared_ptr<IComponent> it = (*this)[i];
      os << "Component " << i <<" : **********" <<std::endl;
      it->printSelf(os);
  }
}

/*! Print information about all the elements in the tree to a stream
 *  Loops through all components in the tree 
 *  and call printSelf(os). 
 *
 * @param os :: output stream 
 */
void ParObjCompAssembly::printTree(std::ostream& os) const
{
  std::vector<IComponent*>::const_iterator it;
  for (int i=0;i<nelements();i++)
  {
    boost::shared_ptr<IComponent> it = (*this)[i];
    const ParObjCompAssembly* test=dynamic_cast<ParObjCompAssembly*>(it.get());
    os << "Element " << i++ << " in the assembly : ";
    if (test)
    {
      os << test->getName() << std::endl;
      os << "Children :******** " << std::endl;
      test->printTree(os);
    }
    else
    os << it->getName() << std::endl;
  }
}

/** Gets the absolute position of the ParametrizedComponentAssembly
 * This attempts to read the cached position value from the parameter map, and creates it if it is not available.
 * @returns A vector of the absolute position
 */
V3D ParObjCompAssembly::getPos() const
{
  V3D pos;
  if (!m_map.getCachedLocation(m_base,pos))
  {
    pos = ParametrizedComponent::getPos();
    m_map.setCachedLocation(m_base,pos);
  }
  return pos;
}

/** Gets the absolute position of the ParametrizedComponentAssembly
 * This attempts to read the cached position value from the parameter map, and creates it if it is not available.
 * @returns A vector of the absolute position
 */
const Quat ParObjCompAssembly::getRotation() const
{
  Quat rot;
  if (!m_map.getCachedRotation(m_base,rot))
  {
    rot = ParametrizedComponent::getRotation();
    m_map.setCachedRotation(m_base,rot);
  }
  return rot;
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
std::ostream& operator<<(std::ostream& os, const ParObjCompAssembly& ass)
{
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid

