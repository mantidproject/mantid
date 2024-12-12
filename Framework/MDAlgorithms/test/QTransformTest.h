// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidMDAlgorithms/QTransform.h"
#include <cxxtest/TestSuite.h>

using Mantid::MDAlgorithms::QTransform;
using namespace Mantid::DataObjects;

class QTransformTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QTransformTest *createSuite() { return new QTransformTest(); }
  static void destroySuite(QTransformTest *suite) { delete suite; }

  void test_exec_1Q() {
    auto inputWS = createMDWorkspace("QTransformTest1", 1, "1,4", "|Q|", "A");

    auto outputWS = runQTransform("QTransformTest1");
    TS_ASSERT(outputWS);

    compare_input_and_outputWS(inputWS, outputWS);
  }

  void test_exec_1Q_1E() {
    auto inputWS = createMDWorkspace("QTransformTest2", 2, "1,4,1,4", "|Q|,E", "A,B");

    auto outputWS = runQTransform("QTransformTest2");
    TS_ASSERT(outputWS);

    compare_input_and_outputWS(inputWS, outputWS, true);
  }

  void test_exec_3Q() {
    auto inputWS =
        createMDWorkspace("QTransformTest3", 3, "1,4,1,4,1,4", "Q_lab_x,Q_lab_y,Q_lab_z", "A,B,C", "QLab,QLab,QLab");

    auto outputWS = runQTransform("QTransformTest3");
    TS_ASSERT(outputWS);

    compare_input_and_outputWS(inputWS, outputWS);
  }

  void test_exec_3Q_1E() {
    auto inputWS = createMDWorkspace("QTransformTest4", 4, "1,4,1,4,1,4,1,4", "Q_sample_x,Q_sample_y,Q_sample_z,DeltaE",
                                     "A,B,C,D", "QSample,QSample,QSample,General Frame");

    auto outputWS = runQTransform("QTransformTest4");
    TS_ASSERT(outputWS);

    compare_input_and_outputWS(inputWS, outputWS, true);
  }

  void test_exec_bad_2Q() {
    // this should throw as invalid input WS, only 2 q dimensions
    auto inputWS =
        createMDWorkspace("QTransformTest5", 2, "1,4,1,4", "Q_sample_x,Q_sample_y", "A,B", "QSample,QSample");

    runQTransform("QTransformTest5", true);
  }

  void test_exec_bad_1Q() {
    // this should throw as invalid input WS, name is "Q" not "|Q|"
    auto inputWS = createMDWorkspace("QTransformTest6", 1, "1,4", "Q", "A");

    runQTransform("QTransformTest6", true);
  }

  void test_exec_bad_order() {
    // this should throw as invalid input WS, q diemnsions are not the first 3
    auto inputWS = createMDWorkspace("QTransformTest7", 4, "1,4,1,4,1,4,1,4", "DeltaE,Q_sample_x,Q_sample_y,Q_sample_z",
                                     "A,B,C,D", "General Frame,QSample,QSample,QSample");

    runQTransform("QTransformTest7", true);
  }

private:
  Mantid::API::IMDEventWorkspace_sptr runQTransform(std::string inputWS, bool expect_throw = false) {
    QTransformTestClass alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    if (expect_throw) {
      TS_ASSERT_THROWS(alg.execute();, const std::runtime_error &);
    } else {
      TS_ASSERT_THROWS_NOTHING(alg.execute(););
      TS_ASSERT(alg.isExecuted());
    }

    return alg.getProperty("OutputWorkspace");
  }

  Mantid::API::IMDEventWorkspace_sptr createMDWorkspace(std::string wsName, int dim, std::string extents,
                                                        std::string names, std::string units, std::string frames = "") {
    auto create_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateMDWorkspace");
    create_alg->initialize();
    create_alg->setProperty("Dimensions", dim);
    create_alg->setProperty("Extents", extents);
    create_alg->setProperty("Names", names);
    create_alg->setProperty("Units", units);
    create_alg->setProperty("Frames", frames);
    create_alg->setPropertyValue("OutputWorkspace", wsName);
    create_alg->execute();

    auto fake_md_events = Mantid::API::AlgorithmManager::Instance().createUnmanaged("FakeMDEventData");
    fake_md_events->initialize();
    fake_md_events->setProperty("InputWorkspace", wsName);
    fake_md_events->setPropertyValue("UniformParams", "-100");
    fake_md_events->execute();

    Mantid::API::IMDEventWorkspace_sptr inputWS =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IMDEventWorkspace>(wsName);

    return inputWS;
  }

  void compare_input_and_outputWS(Mantid::API::IMDEventWorkspace_sptr inputWS,
                                  Mantid::API::IMDEventWorkspace_sptr outputWS, bool q_skip_last = false) {
    auto input_events = get_events_helper(inputWS);
    auto output_events = get_events_helper(outputWS);

    TS_ASSERT_EQUALS(input_events.size(), output_events.size());

    for (size_t i = 0; i < input_events.size(); ++i) {
      TS_ASSERT_EQUALS(input_events[i].size(), output_events[i].size());

      double q2{0};
      // check center values do not change
      for (size_t j = 2; j < input_events[i].size(); ++j) {
        TS_ASSERT_DELTA(input_events[i][j], output_events[i][j], 1e-6);
        if (!q_skip_last || j < input_events[i].size() - 1)
          q2 += input_events[i][j] * input_events[i][j];
      }

      // signal and error input will be 1, output equal to q^2 since our implemented correction function just returns
      // q^2
      TS_ASSERT_EQUALS(input_events[i][0], 1);
      TS_ASSERT_EQUALS(input_events[i][1], 1);
      TS_ASSERT_DELTA(output_events[i][0], q2, 1e-5);
      TS_ASSERT_DELTA(output_events[i][1], q2, 1e-5);
    }
  }

  std::vector<std::vector<double>> get_events_helper(Mantid::API::IMDEventWorkspace_sptr workspace) {
    MDEventWorkspace<MDLeanEvent<1>, 1>::sptr MDEW_MDLEANEVENT_1 =
        std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<1>, 1>>(workspace);
    if (MDEW_MDLEANEVENT_1)
      return get_events<MDLeanEvent<1>, 1>(MDEW_MDLEANEVENT_1);

    MDEventWorkspace<MDLeanEvent<2>, 2>::sptr MDEW_MDLEANEVENT_2 =
        std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<2>, 2>>(workspace);
    if (MDEW_MDLEANEVENT_2)
      return get_events<MDLeanEvent<2>, 2>(MDEW_MDLEANEVENT_2);

    MDEventWorkspace<MDLeanEvent<3>, 3>::sptr MDEW_MDLEANEVENT_3 =
        std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<3>, 3>>(workspace);
    if (MDEW_MDLEANEVENT_3)
      return get_events<MDLeanEvent<3>, 3>(MDEW_MDLEANEVENT_3);

    MDEventWorkspace<MDLeanEvent<4>, 4>::sptr MDEW_MDLEANEVENT_4 =
        std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<4>, 4>>(workspace);
    return get_events<MDLeanEvent<4>, 4>(MDEW_MDLEANEVENT_4);
  }

  template <typename MDE, size_t nd>
  std::vector<std::vector<double>> get_events(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) {
    // return a vector of events, events being (signal, error, x1, x2, x3, ...)
    std::vector<std::vector<double>> events_vector;
    MDBoxBase<MDE, nd> *box1 = ws->getBox();
    std::vector<Mantid::API::IMDNode *> boxes;
    box1->getBoxes(boxes, 1000, true);
    auto numBoxes = int(boxes.size());

    for (int i = 0; i < numBoxes; ++i) {
      auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
      if (box && !box->getIsMasked()) {
        std::vector<MDE> &events = box->getEvents();
        for (auto it = events.begin(); it != events.end(); ++it) {

          std::vector<double> event;
          event.push_back(it->getSignal());
          event.push_back(it->getError());
          for (size_t d = 0; d < nd; ++d)
            event.push_back(it->getCenter(d));
          events_vector.push_back(event);
        }
      }
      if (box) {
        box->releaseEvents();
      }
    }
    return events_vector;
  }

  // small helper class to test abstract class
  class QTransformTestClass : public QTransform {
  public:
    const std::string name() const override { return "QTransformTestClass"; };
    int version() const override { return 1; };
    const std::string category() const override { return "Test"; };
    const std::string summary() const override { return "Summary."; };

    // return the input q^2
    double correction(const double q2) const override { return q2; };
  };
};
