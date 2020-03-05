// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_COMMON_QAPPLICATIONHOLDER_H
#define MANTIDQT_WIDGETS_COMMON_QAPPLICATIONHOLDER_H

#include <cxxtest/GlobalFixture.h>

#include "MantidKernel/WarningSuppressions.h"
#include <QApplication>

/**
 * QApplication
 *
 * Uses setUpWorld/tearDownWorld to initialize & finalize
 * QApplication object
 */
class QApplicationGlobalFixture : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    m_app = new QApplication(m_argc, m_argv);
    return true;
  }

  bool tearDownWorld() override {
    delete m_app;
    return true;
  }

  int m_argc = 1;
  GNU_DIAG_OFF("pedantic")
  char *m_argv[1] = {"QAppForTesting"};
  GNU_DIAG_ON("pedantic")
  QApplication *m_app;
};

#endif // MANTIDQT_WIDGETS_COMMON_QAPPLICATIONHOLDER_H
