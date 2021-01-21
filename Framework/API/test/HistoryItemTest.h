// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/HistoryView.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class HistoryItemTest : public CxxTest::TestSuite {

public:
  void test_Minimum() {
    // not really much to test
    AlgorithmHistory algHist("AnAlg", 1, boost::uuids::to_string(boost::uuids::random_generator()()));
    HistoryItem item(std::make_shared<AlgorithmHistory>(algHist));
    item.unrolled(true);

    TS_ASSERT_EQUALS(*(item.getAlgorithmHistory()), algHist)
    TS_ASSERT(item.isUnrolled())
    TS_ASSERT_EQUALS(item.numberOfChildren(), 0)
  }
};
