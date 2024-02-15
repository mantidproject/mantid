// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DetectorGroupingOptions.h"

namespace MantidQt {
namespace CustomInterfaces {

DetectorGroupingOptions::DetectorGroupingOptions(QWidget *parent) : QWidget(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.cbGroupingOptions, SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(handleGroupingMethodChanged(QString const &)));
  connect(m_uiForm.pbSaveCustomGrouping, SIGNAL(clicked()), this, SLOT(emitSaveCustomGrouping()));

  QRegExp re("([0-9]+[-:+]?[0-9]*([+]?[0-9]*)*,[ ]?)*[0-9]+[-:+]?[0-9]*([+]?[0-9]*)*");
  m_uiForm.leCustomGroups->setValidator(new QRegExpValidator(re, this));

  handleGroupingMethodChanged(QString::fromStdString(groupingMethod()));
}

void DetectorGroupingOptions::includeOption(QString const &option, bool include) {
  if (include && isOptionHidden(option)) {
    m_uiForm.cbGroupingOptions->addItem(option);
    m_uiForm.cbGroupingOptions->setCurrentIndex(optionIndex(option));
  } else if (!include && !isOptionHidden(option)) {
    m_uiForm.cbGroupingOptions->setCurrentIndex(0);
    m_uiForm.cbGroupingOptions->removeItem(optionIndex(option));
  }
}

void DetectorGroupingOptions::handleGroupingMethodChanged(QString const &method) {
  if (method == "File")
    m_uiForm.swGrouping->setCurrentIndex(0);
  else if (method == "Groups")
    m_uiForm.swGrouping->setCurrentIndex(1);
  else if (method == "Custom")
    m_uiForm.swGrouping->setCurrentIndex(2);
  else
    m_uiForm.swGrouping->setCurrentIndex(3);
}

std::string DetectorGroupingOptions::groupingMethod() const {
  return m_uiForm.cbGroupingOptions->currentText().toStdString();
}

std::string DetectorGroupingOptions::mapFile() const { return m_uiForm.dsMapFile->getFirstFilename().toStdString(); }

std::string DetectorGroupingOptions::customGrouping() const { return m_uiForm.leCustomGroups->text().toStdString(); }

int DetectorGroupingOptions::nGroups() const { return m_uiForm.spNumberGroups->value(); }

void DetectorGroupingOptions::emitSaveCustomGrouping() { emit saveCustomGrouping(customGrouping()); }

int DetectorGroupingOptions::optionIndex(QString const &option) const {
  auto const index = m_uiForm.cbGroupingOptions->findText(option);
  return index >= 0 ? index : 0;
}

bool DetectorGroupingOptions::isOptionHidden(QString const &option) const {
  auto const index = m_uiForm.cbGroupingOptions->findText(option);
  return index == -1;
}

} // namespace CustomInterfaces
} // namespace MantidQt