// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DoubleSpinBox.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <cfloat>
#include <cmath>

DoubleSpinBox::DoubleSpinBox(const char format, QWidget *parent)
    : QAbstractSpinBox(parent), d_format(format), d_min_val(-DBL_MAX), d_max_val(DBL_MAX), d_value(0.0), d_step(0.1),
      d_prec(14) {
  if (format == 'f')
    d_prec = 1;

  setFocusPolicy(Qt::StrongFocus);
  lineEdit()->setText(locale().toString(d_value, d_format, d_prec));
  setWrapping(false);
  connect(this, SIGNAL(editingFinished()), this, SLOT(interpretText()));
}

void DoubleSpinBox::setSingleStep(double val) {
  if (d_step != val && val < d_max_val)
    d_step = val;
}

void DoubleSpinBox::setMaximum(double max) {
  if (max == d_max_val || max > DBL_MAX)
    return;

  d_max_val = max;
}

double DoubleSpinBox::getMaximum() { return d_max_val; }

void DoubleSpinBox::setMinimum(double min) {
  if (min == d_min_val || min < -DBL_MAX)
    return;

  d_min_val = min;
}

double DoubleSpinBox::getMinimum() { return d_min_val; }

void DoubleSpinBox::setRange(double min, double max) {
  setMinimum(min);
  setMaximum(max);
}

/**
 * Interpret the text and update the stored value.
 * @param notify If true then emit signals to indicate if the value has changed
 * (default=true)
 * The default is important so that connected signals ensure the correct updates
 * are pushed
 * through but we need to be able to turn them off as there are cases where this
 * causes
 * a recursive call.
 */
void DoubleSpinBox::interpretText(bool notify) {
  // RJT: Keep our version of this, which contains a bug fix (see [10521]).
  // Also, there are lines referring to methods that don't exist in our (older)
  // MyParser class.
  bool ok = false;
  double value = locale().toDouble(text(), &ok);
  if (ok && setValue(value)) {
    if (notify)
      emit valueChanged(d_value);
  } else {
    QString val = text().remove(",");
    value = locale().toDouble(val, &ok);
    if (ok && setValue(value)) {
      if (notify)
        emit valueChanged(d_value);
    } else {
      // Check for any registered test strings that map to a given value
      for (auto &specialTextMapping : m_specialTextMappings) {
        if (specialTextMapping.first == text()) {
          // Found a matching string, try to set the value
          if (setValue(specialTextMapping.second)) {
            lineEdit()->setText(text());
            if (notify)
              emit valueChanged(d_value);
          }
        }
      }

      lineEdit()->setText(textFromValue(d_value));
    }
  }
}

/**
 * Adds a mapping from string whihc may be entered into the edit box and a
 *double value.
 * The mapping is case sensitive
 *
 * @param text QString with text to map
 * @param value Value to map it to
 */
void DoubleSpinBox::addSpecialTextMapping(const QString &text, double value) { m_specialTextMappings[text] = value; }

void DoubleSpinBox::stepBy(int steps) {
  double val = d_value + steps * d_step;
  if (fabs(fabs(d_value) - d_step) < 1e-14 && d_value * steps < 0) // possible zero
    val = 0.0;

  if (setValue(val))
    emit valueChanged(d_value);
}

QAbstractSpinBox::StepEnabled DoubleSpinBox::stepEnabled() const {
  QAbstractSpinBox::StepEnabled stepDown = QAbstractSpinBox::StepNone;
  if (d_value > d_min_val)
    stepDown = StepDownEnabled;

  QAbstractSpinBox::StepEnabled stepUp = QAbstractSpinBox::StepNone;
  if (d_value < d_max_val)
    stepUp = StepUpEnabled;

  return stepDown | stepUp;
}

double DoubleSpinBox::value() {
  const bool notify(false);
  interpretText(notify);
  return d_value;
}

bool DoubleSpinBox::setValue(double val) {
  if (val >= d_min_val && val <= d_max_val) {
    d_value = val;
    lineEdit()->setText(textFromValue(d_value));
    return true;
  }

  lineEdit()->setText(textFromValue(d_value));
  return false;
}

QString DoubleSpinBox::textFromValue(double value) const {
  if (!specialValueText().isEmpty() && value == d_min_val)
    return specialValueText();

  if (d_prec <= 14)
    return locale().toString(value, d_format, d_prec);

  return locale().toString(value, d_format, 6);
}

QValidator::State DoubleSpinBox::validate(QString & /*input*/, int & /*pos*/) const { return QValidator::Acceptable; }

void DoubleSpinBox::focusInEvent(QFocusEvent *e) {
  emit activated(this);
  return QAbstractSpinBox::focusInEvent(e);
}

/*****************************************************************************
 *
 * Class RangeLimitBox
 *
 *****************************************************************************/

RangeLimitBox::RangeLimitBox(LimitType type, QWidget *parent) : QWidget(parent), d_type(type) {
  d_checkbox = new QCheckBox();
  d_spin_box = new DoubleSpinBox();
  d_spin_box->setSpecialValueText(" ");
  d_spin_box->setValue(-DBL_MAX);
  d_spin_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  d_spin_box->setEnabled(false);

  auto *l = new QHBoxLayout(this);
  l->setMargin(0);
  l->setSpacing(0);
  l->addWidget(d_checkbox);
  l->addWidget(d_spin_box);

  setFocusPolicy(Qt::StrongFocus);
  setFocusProxy(d_spin_box);
  connect(d_checkbox, SIGNAL(toggled(bool)), d_spin_box, SLOT(setEnabled(bool)));
}

double RangeLimitBox::value() {
  if (d_checkbox->isChecked())
    return d_spin_box->value();

  double val = -DBL_MAX;
  if (d_type == RightLimit)
    return DBL_MAX;
  return val;
}
