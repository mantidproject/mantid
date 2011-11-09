#ifndef MANTID_KERNEL_ARRAYLENGTHVALIDATORTEST_H_
#define MANTID_KERNEL_ARRAYLENGTHVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <vector>

#include "MantidKernel/ArrayLengthValidator.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class ArrayLengthValidatorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ArrayLengthValidatorTest *createSuite() { return new ArrayLengthValidatorTest(); }
  static void destroySuite( ArrayLengthValidatorTest *suite ) { delete suite; }


  void testConstructor()
  {
    ArrayLengthValidator<int> av1,av2(3);
    TS_ASSERT_EQUALS(av1.hasLength(),false);
    TS_ASSERT_EQUALS(av2.hasLength(),true);
    TS_ASSERT_EQUALS(av2.getLength(),3);
  }

  void testClone()
  {
    ArrayLengthValidator<int> *vi= new ArrayLengthValidator<int>;
    IValidator<std::vector<int> > *vvi = vi->clone();
    TS_ASSERT_DIFFERS( vi, vvi );
    delete vi;
    delete vvi;
  }

  void testSetClear()
  {
    ArrayLengthValidator<int> av1;
    TS_ASSERT_EQUALS(av1.hasLength(),false);
    av1.setLength(4);
    TS_ASSERT_EQUALS(av1.hasLength(),true);
    TS_ASSERT_EQUALS(av1.getLength(),4);
    av1.clearLength();
    TS_ASSERT_EQUALS(av1.hasLength(),false);
    TS_ASSERT_EQUALS(av1.getLength(),0);
  }

  void testValidator()
  {
    ArrayLengthValidator<int> vi(3);
    std::vector<int> a;
    a.push_back(3);
    TS_ASSERT_DIFFERS(vi.isValid(a).length(),0);
    a.push_back(-1);
    a.push_back(11);
    TS_ASSERT_EQUALS(vi.isValid(a).length(),0);
  }
};


#endif /* MANTID_KERNEL_ARRAYLENGTHVALIDATORTEST_H_ */
