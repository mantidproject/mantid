// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InterfaceUtils.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <QDomDocument>
#include <QFile>
#include <QMessageBox>
#include <QtXml>
#include <boost/algorithm/string.hpp>

namespace {
Mantid::Kernel::Logger g_log("InterfaceUtils");

bool isWithinRange(std::size_t const &spectraNumber, std::size_t const &spectraMin, std::size_t const &spectraMax) {
  return spectraMax != 0 && spectraNumber >= spectraMin && spectraNumber <= spectraMax;
}

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

QStringList getSampleFBSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "sample"), ",");
}

QStringList getSampleWSSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "sample"), ",");
}

QStringList getVanadiumFBSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "vanadium"), ",");
}

QStringList getVanadiumWSSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "vanadium"), ",");
}

QStringList getResolutionFBSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "resolution"), ",");
}

QStringList getResolutionWSSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "resolution"), ",");
}

QStringList getCalibrationFBSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "calibration"), ",");
}

QStringList getCalibrationWSSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "calibration"), ",");
}

QStringList getContainerFBSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "container"), ",");
}

QStringList getContainerWSSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "container"), ",");
}

QStringList getCorrectionsFBSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "corrections"), ",");
}

QStringList getCorrectionsWSSuffixes(std::string const &interfaceName) {
  return toQStringList(getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "corrections"), ",");
}

QPair<double, double> convertTupleToQPair(std::tuple<double, double> const &doubleTuple) {
  return QPair<double, double>(std::get<0>(doubleTuple), std::get<1>(doubleTuple));
}

std::pair<double, double> convertTupleToPair(std::tuple<double, double> const &doubleTuple) {
  return std::make_pair(std::get<0>(doubleTuple), std::get<1>(doubleTuple));
}

QString makeQStringNumber(double value, int precision) { return QString::number(value, 'f', precision); }

bool groupingStrInRange(std::string const &customString, std::size_t const &spectraMin, std::size_t const &spectraMax) {
  if (customString.empty()) {
    return false;
  }

  std::vector<std::string> groupingStrings;
  std::vector<std::size_t> groupingNumbers;
  // Split the custom string by its delimiters
  boost::split(groupingStrings, customString, boost::is_any_of(" ,-+:"));
  // Remove empty strings
  groupingStrings.erase(std::remove_if(groupingStrings.begin(), groupingStrings.end(),
                                       [](std::string const &str) { return str.empty(); }),
                        groupingStrings.end());
  // Transform strings to size_t's
  std::transform(groupingStrings.cbegin(), groupingStrings.cend(), std::back_inserter(groupingNumbers),
                 [](std::string const &str) { return std::stoull(str); });
  // Find min and max elements
  auto const range = std::minmax_element(groupingNumbers.cbegin(), groupingNumbers.cend());
  return isWithinRange(*range.first, spectraMin, spectraMax) && isWithinRange(*range.second, spectraMin, spectraMax);
}

} // namespace InterfaceUtils
} // namespace CustomInterfaces
} // namespace MantidQt
