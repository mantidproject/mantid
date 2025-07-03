// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "LoadNXcanSASPerformanceTestBase.h"
#include <cxxtest/TestSuite.h>

class LoadNXcanSASPerformanceTest1D : public ILoadNXcanSASPerformanceTest, public CxxTest::TestSuite {
public:
  void setUp() override { ILoadNXcanSASPerformanceTest::setUp(); }
  void tearDown() override { ILoadNXcanSASPerformanceTest::tearDown(); }
  void test_execute() override { alg.execute(); }

  static LoadNXcanSASPerformanceTest1D *createSuite() { return new LoadNXcanSASPerformanceTest1D(); }
  static void destroySuite(LoadNXcanSASPerformanceTest1D *suite) { delete suite; }
  void setupUniqueParams() override {
    m_parameters.hasDx = true;

    const auto ws = provide1DWorkspace(m_parameters);
    setXValuesOn1DWorkspace(ws, m_parameters.xmin, m_parameters.xmax);
    m_parameters.idf = getIDFfromWorkspace(ws);

    save_no_assert(ws, m_parameters);
  }
};
