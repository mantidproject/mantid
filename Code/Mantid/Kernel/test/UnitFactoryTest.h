#ifndef UNITFACTORYTEST_H_
#define UNITFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;

class UnitFactoryTest : public CxxTest::TestSuite
{
public:
  void testCreate()
  {
    boost::shared_ptr<Unit> first;
    TS_ASSERT_THROWS_NOTHING( first = UnitFactory::Instance().create("TOF") )
    // Test that asking for the same unit again gives the same pointer
    TS_ASSERT_EQUALS( UnitFactory::Instance().create("TOF"), first )
    // And that asking for a different unit gives a different pointer
    TS_ASSERT_DIFFERS( UnitFactory::Instance().create("Wavelength"), first )
  }

};

#endif /*UNITFACTORYTEST_H_*/
