// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectBayesTab.h"

namespace MantidQt {
namespace CustomInterfaces {

IndirectBayesTab::IndirectBayesTab(QWidget *parent)
    : IndirectTab(parent), m_propTree(new QtTreePropertyBrowser()) {
  m_propTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateProperties(QtProperty *, double)));
}

IndirectBayesTab::~IndirectBayesTab() {}

/**
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void IndirectBayesTab::filterInputData(bool filter) {
  setFileExtensionsByName(filter);
}

/**
 * Emits a signal to run a python script using the method in the parent
 * UserSubWindow
 *
 * @param pyInput :: A string of python code to execute
 */
void IndirectBayesTab::runPythonScript(const QString &pyInput) {
  emit runAsPythonScript(pyInput, true);
}

/**
 * Format the tree widget so its easier to read the contents. It changes the
 * background colour and item indentation.
 *
 * @param treeWidget :: The tree widget to format
 * @param properties :: The properties within the tree widget
 */
void IndirectBayesTab::formatTreeWidget(
    QtTreePropertyBrowser *treeWidget,
    QMap<QString, QtProperty *> const &properties) const {
  treeWidget->setIndentation(0);
  for (auto const &item : properties)
    treeWidget->setBackgroundColor(treeWidget->topLevelItem(item),
                                   QColor(246, 246, 246));
}

} // namespace CustomInterfaces
} // namespace MantidQt
