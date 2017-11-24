#include "MantidQtWidgets/Common/QtPropertyBrowser/StringEditorFactory.h"

QWidget *StringEditorFactory::createEditorForManager(QtStringPropertyManager *,
                                                     QtProperty *property,
                                                     QWidget *parent) {
  return new StringEditor(property, parent);
}

StringEditor::StringEditor(QtProperty *property, QWidget *parent)
    : QLineEdit(parent), m_property(property) {
  connect(this, SIGNAL(editingFinished()), this, SLOT(updateProperty()));
  QtStringPropertyManager *mgr =
      dynamic_cast<QtStringPropertyManager *>(property->propertyManager());
  if (mgr) {
    setText(mgr->value(property));
  }
}

void StringEditor::updateProperty() {
  QtStringPropertyManager *mgr =
      dynamic_cast<QtStringPropertyManager *>(m_property->propertyManager());
  if (mgr) {
    mgr->setValue(m_property, this->text());
  }
}
