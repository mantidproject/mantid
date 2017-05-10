#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Memory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;

class CreateWorkspaceTest : public CxxTest::TestSuite {
public:
  void testMeta() {
    Mantid::Algorithms::CreateWorkspace alg;
    TS_ASSERT_EQUALS(alg.name(), "CreateWorkspace");
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void testCreate() {
    Mantid::Algorithms::CreateWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    std::vector<double> dataEYX{0.0, 1.234, 2.468};

    std::vector<std::string> qvals{"9.876"};

    TS_ASSERT_THROWS_NOTHING(alg.setProperty<int>("NSpec", 1));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty<std::vector<double>>("DataX", dataEYX));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty<std::vector<double>>("DataY", dataEYX));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty<std::vector<double>>("DataE", dataEYX));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("UnitX", "Wavelength"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("VerticalAxisUnit", "MomentumTransfer"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty<std::vector<std::string>>("VerticalAxisValues", qvals));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "test_CreateWorkspace"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());
    Mantid::API::MatrixWorkspace_const_sptr ws;

    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                "test_CreateWorkspace")));

    TS_ASSERT(!ws->isHistogramData());
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(ws->x(0)[0], 0);
    TS_ASSERT_EQUALS(ws->x(0)[1], 1.234);
    TS_ASSERT_EQUALS(ws->x(0)[2], 2.468);
    TS_ASSERT_EQUALS(ws->y(0)[0], 0);
    TS_ASSERT_EQUALS(ws->y(0)[1], 1.234);
    TS_ASSERT_EQUALS(ws->y(0)[2], 2.468);
    TS_ASSERT_EQUALS(ws->e(0)[0], 0);
    TS_ASSERT_EQUALS(ws->e(0)[1], 1.234);
    TS_ASSERT_EQUALS(ws->e(0)[2], 2.468);
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->caption(), "Wavelength");

    TS_ASSERT_EQUALS(ws->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_EQUALS(ws->getAxis(1)->unit()->caption(), "q");

    double axisVal = 0.0;

    TS_ASSERT_THROWS_NOTHING(
        axisVal = boost::lexical_cast<double>(ws->getAxis(1)->label(0)));

    TS_ASSERT_DELTA(axisVal, 9.876, 0.001);

    // Remove the created workspace
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().remove(
            "test_CreateWorkspace"));
  }

  void testCreateTextAxis() {
    std::vector<std::string> textAxis{"I've Got", "A Lovely", "Bunch Of",
                                      "Coconuts"};
    // Get hold of the output workspace
    Mantid::API::MatrixWorkspace_sptr workspace =
        executeVerticalAxisTest("Text", textAxis);
    TS_ASSERT(workspace->isHistogramData());
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(workspace->x(0)[0], 1.1);
    TS_ASSERT_EQUALS(workspace->x(2)[1], 2.2);

    Mantid::API::TextAxis *axis =
        dynamic_cast<Mantid::API::TextAxis *>(workspace->getAxis(1));

    TS_ASSERT_EQUALS(axis->label(0), "I've Got");
    TS_ASSERT_EQUALS(axis->label(1), "A Lovely");
    TS_ASSERT_EQUALS(axis->label(2), "Bunch Of");
    TS_ASSERT_EQUALS(axis->label(3), "Coconuts");

    // Remove workspace
    Mantid::API::AnalysisDataService::Instance().remove("test_CreateWorkspace");
  }

  void testFailureOnNumericVerticalAxisSizeMismatch() {
    Mantid::Algorithms::CreateWorkspace alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "test_CreateWorkspace");
    alg.setPropertyValue("UnitX", "Wavelength");
    alg.setPropertyValue("VerticalAxisUnit", "MomentumTransfer");
    const std::vector<std::string> vertValues{"0.0", "1.0", "2.0"};
    alg.setProperty<std::vector<std::string>>("VerticalAxisValues", vertValues);
    alg.setProperty<int>("NSpec", 4);

    std::vector<double> values{1.0, 2.0, 3.0, 4.0};

    alg.setProperty<std::vector<double>>("DataX",
                                         std::vector<double>{1.1, 2.2});
    alg.setProperty<std::vector<double>>("DataY", values);
    alg.setProperty<std::vector<double>>("DataE", values);

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void testParenting() {
    MatrixWorkspace_sptr parent =
        WorkspaceCreationHelper::createEventWorkspace(1, 1, 1);
    WorkspaceCreationHelper::addTSPEntry(parent->mutableRun(), "ALogEntry",
                                         99.0);

    Mantid::Algorithms::CreateWorkspace alg;
    alg.initialize();
    const std::string outWS = "testParenting";
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setProperty<std::vector<double>>("DataX",
                                         std::vector<double>{1.1, 2.2});
    alg.setProperty<std::vector<double>>("DataY", std::vector<double>(2, 1.1));
    alg.setProperty("ParentWorkspace", parent);
    TS_ASSERT(alg.execute());

    MatrixWorkspace_const_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWS);
    TS_ASSERT(output->run().hasProperty("ALogEntry"));

    AnalysisDataService::Instance().remove(outWS);
  }

  void testVerticalAxisValuesAsBinEdges() {
    std::vector<std::string> edgeAxis{"1.1", "2.2", "3.3", "4.4", "5.5"};
    Mantid::API::MatrixWorkspace_sptr workspace =
        executeVerticalAxisTest("DeltaE", edgeAxis);

    Mantid::API::BinEdgeAxis *axis =
        dynamic_cast<Mantid::API::BinEdgeAxis *>(workspace->getAxis(1));
    TS_ASSERT(axis != nullptr)
    TS_ASSERT_EQUALS(axis->length(), 5)
    TS_ASSERT_EQUALS(axis->getValue(0), 1.1);
    TS_ASSERT_EQUALS(axis->getValue(1), 2.2);
    TS_ASSERT_EQUALS(axis->getValue(2), 3.3);
    TS_ASSERT_EQUALS(axis->getValue(3), 4.4);
    TS_ASSERT_EQUALS(axis->getValue(4), 5.5);

    // Remove workspace
    Mantid::API::AnalysisDataService::Instance().remove("test_CreateWorkspace");
  }

  void testVerticalAxisValuesAsPoints() {
    std::vector<std::string> pointAxis{"1.1", "2.2", "3.3", "4.4"};
    Mantid::API::MatrixWorkspace_sptr workspace =
        executeVerticalAxisTest("DeltaE", pointAxis);

    Mantid::API::NumericAxis *axis =
        dynamic_cast<Mantid::API::NumericAxis *>(workspace->getAxis(1));
    TS_ASSERT(axis != nullptr)
    TS_ASSERT_EQUALS(axis->length(), 4)
    TS_ASSERT_EQUALS(axis->getValue(0), 1.1);
    TS_ASSERT_EQUALS(axis->getValue(1), 2.2);
    TS_ASSERT_EQUALS(axis->getValue(2), 3.3);
    TS_ASSERT_EQUALS(axis->getValue(3), 4.4);

    // Remove workspace
    Mantid::API::AnalysisDataService::Instance().remove("test_CreateWorkspace");
  }

private:
  MatrixWorkspace_sptr
  executeVerticalAxisTest(const std::string &vertUnit,
                          const std::vector<std::string> &vertValues) {
    Mantid::Algorithms::CreateWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "test_CreateWorkspace");
    alg.setPropertyValue("UnitX", "Wavelength");
    alg.setPropertyValue("VerticalAxisUnit", vertUnit);

    alg.setProperty<std::vector<std::string>>("VerticalAxisValues", vertValues);
    alg.setProperty<int>("NSpec", 4);

    std::vector<double> values{1.0, 2.0, 3.0, 4.0};

    alg.setProperty<std::vector<double>>("DataX",
                                         std::vector<double>{1.1, 2.2});
    alg.setProperty<std::vector<double>>("DataY", values);
    alg.setProperty<std::vector<double>>("DataE", values);

    alg.execute();

    TS_ASSERT(alg.isExecuted());

    // Get hold of the output workspace
    Mantid::API::MatrixWorkspace_sptr workspace =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                "test_CreateWorkspace"));
    return workspace;
  }
};

class CreateWorkspaceTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    Mantid::Kernel::MemoryStats memInfo;
    // 100,000,000 doubles takes ~760Mb.
    // Copying this vector into the 3 properties then requires about 3Gb of
    // memory
    // and if that's not available it ends up paging which can be very slow
    size_t nelements(100000000);
    if (memInfo.totalMem() < 4000000) {
      nelements = 40000000; // Needs about 1.2Gb
    }
    vec = new std::vector<double>(nelements, 1.0);
  }

  void tearDown() override { delete vec; }

  void testBigWorkspace() {
    Mantid::Algorithms::CreateWorkspace creator;
    creator.initialize();
    // The PropertyHistory operations take an age - this disables them
    creator.setChild(true);
    creator.enableHistoryRecordingForChild(false);

    creator.setPropertyValue("OutputWorkspace", "Out");
    creator.setProperty("DataX", *vec);
    creator.setProperty("DataY", *vec);
    creator.setProperty("DataE", *vec);
    creator.setProperty("NSpec", 10000);
    TS_ASSERT(creator.execute());
  }

private:
  std::vector<double> *vec;
};
