#include "MantidQtMantidWidgets/MuonSequentialFitDialog.h"
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
  MuonSequentialFitDialog::MuonSequentialFitDialog(QWidget* parent) :
    QDialog(parent)
  {
    m_ui.setupUi(this);

    // TODO: set initial values 
    setControlButtonType(Start); 

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

  /**
   * Start fitting process.
   */
  void MuonSequentialFitDialog::startFit()
  {
    g_log.notice("Seq. fitting started");
    setControlButtonType(Stop);
  }

  /**
   * Stop fitting process.
   */
  void MuonSequentialFitDialog::stopFit()
  {
    g_log.notice("Seq. fitting stopped");
    setControlButtonType(Start);
  }

  /**
   * Destructor
   */
  MuonSequentialFitDialog::~MuonSequentialFitDialog()
  {}

} // namespace MantidWidgets
} // namespace Mantid
