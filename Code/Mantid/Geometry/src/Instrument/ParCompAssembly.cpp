#include "MantidGeometry/Instrument/ParCompAssembly.h" 
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include <algorithm>
#include <stdexcept> 
#include <ostream>

namespace Mantid
{
namespace Geometry
{

ParCompAssembly::ParCompAssembly(const CompAssembly* base, const ParameterMap& map)
      :ParametrizedComponent(base,map){}

/*! Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent* ParCompAssembly::clone() const
{
  return new ParCompAssembly(*this);
}


/*! Return the number of components in the assembly
 * @return group.size() 
 */
int ParCompAssembly::nelements() const
{
  return dynamic_cast<const CompAssembly*>(m_base)->nelements();
}

/*! Add method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 * This becomes the new parent of comp.
 */
int ParCompAssembly::add(IComponent* comp)
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
int ParCompAssembly::addCopy(IComponent* comp)
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
int ParCompAssembly::addCopy(IComponent* comp, const std::string& n)
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
boost::shared_ptr<IComponent> ParCompAssembly::operator[](int i) const
{
  if (i<0 || i> nelements()-1)
  {
      throw std::runtime_error("ParCompAssembly::operator[] range not valid");
  }
  boost::shared_ptr<IComponent> child_base = dynamic_cast<const CompAssembly*>(m_base)->operator[](i);

  return ParComponentFactory::create(child_base,m_map);
}

/**
* Get the bounding box for this assembly and store it in the given argument. Note that the bounding box
* is cached in the ParameterMap after the first call
* @param assemblyBox [Out] The resulting bounding box is stored here.
*/
void ParCompAssembly::getBoundingBox(BoundingBox& assemblyBox) const
{
  // Check cache for assembly
  if( m_map.getCachedBoundingBox(this, assemblyBox ) )
  {
    return;
  }
  // Loop over the children and define a box large enough for all of them
  assemblyBox = BoundingBox();
  int nchildren = nelements();
  for(int i = 0; i < nchildren; ++i)
  {
    IComponent_sptr comp = this->operator[](i);
    BoundingBox compBox;
    comp->getBoundingBox(compBox);
    assemblyBox.grow(compBox);
  }
  //Set the cache
  m_map.setCachedBoundingBox(this, assemblyBox);
}

/*! Print information about elements in the assembly to a stream
 * @param os :: output stream 
 * 
 *  Loops through all components in the assembly 
 *  and call printSelf(os). 
 */
void ParCompAssembly::printChildren(std::ostream& os) const
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
void ParCompAssembly::printTree(std::ostream& os) const
{
  std::vector<IComponent*>::const_iterator it;
  for (int i=0;i<nelements();i++)
  {
    boost::shared_ptr<IComponent> it = (*this)[i];
    const ParCompAssembly* test=dynamic_cast<ParCompAssembly*>(it.get());
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
V3D ParCompAssembly::getPos() const
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
const Quat ParCompAssembly::getRotation() const
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
std::ostream& operator<<(std::ostream& os, const ParCompAssembly& ass)
{
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid

