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

class InstrumentWidgetEncoderTest
    : public CxxTest::TestSuite,
      public MantidQt::MantidWidgets::InstrumentWidgetEncoder {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentWidgetEncoderTest *createSuite() {
    return new InstrumentWidgetEncoderTest();
  }
  static void destroySuite(InstrumentWidgetEncoderTest *suite) { delete suite; }

  // Ensure Framework is initialised
  InstrumentWidgetEncoderTest() {
    FrameworkManager::Instance();
    m_instrumentWidget = nullptr;
  }

  void setUp() override {
    auto alg =
        AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws");
    m_instrumentWidget =
        new InstrumentWidget(QString("ws"), 0, false, false, 0, 1000, false);
  }
  void test_encode() { TS_ASSERT(1==1) }
  void test_encodeActor() {}
  void test_encodeTabs() {}
  void test_encodeTreeTab() {}
  void test_encodeRenderTab() {}
  void test_encodeColorBar() {}
  void test_encodeMaskTab() {}
  void test_encodePickTab() {}
  void test_encodeMaskBinsData() {}
  void test_encodeBinMask() {}
  void test_encodeSurface() {}
  void test_encodeShape() {}
  void test_encodeEllipse() {}
  void test_encodeRectangle() {}
  void test_encodeRing() {}
  void test_encodeFree() {}
  void test_encodeMaskShapes() {}
  void test_encodeShapeProperties() {}
  void test_encodeAlignmentInfo() {}

  InstrumentWidget *m_instrumentWidget;
};

#endif /* MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODERTEST_H_ */