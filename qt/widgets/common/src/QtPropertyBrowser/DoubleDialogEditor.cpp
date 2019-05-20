// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h"

#include <QCheckBox>
#include <QDialog>
#include <QDoubleValidator>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>

/**
 * Constructor.
 * @param property :: A property to edit.
 * @param parent :: A widget parent for the editor widget.
 */
DoubleDialogEditor::DoubleDialogEditor(QtProperty *property, QWidget *parent,
                                       bool hasOption, bool isOptionSet)
    : QWidget(parent), m_property(property), m_hasOption(hasOption),
      m_isOptionSet(isOptionSet) {
  QHBoxLayout *layout = new QHBoxLayout;
  m_editor = new DoubleEditor(property, this);
  layout->addWidget(m_editor);
  setFocusProxy(m_editor);
  setFocusPolicy(Qt::StrongFocus);

  m_button = new QPushButton("...", this);
  m_button->setMaximumSize(20, 1000000);
  connect(m_button, SIGNAL(clicked()), this, SLOT(runDialog()));
  layout->addWidget(m_button);
  if (hasOption) {
    m_checkBox = new QCheckBox;
    m_checkBox->setChecked(isOptionSet);
    connect(m_checkBox, SIGNAL(toggled(bool)), this, SLOT(optionToggled(bool)));
    layout->addWidget(m_checkBox);
    if (isOptionSet) {
      m_button->hide();
    }
  }

  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->setStretchFactor(m_button, 0);
  this->setLayout(layout);

  m_editor->installEventFilter(this);
  m_button->installEventFilter(this);
}

bool DoubleDialogEditor::eventFilter(QObject *obj, QEvent *evt) {
  if (evt->type() == QEvent::FocusOut) {
    if (obj == m_editor) {
      if (!m_button->hasFocus()) {
        updateProperty();
        emit closeEditor();
      }
    } else if (obj == m_button) {
      if (!m_editor->hasFocus()) {
        updateProperty();
        emit closeEditor();
      }
    }
  }
  return QWidget::eventFilter(obj, evt);
}

/**
 * Set the text in the editor.
 * @param txt :: A text to set.
 */
void DoubleDialogEditor::setText(const QString &txt) { m_editor->setText(txt); }

/**
 * Get the current text inside the editor.
 */
QString DoubleDialogEditor::getText() const { return m_editor->text(); }

/**
 * Slot which sets the property with the current text in the editor.
 */
void DoubleDialogEditor::updateProperty() {
  auto mgr =
      dynamic_cast<ParameterPropertyManager *>(m_property->propertyManager());
  if (mgr) {
    mgr->setValue(m_property, m_editor->text().toDouble());
    mgr->setGlobal(m_property, m_isOptionSet);
  }
}

void DoubleDialogEditor::runDialog() { emit buttonClicked(m_property); }

void DoubleDialogEditor::optionToggled(bool option) {
  m_button->setVisible(!option);
  m_isOptionSet = option;
  updateProperty();
}

QWidget *DoubleDialogEditorFactory::createEditorForManager(
    ParameterPropertyManager *mgr, QtProperty *property, QWidget *parent) {
  bool isOptionSet = m_hasOption ? mgr->isGlobal(property) : false;
  auto editor =
      new DoubleDialogEditor(property, parent, m_hasOption, isOptionSet);
  connect(editor, SIGNAL(buttonClicked(QtProperty *)), this,
          SIGNAL(buttonClicked(QtProperty *)));
  connect(editor, SIGNAL(closeEditor()), this, SIGNAL(closeEditor()),
          Qt::QueuedConnection);
  return editor;
}
