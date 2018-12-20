#ifndef PARAMETERREFERENCETEST_H_
#define PARAMETERREFERENCETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/ParameterReference.h"

using namespace Mantid;
using namespace Mantid::API;

class ParameterReferenceTest_Fun : public ParamFunction, public IFunction1D {
public:
  ParameterReferenceTest_Fun() {
    declareParameter("a");
    declareParameter("b");
    declareParameter("c");
  }
  std::string name() const override { return "ParameterReferenceTest_Fun"; }
  void function1D(double *, const double *, const size_t) const override {}
};

class ParameterReferenceTest : public CxxTest::TestSuite {
public:
  void testSimple() {
    ParameterReferenceTest_Fun f;
    f.setParameter("a", 1);
    f.setParameter("b", 2);
    f.setParameter("c", 3);

    ParameterReference r2b(&f, 1);
    TS_ASSERT_EQUALS(f.getParameterIndex(r2b), 1);
    TS_ASSERT_EQUALS(r2b.getParameter(), 2);
    r2b.setParameter(14);
    TS_ASSERT_EQUALS(f.getParameter("b"), 14);
  }

  void testComposite() {
    CompositeFunction *cf = new CompositeFunction;
    IFunction_sptr f0 = IFunction_sptr(new ParameterReferenceTest_Fun);

    CompositeFunction_sptr f1 = CompositeFunction_sptr(new CompositeFunction);

    IFunction_sptr f1_0 = IFunction_sptr(new ParameterReferenceTest_Fun);
    IFunction_sptr f1_1 = IFunction_sptr(new ParameterReferenceTest_Fun);
    CompositeFunction_sptr f1_2 = CompositeFunction_sptr(new CompositeFunction);
    IFunction_sptr f1_2_0 = IFunction_sptr(new ParameterReferenceTest_Fun);
    IFunction_sptr f1_2_1 = IFunction_sptr(new ParameterReferenceTest_Fun);

    f1_2->addFunction(f1_2_0);
    f1_2->addFunction(f1_2_1);

    f1->addFunction(f1_0);
    f1->addFunction(f1_1);
    f1->addFunction(f1_2);

    cf->addFunction(f0);
    cf->addFunction(f1);

    TS_ASSERT_EQUALS(cf->nParams(), 15);
    TS_ASSERT_EQUALS(cf->parameterName(10), "f1.f2.f0.b");

    ParameterReference r0(cf, 0);
    ParameterReference r1(cf, 1);
    ParameterReference r2(cf, 2);
    ParameterReference r3(cf, 3);
    ParameterReference r4(cf, 4);
    ParameterReference r5(cf, 5);
    ParameterReference r6(cf, 6);
    ParameterReference r7(cf, 7);
    ParameterReference r8(cf, 8);
    ParameterReference r9(cf, 9);
    ParameterReference r10(cf, 10);
    ParameterReference r11(cf, 11);

    ParameterReference r12(f1_2_1.get(), 1);

    TS_ASSERT_EQUALS(cf->getParameterIndex(r12), 13);
    TS_ASSERT_EQUALS(f1->getParameterIndex(r12), 10);
    TS_ASSERT_EQUALS(f1_2->getParameterIndex(r12), 4);
    TS_ASSERT_EQUALS(f1_2_1->getParameterIndex(r12), 1);

    TS_ASSERT_EQUALS(cf->getContainingFunction(r12), f1);
    TS_ASSERT_EQUALS(f1->getContainingFunction(r12), f1_2);
    TS_ASSERT_EQUALS(f1_2->getContainingFunction(r12), f1_2_1);

    TS_ASSERT_EQUALS(r0.getLocalFunction(), f0.get());
    TS_ASSERT_EQUALS(r1.getLocalFunction(), f0.get());
    TS_ASSERT_EQUALS(r2.getLocalFunction(), f0.get());

    TS_ASSERT_EQUALS(r0.getLocalIndex(), 0);
    TS_ASSERT_EQUALS(r1.getLocalIndex(), 1);
    TS_ASSERT_EQUALS(r2.getLocalIndex(), 2);

    TS_ASSERT_EQUALS(r3.getLocalFunction(), f1_0.get());
    TS_ASSERT_EQUALS(r4.getLocalFunction(), f1_0.get());
    TS_ASSERT_EQUALS(r5.getLocalFunction(), f1_0.get());

    TS_ASSERT_EQUALS(r3.getLocalIndex(), 0);
    TS_ASSERT_EQUALS(r4.getLocalIndex(), 1);
    TS_ASSERT_EQUALS(r5.getLocalIndex(), 2);

    TS_ASSERT_EQUALS(r6.getLocalFunction(), f1_1.get());
    TS_ASSERT_EQUALS(r7.getLocalFunction(), f1_1.get());
    TS_ASSERT_EQUALS(r8.getLocalFunction(), f1_1.get());

    TS_ASSERT_EQUALS(r6.getLocalIndex(), 0);
    TS_ASSERT_EQUALS(r7.getLocalIndex(), 1);
    TS_ASSERT_EQUALS(r8.getLocalIndex(), 2);

    TS_ASSERT_EQUALS(r9.getLocalFunction(), f1_2_0.get());
    TS_ASSERT_EQUALS(r10.getLocalFunction(), f1_2_0.get());
    TS_ASSERT_EQUALS(r11.getLocalFunction(), f1_2_0.get());

    TS_ASSERT_EQUALS(r9.getLocalIndex(), 0);
    TS_ASSERT_EQUALS(r10.getLocalIndex(), 1);
    TS_ASSERT_EQUALS(r11.getLocalIndex(), 2);

    delete cf;
  }
};

#endif /*PARAMETERREFERENCETEST_H_*/
