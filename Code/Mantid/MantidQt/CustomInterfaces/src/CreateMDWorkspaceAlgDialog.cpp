#include "MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h"
#include "MantidKernel/ConfigService.h"
#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>

using namespace Mantid::MDAlgorithms;
/**
Constructor
*/
CreateMDWorkspaceAlgDialog::CreateMDWorkspaceAlgDialog()
{
  m_uiForm.setupUi(this);

  connect(m_uiForm.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(m_uiForm.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  ConvertToMD::ConvertToMDEventsParams ConvParams;
  std::vector<std::string> QModes = ConvParams.getQModes();
  // based on noQ mode being first in the list
  QString name1(QModes[ConvertToMD::ModQ].c_str());
  m_uiForm.combo_q_dimensions->addItem(name1);
  QString name2(QModes[ConvertToMD::Q3D].c_str());
  m_uiForm.combo_q_dimensions->addItem(name2);

  std::vector<std::string> dEModes = ConvParams.getDEModes();
  for(size_t i=0;i<ConvertToMD::ANY_Mode;i++)
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
