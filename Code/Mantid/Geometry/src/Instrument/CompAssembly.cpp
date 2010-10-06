#include "MantidGeometry/Instrument/CompAssembly.h" 
#include <algorithm>
#include <stdexcept> 
#include <ostream>
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

/*! Empty constructor
 */
CompAssembly::CompAssembly() : Component(), m_children(), m_cachedBoundingBox(NULL)
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
CompAssembly::CompAssembly(const std::string& n, Component* reference) :
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

/*! Copy constructor
 *  @param ass :: assembly to copy
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

/*! Destructor
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

/*! Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent* CompAssembly::clone() const
{
  return new CompAssembly(*this);
}

/*! Add method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 * This becomes the new parent of comp.
 */
int CompAssembly::add(IComponent* comp)
{
  if (comp)
  {
    comp->setParent(this);
    m_children.push_back(comp);
  }
  return m_children.size();
}

/*! AddCopy method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 *  Add a copy of a component in the assembly. 
 *  Comp is cloned if valid, then added in the assembly
 *  This becomes the parent of the cloned component
 */
int CompAssembly::addCopy(IComponent* comp)
{
  if (comp)
  {
    IComponent* newcomp=comp->clone();
    newcomp->setParent(this);
    m_children.push_back(newcomp);
  }
  return m_children.size();
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
int CompAssembly::addCopy(IComponent* comp, const std::string& n)
{
  if (comp)
  {
    IComponent* newcomp=comp->clone();
    newcomp->setParent(this);
    newcomp->setName(n);
    m_children.push_back(newcomp);
  }
  return m_children.size();
}

/*! Return the number of components in the assembly
 * @return m_children.size() 
 */
int CompAssembly::nelements() const
{
  return m_children.size();
}

/*! Get a pointer to the ith component in the assembly. Note standard C/C++
 *  array notation used, that is, i most be an integer i = 0,1,..., N-1, where
 *  N is the number of component in the assembly.
 *
 * @param i The index of the component you want
 * @return m_children[i] 
 * 
 *  Throws if i is not in range
 */
boost::shared_ptr<IComponent> CompAssembly::operator[](int i) const
{
  if (i<0 || i> static_cast<int>(m_children.size()-1))
  throw std::runtime_error("CompAssembly::operator[] range not valid");
  return boost::shared_ptr<IComponent>(m_children[i],NoDeleting());
}


/**
 * Get the bounding box for this assembly. It is simply the sum of the bounding boxes of its children
 * @param assemblyBox [Out] The resulting bounding box is stored here.
 */
void CompAssembly::getBoundingBox(BoundingBox & assemblyBox) const
{
  if( !m_cachedBoundingBox )
  {
    m_cachedBoundingBox = new BoundingBox();
    // Loop over the children and define a box large enough for all of them
    for (const_comp_it it = m_children.begin(); it != m_children.end(); ++it)
    {
      BoundingBox compBox;
      (*it)->getBoundingBox(compBox);
      m_cachedBoundingBox->grow(compBox);
    }
  }
  // Use cached box
  assemblyBox = *m_cachedBoundingBox;
}

/*! Print information about elements in the assembly to a stream
 * @param os :: output stream 
 * 
 *  Loops through all components in the assembly 
 *  and call printSelf(os). 
 */
void CompAssembly::printChildren(std::ostream& os) const
{
  //std::vector<IComponent*>::const_iterator it;
  int i=0;
  for (const_comp_it it=m_children.begin();it!=m_children.end();it++)
  {
    os << "Component " << i++ <<" : **********" <<std::endl;
    (*it)->printSelf(os);
  }
}

/*! Print information about all the elements in the tree to a stream
 *  Loops through all components in the tree 
 *  and call printSelf(os). 
 *
 * @param os :: output stream 
 */
void CompAssembly::printTree(std::ostream& os) const
{
  //std::vector<IComponent*>::const_iterator it;
  int i=0;
  for (const_comp_it it=m_children.begin();it!=m_children.end();it++)
  {
    const CompAssembly* test=dynamic_cast<CompAssembly*>(*it);
    os << "Element " << i++ << " in the assembly. ";
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

