// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/RenameLog.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class RenameLogTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RenameLogTest *createSuite() { return new RenameLogTest(); }
  static void destroySuite(RenameLogTest *suite) { delete suite; }

  void test_Init() {
    RenameLog renlog;

    TS_ASSERT_THROWS_NOTHING(renlog.initialize());
    TS_ASSERT(renlog.isInitialized());
  }

  void test_Rename() {
    // 1. Generate workspace & 2 logs
    API::MatrixWorkspace_sptr testWS = generateTestWorkspace();

    // 2. Add workspace to data serive
    AnalysisDataService::Instance().addOrReplace("TestDummy", testWS);

    // 3. Running
    RenameLog renlog;
    renlog.initialize();

    TS_ASSERT_THROWS_NOTHING(renlog.setPropertyValue("Workspace", "TestDummy"));
    TS_ASSERT_THROWS_NOTHING(renlog.setProperty("OriginalLogName", "OriginalLog"));
    TS_ASSERT_THROWS_NOTHING(renlog.setProperty("NewLogName", "NewLog"));

    renlog.execute();
    TS_ASSERT(renlog.isExecuted());

    // 4. Check
    API::MatrixWorkspace_sptr resultWS = AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("TestDummy");
    verifyLog(resultWS, "NewLog");
  }

  void test_RenameCloned() {
    API::MatrixWorkspace_sptr testWS = generateTestWorkspace();
    API::MatrixWorkspace_sptr clonedWS = WorkspaceFactory::Instance().create(testWS);
    AnalysisDataService::Instance().addOrReplace("TestDummy", testWS);
    AnalysisDataService::Instance().addOrReplace("TestClonedDummy", clonedWS);

    RenameLog renlog;
    renlog.initialize();

    TS_ASSERT_THROWS_NOTHING(renlog.setPropertyValue("Workspace", "TestDummy"));
    TS_ASSERT_THROWS_NOTHING(renlog.setProperty("OriginalLogName", "OriginalLog"));
    TS_ASSERT_THROWS_NOTHING(renlog.setProperty("NewLogName", "NewLog"));

    renlog.execute();
    TS_ASSERT(renlog.isExecuted());

    // Verify modified logs on original workspace
    API::MatrixWorkspace_sptr resultWS = AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("TestDummy");
    verifyLog(resultWS, "NewLog");

    // Verify original logs on cloned workspace
    resultWS = AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("TestClonedDummy");
    verifyLog(resultWS, "OriginalLog");
  }

private:
  API::MatrixWorkspace_sptr generateTestWorkspace() {
    API::MatrixWorkspace_sptr testWS = API::WorkspaceFactory::Instance().create("Workspace2D", 1, 100, 100);

    Kernel::TimeSeriesProperty<double> *p1 = new Kernel::TimeSeriesProperty<double>("OriginalLog");

    int64_t t1_ns = 1000000;
    int64_t dt_ns = 400;
    double v1 = -1.0;

    m_num1 = 10;
    m_rawTimes.clear();
    m_rawValues.clear();

    for (size_t i = 0; i < m_num1; i++) {
      Types::Core::DateAndTime time(t1_ns);
      p1->addValue(time, v1);

      m_rawTimes.emplace_back(time);
      m_rawValues.emplace_back(v1);

      t1_ns += dt_ns;
      v1 = -v1;
    }

    testWS->mutableRun().addProperty(p1);

    return testWS;
  }

  void verifyLog(const API::MatrixWorkspace_sptr &resultWS, const std::string &logName) {
    Kernel::TimeSeriesProperty<double> *rp;
    TS_ASSERT_THROWS_NOTHING(
        rp = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(resultWS->run().getProperty(logName)));
    rp = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(resultWS->run().getProperty(logName));

    std::vector<Types::Core::DateAndTime> newtimes = rp->timesAsVector();
    TS_ASSERT_EQUALS(newtimes.size(), m_num1);
    for (size_t i = 0; i < m_num1; i++) {
      double newvalue;
      TS_ASSERT_THROWS_NOTHING(newvalue = rp->getSingleValue(m_rawTimes[i]));
      newvalue = rp->getSingleValue(m_rawTimes[i]);
      TS_ASSERT_DELTA(newvalue, m_rawValues[i], 1.0E-8);
    }
  }

  size_t m_num1;
  std::vector<Types::Core::DateAndTime> m_rawTimes;
  std::vector<double> m_rawValues;
};
