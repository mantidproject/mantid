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
    QtStringPropertyManager * /*manager*/, QtProperty *property, QWidget *parent) {
  return new WorkspaceEditor(property, parent);
}

WorkspaceEditor::WorkspaceEditor(QtProperty *property, QWidget *parent)
    : WorkspaceSelector(parent), m_property(property) {
  this->insertItem(0, "");
  updateProperty(this->itemText(0));
  this->setCurrentIndex(0);
  connect(this, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(updateProperty(const QString &)));
}

void WorkspaceEditor::updateProperty(const QString &text) {
  QtStringPropertyManager *mgr =
      dynamic_cast<QtStringPropertyManager *>(m_property->propertyManager());
  if (mgr) {
    mgr->setValue(m_property, text);
  }
}
} // namespace MantidWidgets
} // namespace MantidQt
