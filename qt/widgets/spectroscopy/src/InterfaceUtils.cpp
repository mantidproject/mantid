// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
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

QPair<double, double> convertTupleToQPair(std::tuple<double, double> const &doubleTuple) {
  return QPair<double, double>(std::get<0>(doubleTuple), std::get<1>(doubleTuple));
}

std::pair<double, double> convertTupleToPair(std::tuple<double, double> const &doubleTuple) {
  return std::make_pair(std::get<0>(doubleTuple), std::get<1>(doubleTuple));
}

QString makeQStringNumber(double value, int precision) { return QString::number(value, 'f', precision); }

} // namespace InterfaceUtils
} // namespace CustomInterfaces
} // namespace MantidQt
