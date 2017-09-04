#ifndef PYTHONOBJECTTEST_H
#define PYTHONOBJECTTEST_H

#include "cxxtest/TestSuite.h"

#include "MantidQtWidgets/MplCpp/PythonObject.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"

using namespace MantidQt::Widgets::MplCpp;

class PythonObjectTest : public CxxTest::TestSuite {
public:
  static PythonObjectTest *createSuite() { return new PythonObjectTest; }
  static void destroySuite(PythonObjectTest *suite) { delete suite; }

  // --------------------------- Success tests -----------------------------
  void test_Default_Constructor_Gives_None() {
    PythonObject obj;
    TSM_ASSERT("Default object should be None", obj.isNone())
  }

  void test_Construction_With_New_Reference_Does_Not_Alter_Ref_Count() {
    // Use something with heap allocation so we know it's a new object
    auto obj = PythonObject::fromNewRef(PyList_New(1));
    TSM_ASSERT_EQUALS("Reference count should not have changed on"
                      "construction ",
                      1, obj.refCount());
  }

  void test_Copy_Construction_Increases_Ref_Count_By_One() {
    // Use something with heap allocation so we know it's a new object
    auto original = PythonObject::fromNewRef(PyList_New(1));
    PythonObject copy(original);
    TSM_ASSERT_EQUALS("Copy should reference the same object", original, copy);
    TSM_ASSERT_EQUALS("Copied object should have ref count of 2", 2,
                      copy.refCount());
    TSM_ASSERT_EQUALS("Original object should have ref count of 2", 2,
                      original.refCount());
  }

  void test_Copy_Assignment_Increases_Ref_Count_By_One() {
    // Use something with heap allocation so we know it's a new object
    auto original = PythonObject::fromNewRef(PyList_New(1));
    PythonObject copy;
    copy = original;
    TSM_ASSERT_EQUALS("Copy should equal orignal", original, copy);
    TSM_ASSERT_EQUALS("Copied object should have ref count of 2", 2,
                      copy.refCount());
    TSM_ASSERT_EQUALS("Original object should have ref count of 2", 2,
                      original.refCount());
  }

  void test_Move_Construction_Keeps_Ref_Count_The_Same_On_Moved_To_Object() {
    // Use something with heap allocation so we know it's a new object
    auto original = PythonObject::fromNewRef(PyList_New(1));
    PythonObject moved(std::move(original));
    TSM_ASSERT_EQUALS("New object should have same reference count", 1,
                      moved.refCount());
  }

  void test_Move_Assignment_Keeps_Ref_Count_The_Same_On_Moved_To_Object() {
    // Use something with heap allocation so we know it's a new object
    auto original = PythonObject::fromNewRef(PyList_New(1));
    PythonObject moved;
    moved = std::move(original);
    TSM_ASSERT_EQUALS("New object should have same reference count", 1,
                      moved.refCount());
  }

  void test_Equality_Operator() {
    auto original = PythonObject::fromNewRef(PyList_New(1));
    TSM_ASSERT_EQUALS("Objects should equal each other", original, original);
    auto other = PythonObject::fromNewRef(PyList_New(1));
    TSM_ASSERT_DIFFERS(
        "Different underlying objects should not equal each other", original,
        other);
  }

  void test_Known_Attribute_Returns_Expected_Object() {
    auto obj = PythonObject::fromNewRef(PyList_New(1));
    auto attrObj = obj.getAttr("__len__");
    TSM_ASSERT("Attribute object should not be None", !obj.isNone());
  }

  // --------------------------- Failure tests -----------------------------
  void test_Unknown_Attribute_Throws_Exception() {
    auto obj = PythonObject::fromNewRef(PyList_New(1));
    TSM_ASSERT_THROWS("getAttr should throw for non-existant attribute",
                      obj.getAttr("not_a_method"), PythonError);
  }
};

#endif // PYTHONOBJECTTEST_H
