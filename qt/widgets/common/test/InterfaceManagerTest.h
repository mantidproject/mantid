/*
 * InterfaceManagerTest.h
 *
 *  Created on: Feb 19, 2013
 *      Author: spu92482
 */

#ifndef INTERFACEMANAGERTEST_H_
#define INTERFACEMANAGERTEST_H_

#include "MantidKernel/Exception.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::API;
class InterfaceManagerTest : public CxxTest::TestSuite {

public:
  void testCreateManyInstances() {
    InterfaceManager objA;
    InterfaceManager objB;
    TS_ASSERT_DIFFERS(&objA, &objB);
  }
};

#endif /* INTERFACEMANAGERTEST_H_ */
