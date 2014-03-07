#include "DoubleEditorFactory.h"

#include "ParameterPropertyManager.h"

#include <QDoubleValidator>

#include <iostream>
#include <cfloat>
#include <sstream>
#include <cmath>
#include <stdexcept>


DoubleEditor::DoubleEditor(QtProperty *property, QWidget *parent)
:QLineEdit(parent),
m_property(property)
{
  auto mgr = dynamic_cast<QtDoublePropertyManager*>(property->propertyManager());
  if (!mgr)
  {
    throw std::runtime_error("QtDoublePropertyManager expected as parent of DoubleEditor");
  }

  m_decimals = mgr->decimals(property);
  setValidator(new QDoubleValidator(
                  mgr->minimum(property),
                  mgr->maximum(property),
                  20,
                  this));
  connect(this,SIGNAL(editingFinished()),this,SLOT(updateProperty()));
  //double val = mgr->value(property);
  setValue(mgr->value(property));
}

DoubleEditor::~DoubleEditor()
{
}

void DoubleEditor::setValue(const double& d)
{
  double absVal = fabs(d);
  char format = absVal > 1e5 || (absVal != 0 && absVal < 1e-5) ? 'e' : 'f';
  setText(QString::number(d,format , m_decimals));
}

void DoubleEditor::updateProperty()
{
  auto mgr = dynamic_cast<QtDoublePropertyManager*>(m_property->propertyManager());
  if (mgr)
  {
    mgr->setValue(m_property,text().toDouble());
  }
}

void ParameterEditor::updateProperty()
{
  auto mgr = dynamic_cast<ParameterPropertyManager*>(m_property->propertyManager());
  if (mgr)
  {
    // As the property get supdated, the error becomes invalid, so clear it
    mgr->clearError(m_property);
  }

  // XXX: this should be done AFTER the error was cleared, because only value change causes property
  //      view to get updated
  DoubleEditor::updateProperty();
}
