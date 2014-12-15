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
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/InstrumentInfo.h"
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

  class LiveDataPostProcessingAlgInputHistoryImpl : public AbstractAlgorithmInputHistory
  {
  private:
    LiveDataPostProcessingAlgInputHistoryImpl() : AbstractAlgorithmInputHistory("LiveDataPostProcessingAlgorithms") {}
    ~LiveDataPostProcessingAlgInputHistoryImpl() {}

  private:
    friend struct Mantid::Kernel::CreateUsingNew<LiveDataPostProcessingAlgInputHistoryImpl>;
  };

  #ifdef _WIN32
  // this breaks new namespace declaraion rules; need to find a better fix
    template class Mantid::Kernel::SingletonHolder<LiveDataPostProcessingAlgInputHistoryImpl>;
  #endif /* _WIN32 */
    /// The specific instantiation of the templated type
    typedef Mantid::Kernel::SingletonHolder<LiveDataPostProcessingAlgInputHistoryImpl> LiveDataPostProcessingAlgInputHistory;
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
  AlgorithmDialog(parent),
  m_useProcessAlgo(false), m_useProcessScript(false),
  m_usePostProcessAlgo(false), m_usePostProcessScript(false)
{
  // Create the input history. This loads it too.
  LiveDataAlgInputHistory::Instance();
}

/// Destructor
StartLiveDataDialog::~StartLiveDataDialog()
{
  // Save the input history to QSettings
  LiveDataAlgInputHistory::Instance().save();
  LiveDataPostProcessingAlgInputHistory::Instance().save();
}

/// Set up the dialog layout
void StartLiveDataDialog::initLayout()
{
  ui.setupUi(this);

  // To save the history of inputs
  // RJT: I don't much like this, but at least it's safe from a lifetime point of view.
  AbstractAlgorithmInputHistory * history1 = &LiveDataAlgInputHistory::Instance();
  ui.processingAlgo->setInputHistory(history1);
  AbstractAlgorithmInputHistory * history2 = &LiveDataPostProcessingAlgInputHistory::Instance();
  ui.postAlgo->setInputHistory(history2);

  // ========== Set previous values from history =============
  fillAndSetComboBox("Instrument", ui.cmbInstrument);
  tie(ui.edtUpdateEvery, "UpdateEvery", ui.layoutUpdateEvery);
  fillAndSetComboBox("AccumulationMethod", ui.cmbAccumulationMethod);

  tie(ui.radNow, "FromNow");
  tie(ui.radStartOfRun, "FromStartOfRun");
  tie(ui.radAbsoluteTime, "FromTime");
  radioTimeClicked();

  tie(ui.chkPreserveEvents, "PreserveEvents");
  chkPreserveEventsToggled();

  tie(ui.cmbRunTransitionBehavior, "RunTransitionBehavior");
  fillAndSetComboBox("RunTransitionBehavior", ui.cmbRunTransitionBehavior);

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
  setDefaultAccumulationMethod( ui.cmbInstrument->currentText() );

  //=========== Listener's properties =============

  initListenerPropLayout(ui.cmbInstrument->currentText());

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

  connect(ui.cmbInstrument,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(setDefaultAccumulationMethod(const QString&)));
  connect(ui.cmbInstrument,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(initListenerPropLayout(const QString&)));

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

//------------------------------------------------------------------------------
/** Slot called when picking a different instrument.
 *  Disables the 'Add' option if the listener is going to pass back histograms.
 *  @param inst :: The instrument name.
 */
void StartLiveDataDialog::setDefaultAccumulationMethod(const QString& inst)
{
  if ( inst.isEmpty() ) return;
  try
  {
    // Make sure 'Add' is enabled ahead of the check (the check may throw)
    int addIndex = ui.cmbAccumulationMethod->findText("Add");
    ui.cmbAccumulationMethod->setItemData(addIndex, QVariant(Qt::ItemIsSelectable | Qt::ItemIsEnabled), Qt::UserRole - 1);

    // Check whether this listener will give back events. If not, disable 'Add' as an option
    // The 'false' 2nd argument means don't connect the created listener
    if ( ! Mantid::API::LiveListenerFactory::Instance().create(inst.toStdString(),false)->buffersEvents() )
    {
      // If 'Add' is currently selected, select 'Replace' instead
      if ( ui.cmbAccumulationMethod->currentIndex() == addIndex )
      {
        ui.cmbAccumulationMethod->setCurrentText("Replace");
      }
      // Disable the 'Add' option in the combobox. It just wouldn't make sense.
      ui.cmbAccumulationMethod->setItemData(addIndex, false, Qt::UserRole - 1);
    }
  }
  // If an exception is thrown, just swallow it and do nothing
  // getInstrument can throw, particularly while we allow listener names to be passed in directly
  catch( Mantid::Kernel::Exception::NotFoundError& )
  {
  }
}

void StartLiveDataDialog::accept()
{
  // Now manually set the StartTime property as there's a computation needed
  DateAndTime startTime = DateAndTime::getCurrentTime() - ui.dateTimeEdit->value()*60.0;
  m_algorithm->setPropertyValue("StartTime",startTime.toISO8601String());

  AlgorithmDialog::accept(); // accept executes the algorithm
}

void StartLiveDataDialog::initListenerPropLayout(const QString& inst)
{
  // remove previous listener's properties
  auto props = m_algorithm->getPropertiesInGroup("ListenerProperties");
  for(auto prop = props.begin(); prop != props.end(); ++prop)
  {
    QString propName = QString::fromStdString((**prop).name());
    if ( m_algProperties.contains( propName ) )
    {
      m_algProperties.remove( propName );
    }
  }

  // update algorithm's properties
  m_algorithm->setPropertyValue("Instrument", inst.toStdString());
  // create or clear the layout
  QLayout *layout = ui.listenerProps->layout();
  if ( !layout )
  {
    QGridLayout *listenerPropLayout = new QGridLayout(ui.listenerProps);
    layout = listenerPropLayout;
  }
  else
  {
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != NULL)  
    {
      child->widget()->close();
      delete child;
    }
  }

  // find the listener's properties
  props = m_algorithm->getPropertiesInGroup("ListenerProperties");

  // no properties - don't show the box
  if ( props.empty() )
  {
    ui.listenerProps->setVisible(false);
    return;
  }
  
  auto gridLayout = static_cast<QGridLayout*>( layout );
  // add widgets for the listener's properties
  for(auto prop = props.begin(); prop != props.end(); ++prop)
  {
    int row = static_cast<int>(std::distance( props.begin(), prop ));
    QString propName = QString::fromStdString((**prop).name());
    gridLayout->addWidget( new QLabel(propName), row, 0 );
    QLineEdit *propWidget = new QLineEdit();
    gridLayout->addWidget( propWidget, row, 1 );
    if ( !m_algProperties.contains( propName ) )
    {
      m_algProperties.append( propName );
    }
    tie(propWidget, propName, gridLayout);
  }
  ui.listenerProps->setVisible(true);

}


}
}
