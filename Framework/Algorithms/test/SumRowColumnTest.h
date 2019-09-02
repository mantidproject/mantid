// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SUMROWCOLUMNTEST_H_
#define SUMROWCOLUMNTEST_H_

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/SumRowColumn.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class SumRowColumnTest : public CxxTest::TestSuite {
public:
  static SumRowColumnTest *createSuite() { return new SumRowColumnTest(); }
  static void destroySuite(SumRowColumnTest *suite) { delete suite; }

  SumRowColumnTest() : inputWS("SumRowColumnTestWS") {
    AnalysisDataService::Instance().add(
        inputWS, WorkspaceCreationHelper::create2DWorkspaceBinned(100, 10));
  }

  ~SumRowColumnTest() override { AnalysisDataService::Instance().clear(); }

  void testName() { TS_ASSERT_EQUALS(summer.name(), "SumRowColumn") }

  void testVersion() { TS_ASSERT_EQUALS(summer.version(), 1) }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(summer.initialize())
    TS_ASSERT(summer.isInitialized())
  }

  void testPropertiesNotSet() {
    TS_ASSERT_THROWS_NOTHING(summer.setPropertyValue("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(summer.setPropertyValue("OutputWorkspace", "nowt"))

    TS_ASSERT_THROWS(summer.execute(), const std::runtime_error &)
    TS_ASSERT(!summer.isExecuted())
  }

  void testHorizontal() {
    Mantid::Algorithms::SumRowColumn summer2;
    TS_ASSERT_THROWS_NOTHING(summer2.initialize())
    TS_ASSERT_THROWS_NOTHING(
        summer2.setPropertyValue("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(summer2.setPropertyValue("OutputWorkspace", "H"))
    TS_ASSERT_THROWS_NOTHING(summer2.setPropertyValue("Orientation", "D_H"))

    TS_ASSERT_THROWS_NOTHING(summer2.execute())
    TS_ASSERT(summer2.isExecuted())

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("H")));
    // Check a couple of values
    TS_ASSERT_EQUALS(output->x(0).size(), 10)
    TS_ASSERT_EQUALS(output->y(0).size(), 10)
    TS_ASSERT_EQUALS(output->x(0)[1], 1)
    TS_ASSERT_EQUALS(output->x(0)[9], 9)
    TS_ASSERT_EQUALS(output->y(0)[1], 200)
    TS_ASSERT_EQUALS(output->y(0)[9], 200)
    // This algorithm doesn't compute errors
    TS_ASSERT_EQUALS(output->e(0)[1], 0)
    TS_ASSERT_EQUALS(output->e(0)[9], 0)

    TSM_ASSERT("Should have an empty unit",
               boost::dynamic_pointer_cast<Mantid::Kernel::Units::Empty>(
                   output->getAxis(0)->unit()))
  }

  void testVertical() {
    Mantid::Algorithms::SumRowColumn summer2;
    TS_ASSERT_THROWS_NOTHING(summer2.initialize())
    TS_ASSERT_THROWS_NOTHING(
        summer2.setPropertyValue("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(summer2.setPropertyValue("OutputWorkspace", "V"))
    TS_ASSERT_THROWS_NOTHING(summer2.setPropertyValue("Orientation", "D_V"))
    TS_ASSERT_THROWS_NOTHING(summer2.setPropertyValue("XMin", "4"))
    TS_ASSERT_THROWS_NOTHING(summer2.setPropertyValue("XMax", "10"))
    TS_ASSERT_THROWS_NOTHING(summer2.setPropertyValue("HOverVMin", "5"))
    TS_ASSERT_THROWS_NOTHING(summer2.setPropertyValue("HOverVMax", "9"))

    TS_ASSERT_THROWS_NOTHING(summer2.execute())
    TS_ASSERT(summer2.isExecuted())

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("V")));
    // Check a couple of values
    TS_ASSERT_EQUALS(output->x(0).size(), 10)
    TS_ASSERT_EQUALS(output->y(0).size(), 10)
    TS_ASSERT_EQUALS(output->x(0)[1], 1)
    TS_ASSERT_EQUALS(output->x(0)[9], 9)
    TS_ASSERT_EQUALS(output->y(0)[1], 60)
    TS_ASSERT_EQUALS(output->y(0)[9], 60)
    // This algorithm doesn't compute errors
    TS_ASSERT_EQUALS(output->e(0)[1], 0)
    TS_ASSERT_EQUALS(output->e(0)[9], 0)
  }

private:
  Mantid::Algorithms::SumRowColumn summer;
  std::string inputWS;
};

#endif /*SUMROWCOLUMNTEST_H_*/
