#ifndef MANTID_KERNEL_UNITLABELTYPESTEST_H_
#define MANTID_KERNEL_UNITLABELTYPESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/UnitLabelTypes.h"

class UnitLabelTypesTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnitLabelTypesTest *createSuite() { return new UnitLabelTypesTest(); }
  static void destroySuite( UnitLabelTypesTest *suite ) { delete suite; }

  //================= Empty =========================
  void test_Empty()
  {
    Mantid::Kernel::Units::EmptyLabel label;
    TS_ASSERT_EQUALS("", label.ascii());
  }

};


#endif /* MANTID_KERNEL_UNITLABELTYPESTEST_H_ */
