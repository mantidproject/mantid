// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * InterfaceManagerTest.h
 *
 *  Created on: Feb 19, 2013
 *      Author: spu92482
 */

#pragma once

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
