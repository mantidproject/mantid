#ifndef MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODELTEST_H_
#define MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingModel.h"

#include <QtTest/QSignalSpy>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;

class ALCPeakFittingModelTest : public CxxTest::TestSuite {
  ALCPeakFittingModel *m_model;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCPeakFittingModelTest *createSuite() {
    return new ALCPeakFittingModelTest();
  }
  static void destroySuite(ALCPeakFittingModelTest *suite) { delete suite; }

  ALCPeakFittingModelTest() {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp() override { m_model = new ALCPeakFittingModel(); }

  void tearDown() override { delete m_model; }

  void test_setData() {
    MatrixWorkspace_sptr data =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    QSignalSpy spy(m_model, SIGNAL(dataChanged()));

    TS_ASSERT_THROWS_NOTHING(m_model->setData(data));

    TS_ASSERT_EQUALS(spy.size(), 1);
    TS_ASSERT_EQUALS(m_model->data(), data);
  }

  void test_fit() {
    MatrixWorkspace_sptr data =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 8, 8);

    data->setHistogram(0,
                       Points{1.00, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00},
                       Counts{0.00, 0.01, 0.02, 0.37, 1.00, 0.37, 0.01, 0.00},
                       CountStandardDeviations(8, 0));

    m_model->setData(data);

    IFunction_const_sptr func =
        FunctionFactory::Instance().createInitialized("name=FlatBackground");

    TS_ASSERT_THROWS_NOTHING(m_model->fitPeaks(func));

    IFunction_const_sptr fittedFunc = m_model->fittedPeaks();
    TS_ASSERT(fittedFunc);

    if (fittedFunc) {
      TS_ASSERT_EQUALS(fittedFunc->name(), "FlatBackground");
      TS_ASSERT_DELTA(fittedFunc->getParameter("A0"), 0.2225, 1E-4);
    }

    ITableWorkspace_sptr parameters = m_model->parameterTable();
    TS_ASSERT(parameters);

    if (parameters) {
      // Check table dimensions
      TS_ASSERT_EQUALS(parameters->rowCount(), 2);
      TS_ASSERT_EQUALS(parameters->columnCount(), 3);

      // Check table entries
      TS_ASSERT_EQUALS(parameters->String(0, 0), "A0");
      TS_ASSERT_DELTA(parameters->Double(0, 1), 0.2225, 1E-4);
      TS_ASSERT_DELTA(parameters->Double(0, 2), 0.3535, 1E-4);
      TS_ASSERT_EQUALS(parameters->String(1, 0), "Cost function value");
      TS_ASSERT_DELTA(parameters->Double(1, 1), 0.1254, 1E-4);
      TS_ASSERT_DELTA(parameters->Double(1, 2), 0.0000, 1E-4);
    }
  }

  void test_exportWorkspace() {
    TS_ASSERT_THROWS_NOTHING(m_model->exportWorkspace());
  }
};

#endif /* MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODELTEST_H_ */
