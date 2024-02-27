// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DetectorGroupingOptions.h"
#include "Common/InterfaceUtils.h"
#include "MantidAPI/AlgorithmProperties.h"

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

void DetectorGroupingOptions::setGroupingMethod(QString const &option) {
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

std::string DetectorGroupingOptions::mapFile() const { return m_uiForm.dsMapFile->getFirstFilename().toStdString(); }

std::string DetectorGroupingOptions::customGrouping() const { return m_uiForm.leCustomGroups->text().toStdString(); }

int DetectorGroupingOptions::nGroups() const { return m_uiForm.spNumberGroups->value(); }

std::optional<std::string> DetectorGroupingOptions::validateGroupingProperties(std::size_t const &spectraMin,
                                                                               std::size_t const &spectraMax) const {
  auto const properties = groupingProperties();
  std::string groupingType = properties->getProperty("GroupingMethod");
  if (groupingType == "File" && !properties->existsProperty("MapFile")) {
    return "Please supply a map file for grouping detectors.";
  } else if (groupingType == "Custom") {
    if (!properties->existsProperty("GroupingString")) {
      return "Please supply a custom string for grouping detectors.";
    } else {
      std::string customString = properties->getProperty("GroupingString");
      return InterfaceUtils::groupingStrInRange(customString, spectraMin, spectraMax)
                 ? std::nullopt
                 : std::optional<std::string>("Please supply a custom grouping within the correct range.");
    }
  } else if (groupingType == "Groups") {
    auto const numberOfSpectra = spectraMax - spectraMin + 1;
    auto const nGroups = std::stoull(properties->getPropertyValue("NGroups"));
    if (nGroups > numberOfSpectra) {
      return "The number of groups must be less or equal to the number of spectra (" + std::to_string(numberOfSpectra) +
             ").";
    }
  }
  return std::nullopt;
}

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> DetectorGroupingOptions::groupingProperties() const {
  auto const method = groupingMethod();
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  Mantid::API::AlgorithmProperties::update("GroupingMethod", method, *properties);
  switch (GROUPING_METHODS[method]) {
  case GroupingMethod::File:
    Mantid::API::AlgorithmProperties::update("MapFile", mapFile(), *properties);
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

int DetectorGroupingOptions::optionIndex(QString const &option) const {
  auto const index = m_uiForm.cbGroupingOptions->findText(option);
  return index >= 0 ? index : 0;
}

} // namespace CustomInterfaces
} // namespace MantidQt