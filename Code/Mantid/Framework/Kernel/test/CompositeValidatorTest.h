#ifndef MANTID_KERNEL_COMPOSITEVALIDATORTEST_H_
#define MANTID_KERNEL_COMPOSITEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class CompositeValidatorTest : public CxxTest::TestSuite
{
public:

  /** Is valid does an AND of the components */
  void test_isValid()
  {
    BoundedValidator<int> * val1 = new BoundedValidator<int>(100, 1000);
    BoundedValidator<int> * val2 = new BoundedValidator<int>(900, 2000);
    CompositeValidator comp;
    comp.add(IValidator_sptr(val1));
    //
    TS_ASSERT(comp.allowedValues().empty());

    TS_ASSERT_EQUALS(comp.isValid(150), "");
    TS_ASSERT_EQUALS(comp.isValid(950), "");
    TS_ASSERT_DIFFERS(comp.isValid(1200), "");
    comp.add(IValidator_sptr(val2));
    TS_ASSERT_DIFFERS(comp.isValid(150), ""); // This one is now blocked by validator 2
    TS_ASSERT_EQUALS(comp.isValid(950), "");
    TS_ASSERT_DIFFERS(comp.isValid(1200), "");
    //
    TS_ASSERT(comp.allowedValues().empty());

    // Test cloning
    IValidator_sptr comp2 = comp.clone();
    TS_ASSERT( !comp2->isValid(150).empty() );
    TS_ASSERT(  comp2->isValid(950).empty() );

    TS_ASSERT( comp2->allowedValues().empty());
  }
  void test_isListObtained()
  {
    std::vector<std::string> allowed_val1(3);
    allowed_val1[0]="a1"; allowed_val1[1]="b2"; allowed_val1[2]="c";

    StringListValidator * val1 = new StringListValidator(allowed_val1);
    CompositeValidator comp;
    comp.add(IValidator_sptr(val1));

    std::vector<std::string> allowed=comp.allowedValues();
    TS_ASSERT_EQUALS(allowed_val1.size(),allowed.size());

    std::vector<std::string> allowed_val2(3);
    allowed_val2[0]="a2"; allowed_val2[1]="b2"; allowed_val2[2]="c2";

    StringListValidator * val2 = new StringListValidator(allowed_val2);
    comp.add(IValidator_sptr(val2));

    std::vector<std::string> allowed2=comp.allowedValues();
    TS_ASSERT_EQUALS(1,allowed2.size());
    TS_ASSERT_EQUALS("b2",*(allowed2.begin()));


  }


};


#endif /* MANTID_KERNEL_COMPOSITEVALIDATORTEST_H_ */

