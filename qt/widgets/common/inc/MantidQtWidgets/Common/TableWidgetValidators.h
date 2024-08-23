// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
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

enum RegexValidatorStrings { MaskValidator, SpectraValidator };

EXPORT_OPT_MANTIDQT_COMMON std::string getRegexValidatorString(const RegexValidatorStrings &validatorMask);
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
  NumericInputDelegate(QWidget *parent = nullptr, int = DEFAULT_NUMERICAL_PRECISION);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;

private:
  int m_precision;
};

} // namespace MantidWidgets
} // namespace MantidQt
