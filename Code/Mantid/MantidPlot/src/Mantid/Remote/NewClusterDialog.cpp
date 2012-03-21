#include "NewClusterDialog.h"
#include "ui_NewClusterDialog.h"

#include <QString>
#include <QUrl>
#include <QPushButton>

NewClusterDialog::NewClusterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewClusterDialog)
{
    ui->setupUi(this);
    
    // OK button starts off disabled
    ui->buttonBox->buttons()[0]->setEnabled( false);
    
    QObject::connect( ui->displayNameEdit, SIGNAL( textChanged( QString)), this, SLOT( validateInput()));
    QObject::connect( ui->serviceBaseURLEdit, SIGNAL( textChanged( QString)), this, SLOT( validateInput()));
    QObject::connect( ui->configFileURLEdit, SIGNAL( textChanged( QString)), this, SLOT( validateInput()));
    QObject::connect( ui->userNameEdit, SIGNAL( textChanged( QString)), this, SLOT( validateInput()));
}

NewClusterDialog::~NewClusterDialog()
{
    delete ui;
}

const QString NewClusterDialog::getDisplayName() const
{
  return ui->displayNameEdit->text();
}

const QUrl NewClusterDialog::getServiceBaseURL() const
{
  return QUrl(ui->serviceBaseURLEdit->text());
}

const QUrl NewClusterDialog::getConfigFileURL() const
{
  return QUrl(ui->configFileURLEdit->text());
}

const QString NewClusterDialog::getUserName() const
{
  return QString(ui->userNameEdit->text());
}

// Will enable the OK button if the input is valid
bool NewClusterDialog::validateInput() const
{
  bool isValid = false;  // assume input is not valid
  
  if (getDisplayName().isEmpty() == false)
    if (getServiceBaseURL().isValid())
      if (getConfigFileURL().isValid())
          if (getUserName().isEmpty() == false)
          {
            isValid = true;
          }
      
  ui->buttonBox->buttons()[0]->setEnabled( isValid);  // Enable/disable the Ok button
  
  return isValid;
}
