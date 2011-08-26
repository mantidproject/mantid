#ifndef MANTID_KERNEL_VISIBLEWHENPROPERTYTEST_H_
#define MANTID_KERNEL_VISIBLEWHENPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/PropertyManagerOwner.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class VisibleWhenPropertyTest : public CxxTest::TestSuite
{
public:


  void test_when_IS_NOT_DEFAULT()
  {
    PropertyManagerOwner alg;
    // Start with a regular property
    alg.declareProperty("MyIntProp", 123);

    // Make a property with its validator. Will be Visible when that other one is NOT the default
    VisibleWhenProperty<int> * val = new VisibleWhenProperty<int>(&alg, "MyIntProp", IS_NOT_DEFAULT);
    alg.declareProperty("MyValidatorProp", 456, val);

    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop); if (!prop) return;
    TSM_ASSERT( "Property always returns enabled.", prop->isEnabled() )
    TSM_ASSERT( "Property always returns valid.", prop->isValid().empty() )

    TSM_ASSERT( "Starts off NOT Visible",  !prop->isVisible());
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT( "Becomes visible when another property has been changed",  prop->isVisible());

    IValidator<int> * val2 = val->clone();
    alg.declareProperty("MySecondValidatorProp", 456, val2);
    prop = alg.getPointerToProperty("MySecondValidatorProp");
    TSM_ASSERT( "Starts off visible",  prop->isVisible());
    alg.setProperty("MyIntProp", 123);
    TSM_ASSERT( "Goes back to not visible", !prop->isVisible());
  }

  void test_when_IS_DEFAULT()
  {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    // Make a property with its validator. Will be Visible when that other one is the default
    VisibleWhenProperty<int> * val = new VisibleWhenProperty<int>(&alg, "MyIntProp", IS_DEFAULT);
    alg.declareProperty("MyValidatorProp", 456, val);
    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop); if (!prop) return;
    TSM_ASSERT( "Starts off visible", prop->isVisible());
    alg.setProperty("MyIntProp", -1);
    TSM_ASSERT( "Becomes not visible when another property has been changed", !prop->isVisible());
  }

  void test_when_IS_EQUAL_TO()
  {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    VisibleWhenProperty<int> * val = new VisibleWhenProperty<int>(&alg, "MyIntProp", IS_EQUAL_TO, "234");
    alg.declareProperty("MyValidatorProp", 456, val);
    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop); if (!prop) return;
    TSM_ASSERT( "Starts off not visible", !prop->isVisible());
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT( "Becomes visible when the other property is equal to the given string", prop->isVisible());
  }

  void test_when_IS_NOT_EQUAL_TO()
  {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    VisibleWhenProperty<int> * val = new VisibleWhenProperty<int>(&alg, "MyIntProp", IS_NOT_EQUAL_TO, "234");
    alg.declareProperty("MyValidatorProp", 456, val);
    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop); if (!prop) return;
    TSM_ASSERT( "Starts off not visible", prop->isVisible());
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT( "Becomes visible when the other property is equal to the given string", !prop->isVisible());
  }


};


#endif /* MANTID_KERNEL_VISIBLEWHENPROPERTYTEST_H_ */

