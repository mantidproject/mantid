// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/ParameterPropertyManager.h"

#include <QDoubleValidator>

#include <cfloat>
#include <cmath>
#include <sstream>
#include <stdexcept>

DoubleEditor::DoubleEditor(QtProperty *property, QWidget *parent) : QLineEdit(parent), m_property(property) {
  auto const *mgr = dynamic_cast<QtDoublePropertyManager const *>(property->propertyManager());
  if (!mgr) {
    throw std::runtime_error("QtDoublePropertyManager expected as parent of DoubleEditor");
  }

  m_decimals = mgr->decimals(property);
  setValidator(new QDoubleValidator(mgr->minimum(property), mgr->maximum(property), 20, this));
  connect(this, SIGNAL(editingFinished()), this, SLOT(updateProperty()));
  setValue(mgr->value(property));
}

DoubleEditor::~DoubleEditor() = default;

void DoubleEditor::setValue(const double &d) { setText(formatValue(d)); }

void DoubleEditor::updateProperty() {
  if (!this->isModified())
    return;
  // Ignore second signal if two were triggered by pressing the Enter key
  // see https://bugreports.qt.io/browse/QTBUG-40
  this->setModified(false);

  auto mgr = dynamic_cast<QtDoublePropertyManager *>(m_property->propertyManager());
  if (mgr)
    mgr->setValue(m_property, text().toDouble());
}

/**
 * @param d :: The value to format
 * @return Formatted string representation of the value
 */
QString DoubleEditor::formatValue(const double &d) const {
  // XXX: this is taken from QtDoublePropertyManager. Ideally,
  // QtDoublePropertyManager should be
  //      refactored to haves such a method, but it doesn't seem to be a good
  //      idea to edit it heavily.
  double absVal = fabs(d);
  char format = absVal > 1e5 || (absVal != 0 && absVal < 1e-5) ? 'e' : 'f';
  return QString::number(d, format, m_decimals);
}

void ParameterEditor::updateProperty() {
  auto mgr = dynamic_cast<ParameterPropertyManager *>(m_property->propertyManager());
  if (mgr) {
    // To find out whether the value was really changed, we format it and
    // compare string values. This
    // ensures that we don't suffer from double precision loss and consider only
    // real changes in value

    QString prevVal = formatValue(mgr->value(m_property));
    QString newVal = formatValue(text().toDouble());

    if (prevVal != newVal) {
      // As the value was changed, the error becomes invalid, so clear it
      mgr->clearError(m_property);
    }
  }

  DoubleEditor::updateProperty();
}
