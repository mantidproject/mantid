// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/QtPropertyBrowser/WorkspaceEditorFactory.h"

namespace MantidQt {
namespace MantidWidgets {

QWidget *WorkspaceEditorFactory::createEditorForManager(
    QtStringPropertyManager *manager, QtProperty *property, QWidget *parent) {
  return new WorkspaceEditor(property, manager, parent);
}

WorkspaceEditor::WorkspaceEditor(QtProperty *property,
                                 QtStringPropertyManager *manager,
                                 QWidget *parent)
    : WorkspaceSelector(parent), m_property(property), m_manager(manager) {
  this->insertItem(0, "");
  auto currentValue = m_manager->value(m_property);
  this->setCurrentIndex(this->findText(currentValue));
  connect(this, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(updateProperty(const QString &)));
}

void WorkspaceEditor::updateProperty(const QString &text) {
  m_manager->setValue(m_property, text);
}
} // namespace MantidWidgets
} // namespace MantidQt
