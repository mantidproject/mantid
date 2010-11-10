#ifndef MANTID_TESTParObjObjCompAssembly__
#define MANTID_TESTParObjObjCompAssembly__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <string>
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"

using namespace Mantid::Geometry;

class ParObjCompAssemblyTest : public CxxTest::TestSuite
{
public:

  void testNameValueConstructor()
  {
    ObjCompAssembly q("Name");

    ParameterMap_sptr pmap( new ParameterMap() );
    ObjCompAssembly pq(&q,pmap);

    TS_ASSERT_EQUALS(pq.nelements(), 0);
    TS_ASSERT_THROWS(pq[0], std::runtime_error);

    TS_ASSERT_EQUALS(pq.getName(), "Name");
    TS_ASSERT(!pq.getParent());
    TS_ASSERT_EQUALS(pq.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(pq.getRelativeRot(), Quat(1, 0, 0, 0));
    //as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(pq.getRelativePos(), pq.getPos());
  }

  void testNameParentValueConstructor()
  {
    ObjCompAssembly* parent = new ObjCompAssembly("Parent");
    //name and parent
    ObjCompAssembly* q = new ObjCompAssembly("Child", parent);

    ParameterMap_sptr pmap( new ParameterMap() );
    ObjCompAssembly pq(q,pmap);

    TS_ASSERT_EQUALS(pq.getName(), "Child");
    TS_ASSERT_EQUALS(pq.nelements(), 0);
    TS_ASSERT_THROWS(pq[0], std::runtime_error);
    //check the parent
    TS_ASSERT(pq.getParent());
    TS_ASSERT_EQUALS(pq.getParent()->getName(), parent->getName());

    TS_ASSERT_EQUALS(pq.getPos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(pq.getRelativeRot(), Quat(1, 0, 0, 0));
    //as the parent is at 0,0,0 GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(pq.getRelativePos(), pq.getPos());
    delete parent;
  }

  void testAdd()
  {
    ObjCompAssembly bank("BankName");
    Component* det1 = new ObjComponent("Det1Name");
    Component* det2 = new ObjComponent("Det2Name");
    Component* det3 = new ObjComponent("Det3Name");
    TS_ASSERT_EQUALS(bank.nelements(), 0);
    TS_ASSERT_THROWS(bank[0], std::runtime_error);
    bank.add(det1);
    bank.add(det2);
    bank.add(det3);

    ParameterMap_sptr pmap( new ParameterMap() );
    ObjCompAssembly pbank(&bank,pmap);

    TS_ASSERT_EQUALS(pbank.nelements(), 3);
    boost::shared_ptr<IComponent> det1copy;
    det1copy = pbank[0];

    TS_ASSERT_EQUALS(det1->getName(), det1copy->getName());
    //show that they are the same object
    det1->setName("ChangedName");
    TS_ASSERT_EQUALS(det1->getName(), det1copy->getName());

    pmap->addV3D(det2,"pos",V3D(1,1,1));
    boost::shared_ptr<IComponent> det2copy;
    det2copy = pbank[1];
    TS_ASSERT_DIFFERS(det2->getPos(), det2copy->getPos());
  }

  void testGetParent()
  {
    Component parent("Parent", V3D(1, 1, 1), Quat(1, 1, 1, 1));

    ObjCompAssembly q("Child", &parent);

    ParameterMap_sptr pmap( new ParameterMap() );
    ObjCompAssembly pq(&q,pmap);

    TS_ASSERT(pq.getParent());
    TS_ASSERT_EQUALS(pq.getParent()->getName(), parent.getName());
    TS_ASSERT_EQUALS(pq.getParent()->getPos(), V3D(1, 1, 1));
    TS_ASSERT_EQUALS(pq.getParent()->getRelativeRot(), Quat(1, 1, 1, 1));
  }

  void testType()
  {
    ObjCompAssembly comp("name");

    ParameterMap_sptr pmap( new ParameterMap() );
    ObjCompAssembly pcomp(&comp,pmap);

    TS_ASSERT_EQUALS(pcomp.type(), "ObjCompAssembly");
  }

  void testCreateOutlineCylinder()
  {
    std::stringstream obj_str;
    obj_str << "<cylinder id=\"stick\">";
    obj_str << "<centre-of-bottom-base ";
    obj_str << "x=\"0\" y=\"0\" z=\"0\" />";
    obj_str << "<axis x=\"0\" y=\"1\" z=\"0\" /> ";
    obj_str << "<radius val=\"0.1\" />";
    obj_str << "<height val=\"0.2\" />";
    obj_str << "</cylinder>";
    boost::shared_ptr<Object> s = Mantid::Geometry::ShapeFactory().createShape(obj_str.str());

    ObjCompAssembly bank("BankName");
    Component* det1 = new ObjComponent("Det1Name",s); det1->setPos(V3D(0,-0.1,0));
    Component* det2 = new ObjComponent("Det2Name",s); det2->setPos(V3D(0, 0.1,0));
    Component* det3 = new ObjComponent("Det3Name",s); det3->setPos(V3D(0, 0.3,0));

    bank.add(det1);
    bank.add(det2);
    bank.add(det3);

    boost::shared_ptr<Object> shape = bank.createOutline();
    TS_ASSERT(shape);

    int otype;
    std::vector<V3D> vectors;
    double radius, height;
    shape->GetObjectGeom(otype, vectors, radius, height);

    TS_ASSERT_EQUALS(otype,3);
    TS_ASSERT_EQUALS(radius,0.1);
    TS_ASSERT_EQUALS(height,0.6);

    ParameterMap_sptr pmap( new ParameterMap() );
    boost::shared_ptr<ObjCompAssembly> pcomp(new ObjCompAssembly(&bank,pmap));
    boost::shared_ptr<Component> ic = boost::dynamic_pointer_cast<Component>(pcomp);
    boost::shared_ptr<ICompAssembly> ica = boost::dynamic_pointer_cast<ICompAssembly>(ic);
  }

};

#endif
