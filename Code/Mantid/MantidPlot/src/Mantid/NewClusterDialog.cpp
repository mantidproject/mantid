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
    
    // OK button starts of disabled
    ui->buttonBox->buttons()[0]->setEnabled( false);
    
    QObject::connect( ui->displayNameEdit, SIGNAL( textChanged( QString)), this, SLOT( validateInput()));
    QObject::connect( ui->serviceBaseURLEdit, SIGNAL( textChanged( QString)), this, SLOT( validateInput()));
    QObject::connect( ui->configFileURLEdit, SIGNAL( textChanged( QString)), this, SLOT( validateInput()));
}

NewClusterDialog::~NewClusterDialog()
{
    delete ui;
}

const QString NewClusterDialog::getDisplayName()
{
  return ui->displayNameEdit->text();
}

const QUrl NewClusterDialog::getServiceBaseURL()
{
  return QUrl(ui->serviceBaseURLEdit->text());
}

const QUrl NewClusterDialog::getConfigFileURL()
{
  return QUrl(ui->configFileURLEdit->text());
}


// Will enable the OK button if the input is valid
bool NewClusterDialog::validateInput()
{
  bool isValid = false;  // assume input is not valid
  
  if (getDisplayName().isEmpty() == false)
    if (getServiceBaseURL().isValid())
      if (getConfigFileURL().isValid())
      {
        isValid = true;
      }
      
  ui->buttonBox->buttons()[0]->setEnabled( isValid);  // Enable/disable the Ok button
  
  return isValid;
}
