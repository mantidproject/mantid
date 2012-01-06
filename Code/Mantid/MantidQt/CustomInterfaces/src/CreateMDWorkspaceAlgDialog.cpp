#include "MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>

CreateMDWorkspaceAlgDialog::CreateMDWorkspaceAlgDialog()
{
  m_uiForm.setupUi(this);

  connect(m_uiForm.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(m_uiForm.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  m_uiForm.combo_q_dimensions->addItem("|Q|");
  m_uiForm.combo_q_dimensions->addItem("QxQyQz");

  m_uiForm.combo_analysis_mode->addItem("Direct");
  m_uiForm.combo_analysis_mode->addItem("Indirect");
  m_uiForm.combo_analysis_mode->addItem("InElastic");

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

bool CreateMDWorkspaceAlgDialog::getPreprocessedEvents() const
{
  return m_uiForm.ck_use_preprocessed_detectors->isChecked();
}

CreateMDWorkspaceAlgDialog::~CreateMDWorkspaceAlgDialog()
{
}
