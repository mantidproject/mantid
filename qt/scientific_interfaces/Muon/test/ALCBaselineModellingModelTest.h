// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "../Muon/ALCBaselineModellingModel.h"

#include <QSignalSpy>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Points;

class ALCBaselineModellingModelTest : public CxxTest::TestSuite {
  std::unique_ptr<ALCBaselineModellingModel> m_model;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCBaselineModellingModelTest *createSuite() { return new ALCBaselineModellingModelTest(); }
  static void destroySuite(ALCBaselineModellingModelTest *suite) { delete suite; }

  ALCBaselineModellingModelTest() {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp() override { m_model = std::make_unique<ALCBaselineModellingModel>(); }

  void test_setData() {

    MatrixWorkspace_sptr data = WorkspaceFactory::Instance().create("Workspace2D", 1, 9, 9);
    data->setHistogram(0, Points{1, 2, 3, 4, 5, 6, 7, 8, 9}, Counts{100, 1, 2, 100, 100, 3, 4, 5, 100});

    TS_ASSERT_THROWS_NOTHING(m_model->setData(data));

    MatrixWorkspace_const_sptr modelData = m_model->data();

    TS_ASSERT_EQUALS(modelData->x(0), data->x(0));
    TS_ASSERT_EQUALS(modelData->y(0), data->y(0));
    TS_ASSERT_EQUALS(modelData->e(0), data->e(0));
  }

  void test_fit() {
    MatrixWorkspace_sptr data = WorkspaceFactory::Instance().create("Workspace2D", 1, 9, 9);
    data->setHistogram(0, Points{1, 2, 3, 4, 5, 6, 7, 8, 9}, Counts{100, 1, 2, 100, 100, 3, 4, 5, 100},
                       CountStandardDeviations{10.0, 1.0, 1.41, 10.0, 10.0, 1.73, 2.0, 2.5, 10.0});

    m_model->setData(data);

    IFunction_const_sptr func = FunctionFactory::Instance().createInitialized("name=FlatBackground,A0=0");

    std::vector<IALCBaselineModellingModel::Section> sections;
    sections.emplace_back(2, 3);
    sections.emplace_back(6, 8);

    // TODO: test that the appropriate signals are thrown
    TS_ASSERT_THROWS_NOTHING(m_model->fit(func, sections));

    IFunction_const_sptr fittedFunc = m_model->fittedFunction();
    TS_ASSERT(fittedFunc);

    if (fittedFunc) {
      TS_ASSERT_EQUALS(fittedFunc->name(), "FlatBackground");
      TS_ASSERT_DELTA(fittedFunc->getParameter("A0"), 2.13979, 1E-5);
      TS_ASSERT_DELTA(fittedFunc->getError(0), 0.66709, 1E-5);
    }

    MatrixWorkspace_const_sptr corrected = m_model->correctedData();
    TS_ASSERT(corrected);

    if (corrected) {
      TS_ASSERT_EQUALS(corrected->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(corrected->blocksize(), 9);

      TS_ASSERT_DELTA(corrected->y(0)[0], 97.86021, 1E-5);
      TS_ASSERT_DELTA(corrected->y(0)[2], -0.13979, 1E-5);
      TS_ASSERT_DELTA(corrected->y(0)[5], 0.86021, 1E-5);
      TS_ASSERT_DELTA(corrected->y(0)[8], 97.86021, 1E-5);

      TS_ASSERT_EQUALS(corrected->e(0), data->e(0));
    }

    ITableWorkspace_sptr parameters = m_model->parameterTable();
    TS_ASSERT(parameters);

    if (parameters) {
      // Check table dimensions
      TS_ASSERT_EQUALS(parameters->rowCount(), 2);
      TS_ASSERT_EQUALS(parameters->columnCount(), 3);

      // Check table entries
      TS_ASSERT_EQUALS(parameters->String(0, 0), "A0");
      TS_ASSERT_DELTA(parameters->Double(0, 1), 2.13978, 1E-5);
      TS_ASSERT_DELTA(parameters->Double(0, 2), 0.66709, 1E-5);
      TS_ASSERT_EQUALS(parameters->String(1, 0), "Cost function value");
      TS_ASSERT_DELTA(parameters->Double(1, 1), 0.46627, 1E-5);
      TS_ASSERT_EQUALS(parameters->Double(1, 2), 0);
    }

    TS_ASSERT_EQUALS(m_model->sections(), sections);
  }

  void test_exportWorkspace() { TS_ASSERT_THROWS_NOTHING(m_model->exportWorkspace()); }

  void test_exportTable() { TS_ASSERT_THROWS_NOTHING(m_model->exportSections()); }

  void test_exportModel() { TS_ASSERT_THROWS_NOTHING(m_model->exportModel()); }

  void test_noData() {
    // Set a null shared pointer
    MatrixWorkspace_sptr data = MatrixWorkspace_sptr();
    m_model->setData(data);

    TS_ASSERT_THROWS_NOTHING(m_model->data());
    TS_ASSERT_THROWS_NOTHING(m_model->correctedData());
  }
};
