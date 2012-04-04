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
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include <Qsci/qscilexerpython.h>

using namespace MantidQt::CustomDialogs;
using namespace MantidQt::API;
using Mantid::API::AlgorithmManager;
using Mantid::API::Algorithm_sptr;
using Mantid::Kernel::DateAndTime;

namespace {
  class LiveDataAlgInputHistoryImpl : public AbstractAlgorithmInputHistory
  {
  private:
    LiveDataAlgInputHistoryImpl() : AbstractAlgorithmInputHistory("LiveDataAlgorithms") {}
    ~LiveDataAlgInputHistoryImpl() {}

  private:
    friend struct Mantid::Kernel::CreateUsingNew<LiveDataAlgInputHistoryImpl>;
  };

  #ifdef _WIN32
  // this breaks new namespace declaraion rules; need to find a better fix
    template class Mantid::Kernel::SingletonHolder<LiveDataAlgInputHistoryImpl>;
  #endif /* _WIN32 */
    /// The specific instantiation of the templated type
    typedef Mantid::Kernel::SingletonHolder<LiveDataAlgInputHistoryImpl> LiveDataAlgInputHistory;
}

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(StartLiveDataDialog);

//----------------------
// Public member functions
//----------------------
///Constructor
StartLiveDataDialog::StartLiveDataDialog(QWidget *parent) :
  AlgorithmDialog(parent)
{
  // Create the input history. This loads it too.
  LiveDataAlgInputHistory::Instance();
}

/// Destructor
StartLiveDataDialog::~StartLiveDataDialog()
{
  // Save the input history to QSettings
  LiveDataAlgInputHistory::Instance().save();
}

/// Set up the dialog layout
void StartLiveDataDialog::initLayout()
{
  ui.setupUi(this);

  // To save the history of inputs
  // RJT: I don't much like this, but at least it's safe from a lifetime point of view.
  AbstractAlgorithmInputHistory * history = &LiveDataAlgInputHistory::Instance();
  ui.processingAlgo->setInputHistory(history);
  ui.postAlgo->setInputHistory(history);

  // ========== Set previous values from history =============
  fillAndSetComboBox("Instrument", ui.cmbInstrument);
  tie(ui.edtUpdateEvery, "UpdateEvery", ui.layoutUpdateEvery);
  fillAndSetComboBox("AccumulationMethod", ui.cmbAccumulationMethod);

  tie(ui.radNow, "FromNow");
  tie(ui.radStartOfRun, "FromStartOfRun");
  tie(ui.radAbsoluteTime, "FromTime");
  tie(ui.dateTimeEdit, "StartTime");
  radioTimeClicked();

  tie(ui.chkPreserveEvents, "PreserveEvents");
  chkPreserveEventsToggled();

  tie(ui.cmbEndRunBehavior, "EndRunBehavior");
  fillAndSetComboBox("EndRunBehavior", ui.cmbEndRunBehavior);

  tie(ui.editAccumulationWorkspace, "AccumulationWorkspace", ui.gridLayout);
  tie(ui.editOutputWorkspace, "OutputWorkspace", ui.gridLayout);

  // ========== Update GUIs =============
  ui.processingAlgo->update();
  ui.postAlgo->update();

  // ========== Layout Tweaks =============
  ui.tabWidget->setCurrentIndex(0);
  ui.splitterMain->setStretchFactor(0, 0);
  ui.splitterMain->setStretchFactor(1, 1);

  // ========== Set previous values for Algorithms/scripts ============
  for (int i=0; i<2; i++)
  {
    bool post = i > 0;
    QString prefix = "Processing";

    if (post) prefix = "PostProcessing";
    QString algo = AlgorithmInputHistory::Instance().previousInput("StartLiveData", prefix+"Algorithm");
    QString algoProps = AlgorithmInputHistory::Instance().previousInput("StartLiveData", prefix+"Properties");
    QString script = AlgorithmInputHistory::Instance().previousInput("StartLiveData", prefix+"Script");

    if (!post)
    {
      if (!algo.isEmpty())
        ui.radProcessAlgorithm->setChecked(true);
      else if (!script.isEmpty())
        ui.radProcessScript->setChecked(true);
      else
        ui.radProcessNone->setChecked(true);
      radioProcessClicked();
      // Set the script to the previous value
      ui.processingAlgo->setScriptText(script);
      ui.processingAlgo->setSelectedAlgorithm(algo);
      changeProcessingAlgorithm();
    }
    else
    {
      if (!algo.isEmpty())
        ui.radPostProcessAlgorithm->setChecked(true);
      else if (!script.isEmpty())
        ui.radPostProcessScript->setChecked(true);
      else
        ui.radPostProcessNone->setChecked(true);
      radioPostProcessClicked();
      // Set the script to the previous value
      ui.postAlgo->setScriptText(script);
      ui.postAlgo->setSelectedAlgorithm(algo);
      changePostProcessingAlgorithm();
    }
  }

  radioPostProcessClicked();

  //=========== SLOTS =============
  connect(ui.processingAlgo, SIGNAL(changedAlgorithm()), this, SLOT(changeProcessingAlgorithm()));
  connect(ui.postAlgo, SIGNAL(changedAlgorithm()), this, SLOT(changePostProcessingAlgorithm()));

  connect(ui.radProcessNone, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));
  connect(ui.radProcessAlgorithm, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));
  connect(ui.radProcessScript, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));

  connect(ui.radPostProcessNone, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));
  connect(ui.radPostProcessAlgorithm, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));
  connect(ui.radPostProcessScript, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));

  connect(ui.radNow, SIGNAL(toggled(bool)), this, SLOT(radioTimeClicked()));
  connect(ui.radStartOfRun, SIGNAL(toggled(bool)), this, SLOT(radioTimeClicked()));
  connect(ui.radAbsoluteTime, SIGNAL(toggled(bool)), this, SLOT(radioTimeClicked()));

  connect(ui.chkPreserveEvents, SIGNAL(toggled(bool)), this, SLOT(chkPreserveEventsToggled()));

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
    storePropertyValue("ProcessingAlgorithm", ui.processingAlgo->getSelectedAlgorithm());
    std::string props;
    props = m_processingAlg->asString(false, ';'); /* use semicolon to properly separate the props */
    storePropertyValue("ProcessingProperties", QString::fromStdString(props));
  }
  else if (m_useProcessScript)
    storePropertyValue("ProcessingScript", ui.processingAlgo->getScriptText());

  storePropertyValue("PostProcessingAlgorithm", "");
  storePropertyValue("PostProcessingProperties", "");
  storePropertyValue("PostProcessingScript", "");
  if (m_usePostProcessAlgo && m_postProcessingAlg)
  {
    storePropertyValue("PostProcessingAlgorithm", ui.postAlgo->getSelectedAlgorithm());
    std::string props;
    props = m_postProcessingAlg->asString(false, ';'); /* use semicolon to properly separate the props */
    storePropertyValue("PostProcessingProperties", QString::fromStdString(props));
  }
  else if (m_usePostProcessScript)
    storePropertyValue("PostProcessingScript", ui.postAlgo->getScriptText());

  // Save to QSettings
  ui.processingAlgo->saveInput();
  ui.postAlgo->saveInput();
}


//------------------------------------------------------------------------------
/** Slot called when one of the radio buttons in "processing" are picked */
void StartLiveDataDialog::radioProcessClicked()
{
  m_useProcessAlgo = ui.radProcessAlgorithm->isChecked();
  ui.processingAlgo->algoVisible(m_useProcessAlgo);
  m_useProcessScript = ui.radProcessScript->isChecked();
  ui.processingAlgo->editorVisible(m_useProcessScript);
}

//------------------------------------------------------------------------------
/** Slot called when one of the radio buttons in "processing" are picked */
void StartLiveDataDialog::radioPostProcessClicked()
{
  m_usePostProcessAlgo = ui.radPostProcessAlgorithm->isChecked();
  ui.postAlgo->algoVisible(m_usePostProcessAlgo);
  m_usePostProcessScript = ui.radPostProcessScript->isChecked();
  ui.postAlgo->editorVisible(m_usePostProcessScript);
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

/** Slot called when the preserve events checkbox changes */
void StartLiveDataDialog::chkPreserveEventsToggled()
{
  ui.lblPreserveEventsWarning->setVisible( ui.chkPreserveEvents->isChecked());
}

//------------------------------------------------------------------------------
/** Slot called when picking a different algorithm
 * in the AlgorithmSelectorWidget */
void StartLiveDataDialog::changeProcessingAlgorithm()
{
  Algorithm_sptr alg = ui.processingAlgo->getAlgorithm();
  if (!alg) return;
  m_processingAlg = alg;
}

//------------------------------------------------------------------------------
/** Slot called when picking a different algorithm
 * in the AlgorithmSelectorWidget */
void StartLiveDataDialog::changePostProcessingAlgorithm()
{
  Algorithm_sptr alg = ui.postAlgo->getAlgorithm();
  if (!alg) return;
  m_postProcessingAlg = alg;
}

}
}
