// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "BayesFittingTab.h"

namespace MantidQt::CustomInterfaces {

BayesFittingTab::BayesFittingTab(QWidget *parent) : InelasticTab(parent), m_propTree(new QtTreePropertyBrowser()) {
  m_propTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateProperties(QtProperty *, double)));
}

BayesFittingTab::~BayesFittingTab() { m_propTree->unsetFactoryForManager(m_dblManager); }

/**
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void BayesFittingTab::filterInputData(bool filter) { setFileExtensionsByName(filter); }

void BayesFittingTab::applySettings(std::map<std::string, QVariant> const &settings) {
  filterInputData(settings.at("RestrictInput").toBool());
}

void BayesFittingTab::setFileExtensionsByName(bool filter) { (void)filter; }

void BayesFittingTab::updateProperties(QtProperty *prop, double val) {
  (void)prop;
  (void)val;
}

/**
 * Format the tree widget so its easier to read the contents. It changes the
 * background colour and item indentation.
 *
 * @param treeWidget :: The tree widget to format
 * @param properties :: The properties within the tree widget
 */
void BayesFittingTab::formatTreeWidget(QtTreePropertyBrowser *treeWidget,
                                       QMap<QString, QtProperty *> const &properties) const {
  treeWidget->setIndentation(0);
  for (auto const &item : properties)
    treeWidget->setBackgroundColor(treeWidget->topLevelItem(item), QColor(246, 246, 246));
}

} // namespace MantidQt::CustomInterfaces
