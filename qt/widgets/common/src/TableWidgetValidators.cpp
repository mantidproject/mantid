// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/TableWidgetValidators.h"
#include <stdexcept>

namespace {

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
} // namespace

namespace MantidQt {
namespace MantidWidgets {

std::string getRegexValidatorString(const RegexValidatorStrings &validatorMask) {
  switch (validatorMask) {
  case RegexValidatorStrings::SpectraValidator:
    return SPECTRA_LIST;
  case RegexValidatorStrings::MaskValidator:
    return MASK_LIST;
  default:
    throw std::logic_error("Invalid or Missing Validator String");
  }
}

QString makeQStringNumber(double value, int precision) { return QString::number(value, 'f', precision); }

RegexInputDelegate::RegexInputDelegate(QWidget *parent, const std::string &validator)
    : QStyledItemDelegate(parent), m_validator(QRegExp(QString::fromStdString(validator))) {}

QWidget *RegexInputDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                                          const QModelIndex & /*index*/) const {
  auto lineEdit = new QLineEdit(parent);
  auto validator = new QRegExpValidator(m_validator, parent);
  lineEdit->setValidator(validator);
  return lineEdit;
}

NumericInputDelegate::NumericInputDelegate(QWidget *parent, int precision)
    : QStyledItemDelegate(parent), m_precision(precision) {}

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

} // namespace MantidWidgets
} // namespace MantidQt
