// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidKernel/Exception.h"

using namespace MantidQt::API;

/**
 * Default constructor
 * @param parent :: The parent widget
 */
MantidWidget::MantidWidget(QWidget *parent) : QWidget(parent), m_pyRunner() {
  // re-emit the run Python code from m_pyRunner, to work this signal must reach
  // the slot in QtiPlot
  connect(&m_pyRunner, SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));
}

/** Run a piece of python code and return any output that it writes to stdout
 *  @param code :: the Python commands to execute
 *  @param no_output :: if set to true this method returns an empty string, if
 * false it returns the output from any Python print statements
 *  @return output from Python print statements unless no_output is false
 */
QString MantidWidget::runPythonCode(const QString &code, bool no_output) {
  return m_pyRunner.runPythonCode(code, no_output);
}
