// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectToolsTab.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectToolsTab::IndirectToolsTab(QWidget *parent) : IndirectTab(parent) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectToolsTab::~IndirectToolsTab() {}

/**
 * Emits a signal to run a python script using the method in the parent
 * UserSubWindow
 *
 * @param pyInput :: A string of python code to execute
 */
void IndirectToolsTab::runPythonScript(const QString &pyInput) { emit executePythonScript(pyInput, false); }
} // namespace CustomInterfaces
} // namespace MantidQt
