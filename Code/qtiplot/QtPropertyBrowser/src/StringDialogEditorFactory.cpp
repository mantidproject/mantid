#include "StringDialogEditorFactory.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QDialog>
#include <QSettings>

#include <iostream>

void StringDialogEditorFactory::connectPropertyManager(QtStringPropertyManager *manager)
{
}

QWidget* StringDialogEditorFactory::createEditor(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent)
{
  return new StringDialogEditor(property,parent);
}

void StringDialogEditorFactory::disconnectPropertyManager(QtStringPropertyManager *manager)
{
}

StringDialogEditor::StringDialogEditor(QtProperty *property, QWidget *parent):QWidget(parent),m_property(property)
{
  QHBoxLayout *layout = new QHBoxLayout;
  m_lineEdit = new QLineEdit(this);
  layout->addWidget(m_lineEdit);
  setFocusProxy(m_lineEdit);
  connect(m_lineEdit,SIGNAL(editingFinished()),this,SLOT(updateProperty()));
  QtStringPropertyManager* mgr = dynamic_cast<QtStringPropertyManager*>(property->propertyManager());
  if (mgr)
  {
    m_lineEdit->setText(mgr->value(property));
  }

  QPushButton* button = new QPushButton("...",this);
  button->setMaximumSize(20,1000000);
  connect(button,SIGNAL(clicked()),this,SLOT(runDialog()));
  layout->addWidget(button);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);
  layout->setStretchFactor(button,0);
  this->setLayout(layout);
}

void StringDialogEditor::runDialog()
{
  QSettings settings;
  QString dir = settings.value("Mantid/FitBrowser/ResolutionDir").toString();
  QString StringDialog = QFileDialog::getOpenFileName(this, tr("Open File"),dir);
  if (!StringDialog.isEmpty())
  {
    m_lineEdit->setText(StringDialog);
    updateProperty();
  }
}

void StringDialogEditor::setText(const QString& txt)
{
  m_lineEdit->setText(txt);
}

QString StringDialogEditor::getText()const
{
  return m_lineEdit->text();
}

StringDialogEditor::~StringDialogEditor()
{
}

void StringDialogEditor::updateProperty()
{
  QtStringPropertyManager* mgr = dynamic_cast<QtStringPropertyManager*>(m_property->propertyManager());
  if (mgr)
  {
    mgr->setValue(m_property,m_lineEdit->text());
  }
}

