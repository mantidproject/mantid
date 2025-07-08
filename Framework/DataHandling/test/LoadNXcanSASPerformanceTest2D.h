// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "LoadNXcanSASPerformanceTestBase.h"
#include <cxxtest/TestSuite.h>

class LoadNXcanSASPerformanceTest2D : public ILoadNXcanSASPerformanceTest, public CxxTest::TestSuite {
public:
  void setUp() override { ILoadNXcanSASPerformanceTest::setUp(); }
  void tearDown() override { ILoadNXcanSASPerformanceTest::tearDown(); }
  void test_execute() override { alg.execute(); }

  static LoadNXcanSASPerformanceTest2D *createSuite() { return new LoadNXcanSASPerformanceTest2D(); }
  static void destroySuite(LoadNXcanSASPerformanceTest2D *suite) { delete suite; }
  void setupUniqueParams() override {
    m_parameters.is2dData = true;

    const auto ws = provide2DWorkspace(m_parameters);
    set2DValues(ws);
    m_parameters.idf = getIDFfromWorkspace(ws);

    save_no_assert(ws, m_parameters);
  }
};
