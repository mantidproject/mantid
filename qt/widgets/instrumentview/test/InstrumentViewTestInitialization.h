// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_INSTRUMENTVIEWTESTINITIALIZATION_H
#define MANTIDQT_WIDGETS_INSTRUMENTVIEWTESTINITIALIZATION_H

#include "MantidPythonInterface/core/Testing/PythonInterpreterGlobalFixture.h"
#include "MantidQtWidgets/Common/Testing/QApplicationGlobalFixture.h"

//------------------------------------------------------------------------------
// Static definitions
//
// We rely on cxxtest only including this file once so that the following
// statements do not cause multiple-definition errors.
//------------------------------------------------------------------------------
static PythonInterpreterGlobalFixture PYTHON_INTERPRETER;
static QApplicationGlobalFixture MAIN_QAPPLICATION;

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEWTESTINITIALIZATION_H
