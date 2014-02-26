#ifndef MANTID_SINQ_POLDICHOPPERFACTORYTEST_H_
#define MANTID_SINQ_POLDICHOPPERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/PoldiChopperFactory.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiUtilities/PoldiBasicChopper.h"

using namespace Mantid::Poldi;

class PoldiChopperFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiChopperFactoryTest *createSuite() { return new PoldiChopperFactoryTest(); }
  static void destroySuite( PoldiChopperFactoryTest *suite ) { delete suite; }


  void testDetectorByType()
  {
    PoldiChopperFactory chopperFactory;

    PoldiAbstractChopper *chopper = chopperFactory.createChopper(std::string("any"));
    TS_ASSERT(chopper);

    PoldiBasicChopper *basicChopper = dynamic_cast<PoldiBasicChopper *>(chopper);
    TS_ASSERT(basicChopper);

    delete chopper;
  }


};


#endif /* MANTID_SINQ_POLDICHOPPERFACTORYTEST_H_ */
