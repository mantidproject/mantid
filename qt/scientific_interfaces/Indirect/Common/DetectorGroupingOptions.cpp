// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DetectorGroupingOptions.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidQtWidgets/Spectroscopy/ValidationUtils.h"

#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
static std::unordered_map<std::string, GroupingMethod> GROUPING_METHODS = {{"Individual", GroupingMethod::Individual},
                                                                           {"All", GroupingMethod::All},
                                                                           {"IPF", GroupingMethod::IPF},
                                                                           {"File", GroupingMethod::File},
                                                                           {"Groups", GroupingMethod::Groups},
                                                                           {"Custom", GroupingMethod::Custom}};

DetectorGroupingOptions::DetectorGroupingOptions(QWidget *parent) : QWidget(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.cbGroupingOptions, SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(handleGroupingMethodChanged(QString const &)));
  connect(m_uiForm.pbSaveCustomGrouping, SIGNAL(clicked()), this, SLOT(emitSaveCustomGrouping()));

  QRegExp re("([0-9]+[-:+]?[0-9]*([+]?[0-9]*)*,[ ]?)*[0-9]+[-:+]?[0-9]*([+]?[0-9]*)*");
  m_uiForm.leCustomGroups->setValidator(new QRegExpValidator(re, this));

  handleGroupingMethodChanged(QString::fromStdString(groupingMethod()));
}

void DetectorGroupingOptions::removeGroupingMethod(std::string const &option) {
  m_uiForm.cbGroupingOptions->removeItem(optionIndex(option));
}

void DetectorGroupingOptions::setGroupingMethod(std::string const &option) {
  m_uiForm.cbGroupingOptions->setCurrentIndex(optionIndex(option));
}

void DetectorGroupingOptions::setSaveCustomVisible(bool const visible) {
  m_uiForm.pbSaveCustomGrouping->setVisible(visible);
}

void DetectorGroupingOptions::handleGroupingMethodChanged(QString const &method) {
  m_uiForm.swGrouping->setCurrentIndex(static_cast<int>(GROUPING_METHODS[method.toStdString()]));
}

std::string DetectorGroupingOptions::groupingMethod() const {
  return m_uiForm.cbGroupingOptions->currentText().toStdString();
}

std::string DetectorGroupingOptions::groupingFile() const {
  return m_uiForm.dsMapFile->getFirstFilename().toStdString();
}

std::string DetectorGroupingOptions::customGrouping() const { return m_uiForm.leCustomGroups->text().toStdString(); }

int DetectorGroupingOptions::nGroups() const { return m_uiForm.spNumberGroups->value(); }

std::optional<std::string> DetectorGroupingOptions::validateGroupingProperties(std::size_t const &spectraMin,
                                                                               std::size_t const &spectraMax) const {
  return ValidationUtils::validateGroupingProperties(groupingProperties(), spectraMin, spectraMax);
}

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> DetectorGroupingOptions::groupingProperties() const {
  auto const method = groupingMethod();
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  Mantid::API::AlgorithmProperties::update("GroupingMethod", method, *properties);
  switch (GROUPING_METHODS[method]) {
  case GroupingMethod::File:
    Mantid::API::AlgorithmProperties::update("GroupingFile", groupingFile(), *properties);
    break;
  case GroupingMethod::Groups:
    Mantid::API::AlgorithmProperties::update("NGroups", std::to_string(nGroups()), *properties);
    break;
  case GroupingMethod::Custom:
    Mantid::API::AlgorithmProperties::update("GroupingString", customGrouping(), *properties);
    break;
  default:
    // No properties to update for 'Individual', 'All' or 'IPF'
    break;
  }
  return properties;
}

void DetectorGroupingOptions::emitSaveCustomGrouping() { emit saveCustomGrouping(customGrouping()); }

int DetectorGroupingOptions::optionIndex(std::string const &option) const {
  auto const index = m_uiForm.cbGroupingOptions->findText(QString::fromStdString(option));
  return index >= 0 ? index : 0;
}

} // namespace CustomInterfaces
} // namespace MantidQt
