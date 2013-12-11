#include "MantidQtMantidWidgets/MuonSequentialFitDialog.h"
#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"

#include "MantidAPI/AnalysisDataService.h" 

namespace MantidQt
{
namespace MantidWidgets
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  Logger& MuonSequentialFitDialog::g_log(Logger::get("MuonSequentialFitDialog"));
  /** 

   * Constructor
   */
  MuonSequentialFitDialog::MuonSequentialFitDialog(MuonFitPropertyBrowser* fitPropBrowser) :
    QDialog(fitPropBrowser), m_fitPropBrowser(fitPropBrowser)
  {
    m_ui.setupUi(this);

    setState(Stopped);

    // Set initial runs text 
    Workspace_const_sptr fitWS = m_fitPropBrowser->getWorkspace();
    m_ui.runs->setText( QString::fromStdString( getRunTitle(fitWS) ) + "-" );

    // TODO: find a better initial one, e.g. previously used
    m_ui.labelInput->setText("Label");

    initDiagnosisTable();

    // After initial values are set, update depending elements accordingly. We don't rely on
    // slot/signal update, as element might be left with default values which means these will
    // never be called on initialication.
    updateLabelError( m_ui.labelInput->text() );
    updateControlButtonState();
    updateControlButtonType(m_state);
    updateInputEnabled(m_state);

    connect( m_ui.labelInput, SIGNAL( textChanged(const QString&) ), 
      this, SLOT( updateLabelError(const QString&) ) );

    connect( m_ui.labelInput, SIGNAL( textChanged(const QString&) ), 
      this, SLOT( updateControlButtonState() ) );
    connect( m_ui.runs, SIGNAL( fileFindingFinished() ), 
      this, SLOT( updateControlButtonState() ) );

    connect( this, SIGNAL( stateChanged(DialogState) ),
      this, SLOT( updateControlButtonType(DialogState) ) );
    connect( this, SIGNAL( stateChanged(DialogState) ),
      this, SLOT( updateInputEnabled(DialogState) ) );
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
      headerLabels << QString::fromStdString( fitFunc->parameterName(i) );

    m_ui.diagnosisTable->setColumnCount( headerLabels.size() );
    m_ui.diagnosisTable->setHorizontalHeaderLabels(headerLabels);

    // Make the table fill all the available space
    m_ui.diagnosisTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
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

    m_ui.diagnosisTable->setItem(newRow, 0, new QTableWidgetItem( QString::fromStdString(runTitle) ) );
    m_ui.diagnosisTable->setItem(newRow, 1, new QTableWidgetItem( QString::number(fitQuality) ) );

    for(int i = 2; i < m_ui.diagnosisTable->columnCount(); ++i)
    {
      std::string paramName = m_ui.diagnosisTable->horizontalHeaderItem(i)->text().toStdString();
      double value = fittedFunction->getParameter(paramName);
      m_ui.diagnosisTable->setItem(newRow, i, new QTableWidgetItem( QString::number(value) ) );
    }

    m_ui.diagnosisTable->scrollToBottom();
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
   * Enables/disables start button depending on wether we are allowed to start.
   */
  void MuonSequentialFitDialog::updateControlButtonState()
  {
    m_ui.controlButton->setEnabled( isInputValid() );
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
    auto buttonSlot = (newState == Stopped) ? SLOT( startFit() ) : SLOT( stopFit() );
    connect( m_ui.controlButton, SIGNAL( pressed() ), this, buttonSlot );

    // Set appropriate text
    QString buttonText = (newState == Stopped) ? "Start" : "Stop";
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
   * Start fitting process.
   */
  void MuonSequentialFitDialog::startFit()
  {
    if ( m_state == Running )
      throw std::runtime_error("Couln't start: already running");

    const std::string label = m_ui.labelInput->text().toStdString();
    const std::string labelGroupName = "MuonSeqFit_" + label;

    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();

    if ( ads.doesExist(labelGroupName) )
    {
      QMessageBox::StandardButton answer = QMessageBox::question(this, "Label already exists", 
          "Label you specified was used for one of the previous fits. Do you want to overwrite it?", 
          QMessageBox::Yes | QMessageBox::Cancel);

      if ( answer != QMessageBox::Yes )
        return;

      ads.deepRemoveGroup(labelGroupName);
    }
   
    // Create a group for label
    ads.add(labelGroupName, boost::make_shared<WorkspaceGroup>());

    QStringList runFilenames = m_ui.runs->getFilenames();

    // Tell progress bar how many iterations we will need to make and reset it
    m_ui.progress->setRange( 0, runFilenames.size() );
    m_ui.progress->setFormat("%p%");
    m_ui.progress->setValue(0);

    // Clear diagnosis table for new fit
    m_ui.diagnosisTable->setRowCount(0);

    setState(Running);
    m_stopRequested = false;

    for ( auto fileIt = runFilenames.constBegin(); fileIt != runFilenames.constEnd(); ++fileIt )
    {
      // Process events (so that Stop button press is processed)
      QApplication::processEvents();

      // Stop if requested by user
      if ( m_stopRequested )
        break;

      Workspace_sptr loadedWS;

      try
      {
        // TODO: should be MuonLoad here
        IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().createUnmanaged("LoadMuonNexus");
        loadAlg->setChild(true);
        loadAlg->setRethrows(true);
        loadAlg->initialize();
        loadAlg->setPropertyValue( "Filename", fileIt->toStdString() );
        loadAlg->setPropertyValue( "OutputWorkspace", "__YouDontSeeMeIAmNinja" ); // Is not used
        loadAlg->execute();

        loadedWS = loadAlg->getProperty("OutputWorkspace");
      }
      catch(std::exception& e)
      {
        QMessageBox::critical(this, "Loading failed", 
            "Unable to load one of the files.\n\nCheck log for details");
        break;
      }

      MatrixWorkspace_sptr ws;

      if ( auto single = boost::dynamic_pointer_cast<MatrixWorkspace>(loadedWS) )
      {
        ws = single;
      }
      else if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWS) )
      {
        auto first = boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(0) );
        ws = first;
      }

      const std::string runTitle = getRunTitle(ws);
      const std::string wsBaseName = labelGroupName + "_" + runTitle; 

      double fitQuality;

      // TODO: fitting function logic

      try 
      {
        IAlgorithm_sptr fit = AlgorithmManager::Instance().createUnmanaged("Fit");
        fit->initialize();
        fit->setRethrows(true);
        fit->setProperty("Function", m_fitPropBrowser->getFittingFunction());
        fit->setProperty("InputWorkspace", ws);
        fit->setProperty("WorkspaceIndex", 0);
        fit->setProperty("StartX", m_fitPropBrowser->startX());
        fit->setProperty("EndX", m_fitPropBrowser->endX());
        fit->setProperty("Output", wsBaseName);
        fit->setProperty("Minimizer", m_fitPropBrowser->minimizer());
        fit->setProperty("CostFunction", m_fitPropBrowser->costFunction());
        fit->execute();

        fitQuality = fit->getProperty("OutputChi2overDoF");
      }
      catch(std::exception& e)
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

      // Add information about the fit to the diagnosis table
      addDiagnosisEntry(runTitle, fitQuality, m_fitPropBrowser->getFittingFunction());

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
    if ( m_state != Running )
      throw std::runtime_error("Couldn't stop: is not running");

    m_stopRequested = true;
  }

} // namespace MantidWidgets
} // namespace Mantid
