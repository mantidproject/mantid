// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <QString>
#include <QUrl>

#include "MantidQtWidgets/Common/PythonHelpBridge.h"
#include "MantidQtWidgets/Common/PythonHelpWindow.h"

#include "MantidKernel/WarningSuppressions.h"
GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;

GNU_DIAG_ON_SUGGEST_OVERRIDE

class PythonHelpWindowTest : public CxxTest::TestSuite {
public:
  void setUp() override {}

  void tearDown() override {}

  // 1) Test the constructor doesn't throw and creates a valid object
  void testConstructorDoesNotThrow() { TS_ASSERT_THROWS_NOTHING(PythonHelpWindow helpWin); }

  // 2) Test showPage(std::string) with an empty string
  void testShowPageWithEmptyString() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showPage(std::string{}));
  }

  // 3) Test showPage(std::string) with a non-empty string
  void testShowPageWithNonEmptyString() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showPage("custom_page.html"));
  }

  // 4) Test showPage(QString) with an empty string
  void testShowPage_QString_Empty() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showPage(QString()));
  }

  // 5) Test showPage(QUrl) with an empty URL
  void testShowPage_QUrl_Empty() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showPage(QUrl()));
  }

  // 6) Test showAlgorithm(std::string) with empty name and version
  void testShowAlgorithm_NoName() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showAlgorithm("", -1));
  }

  // 7) Test showAlgorithm(std::string) with valid name and version
  void testShowAlgorithm_NameAndVersion() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showAlgorithm("Load", 2));
  }

  // 8) Test showConcept(std::string) with empty name
  void testShowConceptEmpty() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showConcept(""));
  }

  // 9) Test showFitFunction(std::string) with a name
  void testShowFitFunctionName() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showFitFunction("GaussPeak"));
  }

  // 10) Test showCustomInterface(std::string) with name, area, section
  void testShowCustomInterface() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.showCustomInterface("Reflectometry", "ISISReflectometry", "Usage"));
  }

  // 11) Test the shutdown method (currently a no-op)
  void testShutdownDoesNotThrow() {
    PythonHelpWindow helpWin;
    TS_ASSERT_THROWS_NOTHING(helpWin.shutdown());
  }
};
