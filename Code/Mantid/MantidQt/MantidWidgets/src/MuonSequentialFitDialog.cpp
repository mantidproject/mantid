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
    setControlButtonType(Start); 

    initDiagnosisTable();

    // After initial values are set, update depending elements accordingly. We don't rely on
    // slot/signal update, as element might be left with default values which means these will
    // never be called on initialication.
    updateLabelError( m_ui.labelInput->text() );
    updateControlButtonState();

    connect( m_ui.labelInput, SIGNAL( textChanged(const QString&) ), 
      this, SLOT( updateLabelError(const QString&) ) );

    connect( m_ui.labelInput, SIGNAL( textChanged(const QString&) ), 
      this, SLOT( updateControlButtonState() ) );
    connect( m_ui.runs, SIGNAL( fileFindingFinished() ), 
      this, SLOT( updateControlButtonState() ) );
  }

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
    QHeaderView* header = m_ui.diagnosisTable->horizontalHeader();
    header->setResizeMode(QHeaderView::Stretch);
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
   * Set the type of the control button. It is Start button when fitting has not been started,
   * and Stop button when fitting is running.
   * @param type :: New type of the button  
   */
  void MuonSequentialFitDialog::setControlButtonType(ControlButtonType type)
  {
    // Disconnect everything connected to pressed() signal of the button 
    disconnect( m_ui.controlButton, SIGNAL( pressed() ), 0, 0);
 
    // Connect to appropriate slot
    auto buttonSlot = (type == Start) ? SLOT( startFit() ) : SLOT( stopFit() );
    connect( m_ui.controlButton, SIGNAL( pressed() ), this, buttonSlot );

    // Set appropriate text
    QString buttonText = (type == Start) ? "Start" : "Stop";
    m_ui.controlButton->setText(buttonText);
  }

  /** * Update enabled state off all the input widgets (except for control ones).
   * @param enabled :: True if widgets should be enabled, false otherwise
   */
  void MuonSequentialFitDialog::setInputEnabled(bool enabled)
  {
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
    g_log.notice("Seq. fitting started");
    setControlButtonType(Stop);
    setInputEnabled(false);
  }

  /**
   * Stop fitting process.
   */
  void MuonSequentialFitDialog::stopFit()
  {
    g_log.notice("Seq. fitting stopped");
    setControlButtonType(Start);
    setInputEnabled(true);
  }

  /**
   * Destructor
   */
  MuonSequentialFitDialog::~MuonSequentialFitDialog()
  {}

} // namespace MantidWidgets
} // namespace Mantid
