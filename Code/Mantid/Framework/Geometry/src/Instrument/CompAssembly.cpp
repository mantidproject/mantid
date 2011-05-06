#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include <algorithm>
#include <ostream>
#include <stdexcept> 

namespace Mantid
{
namespace Geometry
{

/// Void deleter for shared pointers
class NoDeleting
{
public:
    /// deleting operator. Does nothing
    void operator()(void*p)
    {
      (void) p; //Avoid compiler warnings
    }
};



/** Empty constructor
 */
CompAssembly::CompAssembly() : Component(), m_children(), m_cachedBoundingBox(NULL)
{
}


/** Constructor for a parametrized CompAssembly
 * @param base: the base (un-parametrized) IComponent
 * @param map: pointer to the ParameterMap
 * */
CompAssembly::CompAssembly(const IComponent* base, const ParameterMap * map)
: Component(base,map), m_children(), m_cachedBoundingBox(NULL)
{

}


/** Valued constructor
 *  @param n :: name of the assembly
 *  @param reference :: the parent Component
 * 
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */
CompAssembly::CompAssembly(const std::string& n, IComponent* reference) :
  Component(n, reference), m_children(), m_cachedBoundingBox(NULL)
{
  if (reference)
  {
    CompAssembly* test=dynamic_cast<CompAssembly*>(reference);
    if (test)
    {
      test->add(this);
    }
  }
}

/** Copy constructor
 *  @param assem :: assembly to copy
 */
CompAssembly::CompAssembly(const CompAssembly& assem) :
  Component(assem), m_children(assem.m_children), m_cachedBoundingBox(assem.m_cachedBoundingBox)
{
  // Need to do a deep copy
  comp_it it;
  for (it = m_children.begin(); it != m_children.end(); ++it)
  {
    *it = (*it)->clone() ;
    // Move copied component object's parent from old to new CompAssembly
    (*it)->setParent(this);
  }
}

/** Destructor
 */
CompAssembly::~CompAssembly()
{
  if( m_cachedBoundingBox ) delete m_cachedBoundingBox;
  // Iterate over pointers in m_children, deleting them
  for (comp_it it = m_children.begin(); it != m_children.end(); ++it)
  {
    delete *it;
  }
  m_children.clear();
}

/** Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent* CompAssembly::clone() const
{
  return new CompAssembly(*this);
}

/** Add method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 * This becomes the new parent of comp.
 */
int CompAssembly::add(IComponent* comp)
{
  if (m_isParametrized)
    throw std::runtime_error("CompAssembly::add() called for a parametrized CompAssembly.");
    //return static_cast<int>(m_children.size());

  if (comp)
  {
    comp->setParent(this);
    m_children.push_back(comp);
  }
  return static_cast<int>(m_children.size());
}

/** AddCopy method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 *  Add a copy of a component in the assembly. 
 *  Comp is cloned if valid, then added in the assembly
 *  This becomes the parent of the cloned component
 */
int CompAssembly::addCopy(IComponent* comp)
{
  if (m_isParametrized)
    throw std::runtime_error("CompAssembly::addCopy() called for a parametrized CompAssembly.");

  if (comp)
  {
    IComponent* newcomp=comp->clone();
    newcomp->setParent(this);
    m_children.push_back(newcomp);
  }
  return static_cast<int>(m_children.size());
}

/** AddCopy method
 * @param comp :: component to add 
 * @param n :: name of the copied component. 
 * @return number of components in the assembly
 * 
 *  Add a copy of a component in the assembly. 
 *  Comp is cloned if valid, then added in the assembly
 *  This becomes the parent of the cloned component
 */
int CompAssembly::addCopy(IComponent* comp, const std::string& n)
{
  if (m_isParametrized)
    throw std::runtime_error("CompAssembly::addCopy() called for a parametrized CompAssembly.");

  if (comp)
  {
    IComponent* newcomp=comp->clone();
    newcomp->setParent(this);
    newcomp->setName(n);
    m_children.push_back(newcomp);
  }
  return static_cast<int>(m_children.size());
}

/** Return the number of components in the assembly
 * @return m_children.size() 
 */
int CompAssembly::nelements() const
{
  if (m_isParametrized)
    return dynamic_cast<const CompAssembly*>(m_base)->nelements();
  else
    return static_cast<int>(m_children.size());
}

/** Get a pointer to the ith component in the assembly. Note standard C/C++
 *  array notation used, that is, i most be an integer i = 0,1,..., N-1, where
 *  N is the number of component in the assembly. Easier to use than the [] operator 
 *  when you have a pointer
 *
 * @param i :: The index of the component you want
 * @return m_children[i] 
 * @throw std::runtime_error if i is not in range
 */
boost::shared_ptr<IComponent> CompAssembly::getChild(const int i) const
{
  if (m_isParametrized)
  {
    //Get the child of the base (unparametrized) assembly
    boost::shared_ptr<IComponent> child_base = dynamic_cast<const CompAssembly*>(m_base)->getChild(i);
    //And build up a parametrized version of it using the factory, and return that
    return ParComponentFactory::create(child_base,m_map);
  }
  else
  {
    if( i < 0 || i > static_cast<int>(m_children.size()-1) )
    {
      throw std::runtime_error("CompAssembly::getChild() range not valid");
    }
    return boost::shared_ptr<IComponent>(m_children[i],NoDeleting());
  }
}

/** Overloaded index access operator. \link getChild() \endlink
 * @param i :: Element i within the component assembly
 * @return A shared pointer to the ith component
 * @throw std:runtime_error if i is out of range 
 */
boost::shared_ptr<IComponent> CompAssembly::operator[](int i) const
{
  return this->getChild(i);
}


//------------------------------------------------------------------------------------------------
/** Return a vector of all contained children components
 *
 * @param outVector :: vector of IComponent sptr.
 * @param recursive :: if a child is a CompAssembly, returns its children recursively
 */
void CompAssembly::getChildren(std::vector<boost::shared_ptr<IComponent> > & outVector, bool recursive) const
{
  for (int i=0; i < this->nelements(); i++)
  {
    boost::shared_ptr<IComponent> comp = this->getChild(i);
    if (comp)
    {
      outVector.push_back( comp );
      // Look deeper, on option.
      if (recursive)
      {
        boost::shared_ptr<ICompAssembly> assemb = boost::dynamic_pointer_cast<ICompAssembly>(comp);
        if (assemb)
          assemb->getChildren(outVector, recursive);
      }
    }
  }
}



//------------------------------------------------------------------------------------------------
/**
 * Get the bounding box for this assembly. It is simply the sum of the bounding boxes of its children
 * @param assemblyBox :: [Out] The resulting bounding box is stored here.
 */
void CompAssembly::getBoundingBox(BoundingBox & assemblyBox) const
{
  if (m_isParametrized)
  {
    // Check cache for assembly, inside the ParameterMap
    if( m_map->getCachedBoundingBox(this, assemblyBox ) )
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
    m_map->setCachedBoundingBox(this, assemblyBox);
  }

  else
  {
    //Not parametrized
    if( !m_cachedBoundingBox )
    {
      m_cachedBoundingBox = new BoundingBox();
      // Loop over the children and define a box large enough for all of them
      for (const_comp_it it = m_children.begin(); it != m_children.end(); ++it)
      {
        BoundingBox compBox;
        if (*it)
        {
          (*it)->getBoundingBox(compBox);
          m_cachedBoundingBox->grow(compBox);
        }
      }
    }
    // Use cached box
    assemblyBox = *m_cachedBoundingBox;

  }
}



//------------------------------------------------------------------------------------------------
/** Test the intersection of the ray with the children of the component assembly, for InstrumentRayTracer.
 *
 * @param testRay :: Track under test. The results are stored here.
 * @param searchQueue :: If a child is a sub-assembly then it is appended for later searching
 */
void CompAssembly::testIntersectionWithChildren(Track & testRay, std::deque<IComponent_sptr> & searchQueue) const
{
  int nchildren = this->nelements();
  for( int i = 0; i < nchildren; ++i )
  {
    boost::shared_ptr<Geometry::IComponent> comp = this->getChild(i);
    if( ICompAssembly_sptr childAssembly = boost::dynamic_pointer_cast<ICompAssembly>(comp) )
    {
      searchQueue.push_back(comp);
    }
    // Check the physical object intersection
    else if( IObjComponent *physicalObject = dynamic_cast<IObjComponent*>(comp.get()) )
    {
       physicalObject->interceptSurface(testRay);
    }
    else {}
  }
}


//------------------------------------------------------------------------------------------------
/** Print information about elements in the assembly to a stream
 * @param os :: output stream 
 * 
 *  Loops through all components in the assembly 
 *  and call printSelf(os). 
 */
void CompAssembly::printChildren(std::ostream& os) const
{
  std::vector<IComponent*>::const_iterator it;
  for (int i=0;i<nelements();i++)
  {
      boost::shared_ptr<IComponent> it = (*this)[i];
      os << "Component " << i <<" : **********" <<std::endl;
      it->printSelf(os);
  }
}

/** Print information about all the elements in the tree to a stream
 *  Loops through all components in the tree 
 *  and call printSelf(os). 
 *
 * @param os :: output stream 
 */
void CompAssembly::printTree(std::ostream& os) const
{
  std::vector<IComponent*>::const_iterator it;
  for (int i=0;i<nelements();i++)
  {
    boost::shared_ptr<IComponent> it = (*this)[i];
    const CompAssembly* test=dynamic_cast<CompAssembly*>(it.get());
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


/** Gets the absolute position of the Parametrized CompAssembly
 * This attempts to read the cached position value from the parameter map, and creates it if it is not available.
 * @returns A vector of the absolute position
 */
V3D CompAssembly::getPos() const
{
  if (!m_isParametrized)
    return Component::getPos();
  else
  {
    V3D pos;
    if (!m_map->getCachedLocation(m_base,pos))
    {
      pos = Component::getPos();
      m_map->setCachedLocation(m_base,pos);
    }
    return pos;
  }
}

/** Gets the absolute position of the Parametrized CompAssembly
 * This attempts to read the cached position value from the parameter map, and creates it if it is not available.
 * @returns A vector of the absolute position
 */
const Quat CompAssembly::getRotation() const
{
  if (!m_isParametrized)
    return Component::getRotation();
  else
  {
    Quat rot;
    if (!m_map->getCachedRotation(m_base,rot))
    {
      rot = Component::getRotation();
      m_map->setCachedRotation(m_base,rot);
    }
    return rot;
  }
}


/** Print information about elements in the assembly to a stream
 *  Overload the operator <<
 * @param os :: output stream 
 * @param ass :: component assembly 
 * @return stream representation of component assembly
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

