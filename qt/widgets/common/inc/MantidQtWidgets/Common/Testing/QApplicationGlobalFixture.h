// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

  // Removed as it causes a segfault on Ubuntu 20.04 due to PyQt claiming
  // the Qapplication object and deleting it before tearDownWorld occurs.
  // the memory leak caused by m_app not being deleted in some circumstances
  // Doesn't matter as it executes this on exit
  // bool tearDownWorld() override {
  //   delete m_app;
  //   return true;
  // }

  int m_argc = 1;
  GNU_DIAG_OFF("pedantic")
  char *m_argv[1] = {const_cast<char *>("QAppForTesting")};
  GNU_DIAG_ON("pedantic")
  QApplication *m_app;
};
