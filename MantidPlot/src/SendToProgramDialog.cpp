// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "SendToProgramDialog.h"
#include "ConfigDialog.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/pixmaps.h"

#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include <map>

using namespace MantidQt::API;

/**
 * Constructor when adding a new program to the send to list
 */
SendToProgramDialog::SendToProgramDialog(QWidget *parent, Qt::WFlags fl)
    : QDialog(parent, fl), validName(false), validTarget(false),
      validSaveUsing(false) {
  m_uiform.setupUi(this);

  // Adding new information is disabled until selected fields have been
  // validated and passed
  m_uiform.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);

  // Icon image for the browse button
  m_uiform.browseButton->setIcon(QIcon(getQPixmap("choose_folder_xpm")));

  // Setup save and browse button, cancel does nothing but closes the current
  // dialog (does this by default)
  connect(m_uiform.browseButton, SIGNAL(clicked()), this, SLOT(browse()));
  connect(m_uiform.buttonBox, SIGNAL(accepted()), this, SLOT(save()));

  // Setup Validation for the mandatory information
  connect(m_uiform.nameText, SIGNAL(textChanged(const QString &)), this,
          SLOT(validateName()));
  connect(m_uiform.targetText, SIGNAL(textChanged(const QString &)), this,
          SLOT(validateTarget()));
  connect(m_uiform.saveUsingText, SIGNAL(textChanged(const QString &)), this,
          SLOT(validateSaveUsing()));
}

/**
 * Constructor when editing a program settings
 */
SendToProgramDialog::SendToProgramDialog(
    QWidget *parent, QString programName,
    std::map<std::string, std::string> programKeysAndDetails, Qt::WFlags fl)
    : QDialog(parent, fl), validName(true), validTarget(true),
      validSaveUsing(true) {
  m_uiform.setupUi(this);

  // Set the name of the program you wish to edit and make it so that the user
  // can't change it
  m_uiform.nameText->setText(programName);
  QPalette palette;
  palette.setColor(m_uiform.nameText->backgroundRole(), QColor(230, 230, 230));
  m_uiform.nameText->setPalette(palette);
  m_uiform.nameText->setReadOnly(true);

  // Assign the collected data on the program to the form boxes
  if (programKeysAndDetails.count("target") != 0)
    m_uiform.targetText->setText(
        QString::fromStdString(programKeysAndDetails.find("target")->second));
  if (programKeysAndDetails.count("arguments") != 0)
    m_uiform.argumentsText->setText(QString::fromStdString(
        programKeysAndDetails.find("arguments")->second));
  if (programKeysAndDetails.count("saveparameters") != 0)
    m_uiform.saveParametersText->setText(QString::fromStdString(
        programKeysAndDetails.find("saveparameters")->second));

  if (programKeysAndDetails.count("saveusing") != 0)

    m_uiform.saveUsingText->setText(QString::fromStdString(
        programKeysAndDetails.find("saveusing")->second));

  // Validation correct on startup
  validateName();
  validateTarget();
  validateSaveUsing();

  // Icon image for the browse button
  m_uiform.browseButton->setIcon(QIcon(getQPixmap("choose_folder_xpm")));

  // Setup save and browse button, cancel does nothing but closes the current
  // dialog (does this by default)
  connect(m_uiform.browseButton, SIGNAL(clicked()), this, SLOT(browse()));
  connect(m_uiform.buttonBox, SIGNAL(accepted()), this, SLOT(save()));

  // Setup Validation for the mandatory information
  connect(m_uiform.nameText, SIGNAL(textChanged(const QString &)), this,
          SLOT(validateName()));
  connect(m_uiform.targetText, SIGNAL(textChanged(const QString &)), this,
          SLOT(validateTarget()));
  connect(m_uiform.saveUsingText, SIGNAL(textChanged(const QString &)), this,
          SLOT(validateSaveUsing()));
}

/**
 * Open up a new file browsing window
 */
void SendToProgramDialog::browse() {
  // (*) Will let all files be selected
  QFileDialog *dialog = new QFileDialog;
  QString fileName = dialog->getOpenFileName(
      this, tr("Select Program Location"), "C:/", tr("All Files (*)"));

  // Sets the file target that the user selected to be the file path for the
  // program
  m_uiform.targetText->setText(fileName);
}

/**
 * See whether anything has been entered as a program name.
 */
void SendToProgramDialog::validateName() {
  if (m_uiform.nameText->text() == "") {
    m_uiform.validateName->setVisible(true);
    validName = false;
  } else {
    m_uiform.validateName->setVisible(false);
    validName = true;
  }
  validateAll();
}

/**
 * Make sure the user specified target program is executable.
 */
void SendToProgramDialog::validateTarget() {
  QString filePath = m_uiform.targetText->text();
  filePath.replace(QString("\\"), QString("/"));

  if (filePath != "") {
    if (Mantid::Kernel::ConfigService::Instance().isExecutable(
            filePath.toStdString())) {
      m_uiform.validateTarget->setVisible(false);
      validTarget = true;
    } else {
      m_uiform.validateTarget->setVisible(true);
      validTarget = false;
    }
  } else {
    m_uiform.validateTarget->setVisible(true);
    validTarget = false;
  }
  validateAll();
}

/**
 * Make sure the user specified save algorithm exists.
 */
void SendToProgramDialog::validateSaveUsing() {
  validSaveUsing = Mantid::API::AlgorithmFactory::Instance().exists(
      m_uiform.saveUsingText->text().toStdString());
  m_uiform.validateSaveUsing->setVisible(!validSaveUsing);

  validateAll();
}

/**
 * If a validation passes or fails then a validation of the entire
 * dialog needs to be done to enable or disable the save button.
 */
void SendToProgramDialog::validateAll() {
  // If validation passes on name, target and the save algorithm the save button
  // becomes available for the user to press.
  if (validName == true && validTarget == true && validSaveUsing == true)
    m_uiform.buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
  else
    m_uiform.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
}

/**
 * Save the new program or changes to a program
 */
void SendToProgramDialog::save() {
  // Collect mandatory information and then check to see if it has been
  // collected (visible will always be true or false and is therefore not
  // collected yet)
  std::map<std::string, std::string> programKeysAndDetails;

  std::string name = m_uiform.nameText->text().toStdString();

  QString filePath = m_uiform.targetText->text();
  filePath.replace(QString("\\"), QString("/"));

  programKeysAndDetails["target"] = filePath.toStdString();

  programKeysAndDetails["saveusing"] =
      m_uiform.saveUsingText->text().toStdString();

  // No need to check that mandatory data is here due to validation that has
  // been implemented above
  // Collect the rest of the information if there is any (visible will always be
  // true or false)
  if (m_uiform.argumentsText->text() != "")
    programKeysAndDetails["arguments"] =
        m_uiform.argumentsText->text().toStdString();
  if (m_uiform.saveParametersText->text() != "")
    programKeysAndDetails["saveparameters"] =
        m_uiform.saveParametersText->text().toStdString();

  // when a program is saved be it an edit or a new program, visible defaults to
  // "Yes"
  programKeysAndDetails["visible"] = "Yes";

  m_settings.first = name;
  m_settings.second = programKeysAndDetails;
}

/**
 * Get the settings
 *
 * @return m_settings :: Key and detail of what is to go in the config service
 */
std::pair<std::string, std::map<std::string, std::string>>
SendToProgramDialog::getSettings() const {
  return m_settings;
}
