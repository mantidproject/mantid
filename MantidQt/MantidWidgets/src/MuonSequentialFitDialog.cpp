#include "MantidQtMantidWidgets/MuonSequentialFitDialog.h"
#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"

#include "MantidAPI/AnalysisDataService.h" 
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/ITableWorkspace.h"

namespace MantidQt
{
namespace MantidWidgets
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  namespace
  {
    Logger g_log("MuonSequentialFitDialog");
  }
  const std::string MuonSequentialFitDialog::SEQUENTIAL_PREFIX("MuonSeqFit_");

  /** 
   * Constructor
   */
  MuonSequentialFitDialog::MuonSequentialFitDialog(MuonFitPropertyBrowser* fitPropBrowser,
      Algorithm_sptr loadAlg) :
    QDialog(fitPropBrowser), m_fitPropBrowser(fitPropBrowser), m_loadAlg(loadAlg)
  {
    m_ui.setupUi(this);

    setState(Stopped);

    // Set initial run to be run number of the workspace selected in fit browser when starting
    // seq. fit dialog 
    auto fitWS = boost::dynamic_pointer_cast<const MatrixWorkspace>( m_fitPropBrowser->getWorkspace() );
    m_ui.runs->setText( QString::number( fitWS->getRunNumber() ) + "-" );

    // TODO: find a better initial one, e.g. previously used
    m_ui.labelInput->setText("Label");

    initDiagnosisTable();

    // After initial values are set, update depending elements accordingly. We don't rely on
    // slot/signal update, as element might be left with default values which means these will
    // never be called on initialization.
    updateLabelError( m_ui.labelInput->text() );

    updateControlButtonType(m_state);
    updateInputEnabled(m_state);
    updateControlEnabled(m_state);
    updateCursor(m_state);

    connect( m_ui.labelInput, SIGNAL( textChanged(const QString&) ), 
      this, SLOT( updateLabelError(const QString&) ) );

    connect( this, SIGNAL( stateChanged(DialogState) ),
      this, SLOT( updateControlButtonType(DialogState) ) );
    connect( this, SIGNAL( stateChanged(DialogState) ),
      this, SLOT( updateInputEnabled(DialogState) ) );
    connect( this, SIGNAL( stateChanged(DialogState) ),
      this, SLOT( updateControlEnabled(DialogState) ) );
    connect( this, SIGNAL( stateChanged(DialogState) ),
      this, SLOT( updateCursor(DialogState) ) );
  }

  /**
   * Destructor
   */
  MuonSequentialFitDialog::~MuonSequentialFitDialog()
  {}

  /**
   * Checks if specified name is valid as a name for label. 
   * @param label :: The name to check
   * @return Empty string if valid, otherwise a string describing why is invalid
   */
  std::string MuonSequentialFitDialog::isValidLabel(const std::string& label)
  {
    if ( label.empty() )
      return "Can not be empty";
    else
      return AnalysisDataService::Instance().isValid(label);
  }

  /**
   * Returns displayable title for the given workspace;
   * @param ws :: Workpspace to get title from
   * @return The title, or empty string if unable to get one
   */
  std::string MuonSequentialFitDialog::getRunTitle(Workspace_const_sptr ws)
  {
    auto matrixWS = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);

    if ( ! matrixWS )
      return "";

    const std::string& instrName = matrixWS->getInstrument()->getName();
    const int runNumber = matrixWS->getRunNumber();

    if ( instrName.empty() || runNumber == 0 )
      return "";

    std::ostringstream runTitle;
    runTitle << instrName << runNumber;
    return runTitle.str();
  }

  /**
   * Initialize diagnosis table.
   */
  void MuonSequentialFitDialog::initDiagnosisTable()
  {
    QStringList headerLabels;

    // Add two static columns 
    headerLabels << "Run" << "Fit quality";

    // Add remaining columns - one for every fit function parameter
    IFunction_sptr fitFunc = m_fitPropBrowser->getFittingFunction();

    for(size_t i = 0; i < fitFunc->nParams(); i++)
    {
      QString paramName = QString::fromStdString( fitFunc->parameterName(i) ); 
      headerLabels << paramName;
      headerLabels << paramName + "_Err";
    }

    m_ui.diagnosisTable->setColumnCount( headerLabels.size() );
    m_ui.diagnosisTable->setHorizontalHeaderLabels(headerLabels);

    // Make the table fill all the available space and columns be resized to fit contents
    m_ui.diagnosisTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    // Make rows alternate bg colors for better user experience 
    m_ui.diagnosisTable->setAlternatingRowColors(true);
  }

  /**
   * Add a new entry to the diagnosis table.
   * @param runTitle       :: Title of the run fitted
   * @param fitQuality     :: Number representing a goodness of the fit
   * @param fittedFunction :: Function containing fitted parameters 
   */
  void MuonSequentialFitDialog::addDiagnosisEntry(const std::string& runTitle, double fitQuality,
      IFunction_sptr fittedFunction)
  {
    int newRow = m_ui.diagnosisTable->rowCount();

    m_ui.diagnosisTable->insertRow(newRow);

    QString runTitleDisplay = QString::fromStdString(runTitle);
    m_ui.diagnosisTable->setItem( newRow, 0, createTableWidgetItem(runTitleDisplay) );

    QString fitQualityDisplay = QString::number(fitQuality);
    m_ui.diagnosisTable->setItem( newRow, 1, createTableWidgetItem(fitQualityDisplay) );

    for(int i = 2; i < m_ui.diagnosisTable->columnCount(); i += 2)
    {
      std::string paramName = m_ui.diagnosisTable->horizontalHeaderItem(i)->text().toStdString();
      size_t paramIndex = fittedFunction->parameterIndex(paramName);

      QString value = QString::number( fittedFunction->getParameter(paramIndex) );
      QString error = QString::number( fittedFunction->getError(paramIndex) );

      m_ui.diagnosisTable->setItem(newRow, i, createTableWidgetItem(value) );
      m_ui.diagnosisTable->setItem(newRow, i + 1, createTableWidgetItem(error) );
    }

    m_ui.diagnosisTable->scrollToBottom();
  }

  /**
   * Helper function to create new item for Diagnosis table.
   * @return Created and initialized item with text
   */
  QTableWidgetItem* MuonSequentialFitDialog::createTableWidgetItem(const QString& text)
  {
    auto newItem = new QTableWidgetItem(text);
    newItem->setFlags(newItem->flags() ^ Qt::ItemIsEditable);
    return newItem;
  }

  /**
   * Updates visibility/tooltip of label error asterisk.
   * @param label :: New label as specified by user 
   */
  void MuonSequentialFitDialog::updateLabelError(const QString& label)
  {
    std::string error = isValidLabel( label.toStdString() );

    m_ui.labelError->setVisible( ! error.empty() ); 
    m_ui.labelError->setToolTip( QString::fromStdString(error) );
  }

  /**
   * Check if all the input field are valid.
   * @return True if everything valid, false otherwise
   */
  bool MuonSequentialFitDialog::isInputValid()
  {
    if ( ! m_ui.runs->isValid() )
      return false;  

    std::string label = m_ui.labelInput->text().toStdString();
    if ( ! isValidLabel(label).empty() )
      return false;

    return true;
  }

  /**
   * Sets control button to be start/stop depending on new dialog state.
   * @param newState :: New state of the dialog 
   */
  void MuonSequentialFitDialog::updateControlButtonType(DialogState newState)
  {
    // Disconnect everything connected to pressed() signal of the button 
    disconnect( m_ui.controlButton, SIGNAL( pressed() ), 0, 0);
 
    // Connect to appropriate slot
    auto buttonSlot = (newState == Running) ? SLOT( stopFit() ) : SLOT( startFit() );
    connect( m_ui.controlButton, SIGNAL( pressed() ), this, buttonSlot );

    // Set appropriate text
    QString buttonText = (newState == Running) ? "Stop" : "Start";
    m_ui.controlButton->setText(buttonText);
  }

  /** 
   * Updates current state of the dialog.
   */
  void MuonSequentialFitDialog::setState(DialogState newState)
  {
    m_state = newState;
    emit stateChanged(newState);
  }

  /**
   * Update enabled state off all the input widgets depending on new dialog state.
   * @param newState :: New state of the dialog
   */
  void MuonSequentialFitDialog::updateInputEnabled(DialogState newState)
  {
    bool enabled = (newState == Stopped);

    m_ui.runs->setEnabled(enabled);
    m_ui.labelInput->setEnabled(enabled);
   
    foreach(QAbstractButton* button, m_ui.paramTypeGroup->buttons())
      button->setEnabled(enabled); 
  }

  /**
   * Update control button enabled status depending on the new state. 
   * Button is disabled in one case only - when preparing for running.
   * @param newState :: New state of the dialog 
   */
  void MuonSequentialFitDialog::updateControlEnabled(DialogState newState)
  {
    m_ui.controlButton->setEnabled( newState != Preparing );
    
  }

  /**
   * Update cursor depending on the new state of the dialog.
   * Waiting cursor is displayed while preparing so that user does now that something is happening.
   * @param newState :: New state of the dialog
   */
  void MuonSequentialFitDialog::updateCursor(DialogState newState)
  {
    switch(newState)
    {
      case Preparing:
        setCursor(Qt::WaitCursor);
        break;
      case Running:
        setCursor(Qt::BusyCursor);
        break;
      default:
        unsetCursor();
        break;
    }
  }

  /**
   * Start fitting process.
   */
  void MuonSequentialFitDialog::startFit()
  {
    if ( m_state != Stopped )
      throw std::runtime_error("Couln't start: already running");

    setState(Preparing);

    // Explicitly run the file search. This might be needed when Start is clicked straigh after 
    // editing the run box. In that case, lost focus event might not be processed yet and search
    // might not have been started yet. Otherwise, search is not done as the widget sees that it
    // has not been changed. Taken from LoadDialog.cpp:124.
    m_ui.runs->findFiles();

    // Wait for file search to finish.
    while ( m_ui.runs->isSearching() )
    {
      QApplication::processEvents();
    }

    // To process events from the finished thread
    QApplication::processEvents();

    // Validate input fields
    if ( ! isInputValid() )
    {
      QMessageBox::critical(this, "Input is not valid", 
        "One or more input fields are invalid.\n\nInvalid fields are marked with a '*'.");
      setState(Stopped);
      return;
    }

    QStringList runFilenames = m_ui.runs->getFilenames();

    const std::string label = m_ui.labelInput->text().toStdString();
    const std::string labelGroupName = SEQUENTIAL_PREFIX + label;

    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();

    if ( ads.doesExist(labelGroupName) )
    {
      QMessageBox::StandardButton answer = QMessageBox::question(this, "Label already exists", 
          "Label you specified was used for one of the previous fits. Do you want to overwrite it?", 
          QMessageBox::Yes | QMessageBox::Cancel);

      if ( answer != QMessageBox::Yes )
      {
        setState(Stopped);
        return;
      }

      ads.deepRemoveGroup(labelGroupName);
    }
   
    // Create a group for label
    ads.add(labelGroupName, boost::make_shared<WorkspaceGroup>());

    // Tell progress bar how many iterations we will need to make and reset it
    m_ui.progress->setRange( 0, runFilenames.size() );
    m_ui.progress->setFormat("%p%");
    m_ui.progress->setValue(0);

    // Clear diagnosis table for new fit
    m_ui.diagnosisTable->setRowCount(0);

    // Get fit function as specified by user in the fit browser
    IFunction_sptr fitFunction = FunctionFactory::Instance().createInitialized(
        m_fitPropBrowser->getFittingFunction()->asString() );

    // Whether we should use initial function for every fit
    bool useInitFitFunction = (m_ui.paramTypeGroup->checkedButton() == m_ui.paramTypeInitial);

    setState(Running);
    m_stopRequested = false;

    for ( auto fileIt = runFilenames.constBegin(); fileIt != runFilenames.constEnd(); ++fileIt )
    {
      // Process events (so that Stop button press is processed)
      QApplication::processEvents();

      // Stop if requested by user
      if ( m_stopRequested )
        break;

      MatrixWorkspace_sptr ws;

      try {
        // If ApplyDeadTimeCorrection is set but no dead time table is set,
        // we need to load one from the file.
        bool loadDeadTimesFromFile = false;
        bool applyDTC = m_loadAlg->getProperty("ApplyDeadTimeCorrection");
        if (applyDTC) {
          if (auto deadTimes =
                  m_loadAlg->getPointerToProperty("DeadTimeTable")) {
            if (deadTimes->value() == "") {
              // No workspace set for dead time table - we need to load one
              loadDeadTimesFromFile = true;
            }
          }
        }

        // Use LoadMuonNexus to load the file
        auto loadAlg = AlgorithmManager::Instance().create("LoadMuonNexus");
        loadAlg->initialize();
        loadAlg->setChild(true);
        loadAlg->setRethrows(true);
        loadAlg->setPropertyValue("Filename", fileIt->toStdString());
        loadAlg->setPropertyValue("OutputWorkspace", "__NotUsed");
        if (loadDeadTimesFromFile) {
          loadAlg->setPropertyValue("DeadTimeTable", "__DeadTimes");
        }
        loadAlg->execute();
        Workspace_sptr loadedWS = loadAlg->getProperty("OutputWorkspace");
        double loadedTimeZero = loadAlg->getProperty("TimeZero");

        // then use MuonProcess to process it
        auto process = AlgorithmManager::Instance().create("MuonProcess");
        process->initialize();
        process->setChild(true);
        process->setRethrows(true);
        process->updatePropertyValues(*m_loadAlg);
        process->setProperty("InputWorkspace", loadedWS);
        process->setProperty("LoadedTimeZero", loadedTimeZero);
        process->setPropertyValue("OutputWorkspace", "__YouDontSeeMeIAmNinja");
        if (m_fitPropBrowser->rawData()) // TODO: or vice verca?
          process->setPropertyValue("RebinParams", "");
        if (loadDeadTimesFromFile) {
          Workspace_sptr deadTimes = loadAlg->getProperty("DeadTimeTable");
          ITableWorkspace_sptr deadTimesTable;
          if (auto table =
                  boost::dynamic_pointer_cast<ITableWorkspace>(deadTimes)) {
            deadTimesTable = table;
          } else if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(
                         deadTimes)) {
            deadTimesTable =
                boost::dynamic_pointer_cast<ITableWorkspace>(group->getItem(0));
          }
          process->setProperty("DeadTimeTable", deadTimesTable);
        }

        process->execute();

        Workspace_sptr outputWS = process->getProperty("OutputWorkspace");
        ws = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
      } catch (const std::exception &ex) {
        g_log.error(ex.what());
        QMessageBox::critical(
            this, "Loading failed",
            "Unable to load one of the files.\n\nCheck log for details");

      } catch (...) {
        QMessageBox::critical(
            this, "Loading failed",
            "Unable to load one of the files.\n\nCheck log for details");
        break;
      }

      const std::string runTitle = getRunTitle(ws);
      const std::string wsBaseName = labelGroupName + "_" + runTitle; 

      IFunction_sptr functionToFit;

      if ( useInitFitFunction )
        // Create a copy so that the original function is not changed
        functionToFit = FunctionFactory::Instance().createInitialized( fitFunction->asString() );
      else
        // Use the same function over and over, so that previous fitted params are used for the next fit
        functionToFit = fitFunction;

      IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
      fit->setRethrows(true);

      try 
      {

        // Set function. Gets updated when fit is done. 
        fit->setProperty("Function", functionToFit);

        fit->setProperty("InputWorkspace", ws);
        fit->setProperty("Output", wsBaseName);

        // We should have one spectra only in the workspace, so use the first one.
        fit->setProperty("WorkspaceIndex", 0);

        // Various properties from the fit prop. browser
        fit->setProperty("StartX", m_fitPropBrowser->startX());
        fit->setProperty("EndX", m_fitPropBrowser->endX());
        fit->setProperty("Minimizer", m_fitPropBrowser->minimizer());
        fit->setProperty("CostFunction", m_fitPropBrowser->costFunction());

        fit->execute();
      }
      catch(...)
      {
        QMessageBox::critical(this, "Fitting failed", 
            "Unable to fit one of the files.\n\nCheck log for details");
        break;
      }

      // Make sure created fit workspaces end-up in the group
      // TODO: this really should use loop
      ads.addToGroup(labelGroupName, wsBaseName + "_NormalisedCovarianceMatrix");
      ads.addToGroup(labelGroupName, wsBaseName + "_Parameters");
      ads.addToGroup(labelGroupName, wsBaseName + "_Workspace");

      // Copy log values
      auto fitWs = ads.retrieveWS<MatrixWorkspace>(wsBaseName + "_Workspace");
      fitWs->copyExperimentInfoFrom(ws.get());

      // Add information about the fit to the diagnosis table
      addDiagnosisEntry(runTitle, fit->getProperty("OutputChi2OverDof"), functionToFit); 

      // Update progress
      m_ui.progress->setFormat("%p% - " + QString::fromStdString(runTitle) );
      m_ui.progress->setValue( m_ui.progress->value() + 1 );
    }

    setState(Stopped);
  }

  /**
   * Stop fitting process.
   */
  void MuonSequentialFitDialog::stopFit()
  {
    if ( m_state == Stopped )
      throw std::runtime_error("Couldn't stop: is not running");

    m_stopRequested = true;
  }

} // namespace MantidWidgets
} // namespace Mantid
