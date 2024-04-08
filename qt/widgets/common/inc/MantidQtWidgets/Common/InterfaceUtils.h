// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllOption.h"
#include <QLineEdit>
#include <qstyleditemdelegate.h>
#include <string>

namespace {
const std::string DEFAULT_REGEX = "(0|[1-9][0-9]*)";
constexpr auto DEFAULT_NUMERICAL_PRECISION = 6;
} // namespace

namespace MantidQt {
namespace MantidWidgets {
namespace InterfaceUtils {

enum regexValidatorStrings { MaskValidator, SpectraValidator };

EXPORT_OPT_MANTIDQT_COMMON std::string getRegexValidatorString(const regexValidatorStrings &validatorMask);

EXPORT_OPT_MANTIDQT_COMMON std::string
getInterfaceProperty(std::string const &interfaceName, std::string const &propertyName, std::string const &attribute);

EXPORT_OPT_MANTIDQT_COMMON QStringList getExtensions(std::string const &interfaceName);
EXPORT_OPT_MANTIDQT_COMMON QStringList getCalibrationExtensions(std::string const &interfaceName);

EXPORT_OPT_MANTIDQT_COMMON QStringList getSampleFBSuffixes(std::string const &interfaceName);
EXPORT_OPT_MANTIDQT_COMMON QStringList getSampleWSSuffixes(std::string const &interfaceName);

EXPORT_OPT_MANTIDQT_COMMON QStringList getVanadiumFBSuffixes(std::string const &interfaceName);
EXPORT_OPT_MANTIDQT_COMMON QStringList getVanadiumWSSuffixes(std::string const &interfaceName);

EXPORT_OPT_MANTIDQT_COMMON QStringList getResolutionFBSuffixes(std::string const &interfaceName);
EXPORT_OPT_MANTIDQT_COMMON QStringList getResolutionWSSuffixes(std::string const &interfaceName);

EXPORT_OPT_MANTIDQT_COMMON QStringList getCalibrationFBSuffixes(std::string const &interfaceName);
EXPORT_OPT_MANTIDQT_COMMON QStringList getCalibrationWSSuffixes(std::string const &interfaceName);

EXPORT_OPT_MANTIDQT_COMMON QStringList getContainerFBSuffixes(std::string const &interfaceName);
EXPORT_OPT_MANTIDQT_COMMON QStringList getContainerWSSuffixes(std::string const &interfaceName);

EXPORT_OPT_MANTIDQT_COMMON QStringList getCorrectionsFBSuffixes(std::string const &interfaceName);
EXPORT_OPT_MANTIDQT_COMMON QStringList getCorrectionsWSSuffixes(std::string const &interfaceName);

EXPORT_OPT_MANTIDQT_COMMON QPair<double, double> convertTupleToQPair(std::tuple<double, double> const &doubleTuple);
EXPORT_OPT_MANTIDQT_COMMON std::pair<double, double> convertTupleToPair(std::tuple<double, double> const &doubleTuple);
EXPORT_OPT_MANTIDQT_COMMON QString makeQStringNumber(double value, int precision);

class EXPORT_OPT_MANTIDQT_COMMON RegexInputDelegate : public QStyledItemDelegate {
public:
  RegexInputDelegate(QWidget *parent = nullptr, const std::string &validator = DEFAULT_REGEX);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
  QRegExp m_validator;
};

class EXPORT_OPT_MANTIDQT_COMMON NumericInputDelegate : public QStyledItemDelegate {
public:
  NumericInputDelegate(QWidget *parent = nullptr, double = DEFAULT_NUMERICAL_PRECISION);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;

private:
  double m_precision;
};
} // namespace InterfaceUtils
} // namespace MantidWidgets
} // namespace MantidQt
