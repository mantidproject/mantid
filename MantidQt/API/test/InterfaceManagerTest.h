/*
 * InterfaceManagerTest.h
 *
 *  Created on: Feb 19, 2013
 *      Author: spu92482
 */

#ifndef INTERFACEMANAGERTEST_H_
#define INTERFACEMANAGERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidKernel/Exception.h"

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
