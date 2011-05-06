#ifndef MANTID_TESTCOMPONENT__
#define MANTID_TESTCOMPONENT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <string>
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Geometry;

class ComponentTest : public CxxTest::TestSuite
{
public:
  void testEmptyConstructor()
  {
    Component q;
    TS_ASSERT_EQUALS(q.getName(),"");
    TS_ASSERT(!q.getParent());
    TS_ASSERT_EQUALS(q.getRelativePos(),V3D(0,0,0));
    TS_ASSERT_EQUALS(q.getRelativeRot(),Quat(1,0,0,0));
    //as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(),q.getPos());
  }

  void testNameValueConstructor()
  {
    Component q("Name");
    TS_ASSERT_EQUALS(q.getName(),"Name");
    TS_ASSERT(!q.getParent());
    TS_ASSERT_EQUALS(q.getPos(),V3D(0,0,0));
    TS_ASSERT_EQUALS(q.getRelativeRot(),Quat(1,0,0,0));
    //as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(),q.getPos());
  }

  void testNameParentValueConstructor()
  {
    Component parent("Parent");
    //name and parent
    Component q("Child",&parent);
    TS_ASSERT_EQUALS(q.getName(),"Child");
    //check the parent
    TS_ASSERT(q.getParent());
    TS_ASSERT_EQUALS(q.getParent()->getName(),parent.getName());

    TS_ASSERT_EQUALS(q.getPos(),V3D(0,0,0));
    TS_ASSERT_EQUALS(q.getRelativeRot(),Quat(1,0,0,0));
    //as the parent is at 0,0,0 GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(),q.getPos());
  }

  void testNameLocationParentValueConstructor()
  {
    Component parent("Parent",V3D(1,1,1));
    //name and parent
    Component q("Child",V3D(5,6,7),&parent);
    TS_ASSERT_EQUALS(q.getName(),"Child");
    //check the parent
    TS_ASSERT(q.getParent());
    TS_ASSERT_EQUALS(q.getParent()->getName(),parent.getName());

    TS_ASSERT_EQUALS(q.getRelativePos(),V3D(5,6,7));
    TS_ASSERT_EQUALS(q.getPos(),V3D(6,7,8));
    TS_ASSERT_EQUALS(q.getRelativeRot(),Quat(1,0,0,0));
  }

  void testNameLocationOrientationParentValueConstructor()
  {
    Component parent("Parent",V3D(1,1,1));
    //name and parent
    Component q("Child",V3D(5,6,7),Quat(1,1,1,1),&parent);
    TS_ASSERT_EQUALS(q.getName(),"Child");
    //check the parent
    TS_ASSERT(q.getParent());
    TS_ASSERT_EQUALS(q.getParent()->getName(),parent.getName());

    TS_ASSERT_EQUALS(q.getRelativePos(),V3D(5,6,7));
    TS_ASSERT_EQUALS(q.getPos(),V3D(6,7,8));
    TS_ASSERT_EQUALS(q.getRelativeRot(),Quat(1,1,1,1));
  }

  void testCopyConstructor()
  {
    Component parent("Parent",V3D(1,1,1));
    //name and parent
    Component q("Child",V3D(5,6,7),Quat(1,1,1,1),&parent);
    Component copy = q;
    TS_ASSERT_EQUALS(q.getName(),copy.getName());
    TS_ASSERT_EQUALS(q.getParent()->getName(),copy.getParent()->getName());
    TS_ASSERT_EQUALS(q.getRelativePos(),copy.getRelativePos());
    TS_ASSERT_EQUALS(q.getPos(),copy.getPos());
    TS_ASSERT_EQUALS(q.getRelativeRot(),copy.getRelativeRot());
  }

  void testCloneConstructor()
  {
    Component parent("Parent",V3D(1,1,1));
    //name and parent
    Component q("Child",V3D(5,6,7),Quat(1,1,1,1),&parent);
    IComponent *copy = q.clone();
    TS_ASSERT_EQUALS(q.getName(),copy->getName());
    TS_ASSERT_EQUALS(q.getParent()->getName(),copy->getParent()->getName());
    TS_ASSERT_EQUALS(q.getRelativePos(),copy->getRelativePos());
    TS_ASSERT_EQUALS(q.getPos(),copy->getPos());
    TS_ASSERT_EQUALS(q.getRelativeRot(),copy->getRelativeRot());
    delete copy;
  }

  void testGetParent()
  {
    Component parent("Parent",V3D(1,1,1),Quat(1,1,1,1));

    Component q("Child",V3D(5,6,7),&parent);

    TS_ASSERT(q.getParent());
    TS_ASSERT_EQUALS(q.getParent()->getName(),parent.getName());
    TS_ASSERT_EQUALS(q.getParent()->getPos(),V3D(1,1,1));
    TS_ASSERT_EQUALS(q.getParent()->getRelativeRot(),Quat(1,1,1,1));
  }

  void test_isParentNamed()
  {
    Component grandParent("GrandParent",V3D(1,1,1),Quat(1,1,1,1));
    Component parent("Parent",V3D(1,1,1),Quat(1,1,1,1), &grandParent);
    Component q("Child",V3D(5,6,7),&parent);

    TS_ASSERT(q.isParentNamed("Parent"));
    TS_ASSERT(q.isParentNamed("GrandParent"));
    TS_ASSERT(!q.isParentNamed("GrandParent", 1)); // not deep enough
    TS_ASSERT(q.isParentNamed("GrandParent", 2)); // that reaches it
    TS_ASSERT(!q.isParentNamed("DeadbeatDad"));
    TS_ASSERT(!q.isParentNamed("Child"));

  }

  void testGetAncestors()
  {
    Component parent("Parent",V3D(1,1,1),Quat(1,1,1,1));
    Component q("Child",V3D(5,6,7),&parent);

    std::vector<boost::shared_ptr<const IComponent> > ancs = q.getAncestors();
    TS_ASSERT(ancs.size() == 1);
    TS_ASSERT_EQUALS(ancs[0]->getName(), parent.getName());
  }

  void testGetAncestors_Parametrized()
  {
    Component parent("Parent",V3D(1,1,1));
    //name and parent
    Component q("Child",V3D(5,6,7),Quat(1,1,1,1),&parent);
    ParameterMap_const_sptr pmap( new ParameterMap() );
    Component pq(&q,pmap.get());

    TS_ASSERT_EQUALS(pq.getName(),"Child");
    TS_ASSERT(pq.isParametrized() );
    //check the parent
    TS_ASSERT(pq.getParent());
    TS_ASSERT(pq.getParent()->isParametrized() );

    std::vector<boost::shared_ptr<const IComponent> > ancs = pq.getAncestors();
    TS_ASSERT(ancs.size() == 1);
    TS_ASSERT_EQUALS(ancs[0]->getName(), parent.getName());
    TS_ASSERT(ancs[0]->isParametrized());
  }

  void testSetParent()
  {
    Component parent("Parent",V3D(1,1,1));
    Component parent2("Parent2",V3D(10,10,10));

    Component q("Child",V3D(5,6,7),Quat(1,0,0,0),&parent);

    TS_ASSERT_EQUALS(q.getParent()->getName(),parent.getName());
    TS_ASSERT_EQUALS(q.getPos(),V3D(6,7,8));
    q.setParent(&parent2);
    TS_ASSERT_DIFFERS(q.getParent()->getName(),parent.getName());
    TS_ASSERT_EQUALS(q.getParent()->getName(),parent2.getName());
    //check that the absolute pos has moved
    TS_ASSERT_EQUALS(q.getPos(),V3D(15,16,17));
  }

  void testSetName()
  {
    Component q("fred");
    TS_ASSERT_EQUALS(q.getName(),"fred");
    q.setName("bertie");
    TS_ASSERT_EQUALS(q.getName(),"bertie");
  }

  void testSetPos()
  {
    V3D pos1(0,0,0);
    V3D pos2(5,6,7);
    V3D pos3(-999999,999999,999999);
    V3D pos4(0.31,-0.000000000000000001,999999999999.8);
    Component q("testSetPos",pos1);
    TS_ASSERT_EQUALS(q.getPos(),pos1);
    q.setPos(pos2);
    TS_ASSERT_EQUALS(q.getPos(),pos2);
    q.setPos(pos3);
    TS_ASSERT_EQUALS(q.getPos(),pos3);
    q.setPos(pos4.X(),pos4.Y(),pos4.Z());
    TS_ASSERT_EQUALS(q.getPos(),pos4);
  }

  void testSetRot()
  {
    Quat rot1(1,0,0,0);
    Quat rot2(-1,0.01,-0.01,9999);
    Quat rot3(-999999,999999,999999,-9999999);
    Component q("testSetRot",V3D(1,1,1),rot1);
    TS_ASSERT_EQUALS(q.getRelativeRot(),rot1);
    q.setRot(rot2);
    TS_ASSERT_EQUALS(q.getRelativeRot(),rot2);
    q.setRot(rot3);
    TS_ASSERT_EQUALS(q.getRelativeRot(),rot3);
  }


  void testTranslate()
  {
    V3D pos1(1,1,1);
    V3D translate1(5,6,7);
    V3D pos2(6,7,8);
    V3D translate2(-16,-17,-18);
    V3D pos3(-10,-10,-10);

    Component q("testTranslate",pos1);
    TS_ASSERT_EQUALS(q.getPos(),pos1);
    q.translate(translate1);
    TS_ASSERT_EQUALS(q.getPos(),pos2);
    q.translate(translate2.X(),translate2.Y(),translate2.Z());
    TS_ASSERT_EQUALS(q.getPos(),pos3);
  }

  void testRelativeTranslate()
  {
    V3D parentPos(100,100,100);
    V3D pos1(1,1,1);
    V3D translate1(5,6,7);
    V3D pos2(6,7,8);
    V3D translate2(-16,-17,-18);
    V3D pos3(-10,-10,-10);

    Component parent("testTranslate",parentPos);
    Component child("testTranslate",pos1,&parent);
    TS_ASSERT_EQUALS(child.getPos(),pos1+parentPos);
    TS_ASSERT_EQUALS(child.getRelativePos(),pos1);
    child.translate(translate1);
    TS_ASSERT_EQUALS(child.getPos(),pos2+parentPos);
    TS_ASSERT_EQUALS(child.getRelativePos(),pos2);
    child.translate(translate2.X(),translate2.Y(),translate2.Z());
    TS_ASSERT_EQUALS(child.getPos(),pos3+parentPos);
    TS_ASSERT_EQUALS(child.getRelativePos(),pos3);
  }

  void testRotate()
  {
    Quat rot1(1,1,1,1);
    Quat rot2(-1,2,1,3);
    Component comp("testSetRot",V3D(1,1,1),rot1);
    TS_ASSERT_EQUALS(comp.getRelativeRot(),rot1);
    comp.rotate(rot2);
    Quat rot12 = rot1*rot2;
    TS_ASSERT_EQUALS(comp.getRelativeRot(), rot12);

    // Rotate by angle+axis not implemented yet. Check for exception throw
    TS_ASSERT_THROWS(comp.rotate(45,V3D(1,1,1)), Mantid::Kernel::Exception::NotImplementedError );
  }

  void testRelativeRotate()
  {
    Quat rot1(1,1,1,1);
    Quat rot2(-1,2,1,3);
    Quat parentRot(90.0,V3D(0,0,1));
    Component comp("testSetRot",V3D(1,1,1),rot1);
    TS_ASSERT_EQUALS(comp.getRelativeRot(),rot1);
    comp.rotate(rot2);
    TS_ASSERT_EQUALS(comp.getRelativeRot(),rot1*rot2);
    //Get the location of the component
    V3D beforeParentPos = comp.getPos();
    //assign a parent
    Component parent("parent",V3D(0,0,0),parentRot);
    comp.setParent(&parent);
    //check relative values have not moved
    TS_ASSERT_EQUALS(comp.getRelativeRot(),rot1*rot2);
    TS_ASSERT_EQUALS(comp.getRelativePos(),beforeParentPos);
    //but the absolute pos should have changed due to the parent's rotation (the parent is centered on the origin)
    TS_ASSERT_DIFFERS(comp.getPos(),beforeParentPos);
    TS_ASSERT_EQUALS(comp.getPos(),V3D(-1,1,1));
  }

  void testRotation()
  {
    // Very simple test here, not sure my brain can handle anything more complicated!
    Quat rot1(45,V3D(1,0,0));
    Quat rot2(45,V3D(1,0,0));
    Component parent("c1",V3D(2,0,0),rot1);
    Component child("c2",V3D(1,0,0),rot2,&parent);
    TS_ASSERT_EQUALS( child.getRotation(), Quat(90,V3D(1,0,0)) );
    // Check we get back just the child's rotation if we set the parent's to none
    parent.setRot(Quat(1,0,0,0));
    TS_ASSERT_EQUALS( child.getRotation(), rot2 );
    TS_ASSERT_EQUALS( child.getRotation(), child.getRelativeRot() );
    // Or indeed remove the parent completely
    child.setParent(NULL);
    TS_ASSERT_EQUALS( child.getRotation(), rot2 );
    TS_ASSERT_EQUALS( child.getRotation(), child.getRelativeRot() );
  }

  void testGetDistance()
  {
    V3D origin(0,0,0);
    V3D pos1(10,0,0);
    V3D pos2(0,-10,0);
    V3D pos3(0,3,4);
    V3D pos4(-10,-10,-10);

    Component compOrigin("origin",origin);
    Component comp1("comp1",pos1);
    Component comp2("comp2",pos2);
    Component comp3("comp3",pos3);
    Component comp4("comp4",pos4);
    TS_ASSERT_EQUALS(compOrigin.getDistance(comp1),10);
    TS_ASSERT_EQUALS(compOrigin.getDistance(comp2),10);
    TS_ASSERT_EQUALS(compOrigin.getDistance(comp3),5);
    TS_ASSERT_DELTA(compOrigin.getDistance(comp4), 17.3205, 0.001);
    TS_ASSERT_DELTA(comp1.getDistance(comp2), 14.1421, 0.001);
  }

  void testType()
  {
    Component comp;
    TS_ASSERT_EQUALS(comp.type(),"LogicalComponent");
  }

};

#endif
