#include "MantidGeometry/Instrument/ObjCompAssembly.h" 
#include "MantidGeometry/Instrument/ObjComponent.h" 
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Exception.h"
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
    void operator()(void*p){}
};

/*! Empty constructor
 */
//ObjCompAssembly::ObjCompAssembly() : ObjComponent()
//{
//}

/*! Valued constructor
 *  @param n :: name of the assembly
 *  @param reference :: the parent Component
 * 
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */
ObjCompAssembly::ObjCompAssembly(const std::string& n, Component* reference) :
  ObjComponent(n, reference)
{
  if (reference)
  {
    ObjCompAssembly* test=dynamic_cast<ObjCompAssembly*>(reference);
    if (test)
      test->add(this);
  }
}

/*! Copy constructor
 *  @param ass :: assembly to copy
 */
ObjCompAssembly::ObjCompAssembly(const ObjCompAssembly& ass) :
  ObjComponent(ass)
{
  group=ass.group;
  // Need to do a deep copy
  comp_it it;
  for (it = group.begin(); it != group.end(); ++it)
  {
    ObjComponent* c =  dynamic_cast<ObjComponent*>((*it)->clone() );
    if (!c)
    {
      throw Kernel::Exception::InstrumentDefinitionError("ObjCompAssembly cannot contain components of non-ObjComponent type");
    }
    *it =  c;
    // Move copied component object's parent from old to new ObjCompAssembly
    (*it)->setParent(this);
  }
}

/*! Destructor
 */
ObjCompAssembly::~ObjCompAssembly()
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
IComponent* ObjCompAssembly::clone() const
{
  return new ObjCompAssembly(*this);
}

/*! Add method
 * @param comp :: component to add 
 * @return number of components in the assembly
 * 
 * This becomes the new parent of comp.
 */
int ObjCompAssembly::add(IComponent* comp)
{
  if (comp)
  {
    ObjComponent* c =  dynamic_cast<ObjComponent*>(comp);
    if (!c)
    {
      throw Kernel::Exception::InstrumentDefinitionError("ObjCompAssembly cannot contain components of non-ObjComponent type");
    }
    comp->setParent(this);
    group.push_back(c);
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
int ObjCompAssembly::addCopy(IComponent* comp)
{
  if (comp)
  {
    IComponent* newcomp=comp->clone();
    ObjComponent* c =  dynamic_cast<ObjComponent*>(newcomp);
    if (!c)
    {
      throw Kernel::Exception::InstrumentDefinitionError("ObjCompAssembly cannot contain components of non-ObjComponent type");
    }
    newcomp->setParent(this);
    group.push_back(c);
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
int ObjCompAssembly::addCopy(IComponent* comp, const std::string& n)
{
  if (comp)
  {
    IComponent* newcomp=comp->clone();
    ObjComponent* c =  dynamic_cast<ObjComponent*>(newcomp);
    if (!c)
    {
      throw Kernel::Exception::InstrumentDefinitionError("ObjCompAssembly cannot contain components of non-ObjComponent type");
    }
    newcomp->setParent(this);
    newcomp->setName(n);
    group.push_back(c);
  }
  return group.size();
}

/*! Return the number of components in the assembly
 * @return group.size() 
 */
int ObjCompAssembly::nelements() const
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
boost::shared_ptr<IComponent> ObjCompAssembly::operator[](int i) const
{
  if (i<0 || i> static_cast<int>(group.size()-1))
  throw std::runtime_error("ObjCompAssembly::operator[] range not valid");
  return boost::shared_ptr<IComponent>(group[i],NoDeleting());
}

/*! Print information about elements in the assembly to a stream
 * @param os :: output stream 
 * 
 *  Loops through all components in the assembly 
 *  and call printSelf(os). 
 */
void ObjCompAssembly::printChildren(std::ostream& os) const
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
void ObjCompAssembly::printTree(std::ostream& os) const
{
  //std::vector<IComponent*>::const_iterator it;
  int i=0;
  for (const_comp_it it=group.begin();it!=group.end();it++)
  {
    const ObjCompAssembly* test=dynamic_cast<ObjCompAssembly*>(*it);
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

/*! Set the outline of the assembly. Creates an Object and sets shape point to it
 *  @param type The shsape of the outline: "cylinder", "box", ...
 */
boost::shared_ptr<Object> ObjCompAssembly::createOutline()
{
  if (group.empty()) return boost::shared_ptr<Object>();
  std::string type;
  int otype;
  std::vector<Geometry::V3D> vectors;
  double radius, height;
  boost::shared_ptr<const Object> obj = group.front()->shape();
  if (!obj)
  {
    throw Kernel::Exception::InstrumentDefinitionError("Found ObjComponent without shape");
  }
  obj->GetObjectGeom(otype, vectors, radius, height);
  if (otype == 1)
  {
    type = "box";
  }
  else if (otype == 3)
  {
    type = "cylinder";
  }
  // find the basis vectors of the plane
  // find the 'moments of inertia' of the assembly
  double Ixx=0,Iyy=0,Izz=0,Ixy=0,Ixz=0,Iyz=0;
  V3D Cmass;
  for (const_comp_it it=group.begin();it!=group.end();it++)
  {
    V3D p = (**it).getRelativePos();
    //V3D p = (**it).getPos();
    Cmass += p;
  }
  Cmass /= nelements();
  for (const_comp_it it=group.begin();it!=group.end();it++)
  {
    V3D p = (**it).getRelativePos();
    //V3D p = (**it).getPos();
    double x = p.X()-Cmass.X(),x2 = x*x;
    double y = p.Y()-Cmass.Y(),y2 = y*y;
    double z = p.Z()-Cmass.Z(),z2 = z*z;
    Ixx += y2 + z2;
    Iyy += x2 + z2;
    Izz += y2 + x2;
    Ixy -= x*y;
    Ixz -= x*z;
    Iyz -= y*z;
  }
  V3D vx,vy,vz; // principal axes of the outline shape
  if (Ixx == 0) // pixels along x axis
  {
    vx = V3D(0,1,0);
    vy = V3D(0,0,1);
    vz = V3D(1,0,0);
  }
  else if (Iyy == 0) // pixels along y axis
  {
    vx = V3D(0,0,1);
    vy = V3D(1,0,0);
    vz = V3D(0,1,0);
  }
  else if (Izz == 0) // pixels along z axis
  {
    vx = V3D(1,0,0);
    vy = V3D(0,1,0);
    vz = V3D(0,0,1);
  }
  else
  {
    Matrix<double> II(3,3),Vec(3,3),D(3,3);
    II[0][0] = Ixx;
    II[0][1] = Ixy;
    II[0][2] = Ixz;
    II[1][0] = Ixy;
    II[1][1] = Iyy;
    II[1][2] = Iyz;
    II[2][0] = Ixz;
    II[2][1] = Iyz;
    II[2][2] = Izz;
    std::cerr<<"Inertia matrix: "<<II<<II.determinant()<<'\n';
    II.Diagonalise(Vec,D);
    std::cerr<<"Vectors: "<<Vec<<'\n';
    std::cerr<<"Moments: "<<D<<'\n';
    vx = V3D(Vec[0][0],Vec[1][0],Vec[2][0]);
    vy = V3D(Vec[0][1],Vec[1][1],Vec[2][1]);
    vz = V3D(Vec[0][2],Vec[1][2],Vec[2][2]);
  }

  // find assembly sizes along the principal axes
  double hx, hy, hz;
  double hxn = 0,hyn = 0, hzn = 0;
  double hxp = 0,hyp = 0, hzp = 0;
  for (const_comp_it it=group.begin();it!=group.end();it++)
  {
    V3D p = (**it).getRelativePos() - Cmass;
    double h = p.scalar_prod(vx);
    if (h > hxp) hxp = h;
    if (h < hxn) hxn = h;
    h = p.scalar_prod(vy);
    if (h > hyp) hyp = h;
    if (h < hyn) hyn = h;
    h = p.scalar_prod(vz);
    if (h > hzp) hzp = h;
    if (h < hzn) hzn = h;
  }

  hx = hxp - hxn;
  hy = hyp - hyn;
  hz = hzp - hzn;

  std::ostringstream obj_str;
  if (type == "box")
  {
    if (hx == 0) hx = 0.01;
    if (hy == 0) hy = 0.01;
    if (hz == 0) hz = 0.01;

    vx *= hx;
    vy *= hy;
    vz *= hz;

    // define the outline shape as cuboid
    V3D p_lfb = Cmass - vx - vy + vz;
    V3D p_lft = Cmass - vx + vy + vz;
    V3D p_lbb = Cmass - vx - vy - vz;
    V3D p_rfb = Cmass + vx - vy + vz;
    obj_str << "<cuboid id=\"shape\">";
    obj_str << "<left-front-bottom-point ";
    obj_str << "x=\""<<p_lfb.X();
    obj_str << "\" y=\""<<p_lfb.Y();
    obj_str << "\" z=\""<<p_lfb.Z();
    obj_str << "\"  />";
    obj_str << "<left-front-top-point ";
    obj_str << "x=\""<<p_lft.X();
    obj_str << "\" y=\""<<p_lft.Y();
    obj_str << "\" z=\""<<p_lft.Z();
    obj_str << "\"  />";
    obj_str << "<left-back-bottom-point ";
    obj_str << "x=\""<<p_lbb.X();
    obj_str << "\" y=\""<<p_lbb.Y();
    obj_str << "\" z=\""<<p_lbb.Z();
    obj_str << "\"  />";
    obj_str << "<right-front-bottom-point ";
    obj_str << "x=\""<<p_rfb.X();
    obj_str << "\" y=\""<<p_rfb.Y();
    obj_str << "\" z=\""<<p_rfb.Z();
    obj_str << "\"  />";
    obj_str << "</cuboid>";

  }
  else if (type == "cylinder")
  {
    Cmass += vz*hzn; // shift Cmass to the bottom of the cylinder
    obj_str << "<cylinder id=\"stick\">";
    obj_str << "<centre-of-bottom-base ";
    obj_str << "x=\""<<Cmass.X();
    obj_str << "\" y=\""<<Cmass.Y();
    obj_str << "\" z=\""<<Cmass.Z();
    obj_str << "\" />";
    obj_str << "<axis x=\""<<vz.X()<<"\" y=\""<<vz.Y()<<"\" z=\""<<vz.Z()<<"\" /> ";
    obj_str << "<radius val=\""<<radius<<"\" />";
    obj_str << "<height val=\""<<hz<<"\" />";
    obj_str << "</cylinder>";
  }

  if (!obj_str.str().empty())
  {
    boost::shared_ptr<Object> s = ShapeFactory().createShape(obj_str.str());
    setOutline(s);
    std::cerr<<"create shape "<<obj_str.str()<<'\n';
    std::cerr<<"Cmass "<<Cmass<<' '<<getRotation()<<'\n';
    return s;
  }
  return boost::shared_ptr<Object>();
}

void ObjCompAssembly::setOutline(boost::shared_ptr<const Object> obj)
{
  m_shape = obj;
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
std::ostream& operator<<(std::ostream& os, const ObjCompAssembly& ass)
{
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid

