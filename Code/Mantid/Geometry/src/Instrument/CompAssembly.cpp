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
CompAssembly::CompAssembly() : Component()
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
  Component(n, reference)
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
CompAssembly::CompAssembly(const CompAssembly& ass) :
  Component(ass)
{
  group=ass.group;
  // Need to do a deep copy
  comp_it it;
  for (it = group.begin(); it != group.end(); ++it)
  {
    *it =  (*it)->clone() ;
    // Move copied component object's parent from old to new CompAssembly
    (*it)->setParent(this);
  }
}

/*! Destructor
 */
CompAssembly::~CompAssembly()
{
  // Iterate over pointers in group, deleting them
  //std::vector<IComponent*>::iterator it;
  for (comp_it it = group.begin(); it != group.end(); ++it)
  {
    delete *it;
  }
  group.clear();
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
int CompAssembly::addCopy(IComponent* comp)
{
  if (comp)
  {
    IComponent* newcomp=comp->clone();
    newcomp->setParent(this);
    group.push_back(newcomp);
  }
  return group.size();
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

/*! Get a pointer to the ith component in the assembly. Note standard C/C++
 *  array notation used, that is, i most be an integer i = 0,1,..., N-1, where
 *  N is the number of component in the assembly.
 *
 * @param i The index of the component you want
 * @return group[i] 
 * 
 *  Throws if i is not in range
 */
boost::shared_ptr<IComponent> CompAssembly::operator[](int i) const
{
  if (i<0 || i> static_cast<int>(group.size()-1))
  throw std::runtime_error("CompAssembly::operator[] range not valid");
  return boost::shared_ptr<IComponent>(group[i],NoDeleting());
}



/** Return a pointer to the component in the assembly at the
 * (X,Y) pixel position. The index into the list is calculated by:
 *        index = X*yPixels + Y;
 * meaning that Y has to be the fast-moving index and X the slow-moving.
 *
 * @param X index from 0..xPixels-1
 * @param Y index from 0..yPixels-1
 * @throws runtime_error if the x/y pixel width is not set, or X/Y are out of range
 */
boost::shared_ptr<IComponent> CompAssembly::getChildAtXY(int X, int Y) const
{
  if ((xPixels <= 0) || (yPixels <= 0))
  {
    std::cout << "xPixels " << xPixels << " yPixels " << yPixels << "\n";
    throw std::runtime_error("CompAssembly::getChildAtXY: invalid X or Y width set in the object.");
  }
  if ((X < 0) || (X >= xPixels))
    throw std::runtime_error("CompAssembly::getChildAtXY: X specified is out of range.");
  if ((Y < 0) || (Y >= yPixels))
    throw std::runtime_error("CompAssembly::getChildAtXY: Y specified is out of range.");
  //Find the index and return that.
  int i = X*yPixels + Y;
  return this->operator[](i);
}


/// Set the pixel size of the detector (optional)
void CompAssembly::setNumPixels(int num_xPixels, int num_yPixels)
{
  xPixels = num_xPixels;
  yPixels = num_yPixels;
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
  for (const_comp_it it=group.begin();it!=group.end();it++)
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
  for (const_comp_it it=group.begin();it!=group.end();it++)
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

