// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/InterfaceUtils.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <QDomDocument>
#include <QFile>
#include <QMessageBox>
#include <QtXml>
#include <boost/algorithm/string.hpp>

namespace {
Mantid::Kernel::Logger g_log("InterfaceUtils");

std::string OR(const std::string &lhs, const std::string &rhs) { return "(" + lhs + "|" + rhs + ")"; }
std::string NATURAL_NUMBER_WITH_PRECISION(std::size_t digits) {
  return OR("0", "[1-9][0-9]{," + std::to_string(digits - 1) + "}");
}
// non numeric characters
const std::string EMPTY = "^$";
const std::string SPACE = "(\\s)*";
const std::string COMMA = SPACE + "," + SPACE;
const std::string DASH = "\\-";

// number and numeric sets
const std::string NATURAL_NUMBER = "(0|[1-9][0-9]*)";
const std::string REAL_NUMBER = "(-?" + NATURAL_NUMBER + "(\\.[0-9]*)?)";
const std::string REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const std::string NUMBER = NATURAL_NUMBER_WITH_PRECISION(4);
const std::string NATURAL_RANGE = "(" + NUMBER + DASH + NUMBER + ")";
const std::string NATURAL_OR_RANGE = OR(NATURAL_RANGE, NUMBER);

// final lists
const std::string MASK_LIST = "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
const std::string SPECTRA_LIST = "(" + NATURAL_OR_RANGE + "(" + COMMA + NATURAL_OR_RANGE + ")*)";

constexpr auto NUMERICAL_PRECISION = 6;

} // namespace

namespace MantidQt {
namespace MantidWidgets {
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

std::string getRegexValidatorString(const regexValidatorStrings &validatorMask) {
  switch (validatorMask) {
  case regexValidatorStrings::SpectraValidator:
    return SPECTRA_LIST;
  case regexValidatorStrings::MaskValidator:
    return MASK_LIST;
  default:
    return SPECTRA_LIST;
  }
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

RegexInputDelegate::RegexInputDelegate(QWidget *parent, const std::string &validator)
    : QStyledItemDelegate(parent), m_validator(QRegExp(QString::fromStdString(validator))){};

QWidget *RegexInputDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                                          const QModelIndex & /*index*/) const {
  auto lineEdit = new QLineEdit(parent);
  auto validator = new QRegExpValidator(m_validator, parent);
  lineEdit->setValidator(validator);
  return lineEdit;
}

NumericInputDelegate::NumericInputDelegate(QWidget *parent, double precision)
    : QStyledItemDelegate(parent), m_precision(precision){};

QWidget *NumericInputDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {

  auto lineEdit = new QLineEdit(parent);
  auto validator = new QDoubleValidator(parent);

  validator->setDecimals(m_precision);
  validator->setNotation(QDoubleValidator::StandardNotation);
  lineEdit->setValidator(validator);
  return lineEdit;
}

void NumericInputDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  const auto value = index.model()->data(index, Qt::EditRole).toDouble();
  static_cast<QLineEdit *>(editor)->setText(makeQStringNumber(value, m_precision));
}

} // namespace InterfaceUtils
} // namespace MantidWidgets
} // namespace MantidQt
