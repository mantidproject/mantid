// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/StartLiveDataDialog.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/LiveListenerInfo.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/PropertyWidgetFactory.h"
#include "qboxlayout.h"
#include <QStringList>
#include <QVariant>

using namespace MantidQt::CustomDialogs;
using namespace MantidQt::API;
using Mantid::API::Algorithm_sptr;
using Mantid::API::AlgorithmManager;
using Mantid::Kernel::ConfigService;
using Mantid::Types::Core::DateAndTime;

namespace {
class LiveDataAlgInputHistoryImpl : public AbstractAlgorithmInputHistory {
private:
  LiveDataAlgInputHistoryImpl() : AbstractAlgorithmInputHistory("LiveDataAlgorithms") {}
  ~LiveDataAlgInputHistoryImpl() override = default;

private:
  friend struct Mantid::Kernel::CreateUsingNew<LiveDataAlgInputHistoryImpl>;
};

#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class Mantid::Kernel::SingletonHolder<LiveDataAlgInputHistoryImpl>;
#endif /* _WIN32 */
/// The specific instantiation of the templated type
using LiveDataAlgInputHistory = Mantid::Kernel::SingletonHolder<LiveDataAlgInputHistoryImpl>;

class LiveDataPostProcessingAlgInputHistoryImpl : public AbstractAlgorithmInputHistory {
private:
  LiveDataPostProcessingAlgInputHistoryImpl() : AbstractAlgorithmInputHistory("LiveDataPostProcessingAlgorithms") {}
  ~LiveDataPostProcessingAlgInputHistoryImpl() override = default;

private:
  friend struct Mantid::Kernel::CreateUsingNew<LiveDataPostProcessingAlgInputHistoryImpl>;
};

#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class Mantid::Kernel::SingletonHolder<LiveDataPostProcessingAlgInputHistoryImpl>;
#endif /* _WIN32 */
/// The specific instantiation of the templated type
using LiveDataPostProcessingAlgInputHistory =
    Mantid::Kernel::SingletonHolder<LiveDataPostProcessingAlgInputHistoryImpl>;
} // namespace

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt::CustomDialogs {
DECLARE_DIALOG(StartLiveDataDialog)

// Initialize static members
const QString StartLiveDataDialog::CUSTOM_CONNECTION = "[Custom]";

//----------------------
// Public member functions
//----------------------
/// Constructor
StartLiveDataDialog::StartLiveDataDialog(QWidget *parent)
    : AlgorithmDialog(parent), m_scrollbars(this), m_useProcessAlgo(false), m_useProcessScript(false),
      m_usePostProcessAlgo(false), m_usePostProcessScript(false) {
  // Create the input history. This loads it too.
  LiveDataAlgInputHistory::Instance();
}

/// Destructor
StartLiveDataDialog::~StartLiveDataDialog() {
  // Save the input history to QSettings
  LiveDataAlgInputHistory::Instance().save();
  LiveDataPostProcessingAlgInputHistory::Instance().save();
}

/// Set up the dialog layout
void StartLiveDataDialog::initLayout() {
  ui.setupUi(this);

  // Enable scrollbars (must happen after setupUi()!)
  m_scrollbars.setEnabled(true);

  // To save the history of inputs
  // RJT: I don't much like this, but at least it's safe from a lifetime point
  // of view.
  AbstractAlgorithmInputHistory *history1 = &LiveDataAlgInputHistory::Instance();
  ui.processingAlgo->setInputHistory(history1);
  AbstractAlgorithmInputHistory *history2 = &LiveDataPostProcessingAlgInputHistory::Instance();
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
  for (int i = 0; i < 2; i++) {
    bool post = i > 0;
    QString prefix = "Processing";

    if (post)
      prefix = "PostProcessing";
    QString algo = AlgorithmInputHistory::Instance().previousInput("StartLiveData", prefix + "Algorithm");
    QString script = AlgorithmInputHistory::Instance().previousInput("StartLiveData", prefix + "Script");

    if (!post) {
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
    } else {
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

  //=========== Load Listener Class Names =============
  // Add available listeners to combo box
  ui.cmbConnListener->clear();
  std::vector<std::string> listeners = Mantid::API::LiveListenerFactory::Instance().getKeys();
  for (const auto &listener : listeners) {
    ui.cmbConnListener->addItem(QString::fromStdString(listener));
  }

  //=========== Update UI Elements =============
  radioPostProcessClicked();
  updateUiElements(ui.cmbInstrument->currentText());
  updateConnectionChoices(ui.cmbInstrument->currentText());
  updateConnectionDetails(ui.cmbConnection->currentText());
  setDefaultAccumulationMethod(ui.cmbConnListener->currentText());
  initListenerPropLayout(ui.cmbConnListener->currentText());

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

  connect(ui.cmbConnListener, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(setDefaultAccumulationMethod(const QString &)));
  connect(ui.cmbConnListener, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(initListenerPropLayout(const QString &)));
  connect(ui.cmbInstrument, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(updateUiElements(const QString &)));
  connect(ui.cmbInstrument, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(updateConnectionChoices(const QString &)));

  connect(ui.cmbConnection, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(updateConnectionDetails(const QString &)));

  QLayout *buttonLayout = this->createDefaultButtonLayout();
  ui.mainLayout->addLayout(buttonLayout);
}

//------------------------------------------------------------------------------
/// Parse input when the dialog is accepted
void StartLiveDataDialog::parseInput() {
  storePropertyValue("Instrument", ui.cmbInstrument->currentText());

  // "Connection" property does not need to be set, since these override it
  storePropertyValue("Listener", ui.cmbConnListener->currentText());
  storePropertyValue("Address", ui.edtConnAddress->text());

  storePropertyValue("AccumulationMethod", ui.cmbAccumulationMethod->currentText());

  storePropertyValue("AccumulationWorkspace", ui.editAccumulationWorkspace->text());
  if (!m_usePostProcessAlgo && !m_usePostProcessScript)
    storePropertyValue("AccumulationWorkspace", "");

  storePropertyValue("OutputWorkspace", ui.editOutputWorkspace->text());

  storePropertyValue("ProcessingAlgorithm", "");
  storePropertyValue("ProcessingProperties", "");
  storePropertyValue("ProcessingScript", "");
  if (m_useProcessAlgo && m_processingAlg) {
    storePropertyValue("ProcessingAlgorithm", ui.processingAlgo->getSelectedAlgorithm());
    std::string props;
    props = m_processingAlg->asString(false);
    storePropertyValue("ProcessingProperties", QString::fromStdString(props));
  } else if (m_useProcessScript)
    storePropertyValue("ProcessingScript", ui.processingAlgo->getScriptText());

  storePropertyValue("PostProcessingAlgorithm", "");
  storePropertyValue("PostProcessingProperties", "");
  storePropertyValue("PostProcessingScript", "");
  if (m_usePostProcessAlgo && m_postProcessingAlg) {
    storePropertyValue("PostProcessingAlgorithm", ui.postAlgo->getSelectedAlgorithm());
    std::string props;
    props = m_postProcessingAlg->asString(false);
    storePropertyValue("PostProcessingProperties", QString::fromStdString(props));
  } else if (m_usePostProcessScript)
    storePropertyValue("PostProcessingScript", ui.postAlgo->getScriptText());

  // Save to QSettings
  ui.processingAlgo->saveInput();
  ui.postAlgo->saveInput();
}

//------------------------------------------------------------------------------
/** Slot called when one of the radio buttons in "processing" are picked */
void StartLiveDataDialog::radioProcessClicked() {
  m_useProcessAlgo = ui.radProcessAlgorithm->isChecked();
  ui.processingAlgo->algoVisible(m_useProcessAlgo);
  m_useProcessScript = ui.radProcessScript->isChecked();
  ui.processingAlgo->editorVisible(m_useProcessScript);
}

//------------------------------------------------------------------------------
/** Slot called when one of the radio buttons in "processing" are picked */
void StartLiveDataDialog::radioPostProcessClicked() {
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
void StartLiveDataDialog::radioTimeClicked() { ui.dateTimeEdit->setEnabled(ui.radAbsoluteTime->isChecked()); }

/** Slot called when the preserve events checkbox changes */
void StartLiveDataDialog::chkPreserveEventsToggled() {
  ui.lblPreserveEventsWarning->setVisible(ui.chkPreserveEvents->isChecked());
}

//------------------------------------------------------------------------------
/** Slot called when picking a different algorithm
 * in the AlgorithmSelectorWidget */
void StartLiveDataDialog::changeProcessingAlgorithm() {
  Algorithm_sptr alg = ui.processingAlgo->getAlgorithm();
  if (!alg)
    return;
  m_processingAlg = alg;
}

//------------------------------------------------------------------------------
/** Slot called when picking a different algorithm
 * in the AlgorithmSelectorWidget */
void StartLiveDataDialog::changePostProcessingAlgorithm() {
  Algorithm_sptr alg = ui.postAlgo->getAlgorithm();
  if (!alg)
    return;
  m_postProcessingAlg = alg;
}

//------------------------------------------------------------------------------
/** Slot called when picking a different instrument.
 *  Disables the 'Add' option if the listener is going to pass back histograms.
 *  @param listener :: The listener class name.
 */
void StartLiveDataDialog::setDefaultAccumulationMethod(const QString &listener) {
  if (listener.isEmpty())
    return;
  try {
    // Make sure 'Add' is enabled ahead of the check (the check may throw)
    int addIndex = ui.cmbAccumulationMethod->findText("Add");
    ui.cmbAccumulationMethod->setItemData(addIndex, QVariant(Qt::ItemIsSelectable | Qt::ItemIsEnabled),
                                          Qt::UserRole - 1);

    // Check whether this listener will give back events. If not, disable 'Add'
    // as an option
    // The 'false' 2nd argument means don't connect the created listener
    Mantid::Kernel::LiveListenerInfo info(listener.toStdString());
    if (!Mantid::API::LiveListenerFactory::Instance().create(info, false)->buffersEvents()) {
      // If 'Add' is currently selected, select 'Replace' instead
      if (ui.cmbAccumulationMethod->currentIndex() == addIndex) {
        int replaceIndex = ui.cmbAccumulationMethod->findText("Replace");
        ui.cmbAccumulationMethod->setCurrentIndex(replaceIndex);
      }
      // Disable the 'Add' option in the combobox. It just wouldn't make sense.
      ui.cmbAccumulationMethod->setItemData(addIndex, false, Qt::UserRole - 1);
    }
  }
  // If an exception is thrown, just swallow it and do nothing
  // getInstrument can throw, particularly while we allow listener names to be
  // passed in directly
  catch (Mantid::Kernel::Exception::NotFoundError &) {
  }
}

//------------------------------------------------------------------------------
/** Another slot called when picking a different instrument.
 *  Disables UI elements that are not used by the instrument
 *  Currently, only TOPAZ listener uses this (and only for the
 *  "Starting Time" group.
 *  @param inst :: The instrument name.
 */
void StartLiveDataDialog::updateUiElements(const QString &inst) {
  if (inst.isEmpty())
    return;
  try {
    if (inst == "TOPAZ") {
      ui.groupBox->setEnabled(false);
      ui.radNow->setChecked(true);
    } else {
      ui.groupBox->setEnabled(true);
    }
  }
  // If an exception is thrown, just swallow it and do nothing
  // getInstrument can throw, particularly while we allow listener names to be
  // passed in directly
  catch (Mantid::Kernel::Exception::NotFoundError &) {
  }
}

void StartLiveDataDialog::accept() {
  // Now manually set the StartTime property as there's a computation needed
  DateAndTime startTime = DateAndTime::getCurrentTime() - ui.dateTimeEdit->value() * 60.0;
  std::string starttime = startTime.toISO8601String();
  // Store the value to property value map: property value can be only set from the map to m_algorithm
  // as the last step before executing
  QString propertyname = QString::fromStdString("StartTime");
  QString propertyvalue = QString::fromStdString(starttime);
  this->storePropertyValue(propertyname, propertyvalue);

  // Call base class
  AlgorithmDialog::accept(); // accept executes the algorithm
}

/**
 * Update the Listener Properties group box for the current LiveListener.
 *
 */
void StartLiveDataDialog::initListenerPropLayout(const QString &listener) {
  // remove previous listener's properties
  auto props = m_algorithm->getPropertiesInGroup("ListenerProperties");
  for (const auto &prop : props) {
    QString propName = QString::fromStdString((*prop).name());
    if (m_algProperties.contains(propName)) {
      m_algProperties.removeAll(propName);
    }
  }

  // update algorithm's properties
  if (ui.cmbInstrument->currentText().toStdString() != "") {
    // create or clear the layout
    QLayout *layout = ui.listenerProps->layout();
    if (!layout) {
      QGridLayout *listenerPropLayout = new QGridLayout(ui.listenerProps);
      layout = listenerPropLayout;
    } else {
      QLayoutItem *child;
      while ((child = layout->takeAt(0)) != nullptr) {
        child->widget()->close();
        delete child;
      }
    }

    // set the instrument and listener properties early to get the listener's properties
    // this will be overriden by the same values in parseInput() function
    m_algorithm->setPropertyValue("Instrument", ui.cmbInstrument->currentText().toStdString());
    m_algorithm->setPropertyValue("Listener", listener.toStdString());
    // find the listener's properties
    props = m_algorithm->getPropertiesInGroup("ListenerProperties");

    // no properties - don't show the box
    if (props.empty()) {
      ui.listenerProps->setVisible(false);
      return;
    }

    auto gridLayout = static_cast<QGridLayout *>(layout);
    // add widgets for the listener's properties
    for (auto prop = props.begin(); prop != props.end(); ++prop) {
      int row = static_cast<int>(std::distance(props.begin(), prop));
      QString propName = QString::fromStdString((**prop).name());
      gridLayout->addWidget(new QLabel(propName), row, 0);
      QLineEdit *propWidget = new QLineEdit();
      gridLayout->addWidget(propWidget, row, 1);
      if (!m_algProperties.contains(propName)) {
        m_algProperties.append(propName);
      }
      tie(propWidget, propName, gridLayout);
    }
    ui.listenerProps->setVisible(true);
  }
}

/**
 * Slot to update list of available connections when instrument is changed
 *
 * @param inst_name Name of selected instrument
 */
void StartLiveDataDialog::updateConnectionChoices(const QString &inst_name) {
  // Reset the connections listed
  ui.cmbConnection->clear();
  ui.cmbConnection->addItem(CUSTOM_CONNECTION);

  // Add available LiveListenerInfo names based on selected instrument
  const auto &inst = ConfigService::Instance().getInstrument(inst_name.toStdString());
  for (const auto &listener : inst.liveListenerInfoList()) {
    ui.cmbConnection->addItem(QString::fromStdString(listener.name()));
  }

  // Select default item
  try {
    auto selectName = QString::fromStdString(inst.liveListenerInfo().name());
    auto index = ui.cmbConnection->findText(selectName);
    ui.cmbConnection->setCurrentIndex(index);
  } catch (std::runtime_error &) {
    // the default instrument has no live listener, don't make a selection
  }
}

/**
 * Slot to update connection parameters when connection selected
 *
 * @param connection Name of selected live listener connection
 */
void StartLiveDataDialog::updateConnectionDetails(const QString &connection) {
  // Custom connections just enable editting connection parameters
  if (connection == CUSTOM_CONNECTION) {
    ui.cmbConnListener->setEnabled(true);
    ui.edtConnAddress->setEnabled(true);
    return;
  }

  // User shouldn't be able to edit values loaded from Facilities.xml
  ui.cmbConnListener->setEnabled(false);
  ui.edtConnAddress->setEnabled(false);

  // Get live listener for select instrument and connection
  const auto &inst = ConfigService::Instance().getInstrument(ui.cmbInstrument->currentText().toStdString());
  const auto &info = inst.liveListenerInfo(connection.toStdString());

  // Select correct listener
  auto listener = QString::fromStdString(info.listener());
  auto index = ui.cmbConnListener->findText(listener);
  ui.cmbConnListener->setCurrentIndex(index);

  // Set address text box
  auto address = QString::fromStdString(info.address());
  ui.edtConnAddress->setText(address);
  ui.edtConnAddress->home(false); // display long lines from beginning, not end
}
} // namespace MantidQt::CustomDialogs
