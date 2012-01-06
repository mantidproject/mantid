#include "MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>

CreateMDWorkspaceAlgDialog::CreateMDWorkspaceAlgDialog()
{
  m_uiForm.setupUi(this);

  connect(m_uiForm.btn_output_location, SIGNAL(clicked()), this, SLOT(outputLocationClicked()));
  connect(m_uiForm.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(m_uiForm.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  m_uiForm.combo_q_dimensions->addItem("|Q|");
  m_uiForm.combo_q_dimensions->addItem("QxQyQz");

  m_uiForm.combo_analysis_mode->addItem("Direct");
  m_uiForm.combo_analysis_mode->addItem("Elastic");
  m_uiForm.combo_analysis_mode->addItem("Indirect");
  
  this->setWindowTitle("Set MDWorkspace Creation Parameters");
}

QString CreateMDWorkspaceAlgDialog::getQDimension() const
{
  return m_uiForm.combo_q_dimensions->currentText();
}

QString CreateMDWorkspaceAlgDialog::getAnalysisMode() const
{
  return m_uiForm.combo_analysis_mode->currentText();
}

QString CreateMDWorkspaceAlgDialog::getOtherDimensions() const
{
  return m_uiForm.txt_other_dimensions->text();
}

QString CreateMDWorkspaceAlgDialog::getMaxExtents() const
{
  return m_uiForm.txt_max_values->text();
}

QString CreateMDWorkspaceAlgDialog::getMinExtents() const
{
  return m_uiForm.txt_min_values->text();
}

QString CreateMDWorkspaceAlgDialog::getPreprocessedDetectors() const
{
  QString result;
  m_uiForm.ck_use_preprocessed_detectors->isChecked() ? result = "1" : result = "0";
  return result;
}

QString CreateMDWorkspaceAlgDialog::getLocation() const
{
  return m_location;
}

CreateMDWorkspaceAlgDialog::~CreateMDWorkspaceAlgDialog()
{
}

void CreateMDWorkspaceAlgDialog::outputLocationClicked()
{
  this->m_location = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home");
}
