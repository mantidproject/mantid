// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TESTCompAssembly__
#define MANTID_TESTCompAssembly__

#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <string>

using namespace Mantid::Geometry;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

class CompAssemblyTest : public CxxTest::TestSuite {
public:
  void testEmptyConstructor() {
    CompAssembly q;
    TS_ASSERT_EQUALS(q.nelements(), 0);
    TS_ASSERT_THROWS(q[0], const std::runtime_error &);

    TS_ASSERT_EQUALS(q.getName(), "");
    TS_ASSERT(!q.getParent());
    TS_ASSERT_EQUALS(q.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(q.getRelativeRot(), Quat(1, 0, 0, 0));
    // as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(), q.getPos());
  }

  void testNameValueConstructor() {
    CompAssembly q("Name");
    TS_ASSERT_EQUALS(q.nelements(), 0);
    TS_ASSERT_THROWS(q[0], const std::runtime_error &);

    TS_ASSERT_EQUALS(q.getName(), "Name");
    TS_ASSERT(!q.getParent());
    TS_ASSERT_EQUALS(q.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(q.getRelativeRot(), Quat(1, 0, 0, 0));
    // as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(), q.getPos());
  }

  void testNameParentValueConstructor() {
    CompAssembly *parent = new CompAssembly("Parent");
    // name and parent
    CompAssembly *q = new CompAssembly("Child", parent);
    TS_ASSERT_EQUALS(q->getName(), "Child");
    TS_ASSERT_EQUALS(q->nelements(), 0);
    TS_ASSERT_THROWS((*q)[0], const std::runtime_error &);
    // check the parent
    TS_ASSERT(q->getParent());
    TS_ASSERT_EQUALS(q->getParent()->getName(), parent->getName());

    TS_ASSERT_EQUALS(q->getPos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(q->getRelativeRot(), Quat(1, 0, 0, 0));
    // as the parent is at 0,0,0 GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q->getRelativePos(), q->getPos());
    delete parent;
  }

  void testAdd() {
    CompAssembly bank("BankName");
    Component *det1 = new Component("Det1Name");
    Component *det2 = new Component("Det2Name");
    Component *det3 = new Component("Det3Name");
    TS_ASSERT_EQUALS(bank.nelements(), 0);
    TS_ASSERT_THROWS(bank[0], const std::runtime_error &);
    bank.add(det1);
    bank.add(det2);
    bank.add(det3);
    TS_ASSERT_EQUALS(bank.nelements(), 3);
    boost::shared_ptr<IComponent> det1copy;
    TS_ASSERT_THROWS_NOTHING(det1copy = bank[0]);
    TS_ASSERT_EQUALS(det1->getName(), det1copy->getName());
    // show that they are the same object
    det1->setName("ChangedName");
    TS_ASSERT_EQUALS(det1->getName(), det1copy->getName());
  }

  void testRemove() {
    CompAssembly bank("BankName");
    Component *det1 = new Component("Det1Name");
    bank.add(det1);
    TS_ASSERT_EQUALS(bank.nelements(), 1);
    // Throws if you try to remove a component that's not in there
    Component *det2 = new Component("Det2Name");
    TS_ASSERT_THROWS(bank.remove(det2), const std::runtime_error &);
    delete det2;
    // Works on components that are in the assembly
    TS_ASSERT_EQUALS(bank.remove(det1), 0);
    TS_ASSERT_THROWS(bank[0], const std::runtime_error &);
  }

  //-----------------------------------------------------------------------------
  ICompAssembly_sptr setUpGetChildren() {
    ICompAssembly_sptr bank(new CompAssembly("BankName"));
    for (int i = 0; i < 3; i++) {
      Component *det1 = new Component("Det1Name");
      bank->add(det1);
    }
    CompAssembly *childbank = new CompAssembly("ChildBank");
    for (int i = 0; i < 5; i++) {
      Component *det1 = new Component("ChildDet1Name");
      childbank->add(det1);
    }
    bank->add(childbank);
    return bank;
  }

  void test_GetChildren_NonRecursive() {
    ICompAssembly_const_sptr bank = setUpGetChildren();
    TS_ASSERT_EQUALS(bank->nelements(), 4);
    std::vector<IComponent_const_sptr> kids;
    bank->getChildren(kids, false);
    TS_ASSERT_EQUALS(kids.size(), 4);
    TS_ASSERT_EQUALS(kids[0]->getName(), "Det1Name");
  }

  void test_GetChildren_Recursive() {
    ICompAssembly_const_sptr bank = setUpGetChildren();
    TS_ASSERT_EQUALS(bank->nelements(), 4);
    std::vector<IComponent_const_sptr> kids;
    bank->getChildren(kids, true);
    TS_ASSERT_EQUALS(kids.size(), 9);
    TS_ASSERT_EQUALS(kids[0]->getName(), "Det1Name");
    TS_ASSERT_EQUALS(kids[8]->getName(), "ChildDet1Name");
  }

  void testGetComponentByName() {
    // put together a bare "deep" instrument
    Instrument_sptr instr(new Instrument("deep_instrument"));
    CompAssembly *group1 = new CompAssembly("group1");
    for (int colnum = 1; colnum <= 5; ++colnum) {
      std::ostringstream colname;
      colname << "column" << colnum;
      CompAssembly *column = new CompAssembly(colname.str());
      for (int banknum = 5 * (colnum - 1) + 1; banknum <= 5 * (colnum);
           ++banknum) {
        std::ostringstream bankname;
        bankname << "bank" << banknum;
        CompAssembly *bank = new CompAssembly(bankname.str());
        column->add(bank);
      }
      group1->add(column);
    }
    instr->add(group1);

    // variable to hold the name of the bank
    std::string bankname;

    // look for each bank - recursing down three levels
    for (int i = 1; i < 26; ++i) {
      std::ostringstream temp_oss;
      temp_oss << "bank" << i;
      bankname = temp_oss.str();
      auto temp = instr->getComponentByName(bankname, 3);
      TS_ASSERT(bool(temp));
      TS_ASSERT_EQUALS(temp->getName(), bankname);
    }

    // look for bank13 - recursing all the way down the instrument tree
    bankname = std::string("bank13");
    auto temp = instr->getComponentByName(bankname);
    TS_ASSERT(bool(temp));
    TS_ASSERT_EQUALS(temp->getName(), bankname);

    // look for bank13 again - recursing just one level (should fail)
    temp = instr->getComponentByName(bankname, 1);
    TS_ASSERT(!bool(temp));
  }

  //-----------------------------------------------------------------------------
  void testAddCopy() {
    CompAssembly bank("BankName");
    Component det1("Det1Name");
    Component det2("Det2Name");
    Component det3("Det3Name");
    TS_ASSERT_EQUALS(bank.nelements(), 0);
    TS_ASSERT_THROWS(bank[0], const std::runtime_error &);
    bank.addCopy(&det1);
    bank.addCopy(&det2);
    bank.addCopy(&det3, "ChangedDet3Name");
    TS_ASSERT_EQUALS(bank.nelements(), 3);
    boost::shared_ptr<IComponent> detcopy;
    TS_ASSERT_THROWS_NOTHING(detcopy = bank[0]);
    TS_ASSERT_EQUALS(det1.getName(), detcopy->getName());
    // show that they are NOT the same object
    det1.setName("ChangedName");
    TS_ASSERT_DIFFERS(det1.getName(), detcopy->getName());

    // check out the in process rename made to det3 on input
    TS_ASSERT_THROWS_NOTHING(detcopy = bank[0]);
    TS_ASSERT_DIFFERS(det1.getName(), detcopy->getName());
  }

  void testCopyConstructor() {
    Component *parent = new Component("Parent", V3D(1, 1, 1));
    // name and parent
    CompAssembly q("Child", parent);
    q.setPos(V3D(5, 6, 7));
    q.setRot(Quat(1, 1, 1, 1));
    Component gc1("Grandchild1");
    q.addCopy(&gc1);
    Component *gc2 = new Component("Grandchild2");
    q.add(gc2);
    Component gc3("Grandchild3");
    q.addCopy(&gc3);
    TS_ASSERT_EQUALS(q.nelements(), 3);
    CompAssembly copy = q;
    TS_ASSERT_EQUALS(q.getName(), copy.getName());
    TS_ASSERT_EQUALS(q.getParent()->getName(), copy.getParent()->getName());
    TS_ASSERT_EQUALS(q.nelements(), copy.nelements());
    TS_ASSERT_EQUALS(q[0]->getName(), copy[0]->getName());
    TS_ASSERT_EQUALS(q[2]->getName(), copy[2]->getName());
    TS_ASSERT_EQUALS(q.getRelativePos(), copy.getRelativePos());
    TS_ASSERT_EQUALS(q.getPos(), copy.getPos());
    TS_ASSERT_EQUALS(q.getRelativeRot(), copy.getRelativeRot());
    delete parent;
  }

  void testClone() {
    Component *parent = new Component("Parent", V3D(1, 1, 1));
    // name and parent
    CompAssembly q("Child", parent);
    q.setPos(V3D(5, 6, 7));
    q.setRot(Quat(1, 1, 1, 1));
    Component gc1("Grandchild1");
    q.addCopy(&gc1);
    Component *gc2 = new Component("Grandchild2");
    q.add(gc2);
    Component gc3("Grandchild3");
    q.addCopy(&gc3);
    TS_ASSERT_EQUALS(q.nelements(), 3);
    IComponent *copyAsComponent = q.clone();
    CompAssembly *copy = dynamic_cast<CompAssembly *>(copyAsComponent);
    TS_ASSERT_EQUALS(q.getName(), copy->getName());
    TS_ASSERT_EQUALS(q.getParent()->getName(), copy->getParent()->getName());
    TS_ASSERT_EQUALS(q.nelements(), copy->nelements());
    TS_ASSERT_EQUALS(q[0]->getName(), (*copy)[0]->getName());
    TS_ASSERT_EQUALS(q[2]->getName(), (*copy)[2]->getName());
    TS_ASSERT_EQUALS(q.getRelativePos(), copy->getRelativePos());
    TS_ASSERT_EQUALS(q.getPos(), copy->getPos());
    TS_ASSERT_EQUALS(q.getRelativeRot(), copy->getRelativeRot());
    delete copyAsComponent;
    delete parent;
  }

  void testGetParent() {
    Component parent("Parent", V3D(1, 1, 1), Quat(1, 1, 1, 1));

    CompAssembly q("Child", &parent);

    TS_ASSERT(q.getParent());
    TS_ASSERT_EQUALS(q.getParent()->getName(), parent.getName());
    TS_ASSERT_EQUALS(q.getParent()->getPos(), V3D(1, 1, 1));
    TS_ASSERT_EQUALS(q.getParent()->getRelativeRot(), Quat(1, 1, 1, 1));
  }

  void testSetParent() {
    Component parent("Parent", V3D(1, 1, 1));
    Component parent2("Parent2", V3D(10, 10, 10));

    CompAssembly q("Child", &parent);
    q.setPos(V3D(5, 6, 7));
    q.setRot(Quat(1, 0, 0, 0));
    TS_ASSERT_EQUALS(q.getParent()->getName(), parent.getName());
    TS_ASSERT_EQUALS(q.getPos(), V3D(6, 7, 8));
    q.setParent(&parent2);
    TS_ASSERT_DIFFERS(q.getParent()->getName(), parent.getName());
    TS_ASSERT_EQUALS(q.getParent()->getName(), parent2.getName());
    // check that the absolute pos has moved
    TS_ASSERT_EQUALS(q.getPos(), V3D(15, 16, 17));
  }

  void testSetName() {
    CompAssembly q("fred");
    TS_ASSERT_EQUALS(q.getName(), "fred");
    q.setName("bertie");
    TS_ASSERT_EQUALS(q.getName(), "bertie");
  }

  void testSetPos() {
    V3D pos1(0, 0, 0);
    V3D pos2(5, 6, 7);
    V3D pos3(-999999, 999999, 999999);
    V3D pos4(0.31, -0.000000000000000001, 999999999999.8);
    CompAssembly q("testSetPos");
    q.setPos(pos1);
    TS_ASSERT_EQUALS(q.getPos(), pos1);
    q.setPos(pos2);
    TS_ASSERT_EQUALS(q.getPos(), pos2);
    q.setPos(pos3);
    TS_ASSERT_EQUALS(q.getPos(), pos3);
    q.setPos(pos4.X(), pos4.Y(), pos4.Z());
    TS_ASSERT_EQUALS(q.getPos(), pos4);
  }

  void testSetRot() {
    Quat rot1(1, 0, 0, 0);
    Quat rot2(-1, 0.01, -0.01, 9999);
    Quat rot3(-999999, 999999, 999999, -9999999);
    CompAssembly q("testSetRot");
    q.setPos(V3D(1, 1, 1));
    q.setRot(rot1);
    TS_ASSERT_EQUALS(q.getRelativeRot(), rot1);
    q.setRot(rot2);
    TS_ASSERT_EQUALS(q.getRelativeRot(), rot2);
    q.setRot(rot3);
    TS_ASSERT_EQUALS(q.getRelativeRot(), rot3);
  }

  //  void testCopyRot() //copyRot unused anywhere in code
  //  {
  //    Quat rot1(1, 0, 0, 0);
  //    Quat rot2(-1, 0.01, -0.01, 9999);
  //    CompAssembly p("testSetRot");
  //    p.setPos(V3D(1, 1, 1));
  //    p.setRot(rot1);
  //    CompAssembly q("testCopyRot2");
  //    q.setPos(V3D(2, 2, 2));
  //    q.setRot(rot2);
  //    TS_ASSERT_EQUALS(p.getRelativeRot(), rot1);
  //    TS_ASSERT_EQUALS(q.getRelativeRot(), rot2);
  //    q.copyRot(p);
  //    TS_ASSERT_EQUALS(p.getRelativeRot(), rot1);
  //    TS_ASSERT_EQUALS(q.getRelativeRot(), rot1);
  //    //check it just copied the rotation and not everything else
  //    TS_ASSERT_EQUALS(q.getPos(), V3D(2, 2, 2));
  //    TS_ASSERT_EQUALS(q.getName(), "testCopyRot2");
  //  }

  void testTranslate() {
    V3D pos1(1, 1, 1);
    V3D translate1(5, 6, 7);
    V3D pos2(6, 7, 8);
    V3D translate2(-16, -17, -18);
    V3D pos3(-10, -10, -10);

    CompAssembly q("testTranslate");
    q.setPos(pos1);
    TS_ASSERT_EQUALS(q.getPos(), pos1);
    q.translate(translate1);
    TS_ASSERT_EQUALS(q.getPos(), pos2);
    q.translate(translate2.X(), translate2.Y(), translate2.Z());
    TS_ASSERT_EQUALS(q.getPos(), pos3);
  }

  void testRelativeTranslate() {
    V3D parentPos(100, 100, 100);
    V3D pos1(1, 1, 1);
    V3D translate1(5, 6, 7);
    V3D pos2(6, 7, 8);
    V3D translate2(-16, -17, -18);
    V3D pos3(-10, -10, -10);

    CompAssembly *parent = new CompAssembly("testTranslate");
    parent->setPos(parentPos);
    CompAssembly *child = new CompAssembly("testTranslate", parent);
    child->setPos(pos1);
    TS_ASSERT_EQUALS(child->getPos(), pos1 + parentPos);
    TS_ASSERT_EQUALS(child->getRelativePos(), pos1);
    child->translate(translate1);
    TS_ASSERT_EQUALS(child->getPos(), pos2 + parentPos);
    TS_ASSERT_EQUALS(child->getRelativePos(), pos2);
    child->translate(translate2.X(), translate2.Y(), translate2.Z());
    TS_ASSERT_EQUALS(child->getPos(), pos3 + parentPos);
    TS_ASSERT_EQUALS(child->getRelativePos(), pos3);
    delete parent;
  }

  void testRotate() {
    Quat rot1(1, 1, 1, 1);
    Quat rot2(-1, 2, 1, 3);
    CompAssembly comp("testSetRot");
    comp.setPos(V3D(1, 1, 1));
    comp.setRot(rot1);
    TS_ASSERT_EQUALS(comp.getRelativeRot(), rot1);
    comp.rotate(rot2);
    TS_ASSERT_EQUALS(comp.getRelativeRot(), rot1 * rot2);
  }

  void testRelativeRotate() {
    Quat rot1(1, 1, 1, 1);
    Quat rot2(-1, 2, 1, 3);
    Quat parentRot(90.0, V3D(0, 0, 1));
    CompAssembly comp("testSetRot");
    comp.setPos(V3D(1, 1, 1));
    comp.setRot(rot1);
    TS_ASSERT_EQUALS(comp.getRelativeRot(), rot1);
    comp.rotate(rot2);
    TS_ASSERT_EQUALS(comp.getRelativeRot(), rot1 * rot2);
    // Get the location of the CompAssembly
    V3D beforeParentPos = comp.getPos();
    // assign a parent
    Component parent("parent", V3D(0, 0, 0), parentRot);
    comp.setParent(&parent);
    // check relative values have not moved
    TS_ASSERT_EQUALS(comp.getRelativeRot(), rot1 * rot2);
    TS_ASSERT_EQUALS(comp.getRelativePos(), beforeParentPos);
    // but the absolute pos should have changed due to the parents roatation
    // (the parent is centered on the origin)
    TS_ASSERT_DIFFERS(comp.getPos(), beforeParentPos);
    TS_ASSERT_EQUALS(comp.getPos(), V3D(-1, 1, 1));
  }

  void testGetDistance() {
    V3D origin(0, 0, 0);
    V3D pos1(10, 0, 0);
    V3D pos2(0, -10, 0);
    V3D pos3(0, 3, 4);
    V3D pos4(-10, -10, -10);

    CompAssembly compOrigin("origin");
    compOrigin.setPos(origin);
    CompAssembly comp1("comp1");
    comp1.setPos(pos1);
    Component comp2("comp2", pos2);
    Component comp3("comp3", pos3);
    Component comp4("comp4", pos4);
    TS_ASSERT_EQUALS(compOrigin.getDistance(comp1), 10);
    TS_ASSERT_EQUALS(compOrigin.getDistance(comp2), 10);
    TS_ASSERT_EQUALS(compOrigin.getDistance(comp3), 5);
    TS_ASSERT_DELTA(compOrigin.getDistance(comp4), 17.3205, 0.001);
    TS_ASSERT_DELTA(comp1.getDistance(comp2), 14.1421, 0.001);
  }

  void testType() {
    CompAssembly comp;
    TS_ASSERT_EQUALS(comp.type(), "CompAssembly");
  }

  void
  test_That_The_Bounding_Box_Is_The_Correct_Size_For_All_Of_The_Constituents() {
    boost::shared_ptr<CompAssembly> bank =
        ComponentCreationHelper::createTestAssemblyOfFourCylinders();
    TS_ASSERT(bank);
    TS_ASSERT_EQUALS(bank->nelements(), 4);

    BoundingBox bbox;
    bank->getBoundingBox(bbox);
    TS_ASSERT_DELTA(bbox.xMin(), 0.5, 1e-08);
    TS_ASSERT_DELTA(bbox.xMax(), 4.5, 1e-08);
    TS_ASSERT_DELTA(bbox.yMin(), 0.0, 1e-08);
    TS_ASSERT_DELTA(bbox.yMax(), 1.5, 1e-08);
    TS_ASSERT_DELTA(bbox.zMin(), -0.5, 1e-08);
    TS_ASSERT_DELTA(bbox.zMax(), 0.5, 1e-08);
  }

  void testMultipleCompAssemblyRelativeTranslate() {
    V3D instInitialPos(0, 0, 0);
    V3D parentInitialPos(100, 100, 100);
    V3D pos2(6, 7, 8);
    V3D pos3(-10, -10, -10);
    V3D translate1(5, 6, 7);
    V3D translate2(-16, -17, -18);
    V3D translate3(2, 10, 3);
    V3D translate4(-1, -100, -8);

    CompAssembly *inst = new CompAssembly("Instrument");
    inst->setPos(instInitialPos);
    CompAssembly *parent = new CompAssembly("parent", inst);
    parent->setPos(parentInitialPos);
    CompAssembly *child = new CompAssembly("child", parent);
    child->setPos(pos2);
    Component *det = new Component("det1");
    det->setPos(pos3);
    child->add(det);

    V3D instPos = inst->getPos();
    V3D parentPos = parent->getPos();
    V3D childPos = child->getPos();
    V3D detPos = det->getPos();

    inst->translate(translate1);
    parent->translate(translate2);
    child->translate(translate3);
    det->translate(translate4);
    TS_ASSERT_EQUALS(inst->getPos(), translate1 + instPos);
    TS_ASSERT_EQUALS(parent->getPos(), translate1 + translate2 + parentPos);
    TS_ASSERT_EQUALS(child->getPos(),
                     translate1 + translate2 + translate3 + childPos);
    TS_ASSERT_EQUALS(det->getPos(), translate1 + translate2 + translate3 +
                                        translate4 + detPos);
    delete inst;
  }

  void test_get_component_by_name_with_rect_detectors() {

    CompAssembly *inst = new CompAssembly("inst");
    CompAssembly *monitors = new CompAssembly("monitors", inst);
    for (size_t i = 0; i < 5; i++) {
      std::ostringstream sstr;
      sstr << "monitor " << i;
      Component *monitor = new Component(sstr.str());
      monitors->add(monitor);
    }
    CompAssembly *detectors = new CompAssembly("detectors", inst);
    CompAssembly *bank1 = new CompAssembly("bank 1", detectors);
    CompAssembly *bank2 = new CompAssembly("bank 11", detectors);
    CompAssembly *bank3 = new CompAssembly("bank 111", detectors);

    // add some rectangular detectors
    boost::shared_ptr<IObject> cuboidShape =
        ComponentCreationHelper::createCuboid(0.5);

    for (size_t i = 0; i < 15; i++) {
      std::ostringstream sstr;
      sstr << "Rectangle bank " << i;
      RectangularDetector *det = new RectangularDetector(sstr.str(), detectors);

      // Initialize with these parameters
      det->initialize(cuboidShape, 100, -50.0, 1.0, 200, -100.0, 1.0, 1000000,
                      true, 1000);
    }

    // and a couple more assemblies
    CompAssembly *bank4 = new CompAssembly("bank 12", detectors);
    CompAssembly *bank5 = new CompAssembly("bank 121", detectors);

    TS_ASSERT_EQUALS(inst->getComponentByName("bank 1")->getFullName(),
                     bank1->getFullName());
    TS_ASSERT_EQUALS(
        inst->getComponentByName("inst/detectors/bank 1")->getFullName(),
        bank1->getFullName());
    TS_ASSERT_EQUALS(inst->getComponentByName("monitor 2")->getFullName(),
                     "inst/monitors/monitor 2");
    TS_ASSERT_EQUALS(
        inst->getComponentByName("Rectangle bank 2")->getFullName(),
        "inst/detectors/Rectangle bank 2");
    TS_ASSERT_EQUALS(inst->getComponentByName("bank 12")->getFullName(),
                     bank4->getFullName());
    TS_ASSERT_EQUALS(inst->getComponentByName("bank 121")->getFullName(),
                     bank5->getFullName());
    TS_ASSERT_EQUALS(inst->getComponentByName("bank 11")->getFullName(),
                     bank2->getFullName());
    TS_ASSERT_EQUALS(inst->getComponentByName("bank 111")->getFullName(),
                     bank3->getFullName());
    TS_ASSERT_EQUALS(
        inst->getComponentByName("Rectangle bank 1(1,1)")->getFullName(),
        "inst/detectors/Rectangle bank 1/Rectangle bank 1(x=1)/Rectangle bank "
        "1(1,1)");
    TS_ASSERT_EQUALS(
        inst->getComponentByName("Rectangle bank 11(1,1)")->getFullName(),
        "inst/detectors/Rectangle bank 11/Rectangle bank 11(x=1)/Rectangle "
        "bank 11(1,1)");
    TS_ASSERT_EQUALS(
        inst->getComponentByName(
                "inst/detectors/Rectangle bank 4/Rectangle bank 4(3,5)")
            ->getFullName(),
        "inst/detectors/Rectangle bank 4/Rectangle bank 4(x=3)/Rectangle bank "
        "4(3,5)");
    TS_ASSERT_EQUALS(
        inst->getComponentByName("Rectangle bank 11")->getFullName(),
        "inst/detectors/Rectangle bank 11");

    delete (inst);
  }
};

#endif
