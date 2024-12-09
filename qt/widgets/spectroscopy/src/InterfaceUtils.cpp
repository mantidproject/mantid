// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"
#include <QDomDocument>
#include <QFile>
#include <QMessageBox>
#include <QtXml>
#include <boost/algorithm/string.hpp>

namespace {
Mantid::Kernel::Logger g_log("InterfaceUtils");

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace InterfaceUtils {

/**
 * General functions used in inelastic/indirect tabs.
 */

static QStringList toQStringList(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  return MantidWidgets::stdVectorToQStringList(subStrings);
}

static std::string getAttributeFromTag(QDomElement const &tag, QString const &attribute, QString const &defaultValue) {
  if (tag.hasAttribute(attribute))
    return tag.attribute(attribute, defaultValue).toStdString();
  return defaultValue.toStdString();
}

static bool hasCorrectAttribute(QDomElement const &child, std::string const &attributeName,
                                std::string const &searchValue) {
  auto const name = QString::fromStdString(attributeName);
  return child.hasAttribute(name) && child.attribute(name).toStdString() == searchValue;
}

static std::string getInterfaceAttribute(QDomElement const &root, std::string const &interfaceName,
                                         std::string const &propertyName, std::string const &attribute) {
  // Loop through interfaces
  auto interfaceChild = root.firstChild().toElement();
  while (!interfaceChild.isNull()) {
    if (hasCorrectAttribute(interfaceChild, "id", interfaceName)) {

      // Loop through interface properties
      auto propertyChild = interfaceChild.firstChild().toElement();
      while (!propertyChild.isNull()) {

        // Return value of an attribute of the property if it is found
        if (propertyChild.tagName().toStdString() == propertyName)
          return getAttributeFromTag(propertyChild, QString::fromStdString(attribute), "");

        propertyChild = propertyChild.nextSibling().toElement();
      }
    }
    interfaceChild = interfaceChild.nextSibling().toElement();
  }
  return "";
}

/// The function to use to check whether input data should be restricted based on its name.
/// This is defined, rather than calling SettingsHelper::restrictInputDataByName() directly, to make it
/// possible to override it in tests in order to mock out the SettingsHelper.
std::function<bool()> restrictInputDataByName = SettingsHelper::restrictInputDataByName;

std::string getInterfaceProperty(std::string const &interfaceName, std::string const &propertyName,
                                 std::string const &attribute) {
  QFile file(":/interface-properties.xml");
  if (file.open(QIODevice::ReadOnly)) {
    QDomDocument xmlBOM;
    xmlBOM.setContent(&file);
    return getInterfaceAttribute(xmlBOM.documentElement(), interfaceName, propertyName, attribute);
  }
  g_log.warning("There was an error while loading InterfaceProperties.xml.");
  return "";
}

QStringList getExtensions(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "EXTENSIONS", "all"), ",");
}

QStringList getCalibrationExtensions(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "EXTENSIONS", "calibration"), ",");
}

QStringList getFBSuffixes(std::string const &interfaceName, std::string const &fileType) {
  if (!restrictInputDataByName()) {
    return getExtensions(interfaceName);
  }
  return toQStringList(getInterfaceProperty(interfaceName, "FILE-SUFFIXES", fileType), ",");
}

QStringList getWSSuffixes(std::string const &interfaceName, std::string const &fileType) {
  if (!restrictInputDataByName()) {
    return QStringList{};
  }
  return toQStringList(getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", fileType), ",");
}

QStringList getSampleFBSuffixes(std::string const &interfaceName) { return getFBSuffixes(interfaceName, "sample"); }

QStringList getSampleWSSuffixes(std::string const &interfaceName) { return getWSSuffixes(interfaceName, "sample"); }

QStringList getVanadiumFBSuffixes(std::string const &interfaceName) { return getFBSuffixes(interfaceName, "vanadium"); }

QStringList getVanadiumWSSuffixes(std::string const &interfaceName) { return getWSSuffixes(interfaceName, "vanadium"); }

QStringList getResolutionFBSuffixes(std::string const &interfaceName) {
  return getFBSuffixes(interfaceName, "resolution");
}

QStringList getResolutionWSSuffixes(std::string const &interfaceName) {
  return getWSSuffixes(interfaceName, "resolution");
}

QStringList getCalibrationFBSuffixes(std::string const &interfaceName) {
  return getFBSuffixes(interfaceName, "calibration");
}

QStringList getCalibrationWSSuffixes(std::string const &interfaceName) {
  return getWSSuffixes(interfaceName, "calibration");
}

QStringList getContainerFBSuffixes(std::string const &interfaceName) {
  return getFBSuffixes(interfaceName, "container");
}

QStringList getContainerWSSuffixes(std::string const &interfaceName) {
  return getWSSuffixes(interfaceName, "container");
}

QStringList getCorrectionsFBSuffixes(std::string const &interfaceName) {
  return getFBSuffixes(interfaceName, "corrections");
}

QStringList getCorrectionsWSSuffixes(std::string const &interfaceName) {
  return getWSSuffixes(interfaceName, "corrections");
}

std::pair<double, double> convertTupleToPair(std::tuple<double, double> const &doubleTuple) {
  return std::make_pair(std::get<0>(doubleTuple), std::get<1>(doubleTuple));
}

QString makeQStringNumber(double value, int precision) { return QString::number(value, 'f', precision); }

/**
 * Sets the edge bounds of plot to prevent the user inputting invalid values
 * Also sets limits for range selector movement
 *
 * @param dblPropertyManager :: Pointer to the Qt Double Property Manager.
 * @param rs :: Pointer to the RangeSelector
 * @param min :: The lower bound property in the property browser
 * @param max :: The upper bound property in the property browser
 * @param bounds :: The upper and lower bounds to be set
 */
void setPlotPropertyRange(QtDoublePropertyManager *dblPropertyManager, MantidWidgets::RangeSelector *rs,
                          QtProperty *min, QtProperty *max, const std::pair<double, double> &bounds) {
  dblPropertyManager->setRange(min, bounds.first, bounds.second);
  dblPropertyManager->setRange(max, bounds.first, bounds.second);
  rs->setBounds(bounds.first, bounds.second);
}

/**
 * Set the position of the range selectors on the mini plot
 *
 * @param dblPropertyManager :: Pointer to the Qt Double Property Manager.
 * @param rs :: Pointer to the RangeSelector
 * @param lower :: The lower bound property in the property browser
 * @param upper :: The upper bound property in the property browser
 * @param bounds :: The upper and lower bounds to be set
 * @param range :: The range to set the range selector to.
 */
void setRangeSelector(QtDoublePropertyManager *dblPropertyManager, MantidWidgets::RangeSelector *rs, QtProperty *lower,
                      QtProperty *upper, const std::pair<double, double> &range,
                      const std::optional<std::pair<double, double>> &bounds) {
  dblPropertyManager->setValue(lower, range.first);
  dblPropertyManager->setValue(upper, range.second);
  rs->setRange(range.first, range.second);
  if (bounds) {
    // clamp the bounds of the selector
    const auto values = bounds.value();
    rs->setBounds(values.first, values.second);
  }
}

/**
 * Set the minimum of a range selector if it is less than the maximum value.
 * To be used when changing the min or max via the Property table
 *
 * @param dblPropertyManager :: Pointer to the Qt Double Property Manager.
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the minimum
 */
void setRangeSelectorMin(QtDoublePropertyManager *dblPropertyManager, QtProperty *minProperty, QtProperty *maxProperty,
                         MantidWidgets::RangeSelector *rangeSelector, double newValue) {
  if (newValue <= maxProperty->valueText().toDouble())
    rangeSelector->setMinimum(newValue);
  else
    dblPropertyManager->setValue(minProperty, rangeSelector->getMinimum());
}

/**
 * Set the maximum of a range selector if it is greater than the minimum value
 * To be used when changing the min or max via the Property table
 *
 * @param dblPropertyManager :: Pointer to the Qt Double Property Manager.
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the maximum
 */
void setRangeSelectorMax(QtDoublePropertyManager *dblPropertyManager, QtProperty *minProperty, QtProperty *maxProperty,
                         MantidWidgets::RangeSelector *rangeSelector, double newValue) {
  if (newValue >= minProperty->valueText().toDouble())
    rangeSelector->setMaximum(newValue);
  else
    dblPropertyManager->setValue(maxProperty, rangeSelector->getMaximum());
}

bool checkADSForPlotSaveWorkspace(const std::string &workspaceName, const bool plotting, const bool warn) {
  const auto workspaceExists = Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName);
  if (warn && !workspaceExists) {
    const std::string plotSave = plotting ? "plotting" : "saving";
    const auto errorMessage =
        "Error while " + plotSave + ":\nThe workspace \"" + workspaceName + "\" could not be found.";
    const char *textMessage = errorMessage.c_str();
    QMessageBox::warning(nullptr, QObject::tr("Indirect "), QObject::tr(textMessage));
  }
  return workspaceExists;
}

} // namespace InterfaceUtils
} // namespace CustomInterfaces
} // namespace MantidQt
