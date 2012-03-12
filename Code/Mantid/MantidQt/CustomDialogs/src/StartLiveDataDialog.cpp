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

  tie(ui.editAccumulationWorkspace, "AccumulationWorkspace");
  tie(ui.editOutputWorkspace, "OutputWorkspace");

  ui.editAccumulationWorkspace->setEnabled(false);

  ui.processingAlgoSelector->update();
  ui.postAlgoSelector->update();

  // ========== Layout Tweaks =============
  ui.processingScript->setVisible(false);
  QList<int> sizes; sizes.push_back(200); sizes.push_back(1000);
  ui.splitterProcessing->setSizes(sizes);
  ui.splitterProcessing->setStretchFactor(0, 0);
  ui.splitterProcessing->setStretchFactor(1, 0);
  ui.tabWidget->setCurrentIndex(0);

  //=========== SLOTS =============
  connect(ui.processingAlgoSelector, SIGNAL(algorithmSelectionChanged(const QString &, int)),
      this, SLOT(changeProcessingAlgorithm()));

  connect(ui.radProcessNone, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));
  connect(ui.radProcessAlgorithm, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));
  connect(ui.radProcessScript, SIGNAL(toggled(bool)), this, SLOT(radioProcessClicked()));

  connect(ui.radPostProcessNone, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));
  connect(ui.radPostProcessAlgorithm, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));
  connect(ui.radPostProcessScript, SIGNAL(toggled(bool)), this, SLOT(radioPostProcessClicked()));

  QHBoxLayout * buttonLayout = this->createDefaultButtonLayout();
  ui.mainLayout->addLayout(buttonLayout);
}

/// Parse input when the dialog is accepted
void StartLiveDataDialog::parseInput()
{
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
}


//------------------------------------------------------------------------------
/** Slot called when picking a different algorithm
 * in the AlgorithmSelectorWidget */
void StartLiveDataDialog::changeProcessingAlgorithm()
{
  QString algName;
  int version;
  ui.processingAlgoSelector->getSelectedAlgorithm(algName,version);
  try
  {
    m_processingAlg = AlgorithmManager::Instance().createUnmanaged(algName.toStdString(), version);
    m_processingAlg->initialize();
  }
  catch (std::runtime_error &)
  {
    // Ignore when the algorithm is not found
    return;
  }
  QStringList disabled;
  disabled.push_back("OutputWorkspace");
  disabled.push_back("InputWorkspace");
  ui.processingAlgoProperties->addEnabledAndDisableLists(QStringList(), disabled);
  ui.processingAlgoProperties->setAlgorithm(m_processingAlg.get());
  // TODO: Set from history?
  ui.processingAlgoProperties->hideOrDisableProperties();
}
