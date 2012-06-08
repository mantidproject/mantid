#include "MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h"
#include "MantidKernel/ConfigService.h"
#include "MantidMDEvents/MDTransfFactory.h"
#include "MantidMDEvents/MDTransfDEHelper.h"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>

//using namespace Mantid::MDAlgorithms;
/**
Constructor
*/
CreateMDWorkspaceAlgDialog::CreateMDWorkspaceAlgDialog()
{
  m_uiForm.setupUi(this);

  connect(m_uiForm.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(m_uiForm.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  //ConvertToMD::ConvertToMDEventsParams ConvParams;
  std::vector<std::string> QModes =Mantid::MDEvents::MDTransfFactory::Instance().getKeys();
  if(QModes.empty()) // avoid werid situations with factory not initiated
  {
      QModes.assign(1," No Q modes availible; error Initiating Q-conversion factory");
  }
  for(size_t i=0;i<QModes.size();i++)
  {
      QString name(QModes[i].c_str());
      m_uiForm.combo_q_dimensions->addItem(name);

  }
  Mantid::MDEvents::MDTransfDEHelper AlldEModes;
  std::vector<std::string> dEModes = AlldEModes.getEmodes();
  for(size_t i=0;i<dEModes.size();i++)
  {
      QString name(dEModes[i].c_str());
      m_uiForm.combo_analysis_mode->addItem(name);
  }

  //m_uiForm.combo_analysis_mode->addItem(dEModes[Elastic]);
 // m_uiForm.combo_analysis_mode->addItem(dEModes[Indirect]);

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
