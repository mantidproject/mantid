#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include <algorithm>
#include <stdexcept>
#include <ostream>
#include <iostream>

namespace {
  Mantid::Kernel::Logger g_log("ObjCompAssembly");
}

namespace Mantid {
namespace Geometry {
using Kernel::V3D;
using Kernel::Quat;
using Kernel::DblMatrix;

/// Void deleter for shared pointers
class NoDeleting {
public:
  /// deleting operator. Does nothing
  void operator()(void *p) { (void)p; }
};

/** Constructor for a parametrized ObjComponent
 * @param base: the base (un-parametrized) IComponent
 * @param map: pointer to the ParameterMap
 */
ObjCompAssembly::ObjCompAssembly(const IComponent *base,
                                 const ParameterMap *map)
    : ObjComponent(base, map) {}

/** Valued constructor
 *  @param n :: name of the assembly
 *  @param reference :: the parent Component
 *
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */
ObjCompAssembly::ObjCompAssembly(const std::string &n, IComponent *reference)
    : ObjComponent(n, reference) {
  if (reference) {
    ICompAssembly *test = dynamic_cast<ICompAssembly *>(reference);
    if (test)
      test->add(this);
  }
}

/** Copy constructor
 *  @param ass :: assembly to copy
 */
ObjCompAssembly::ObjCompAssembly(const ObjCompAssembly &ass)
    : ICompAssembly(ass), IObjComponent(ass), ObjComponent(ass) {
  group = ass.group;
  // Need to do a deep copy
  comp_it it;
  for (it = group.begin(); it != group.end(); ++it) {
    ObjComponent *c = dynamic_cast<ObjComponent *>((*it)->clone());
    if (!c) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "ObjCompAssembly cannot contain components of non-ObjComponent type");
    }
    *it = c;
    // Move copied component object's parent from old to new ObjCompAssembly
    (*it)->setParent(this);
  }
}

/** Destructor
 */
ObjCompAssembly::~ObjCompAssembly() {
  // Iterate over pointers in group, deleting them
  // std::vector<IComponent*>::iterator it;
  for (comp_it it = group.begin(); it != group.end(); ++it) {
    delete *it;
  }
  group.clear();
}

/** Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent *ObjCompAssembly::clone() const {
  return new ObjCompAssembly(*this);
}

/** Add method
 * @param comp :: component to add
 * @return number of components in the assembly
 *
 * This becomes the new parent of comp.
 */
int ObjCompAssembly::add(IComponent *comp) {
  if (m_map)
    throw std::runtime_error(
        "ObjCompAssembly::add() called on a Parametrized object.");

  if (comp) {
    ObjComponent *c = dynamic_cast<ObjComponent *>(comp);
    if (!c) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "ObjCompAssembly cannot contain components of non-ObjComponent type");
    }
    comp->setParent(this);
    group.push_back(c);
  }
  return static_cast<int>(group.size());
}

/** AddCopy method
 * @param comp :: component to add
 * @return number of components in the assembly
 *
 *  Add a copy of a component in the assembly.
 *  Comp is cloned if valid, then added in the assembly
 *  This becomes the parent of the cloned component
 */
int ObjCompAssembly::addCopy(IComponent *comp) {
  if (m_map)
    throw std::runtime_error(
        "ObjCompAssembly::addCopy() called on a Parametrized object.");

  if (comp) {
    IComponent *newcomp = comp->clone();
    ObjComponent *c = dynamic_cast<ObjComponent *>(newcomp);
    if (!c) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "ObjCompAssembly cannot contain components of non-ObjComponent type");
    }
    newcomp->setParent(this);
    group.push_back(c);
  }
  return static_cast<int>(group.size());
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
int ObjCompAssembly::addCopy(IComponent *comp, const std::string &n) {
  if (m_map)
    throw std::runtime_error(
        "ObjCompAssembly::addCopy() called on a Parametrized object.");

  if (comp) {
    IComponent *newcomp = comp->clone();
    ObjComponent *c = dynamic_cast<ObjComponent *>(newcomp);
    if (!c) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "ObjCompAssembly cannot contain components of non-ObjComponent type");
    }
    newcomp->setParent(this);
    newcomp->setName(n);
    group.push_back(c);
  }
  return static_cast<int>(group.size());
}

/** Return the number of components in the assembly
 * @return group.size()
 */
int ObjCompAssembly::nelements() const {
  if (m_map)
    return dynamic_cast<const ObjCompAssembly *>(m_base)->nelements();
  else
    return static_cast<int>(group.size());
}

/** Get a pointer to the ith component in the assembly. Note standard C/C++
 *  array notation used, that is, i most be an integer i = 0,1,..., N-1, where
 *  N is the number of component in the assembly.
 *
 * @param i :: The index of the component you want
 * @return group[i]
 *
 *  Throws if i is not in range
 */
boost::shared_ptr<IComponent> ObjCompAssembly::operator[](int i) const {
  if (i < 0 || i > nelements() - 1) {
    throw std::runtime_error("ObjCompAssembly::operator[] range not valid");
  }

  if (m_map) {
    boost::shared_ptr<IComponent> child_base =
        dynamic_cast<const ObjCompAssembly *>(m_base)->operator[](i);
    return ParComponentFactory::create(child_base, m_map);
  } else {
    // Unparamterized - return the normal one
    return boost::shared_ptr<IComponent>(group[i], NoDeleting());
  }
}

//------------------------------------------------------------------------------------------------
/** Return a vector of all contained children components
 *
 * @param outVector :: vector of IComponent sptr.
 * @param recursive :: if a child is a CompAssembly, returns its children
 *recursively
 */
void ObjCompAssembly::getChildren(std::vector<IComponent_const_sptr> &outVector,
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
* @param nlevels :: Optional argument to limit number of levels searched.
* @returns A shared pointer to the component
*/
boost::shared_ptr<const IComponent>
ObjCompAssembly::getComponentByName(const std::string &cname,
                                    int nlevels) const {
  int nchildren = this->nelements();
  if (nlevels > 1)
    std::cout << "only implemented for children\n";
  for (int i = 0; i < nchildren; ++i) {
    boost::shared_ptr<Geometry::IComponent> comp = this->getChild(i);
    if (comp->getName() == cname)
      return comp;
  }
  return boost::shared_ptr<const IComponent>();
}

/** Print information about elements in the assembly to a stream
 * @param os :: output stream
 *
 *  Loops through all components in the assembly
 *  and call printSelf(os).
 */
void ObjCompAssembly::printChildren(std::ostream &os) const {
  // std::vector<IComponent*>::const_iterator it;
  int i = 0;
  for (i = 0; i < this->nelements(); i++) {
    os << "Component " << i << " : **********" << std::endl;
    this->operator[](i)->printSelf(os);
  }
}

/** Print information about all the elements in the tree to a stream
 *  Loops through all components in the tree
 *  and call printSelf(os).
 *
 * @param os :: output stream
 */
void ObjCompAssembly::printTree(std::ostream &os) const {
  // std::vector<IComponent*>::const_iterator it;
  int i = 0;
  for (i = 0; i < this->nelements(); i++) {
    boost::shared_ptr<const ObjCompAssembly> test =
        boost::dynamic_pointer_cast<const ObjCompAssembly>(this->operator[](i));
    os << "Element " << i << " in the assembly : ";
    if (test) {
      os << test->getName() << std::endl;
      os << "Children :******** " << std::endl;
      test->printTree(os);
    } else
      os << this->operator[](i)->getName() << std::endl;
  }
}

/** Gets the absolute position of the Parametrized ObjCompAssembly
 * This attempts to read the cached position value from the parameter map, and
 * creates it if it is not available.
 * @returns A vector of the absolute position
 */
V3D ObjCompAssembly::getPos() const {
  if (m_map) {
    V3D pos;
    if (!m_map->getCachedLocation(m_base, pos)) {
      pos = Component::getPos();
      m_map->setCachedLocation(m_base, pos);
    }
    return pos;
  } else
    return Component::getPos();
}

/** Gets the absolute position of the Parametrized ObjCompAssembly
 * This attempts to read the cached position value from the parameter map, and
 * creates it if it is not available.
 * @returns A vector of the absolute position
 */
const Quat ObjCompAssembly::getRotation() const {
  if (m_map) {
    Quat rot;
    if (!m_map->getCachedRotation(m_base, rot)) {
      rot = Component::getRotation();
      m_map->setCachedRotation(m_base, rot);
    }
    return rot;
  } else
    return Component::getRotation();
}

//------------------------------------------------------------------------------------------------
/** Test the intersection of the ray with the children of the component
 *assembly, for InstrumentRayTracer.
 *
 * @param testRay :: Track under test. The results are stored here.
 * @param searchQueue :: If a child is a sub-assembly then it is appended for
 *later searching
 */
void ObjCompAssembly::testIntersectionWithChildren(
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

/** Set the outline of the assembly. Creates an Object and sets m_shape point to
 * it.
 *  All child components must be detectors and positioned along a straight line
 * and have the same shape.
 *  The shape can be either a box or a cylinder.
 *  @return The shape of the outline: "cylinder", "box", ...
 */
boost::shared_ptr<Object> ObjCompAssembly::createOutline() {
  if (group.empty()) {
    throw Kernel::Exception::InstrumentDefinitionError("Empty ObjCompAssembly");
  }

  if (nelements() < 2) {
    g_log.warning("Creating outline with fewer than 2 elements. The outline displayed may be inaccurate.");
  }

  // Get information about the shape and size of a detector
  std::string type;
  int otype;
  std::vector<Kernel::V3D> vectors;
  double radius, height;
  boost::shared_ptr<const Object> obj = group.front()->shape();
  if (!obj) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Found ObjComponent without shape");
  }
  obj->GetObjectGeom(otype, vectors, radius, height);
  if (otype == 1) {
    type = "box";
  } else if (otype == 3) {
    type = "cylinder";
  } else {
    throw std::runtime_error(
        "IDF \"outline\" option is only allowed for assemblies containing "
        "components of types \"box\" or \"cylinder\".");
  }

  // Calculate the dimensions of the outline object

  // find the 'moments of inertia' of the assembly
  double Ixx = 0, Iyy = 0, Izz = 0, Ixy = 0, Ixz = 0, Iyz = 0;
  V3D Cmass; // 'center of mass' of the assembly
  for (const_comp_it it = group.begin(); it != group.end(); it++) {
    V3D p = (**it).getRelativePos();
    Cmass += p;
  }
  Cmass /= nelements();
  for (const_comp_it it = group.begin(); it != group.end(); it++) {
    V3D p = (**it).getRelativePos();
    double x = p.X() - Cmass.X(), x2 = x * x;
    double y = p.Y() - Cmass.Y(), y2 = y * y;
    double z = p.Z() - Cmass.Z(), z2 = z * z;
    Ixx += y2 + z2;
    Iyy += x2 + z2;
    Izz += y2 + x2;
    Ixy -= x * y;
    Ixz -= x * z;
    Iyz -= y * z;
  }
  // principal axes of the outline shape
  // vz defines the line through all pixel centres
  V3D vx, vy, vz;

  if (Ixx == 0) // pixels along x axis
  {
    vx = V3D(0, 1, 0);
    vy = V3D(0, 0, 1);
    vz = V3D(1, 0, 0);
  } else if (Iyy == 0) // pixels along y axis
  {
    vx = V3D(0, 0, 1);
    vy = V3D(1, 0, 0);
    vz = V3D(0, 1, 0);
  } else if (Izz == 0) // pixels along z axis
  {
    vx = V3D(1, 0, 0);
    vy = V3D(0, 1, 0);
    vz = V3D(0, 0, 1);
  } else {
    // Either the detectors are not perfectrly aligned or
    // vz is parallel to neither of the 3 axis x,y,z
    // This code is unfinished
    DblMatrix II(3, 3), Vec(3, 3), D(3, 3);
    II[0][0] = Ixx;
    II[0][1] = Ixy;
    II[0][2] = Ixz;
    II[1][0] = Ixy;
    II[1][1] = Iyy;
    II[1][2] = Iyz;
    II[2][0] = Ixz;
    II[2][1] = Iyz;
    II[2][2] = Izz;
    std::cerr << "Inertia matrix: " << II << II.determinant() << '\n';
    II.Diagonalise(Vec, D);
    std::cerr << "Vectors: " << Vec << '\n';
    std::cerr << "Moments: " << D << '\n';
    vx = V3D(Vec[0][0], Vec[1][0], Vec[2][0]);
    vy = V3D(Vec[0][1], Vec[1][1], Vec[2][1]);
    vz = V3D(Vec[0][2], Vec[1][2], Vec[2][2]);
  }

  // find assembly sizes along the principal axes
  double hx, hy, hz; // sizes along x,y, and z axis

  // maximum displacements from the mass centre along axis vx,vy, and vz
  // in positive (p) and negative (n) directions.
  // positive displacements are positive numbers and negative ones are negative
  double hxn = 0, hyn = 0, hzn = 0;
  double hxp = 0, hyp = 0, hzp = 0;
  for (const_comp_it it = group.begin(); it != group.end(); it++) {
    // displacement vector of a detector
    V3D p = (**it).getRelativePos() - Cmass;
    // its projection on the vx axis
    double h = p.scalar_prod(vx);
    if (h > hxp)
      hxp = h;
    if (h < hxn)
      hxn = h;
    // its projection on the vy axis
    h = p.scalar_prod(vy);
    if (h > hyp)
      hyp = h;
    if (h < hyn)
      hyn = h;
    // its projection on the vz axis
    h = p.scalar_prod(vz);
    if (h > hzp)
      hzp = h;
    if (h < hzn)
      hzn = h;
  }

  // calc the assembly sizes
  hx = hxp - hxn;
  hy = hyp - hyn;
  hz = hzp - hzn;
  // hx and hy must be practically zero
  if (hx > 1e-3 || hy > 1e-3) // arbitrary numbers
  {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Detectors of a ObjCompAssembly do not lie on a staraight line");
  }

  // determine the order of the detectors to make sure that the texture
  // coordinates are correct
  bool firstAtBottom; // first detector is at the bottom of the outline shape
  // the bottom end is the one with the negative displacement from the centre
  firstAtBottom =
      ((**group.begin()).getRelativePos() - Cmass).scalar_prod(vz) < 0;

  // form the input string for the ShapeFactory
  std::ostringstream obj_str;
  if (type == "box") {

    if (hz == 0)
      hz = 0.1;

    hx = hy = 0;
    height = 0;
    V3D p0 = vectors[0];
    for (size_t i = 1; i < vectors.size(); ++i) {
      V3D p = vectors[i] - p0;
      double h = fabs(p.scalar_prod(vx));
      if (h > hx)
        hx = h;
      h = fabs(p.scalar_prod(vy));
      if (h > hy)
        hy = h;
      height = fabs(p.scalar_prod(vz));
      if (h > height)
        height = h;
    }

    vx *= hx / 2;
    vy *= hy / 2;
    vz *= hzp + height / 2;

    if (!firstAtBottom) {
      vz = vz * (-1);
    }

    // define the outline shape as cuboid
    V3D p_lfb = Cmass - vx - vy - vz;
    V3D p_lft = Cmass - vx - vy + vz;
    V3D p_lbb = Cmass - vx + vy - vz;
    V3D p_rfb = Cmass + vx - vy - vz;
    obj_str << "<cuboid id=\"shape\">";
    obj_str << "<left-front-bottom-point ";
    obj_str << "x=\"" << p_lfb.X();
    obj_str << "\" y=\"" << p_lfb.Y();
    obj_str << "\" z=\"" << p_lfb.Z();
    obj_str << "\"  />";
    obj_str << "<left-front-top-point ";
    obj_str << "x=\"" << p_lft.X();
    obj_str << "\" y=\"" << p_lft.Y();
    obj_str << "\" z=\"" << p_lft.Z();
    obj_str << "\"  />";
    obj_str << "<left-back-bottom-point ";
    obj_str << "x=\"" << p_lbb.X();
    obj_str << "\" y=\"" << p_lbb.Y();
    obj_str << "\" z=\"" << p_lbb.Z();
    obj_str << "\"  />";
    obj_str << "<right-front-bottom-point ";
    obj_str << "x=\"" << p_rfb.X();
    obj_str << "\" y=\"" << p_rfb.Y();
    obj_str << "\" z=\"" << p_rfb.Z();
    obj_str << "\"  />";
    obj_str << "</cuboid>";

  } else if (type == "cylinder") {
    // the outline is one detector height short
    hz += height;
    // shift Cmass to the end of the cylinder where the first detector is
    if (firstAtBottom) {
      Cmass += vz * hzn;
    } else {
      hzp += height;
      Cmass += vz * hzp;
      // inverse the vz axis
      vz = vz * (-1);
    }
    obj_str << "<segmented-cylinder id=\"stick\">";
    obj_str << "<centre-of-bottom-base ";
    obj_str << "x=\"" << Cmass.X();
    obj_str << "\" y=\"" << Cmass.Y();
    obj_str << "\" z=\"" << Cmass.Z();
    obj_str << "\" />";
    obj_str << "<axis x=\"" << vz.X() << "\" y=\"" << vz.Y() << "\" z=\""
            << vz.Z() << "\" /> ";
    obj_str << "<radius val=\"" << radius << "\" />";
    obj_str << "<height val=\"" << hz << "\" />";
    obj_str << "</segmented-cylinder>";
  }

  if (!obj_str.str().empty()) {
    boost::shared_ptr<Object> s = ShapeFactory().createShape(obj_str.str());
    setOutline(s);
    return s;
  }
  return boost::shared_ptr<Object>();
}

/**
 * Sets the outline shape for this assembly
 * @param obj :: The outline shape created previously fith createOutline()
 */
void ObjCompAssembly::setOutline(boost::shared_ptr<const Object> obj) {
  m_shape = obj;
}

/** Print information about elements in the assembly to a stream
 *  Overload the operator <<
 * @param os :: output stream
 * @param ass :: component assembly
 * @return the stream representation of the object component assembly
 *  Loops through all components in the assembly
 *  and call printSelf(os).
 *  Also output the number of children
 */
std::ostream &operator<<(std::ostream &os, const ObjCompAssembly &ass) {
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid
