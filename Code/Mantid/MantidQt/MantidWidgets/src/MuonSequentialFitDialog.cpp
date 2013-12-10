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

    // TODO: set initial values 
    initDiagnosisTable();
    setState(Stopped);

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
   * Initialize diagnosis table.
   */
  void MuonSequentialFitDialog::initDiagnosisTable()
  {
    QStringList headerLabels;

    // Add two static columns 
    headerLabels << "Run" << "Fit quality";

    // Add remaining columns - one for every fit function parameter
    CompositeFunction_const_sptr fitFunc = m_fitPropBrowser->compositeFunction();

    for(size_t i = 0; i < fitFunc->nParams(); i++)
      headerLabels << QString::fromStdString( fitFunc->parameterName(i) );

    m_ui.diagnosisTable->setColumnCount( headerLabels.size() );
    m_ui.diagnosisTable->setHorizontalHeaderLabels(headerLabels);

    // Make the table fill all the available space
    m_ui.diagnosisTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
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

    QStringList runFilenames = m_ui.runs->getFilenames();

    // Tell progress bar how many iterations we will need to make and reset it
    m_ui.progress->setRange( 0, runFilenames.size() );
    m_ui.progress->setValue(0);

    setState(Running);
    m_stopRequested = false;

    for ( auto runIt = runFilenames.constBegin(); runIt != runFilenames.constEnd(); ++runIt )
    {
      // Process events (so that Stop button press is processed)
      QApplication::processEvents();

      if ( m_stopRequested )
      {
        setState(Stopped);
        return;
      }

      // Update progress
      m_ui.progress->setValue( m_ui.progress->value() + 1 );

      try
      {
        // TODO: should be MuonLoad here
        IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().createUnmanaged("LoadMuonNexus");
        loadAlg->initialize();
        loadAlg->setPropertyValue( "Filename", runIt->toStdString() );
        loadAlg->setPropertyValue( "OutputWorkspace", "Loaded" );
        loadAlg->execute();
      }
      catch(std::exception)
      {
        // TODO: probably should show QMEssageBox
        setState(Stopped);
      }
    }

    setState(Stopped);
  }

  /**
   * Stop fitting process.
   */
  void MuonSequentialFitDialog::stopFit()
  {
    if ( m_state != Running )
      throw std::runtime_error("Coulnd't stop: is not running");

    m_stopRequested = true;
  }

} // namespace MantidWidgets
} // namespace Mantid
