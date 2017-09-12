#ifndef MANTID_FACTORY_WIDGETFACTORYTEST_H_
#define MANTID_FACTORY_WIDGETFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidFactory/WidgetFactory.h"

using namespace Mantid;
using namespace Mantid::Factory;
using namespace Mantid::API;

class WidgetFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WidgetFactoryTest *createSuite() { return new WidgetFactoryTest(); }
  static void destroySuite(WidgetFactoryTest *suite) { delete suite; }

  void test_Something() {}
};

#endif /* MANTID_FACTORY_WIDGETFACTORYTEST_H_ */