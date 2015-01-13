#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include <algorithm>
#include <ostream>
#include <stdexcept>

namespace Mantid {
namespace Geometry {

using Kernel::V3D;
using Kernel::Quat;

/** Empty constructor
 */
CompAssembly::CompAssembly()
    : Component(), m_children(), m_cachedBoundingBox(NULL) {}

/** Constructor for a parametrized CompAssembly
 * @param base: the base (un-parametrized) IComponent
 * @param map: pointer to the ParameterMap
 * */
CompAssembly::CompAssembly(const IComponent *base, const ParameterMap *map)
    : Component(base, map), m_children(), m_cachedBoundingBox(NULL) {}

/** Valued constructor
 *  @param n :: name of the assembly
 *  @param reference :: the parent Component
 *
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */
CompAssembly::CompAssembly(const std::string &n, IComponent *reference)
    : Component(n, reference), m_children(), m_cachedBoundingBox(NULL) {
  if (reference) {
    ICompAssembly *test = dynamic_cast<ICompAssembly *>(reference);
    if (test) {
      test->add(this);
    }
  }
}

/** Copy constructor
 *  @param assem :: assembly to copy
 */
CompAssembly::CompAssembly(const CompAssembly &assem)
    : Component(assem), m_children(assem.m_children),
      m_cachedBoundingBox(assem.m_cachedBoundingBox) {
  // Need to do a deep copy
  comp_it it;
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    *it = (*it)->clone();
    // Move copied component object's parent from old to new CompAssembly
    (*it)->setParent(this);
  }
}

/** Destructor
 */
CompAssembly::~CompAssembly() {
  if (m_cachedBoundingBox)
    delete m_cachedBoundingBox;
  // Iterate over pointers in m_children, deleting them
  for (comp_it it = m_children.begin(); it != m_children.end(); ++it) {
    delete *it;
  }
  m_children.clear();
}

/** Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent *CompAssembly::clone() const { return new CompAssembly(*this); }

/** Add method
 * @param comp :: component to add
 * @return number of components in the assembly
 *
 * This becomes the new parent of comp.
 */
int CompAssembly::add(IComponent *comp) {
  if (m_map)
    throw std::runtime_error(
        "CompAssembly::add() called for a parametrized CompAssembly.");

  if (comp) {
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
int CompAssembly::addCopy(IComponent *comp) {
  if (m_map)
    throw std::runtime_error(
        "CompAssembly::addCopy() called for a parametrized CompAssembly.");

  if (comp) {
    IComponent *newcomp = comp->clone();
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
int CompAssembly::addCopy(IComponent *comp, const std::string &n) {
  if (m_map)
    throw std::runtime_error(
        "CompAssembly::addCopy() called for a parametrized CompAssembly.");

  if (comp) {
    IComponent *newcomp = comp->clone();
    newcomp->setParent(this);
    newcomp->setName(n);
    m_children.push_back(newcomp);
  }
  return static_cast<int>(m_children.size());
}

/** Removes a component from the assembly. The component will be deleted.
 *  @param comp The component to be removed
 *  @return The (new) number of components in the assembly
 *  @throws std::runtime_error if the component is not a child of this assembly
 */
int CompAssembly::remove(IComponent *comp) {
  if (m_map)
    throw std::runtime_error(
        "CompAssembly::remove() called for a parameterized CompAssembly.");

  // Look for the passed in component in the list of children
  std::vector<IComponent *>::iterator it =
      std::find(m_children.begin(), m_children.end(), comp);
  if (it != m_children.end()) {
    // If it's found, remove it from the list and then delete it
    m_children.erase(it);
    delete comp;
  } else {
    throw std::runtime_error("Component " + comp->getName() +
                             " is not a child of this assembly.");
  }

  return static_cast<int>(m_children.size());
}

/** Return the number of components in the assembly
 * @return m_children.size()
 */
int CompAssembly::nelements() const {
  if (m_map)
    return dynamic_cast<const CompAssembly *>(m_base)->nelements();
  else
    return static_cast<int>(m_children.size());
}

/** Get a pointer to the ith component in the assembly. Note standard C/C++
 *  array notation used, that is, i most be an integer i = 0,1,..., N-1, where
 *  N is the number of component in the assembly. Easier to use than the []
 *operator
 *  when you have a pointer
 *
 * @param i :: The index of the component you want
 * @return m_children[i]
 * @throw std::runtime_error if i is not in range
 */
boost::shared_ptr<IComponent> CompAssembly::getChild(const int i) const {
  if (m_map) {
    // Get the child of the base (unparametrized) assembly
    boost::shared_ptr<IComponent> child_base =
        dynamic_cast<const CompAssembly *>(m_base)->getChild(i);
    // And build up a parametrized version of it using the factory, and return
    // that
    return ParComponentFactory::create(child_base, m_map);
  } else {
    if (i < 0 || i > static_cast<int>(m_children.size() - 1)) {
      throw std::runtime_error("CompAssembly::getChild() range not valid");
    }
    return boost::shared_ptr<IComponent>(m_children[i], NoDeleting());
  }
}

/** Overloaded index access operator. \link getChild() \endlink
 * @param i :: Element i within the component assembly
 * @return A shared pointer to the ith component
 * @throw std:runtime_error if i is out of range
 */
boost::shared_ptr<IComponent> CompAssembly::operator[](int i) const {
  return this->getChild(i);
}

//------------------------------------------------------------------------------------------------
/** Return a vector of all contained children components
 *
 * @param outVector :: vector of IComponent sptr.
 * @param recursive :: if a child is a CompAssembly, returns its children
 *recursively
 */
void CompAssembly::getChildren(std::vector<IComponent_const_sptr> &outVector,
                               bool recursive) const {
  for (int i = 0; i < this->nelements(); i++) {
    boost::shared_ptr<IComponent> comp = this->getChild(i);
    if (comp) {
      outVector.push_back(comp);
      // Look deeper, on option.
      if (recursive) {
        boost::shared_ptr<ICompAssembly> assemb =
            boost::dynamic_pointer_cast<ICompAssembly>(comp);
        if (assemb)
          assemb->getChildren(outVector, recursive);
      }
    }
  }
}

/**
* Find a component by name.
* @param cname :: The name of the component. If there are multiple matches, the
* first one found is returned.
* If the name contains '/', it will search the the component whose name occurs
* before the '/'
* then within that component's assembly,
* search the component whose name occurs after the '/' and so on with any
* subsequent '/'.
* For example to find 'tube020' in 'panel07', one could use the cname
* 'panel07/tube020',
* given that tube020 is unique within panel07.
* @param nlevels :: Optional argument to limit number of levels searched.
* If cname has a '/', then nlevels will apply to each step delimited by the
* '/'s, rather than the whole search.
* In particular, nlevels=1, would force cname to be a full path name.
* @returns A shared pointer to the component
*/
boost::shared_ptr<const IComponent>
CompAssembly::getComponentByName(const std::string &cname, int nlevels) const {
  boost::shared_ptr<const IComponent> node =
      boost::shared_ptr<const IComponent>(this, NoDeleting());

  // If name has '/' in it, it is taken as part of a path name of the component.
  // Steps may be skipped at a '/' from the path name,
  // but if what follows the skip is not unique within what precedes it, only
  // the first found is returned.
  size_t cut = cname.find('/');
  if (cut < cname.length()) {
    node = this->getComponentByName(cname.substr(0, cut), nlevels);
    if (node) {
      boost::shared_ptr<const ICompAssembly> asmb =
          boost::dynamic_pointer_cast<const ICompAssembly>(node);
      return asmb->getComponentByName(cname.substr(cut + 1, std::string::npos),
                                      nlevels);
    } else {
      return boost::shared_ptr<const IComponent>(); // Search failed
    }
  }

  // Check the instrument name first
  if (this->getName() == cname) {
    return node;
  }
  // Otherwise Search the instrument tree using a breadth-first search algorithm
  // since most likely candidates
  // are higher-level components
  // I found some useful info here
  // http://www.cs.bu.edu/teaching/c/tree/breadth-first/
  std::deque<boost::shared_ptr<const IComponent>> nodeQueue;
  // Need to be able to enter the while loop
  nodeQueue.push_back(node);
  const bool limitSearch(nlevels > 0);
  while (!nodeQueue.empty()) {
    // get the next node in the queue
    node = nodeQueue.front();
    nodeQueue.pop_front();

    // determine the depth
    int depth(1);
    if (limitSearch) {
      auto parent = node->getParent();
      while (parent && (this->getName() != parent->getName())) {
        parent = parent->getParent();
        depth++;
      }
    }
    int nchildren(0);
    boost::shared_ptr<const ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const ICompAssembly>(node);
    if (asmb) {
      nchildren = asmb->nelements();
    }
    for (int i = 0; i < nchildren; ++i) {
      boost::shared_ptr<const IComponent> comp = (*asmb)[i];
      if (comp->getName() == cname) {
        return comp;
      } else {
        // only add things if max-recursion depth hasn't been reached
        if ((!limitSearch) || (depth + 1 < nlevels)) {
          // don't bother adding things to the queue that aren't assemblies
          if (bool(boost::dynamic_pointer_cast<const ICompAssembly>(comp))) {
            // for rectangular detectors search the depth rather than siblings
            // as there
            // is a specific naming convention to speed things along
            auto rectDet =
                boost::dynamic_pointer_cast<const RectangularDetector>(comp);
            if (bool(rectDet)) {
              auto child = rectDet->getComponentByName(cname, nlevels);
              if (child)
                return child;
            } else {
              nodeQueue.push_back(comp);
            }
          }
        }
      }
    }
  } // while-end

  // If we have reached here then the search failed
  return boost::shared_ptr<const IComponent>();
}

//------------------------------------------------------------------------------------------------
/**
* Get the bounding box for this assembly. It is simply the sum of the bounding
* boxes of its children
 * @param assemblyBox :: [Out] The resulting bounding box is stored here.
 */
void CompAssembly::getBoundingBox(BoundingBox &assemblyBox) const {
  if (m_map) {
    // Check cache for assembly, inside the ParameterMap
    if (m_map->getCachedBoundingBox(this, assemblyBox)) {
      return;
    }

    // Loop over the children and define a box large enough for all of them
    assemblyBox = BoundingBox(); // this makes assembly box always axis alighned
    int nchildren = nelements();
    for (int i = 0; i < nchildren; ++i) {
      IComponent_sptr comp = this->operator[](i);
      BoundingBox compBox;
      comp->getBoundingBox(compBox);
      assemblyBox.grow(compBox);
    }
    // Set the cache
    m_map->setCachedBoundingBox(this, assemblyBox);
  }

  else {
    // Not parametrized
    if (!m_cachedBoundingBox) {
      m_cachedBoundingBox = new BoundingBox();
      // Loop over the children and define a box large enough for all of them
      for (const_comp_it it = m_children.begin(); it != m_children.end();
           ++it) {
        BoundingBox compBox;
        if (*it) {
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
/** Test the intersection of the ray with the children of the component
 *assembly, for InstrumentRayTracer.
 *
 * @param testRay :: Track under test. The results are stored here.
 * @param searchQueue :: If a child is a sub-assembly then it is appended for
 *later searching
 */
void CompAssembly::testIntersectionWithChildren(
    Track &testRay, std::deque<IComponent_const_sptr> &searchQueue) const {
  int nchildren = this->nelements();
  for (int i = 0; i < nchildren; ++i) {
    boost::shared_ptr<Geometry::IComponent> comp = this->getChild(i);
    if (ICompAssembly_sptr childAssembly =
            boost::dynamic_pointer_cast<ICompAssembly>(comp)) {
      searchQueue.push_back(comp);
    }
    // Check the physical object intersection
    else if (IObjComponent *physicalObject =
                 dynamic_cast<IObjComponent *>(comp.get())) {
      physicalObject->interceptSurface(testRay);
    } else {
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Print information about elements in the assembly to a stream
 * @param os :: output stream
 *
 *  Loops through all components in the assembly
 *  and call printSelf(os).
 */
void CompAssembly::printChildren(std::ostream &os) const {
  for (int i = 0; i < nelements(); i++) {
    boost::shared_ptr<IComponent> it = (*this)[i];
    os << "Component " << i << " : **********" << std::endl;
    it->printSelf(os);
  }
}

/** Print information about all the elements in the tree to a stream
 *  Loops through all components in the tree
 *  and call printSelf(os).
 *
 * @param os :: output stream
 */
void CompAssembly::printTree(std::ostream &os) const {
  for (int i = 0; i < nelements(); i++) {
    boost::shared_ptr<IComponent> it = (*this)[i];
    const CompAssembly *test = dynamic_cast<CompAssembly *>(it.get());
    os << "Element " << i << " from " << nelements() << " in the assembly : ";
    if (test) {
      os << test->getName() << std::endl;
      os << "Children :******** " << std::endl;
      test->printTree(os);
    } else
      os << it->getName() << std::endl;
  }
}

/** Gets the absolute position of the Parametrized CompAssembly
 * This attempts to read the cached position value from the parameter map, and
 * creates it if it is not available.
 * @returns A vector of the absolute position
 */
V3D CompAssembly::getPos() const {
  if (!m_map)
    return Component::getPos();
  else {
    V3D pos;
    if (!m_map->getCachedLocation(m_base, pos)) {
      pos = Component::getPos();
      m_map->setCachedLocation(m_base, pos);
    }
    return pos;
  }
}

/** Gets the absolute position of the Parametrized CompAssembly
 * This attempts to read the cached position value from the parameter map, and
 * creates it if it is not available.
 * @returns A vector of the absolute position
 */
const Quat CompAssembly::getRotation() const {
  if (!m_map)
    return Component::getRotation();
  else {
    Quat rot;
    if (!m_map->getCachedRotation(m_base, rot)) {
      rot = Component::getRotation();
      m_map->setCachedRotation(m_base, rot);
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
std::ostream &operator<<(std::ostream &os, const CompAssembly &ass) {
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid
