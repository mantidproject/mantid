// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_INSTRUMENTWIDGETENCODERTEST_H_
#define MANTIDQT_WIDGETS_INSTRUMENTWIDGETENCODERTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetEncoder.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class InstrumentWidgetEncoderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentWidgetEncoderTest *createSuite() {
    return new InstrumentWidgetEncoderTest();
  }
  static void destroySuite(InstrumentWidgetEncoderTest *suite) { delete suite; }

  void setUp() override {
    FrameworkManager::Instance();
    auto alg =
        AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws");
    alg->execute();
    m_instrumentWidget = new InstrumentWidget(QString("ws"));
    m_encoder = new InstrumentWidgetEncoder();
  }

  void test_encode() {
    const auto result =
        m_encoder->encode(*m_instrumentWidget, QString(""), false);
    TS_ASSERT_EQUALS(result.size(), 7);
  }

  InstrumentWidget *m_instrumentWidget;
  InstrumentWidgetEncoder *m_encoder;
};

#endif /* MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODERTEST_H_ */
