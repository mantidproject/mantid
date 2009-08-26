#include "MantidGeometry/ParCompAssembly.h" 
#include "MantidGeometry/CompAssembly.h" 
#include "MantidGeometry/Detector.h" 
#include "MantidGeometry/ParDetector.h" 
#include "MantidGeometry/ParObjComponent.h" 
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
  Detector* dc = dynamic_cast<Detector*>(child_base.get());
  CompAssembly* ac = dynamic_cast<CompAssembly*>(child_base.get());
  ObjComponent* oc = dynamic_cast<ObjComponent*>(child_base.get());
  Component* cc = dynamic_cast<Component*>(child_base.get());
  if (dc)
      return boost::shared_ptr<IComponent>(new ParDetector(dc,m_map));
  else if (ac)
      return boost::shared_ptr<IComponent>(new ParCompAssembly(ac,m_map));
  else if (oc)
      return boost::shared_ptr<IComponent>(new ParObjComponent(oc,m_map));
  else if (cc)
      return boost::shared_ptr<IComponent>(new ParametrizedComponent(cc,m_map));
  else
  {
      throw std::runtime_error("ParCompAssembly::operator[] zero pointer.");
  }
    //  return new ParObjComponent(dynamic_cast<ObjComponent*>(child_base),m_map);
  return  boost::shared_ptr<IComponent>() ;
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

