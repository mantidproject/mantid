#ifndef MANTID_KERNEL_COMPOSITEVALIDATORTEST_H_
#define MANTID_KERNEL_COMPOSITEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/BoundedValidator.h"

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
    CompositeValidator<int> comp;
    comp.add(val1);
    TS_ASSERT(  comp.isValid(150).empty() );
    TS_ASSERT(  comp.isValid(950).empty() );
    TS_ASSERT( !comp.isValid(1200).empty() );
    comp.add(val2);
    TS_ASSERT( !comp.isValid(150).empty() ); // This one is now blocked by validator 2
    TS_ASSERT(  comp.isValid(950).empty() );
    TS_ASSERT( !comp.isValid(1200).empty() );

    // Test cloning
    IValidator<int> * comp2 = comp.clone();
    TS_ASSERT( !comp2->isValid(150).empty() );
    TS_ASSERT(  comp2->isValid(950).empty() );
  }

};


#endif /* MANTID_KERNEL_COMPOSITEVALIDATORTEST_H_ */

