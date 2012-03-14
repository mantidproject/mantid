//----------------------
// Includes
//----------------------
#include "MantidQtCustomDialogs/StartLiveDataDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/PropertyWidgetFactory.h"
#include "qboxlayout.h"
#include "MantidAPI/AlgorithmManager.h"
#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/SingletonHolder.h"
#include <QtGui>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(StartLiveDataDialog);
}
}

using namespace MantidQt::CustomDialogs;
using namespace MantidQt::API;
using Mantid::API::AlgorithmManager;
using Mantid::API::Algorithm_sptr;
using Mantid::Kernel::DateAndTime;

//----------------------
// Public member functions
//----------------------
///Constructor
StartLiveDataDialog::StartLiveDataDialog(QWidget *parent) :
  AlgorithmDialog(parent)
{
}


/// Set up the dialog layout
void StartLiveDataDialog::initLayout()
{
  ui.setupUi(this);

  // ========== Set previous values from history =============
  fillAndSetComboBox("Instrument", ui.cmbInstrument);
  tie(ui.edtUpdateEvery, "UpdateEvery", ui.layoutUpdateEvery);
  fillAndSetComboBox("AccumulationMethod", ui.cmbAccumulationMethod);

  tie(ui.radNow, "FromNow");
  tie(ui.radStartOfRun, "FromStartOfRun");
  tie(ui.radAbsoluteTime, "FromTime");
  tie(ui.dateTimeEdit, "StartTime");
  radioTimeClicked();

  tie(ui.editAccumulationWorkspace, "AccumulationWorkspace", ui.gridLayout);
  tie(ui.editOutputWorkspace, "OutputWorkspace", ui.gridLayout);

  // ========== Update GUIs =============
  ui.processingAlgoSelector->update();
  ui.postAlgoSelector->update();
  radioProcessClicked();
  radioPostProcessClicked();

  // ========== Layout Tweaks =============
  ui.processingScript->setVisible(false);
  QList<int> sizes; sizes.push_back(300); sizes.push_back(1000);
  ui.splitterProcessing->setSizes(sizes);
  ui.splitterProcessing->setStretchFactor(0, 0);
  ui.splitterProcessing->setStretchFactor(1, 0);
  ui.splitterPost->setSizes(sizes);
  ui.splitterPost->setStretchFactor(0, 0);
  ui.splitterPost->setStretchFactor(1, 0);
  ui.tabWidget->setCurrentIndex(0);

  //=========== SLOTS =============
  connect(ui.processingAlgoSelector, SIGNAL(algorithmSelectionChanged(const QString &, int)),
      this, SLOT(changeProcessingAlgorithm()));
  connect(ui.postAlgoSelector, SIGNAL(algorithmSelectionChanged(const QString &, int)),
      this, SLOT(changePostProcessingAlgorithm()));

  connect(ui.radProcessNone, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));
  connect(ui.radProcessAlgorithm, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));
  connect(ui.radProcessScript, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));

  connect(ui.radPostProcessNone, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));
  connect(ui.radPostProcessAlgorithm, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));
  connect(ui.radPostProcessScript, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));

  connect(ui.radNow, SIGNAL(toggled(bool)), this, SLOT(radioTimeClicked()));
  connect(ui.radStartOfRun, SIGNAL(toggled(bool)), this, SLOT(radioTimeClicked()));
  connect(ui.radAbsoluteTime, SIGNAL(toggled(bool)), this, SLOT(radioTimeClicked()));

  QHBoxLayout * buttonLayout = this->createDefaultButtonLayout();
  ui.mainLayout->addLayout(buttonLayout);
}

//------------------------------------------------------------------------------
/// Parse input when the dialog is accepted
void StartLiveDataDialog::parseInput()
{
  storePropertyValue("Instrument", ui.cmbInstrument->currentText());
  storePropertyValue("AccumulationMethod", ui.cmbAccumulationMethod->currentText());

  storePropertyValue("AccumulationWorkspace", ui.editAccumulationWorkspace->text());
  if (!m_usePostProcessAlgo && !m_usePostProcessScript)
    storePropertyValue("AccumulationWorkspace", "");

  storePropertyValue("OutputWorkspace", ui.editOutputWorkspace->text());

  storePropertyValue("ProcessingAlgorithm", "");
  storePropertyValue("ProcessingProperties", "");
  storePropertyValue("ProcessingScript", "");
  if (m_useProcessAlgo && m_processingAlg)
  {
    storePropertyValue("ProcessingAlgorithm", ui.processingAlgoSelector->getSelectedAlgorithm());
    std::string props;
    props = m_processingAlg->asString();
    storePropertyValue("ProcessingProperties", QString::fromStdString(props));
  }
  else if (m_useProcessScript)
    storePropertyValue("ProcessingScript", ui.processingScript->text());

  storePropertyValue("PostProcessingAlgorithm", "");
  storePropertyValue("PostProcessingProperties", "");
  storePropertyValue("PostProcessingScript", "");
  if (m_usePostProcessAlgo && m_postProcessingAlg)
  {
    storePropertyValue("PostProcessingAlgorithm", ui.postAlgoSelector->getSelectedAlgorithm());
    std::string props;
    props = m_postProcessingAlg->asString();
    storePropertyValue("PostProcessingProperties", QString::fromStdString(props));
  }
  else if (m_usePostProcessScript)
    storePropertyValue("PostProcessingScript", ui.postScript->text());
}


//------------------------------------------------------------------------------
/** Slot called when one of the radio buttons in "processing" are picked */
void StartLiveDataDialog::radioProcessClicked()
{
  m_useProcessAlgo = ui.radProcessAlgorithm->isChecked();
  m_useProcessScript = ui.radProcessScript->isChecked();
  ui.splitterProcessing->setVisible(m_useProcessAlgo);
  ui.processingScript->setVisible(m_useProcessScript);
}

//------------------------------------------------------------------------------
/** Slot called when one of the radio buttons in "processing" are picked */
void StartLiveDataDialog::radioPostProcessClicked()
{
  m_usePostProcessAlgo = ui.radPostProcessAlgorithm->isChecked();
  m_usePostProcessScript = ui.radPostProcessScript->isChecked();
  ui.splitterPost->setVisible(m_usePostProcessAlgo);
  ui.postScript->setVisible(m_usePostProcessScript);
  // Disable the AccumulationWorkspace widget unless it is needed
  ui.editAccumulationWorkspace->setEnabled(m_usePostProcessAlgo || m_usePostProcessScript);
  ui.lblAccumulationWorkspace->setEnabled(m_usePostProcessAlgo || m_usePostProcessScript);
}

//------------------------------------------------------------------------------
/** Slot called when one of the radio buttons in "starting time" are picked */
void StartLiveDataDialog::radioTimeClicked()
{
  ui.dateTimeEdit->setEnabled( ui.radAbsoluteTime->isChecked() );
}

//------------------------------------------------------------------------------
/** Slot called when picking a different algorithm
 * in the AlgorithmSelectorWidget */
void StartLiveDataDialog::changeProcessingAlgorithm()
{
  Algorithm_sptr alg = this->changeAlgorithm(ui.processingAlgoSelector, ui.processingAlgoProperties);
  if (!alg) return;
  m_processingAlg = alg;
}

//------------------------------------------------------------------------------
/** Slot called when picking a different algorithm
 * in the AlgorithmSelectorWidget */
void StartLiveDataDialog::changePostProcessingAlgorithm()
{
  Algorithm_sptr alg = this->changeAlgorithm(ui.postAlgoSelector, ui.postAlgoProperties);
  if (!alg) return;
  m_postProcessingAlg = alg;
}

//------------------------------------------------------------------------------
/** Called when the pre- or post-processing algorithm is being modified
 *
 * @param selector :: AlgorithmSelectorWidget
 * @param propWidget :: AlgorithmPropertiesWidget
 * @return the created algorithm
 */
Mantid::API::Algorithm_sptr StartLiveDataDialog::changeAlgorithm(MantidQt::MantidWidgets::AlgorithmSelectorWidget * selector,
    MantidQt::API::AlgorithmPropertiesWidget * propWidget)
{
  Algorithm_sptr alg;
  QString algName;
  int version;
  selector->getSelectedAlgorithm(algName,version);
  try
  {
    alg = AlgorithmManager::Instance().createUnmanaged(algName.toStdString(), version);
    alg->initialize();
  }
  catch (std::runtime_error &)
  {
    // Ignore when the algorithm is not found
    return Algorithm_sptr();
  }
  QStringList disabled;
  disabled.push_back("OutputWorkspace");
  disabled.push_back("InputWorkspace");
  propWidget->addEnabledAndDisableLists(QStringList(), disabled);
  propWidget->setAlgorithm(alg.get());
  // TODO: Set from history?
  propWidget->hideOrDisableProperties();
  return alg;
}

