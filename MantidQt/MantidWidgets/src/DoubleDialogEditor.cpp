#include "MantidQtMantidWidgets/DoubleDialogEditor.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialog>
#include <QSettings>
#include <QDoubleValidator>
#include <QEvent>

/**
 * Constructor.
 * @param property :: A property to edit.
 * @param parent :: A widget parent for the editor widget.
 */
DoubleDialogEditor::DoubleDialogEditor(QtProperty *property, QWidget *parent)
    : QWidget(parent), m_property(property) {
  QHBoxLayout *layout = new QHBoxLayout;
  m_editor = new DoubleEditor(property, this);
  layout->addWidget(m_editor);
  setFocusProxy(m_editor);
  setFocusPolicy(Qt::StrongFocus);

  m_button = new QPushButton("...", this);
  m_button->setMaximumSize(20, 1000000);
  connect(m_button, SIGNAL(clicked()), this, SLOT(runDialog()));
  layout->addWidget(m_button);
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
  }
}

void DoubleDialogEditor::runDialog() { emit buttonClicked(m_property); }
