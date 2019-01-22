// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODERTEST_H_
#define MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODERTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetDecoder.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetEncoder.h"

#include <cxxtest/TestSuite.h>

// PythonInterpreter and QApplicationHolder imports
#include "MantidPythonInterface/core/NDArray.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#include <QApplication>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class InstrumentWidgetDecoderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentWidgetDecoderTest *createSuite() {
    return new InstrumentWidgetDecoderTest();
  }
  static void destroySuite(InstrumentWidgetDecoderTest *suite) { delete suite; }

  void setUp() override {
    FrameworkManager::Instance();
    auto alg =
        AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws");
    alg->execute();
    m_instrumentWidget = new InstrumentWidget(QString("ws"));
    m_decoder = new InstrumentWidgetDecoder();

    // Setup the infomap for decoding
    setUpInfoMap();
  }

  void tearDown() override {
    delete m_instrumentWidget;
    delete m_decoder;
  }

  void test_decode() {
    TS_ASSERT_THROWS_NOTHING(
        m_decoder->decode(m_infoMap, *m_instrumentWidget, QString(""), false));
    // Set to 2
    TS_ASSERT_EQUALS(m_instrumentWidget->getCurrentTab(), 2)
  }

private:
  void setUpInfoMap() {
    // Instead of manually defining a QMap by inserting many variables we can
    // use the encoder
    InstrumentWidgetEncoder encoder;
    m_infoMap = encoder.encode(*m_instrumentWidget, QString(""), false);

    // Check that the widget originally was not on Tab 2
    TS_ASSERT_EQUALS(m_instrumentWidget->getCurrentTab(), 0)

    // Edit the map a little to change the end instrument
    m_infoMap[QString("currentTab")] = 2;
  }

  InstrumentWidget *m_instrumentWidget;
  InstrumentWidgetDecoder *m_decoder;
  QMap<QString, QVariant> m_infoMap;
};

#endif /* MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODERTEST_H_ */