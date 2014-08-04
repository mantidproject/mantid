#include "MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h"
#include "MantidAPI/AlgorithmManager.h"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>

namespace MantidQt
{
namespace CustomInterfaces
{

/**
Constructor
*/
CreateMDWorkspaceAlgDialog::CreateMDWorkspaceAlgDialog()
{
  m_uiForm.setupUi(this);

  connect(m_uiForm.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(m_uiForm.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  using namespace Mantid::API;
  IAlgorithm_sptr convertToMD = AlgorithmManager::Instance().createUnmanaged("ConvertToMD");
  convertToMD->initialize();
  auto QModes = convertToMD->getPointerToProperty("QDimensions")->allowedValues();
  if ( QModes.empty() ) // avoid weird situations with factory not initiated
  {
    QModes.push_back("No Q modes available; error Initiating Q-conversion factory");
  }
  for ( auto it = QModes.begin(); it != QModes.end(); ++it )
  {
    m_uiForm.combo_q_dimensions->addItem(QString::fromStdString(*it));
  }

  auto dEModes = convertToMD->getPointerToProperty("dEAnalysisMode")->allowedValues();
  for ( auto it = dEModes.begin(); it != dEModes.end(); ++it )
  {
    m_uiForm.combo_analysis_mode->addItem(QString::fromStdString(*it));
  }

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

CreateMDWorkspaceAlgDialog::~CreateMDWorkspaceAlgDialog()
{
}

}
}
