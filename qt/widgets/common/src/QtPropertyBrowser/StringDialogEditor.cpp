// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/QtPropertyBrowser/StringDialogEditor.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>

/**
 * Do nothing to connect a manager.
 */
void StringDialogEditorFactory::connectPropertyManager(QtStringPropertyManager * /*manager*/) {}

/**
 * Do nothing to disconnect a manager - it was never connected.
 */
void StringDialogEditorFactory::disconnectPropertyManager(QtStringPropertyManager * /*manager*/) {}

/**
 * Constructor.
 * @param property :: A property to edit.
 * @param parent :: A widget parent for the editor widget.
 */
StringDialogEditor::StringDialogEditor(QtProperty *property, QWidget *parent) : QWidget(parent), m_property(property) {
  auto *layout = new QHBoxLayout;
  m_lineEdit = new QLineEdit(this);
  layout->addWidget(m_lineEdit);
  setFocusProxy(m_lineEdit);
  connect(m_lineEdit, SIGNAL(editingFinished()), this, SLOT(updateProperty()));
  auto *mgr = dynamic_cast<QtStringPropertyManager *>(property->propertyManager());
  if (mgr) {
    m_lineEdit->setText(mgr->value(property));
  }

  auto *button = new QPushButton("...", this);
  button->setMaximumSize(20, 1000000);
  connect(button, SIGNAL(clicked()), this, SLOT(runDialog()));
  layout->addWidget(button);
  layout->setContentsMargins(0, 0, 20, 0);
  layout->setStretchFactor(button, 0);
  this->setLayout(layout);
}

/**
 * Set the text in the editor.
 * @param txt :: A text to set.
 */
void StringDialogEditor::setText(const QString &txt) { m_lineEdit->setText(txt); }

/**
 * Get the current text inside the editor.
 */
QString StringDialogEditor::getText() const { return m_lineEdit->text(); }

StringDialogEditor::~StringDialogEditor() = default;

/**
 * Slot which sets the property with the current text in the editor.
 */
void StringDialogEditor::updateProperty() {
  auto *mgr = dynamic_cast<QtStringPropertyManager *>(m_property->propertyManager());
  if (mgr) {
    mgr->setValue(m_property, m_lineEdit->text());
  }
}
