#ifndef MANTID_TESTPARCOMPASSEMBLY_H_
#define MANTID_TESTPARCOMPASSEMBLY_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <string>
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"

using namespace Mantid::Geometry;

class ParCompAssemblyTest : public CxxTest::TestSuite
{
public:
	void testEmptyConstructor()
  {
    CompAssembly q;

    ParameterMap_sptr pmap( new ParameterMap() );
    CompAssembly pq(&q,pmap.get());

    TS_ASSERT_EQUALS(pq.nelements(), 0);
    TS_ASSERT_THROWS(pq[0], std::runtime_error);

    TS_ASSERT_EQUALS(pq.getName(), "");
    TS_ASSERT(!pq.getParent());
    TS_ASSERT_EQUALS(pq.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(pq.getRelativeRot(), Quat(1, 0, 0, 0));
    //as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(pq.getRelativePos(), pq.getPos());
  }

  void testNameValueConstructor()
  {
    CompAssembly q("Name");

    ParameterMap_sptr pmap( new ParameterMap() );
    CompAssembly pq(&q,pmap.get());

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
    CompAssembly* parent = new CompAssembly("Parent");
    //name and parent
    CompAssembly* q = new CompAssembly("Child", parent);

    ParameterMap_sptr pmap( new ParameterMap() );
    CompAssembly pq(q,pmap.get());

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
    CompAssembly bank("BankName");
    Component* det1 = new Component("Det1Name");
    Component* det2 = new Component("Det2Name");
    Component* det3 = new Component("Det3Name");
    TS_ASSERT_EQUALS(bank.nelements(), 0);
    TS_ASSERT_THROWS(bank[0], std::runtime_error);
    bank.add(det1);
    bank.add(det2);
    bank.add(det3);

    ParameterMap_sptr pmap( new ParameterMap() );
    CompAssembly pbank(&bank,pmap.get()); //parametrized one

    TS_ASSERT_EQUALS(pbank.nelements(), 3);
    boost::shared_ptr<IComponent> det1copy;
    det1copy = pbank[0];
    TS_ASSERT(det1copy);
    TS_ASSERT_EQUALS(det1->getName(), det1copy->getName());
    //show that they are the same object
    det1->setName("ChangedName");
    TS_ASSERT_EQUALS(det1->getName(), det1copy->getName());

    pmap->addV3D(det2,"pos",V3D(1,1,1));
    boost::shared_ptr<IComponent> det2copy;
    TS_ASSERT_THROWS_NOTHING(det2copy = pbank[1]);
    TS_ASSERT_DIFFERS(det2->getPos(), det2copy->getPos());
  }

  void testGetParent()
  {
    Component parent("Parent", V3D(1, 1, 1), Quat(1, 1, 1, 1));

    CompAssembly q("Child", &parent);

    ParameterMap_sptr pmap( new ParameterMap() );
    CompAssembly pq(&q,pmap.get());

    TS_ASSERT(pq.getParent());
    TS_ASSERT_EQUALS(pq.getParent()->getName(), parent.getName());
    TS_ASSERT_EQUALS(pq.getParent()->getPos(), V3D(1, 1, 1));
    TS_ASSERT_EQUALS(pq.getParent()->getRelativeRot(), Quat(1, 1, 1, 1));

    //Get the position again - this'll retrieve from the cache
    TS_ASSERT_EQUALS(pq.getParent()->getPos(), V3D(1, 1, 1));
}

  void testType()
  {
    CompAssembly comp;

    ParameterMap_sptr pmap( new ParameterMap() );
    CompAssembly pcomp(&comp,pmap.get());

    TS_ASSERT_EQUALS(pcomp.type(), "CompAssembly");
  }

  //void test_That_The_Bounding_Box_Is_In_The_Correct_Position()
  //{
  //  CompAssembly *bank = createTestParAssembly();
  //}

  //private:

  //  CompAssembly * createTestParAssembly()
  //  {
  //    boost::shared_ptr<CompAssembly> bank = boost::shared_ptr<CompAssembly>(new CompAssembly("Bank"));
  //    m_det1 = boost::shared_ptr<Component>(new Component("Det1Name"));
  //    m_det2 = boost::shared_ptr<Component>(new Component("Det2Name"));
  //    m_det3 = boost::shared_ptr<Component>(new Component("Det3Name"));
  //    bank->add(det1.get());
  //    bank->add(det2.get());
  //    bank->add(det3.get());
  //  }
  //  // Keep references to detectors alive
  //  boost::shared_ptr<Component> m_det1, m_det2, m_det3
};

#endif
